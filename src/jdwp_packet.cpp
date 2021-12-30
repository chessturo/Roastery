/* Implements an abstraction for Java Debug Wire Protocol messaging
   Copyright 2021 Mitchell Levy

This file is a part of Roastery

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <sstream>
#include <mutex>
#include <variant>

#include "jdwp_con.hpp"
#include "jdwp_packet.hpp"

namespace roastery {

namespace impl {

static uint32_t next_id = 0;
static std::mutex next_id_lck;
uint32_t GetNextId() {
  std::lock_guard<std::mutex> lk(next_id_lck);
  uint32_t res;
  res = next_id;
  next_id++;
  return res;
}

}  // namespace impl

// Defining pure virtual dtors for house keeping
IJdwpCommandPacket::~IJdwpCommandPacket() { }

IJdwpCommandPacket::IJdwpCommandPacket() : id(impl::GetNextId()) { }
uint32_t IJdwpCommandPacket::GetId() const { return id; }

string IJdwpCommandPacket::Serialize(IJdwpCon& con) const {
  return this->SerializeImpl(con);
}

string IJdwpCommandPacket::ProduceHeader(uint8_t command_set, uint8_t command,
    size_t body_len, uint32_t id) {
  if (body_len > UINT32_MAX - 11) {
    throw JdwpException("Body too long");
  }

  uint32_t len_nbo = htonl(static_cast<uint32_t>(body_len) + impl::kHeaderLen);
  uint32_t id_nbo = htonl(id);
  const unsigned char* len_nbo_bytes =
    reinterpret_cast<unsigned char*>(&len_nbo);
  const unsigned char* id_nbo_bytes =
    reinterpret_cast<unsigned char*>(&id_nbo);

  std::stringstream res;
  for (size_t i = 0; i < sizeof(len_nbo); i++) res << len_nbo_bytes[i];
  for (size_t i = 0; i < sizeof(id_nbo); i++) res << id_nbo_bytes[i];
  res << static_cast<unsigned char>(JdwpFlags::kNone);
  res << command_set;
  res << command;

  return res.str();
}


using namespace command_packets;
using std::get;

string class_type::SetValuesCommand::SerializeImpl(IJdwpCon& con) const {
  std::ostringstream res;
  const auto& values = this->GetFields();

  res << get<JdwpClassId>(values).Serialize(con);
  for (const tuple<JdwpFieldId, JdwpValue>& value : get<1>(values)) {
    res << get<JdwpFieldId>(value).Serialize(con);
    res << get<JdwpValue>(value).SerializeAsUntagged(con);
  }

  return res.str();
}

string object_reference::SetValuesCommand::SerializeImpl(IJdwpCon& con) const {
  std::ostringstream res;
  const auto& values = this->GetFields();

  res << get<JdwpObjId>(values).Serialize(con);
  for (const tuple<JdwpFieldId, JdwpValue>& value : get<1>(values)) {
    res << get<JdwpFieldId>(value).Serialize(con);
    res << get<JdwpValue>(value).SerializeAsUntagged(con);
  }

  return res.str();
}

string array_reference::SetValuesCommand::SerializeImpl(IJdwpCon& con) const {
  std::ostringstream res;
  const auto& values = this->GetFields();

  res << get<JdwpArrayId>(values).Serialize(con);
  res << get<JdwpInt>(values).Serialize(con);
  for (const tuple<JdwpValue>& value : get<2>(values)) {
    res << get<JdwpValue>(value).SerializeAsUntagged(con);
  }

  return res.str();
}

event_request::SetCommand::SetCommand() : IJdwpCommandPacket() { }

event_request::SetCommand::Fields& event_request::SetCommand::GetFields() {
  return this->fields;
}

const event_request::SetCommand::Fields& event_request::SetCommand::
    GetFields() const {
  return this->fields;
}

string event_request::SetCommand::SerializeImpl(IJdwpCon& con) const {
  std::ostringstream body_acc;

  JdwpByte event_kind = get<0>(this->fields);
  JdwpByte suspend_policy = get<1>(this->fields);
  vector<Modifier> modifiers = get<2>(this->fields);

  body_acc << event_kind.Serialize(con);
  body_acc << suspend_policy.Serialize(con);
  JdwpInt mod_len; mod_len << modifiers.size();
  body_acc << mod_len.Serialize(con);
  for (const Modifier& modifier : modifiers) {
    JdwpByte mod_kind; mod_kind << modifier.index() + 1;
    body_acc << mod_kind.Serialize(con);
    std::visit([&body_acc, &con](const auto& s) {
      impl::TupleForEach(s, [&body_acc, &con](const auto& s) {
        body_acc << s.Serialize(con);
      });
    }, modifier);
  }

  string body = body_acc.str();
  return ProduceHeader(
      static_cast<uint8_t>(commands::CommandSet::kEventRequest),
      static_cast<uint8_t>(commands::EventRequest::kSet),
      body.length(),
      this->id) +
    body;
}

bool HeaderIsEvent(const string& header) {
  return
    // Header is not a reply
    !(header[8] & 0x80) &&
    // Header is in the event command-set
    header[9] == static_cast<uint8_t>(CommandSet::kEvent);
}

IJdwpEvent::~IJdwpEvent() = default;

vector<unique_ptr<IJdwpEvent>> IJdwpEvent::FromComposite(const string& encoded,
    IJdwpCon& con) {
  if (!HeaderIsEvent(encoded))
    throw JdwpException("Cannot parse non-event packet as a composite event");

  size_t idx = impl::kHeaderLen;
  // ignore suspend policy for now
  idx += JdwpByte::value_size;

  JdwpInt event_cnt;
  event_cnt.FromEncoded(encoded.substr(idx, JdwpInt::value_size), con);
  idx += JdwpInt::value_size;

  auto res = vector<unique_ptr<IJdwpEvent>>();
  for (int i = 0; i < event_cnt.GetValue(); i++) {
    using namespace roastery::events;

    JdwpByte event_kind;
    event_kind.FromEncoded(encoded.substr(idx, JdwpByte::value_size), con);

    unique_ptr<IJdwpEvent> ev;
    switch(static_cast<JdwpEventKind>(event_kind.GetValue())) {
      case JdwpEventKind::kVmStart:
          ev = unique_ptr<VmStart>(new VmStart());
          break;
      case JdwpEventKind::kSingleStep:
          ev = unique_ptr<SingleStep>(new SingleStep());
          break;
      case JdwpEventKind::kBreakpoint:
          ev = unique_ptr<Breakpoint>(new Breakpoint());
          break;
      case JdwpEventKind::kMethodEntry:
          ev = unique_ptr<MethodEntry>(new MethodEntry());
          break;
      case JdwpEventKind::kMethodExit:
          ev = unique_ptr<MethodExit>(new MethodExit());
          break;
      case JdwpEventKind::kMethodExitWithReturnValue:
          ev = unique_ptr<MethodExitWithReturnValue>(
              new MethodExitWithReturnValue());
          break;
      case JdwpEventKind::kMonitorContendedEnter:
          ev = unique_ptr<MonitorContendedEnter>(new MonitorContendedEnter());
          break;
      case JdwpEventKind::kMonitorContendedEntered:
          ev = unique_ptr<MonitorContendedEntered>(
              new MonitorContendedEntered());
          break;
      case JdwpEventKind::kMonitorWait:
          ev = unique_ptr<MonitorWait>(new MonitorWait());
          break;
      case JdwpEventKind::kMonitorWaited:
          ev = unique_ptr<MonitorWaited>(new MonitorWaited());
          break;
      case JdwpEventKind::kException:
          ev = unique_ptr<Exception>(new Exception());
          break;
      case JdwpEventKind::kThreadStart:
          ev = unique_ptr<ThreadStart>(new ThreadStart());
          break;
      case JdwpEventKind::kThreadDeath:
          ev = unique_ptr<ThreadDeath>(new ThreadDeath());
          break;
      case JdwpEventKind::kClassPrepare:
          ev = unique_ptr<ClassPrepare>(new ClassPrepare());
          break;
      case JdwpEventKind::kClassUnload:
          ev = unique_ptr<ClassUnload>(new ClassUnload());
          break;
      case JdwpEventKind::kFieldAccess:
          ev = unique_ptr<FieldAccess>(new FieldAccess());
          break;
      case JdwpEventKind::kFieldModification:
          ev = unique_ptr<FieldModification>(new FieldModification());
          break;
      case JdwpEventKind::kVmDeath:
          ev = unique_ptr<VmDeath>(new VmDeath());
          break;
      default:
          throw JdwpException("Illegal eventKind in composite event");
    }

    idx += ev->FromEncoded(encoded.substr(idx), con);
    res.emplace_back(move(ev));
  }

  return res;
}

JdwpEventKind IJdwpEvent::GetKind() const { return this->GetKindImpl(); }

size_t IJdwpEvent::FromEncoded(const string& encoded, IJdwpCon& con) {
  JdwpByte event_kind; event_kind.FromEncoded(encoded, con);
  if (static_cast<JdwpEventKind>(event_kind.GetValue()) != this->GetKind())
    throw JdwpException("Wrong IJdwpEvent instance for event");

  return 1 + this->FromEncodedImpl(encoded.substr(1), con);
}

void IJdwpEvent::Dispatch(Handler& handler) {
  this->DispatchImpl(handler);
}

}  // namespace roastery

