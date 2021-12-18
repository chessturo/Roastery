/* Provides tests for `jdwp_packet.hpp` and `jdwp_packet.cpp`
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

#include <arpa/inet.h>

#include <array>
#include <sstream>
#include <string>

#include "gtest/gtest.h"

#include "jdwp_packet.hpp"
#include "mock_jdwp_con.hpp"

using namespace roastery;
using namespace roastery::test;

using impl::kHeaderLen;

using testing::Return;

namespace {

using std::array;
using std::ostringstream;
using std::string;

array<unsigned char, kHeaderLen> MakeCommandPacketHeader(uint32_t len,
    uint32_t id, uint8_t flags, uint8_t command_set, uint8_t command) {
  array<unsigned char, kHeaderLen> res;
  auto itr = res.begin();

  uint32_t len_nbo = htonl(len);
  uint32_t id_nbo = htonl(id);
  const unsigned char* len_nbo_bytes =
    reinterpret_cast<unsigned char*>(&len_nbo);
  const unsigned char* id_nbo_bytes =
    reinterpret_cast<unsigned char*>(&id_nbo);

  for (size_t i = 0; i < sizeof(len_nbo); i++) {
    *itr = len_nbo_bytes[i];
    ++itr;
  }
  for (size_t i = 0; i < sizeof(id_nbo); i++) {
    *itr = id_nbo_bytes[i];
    ++itr;
  }
  *itr = flags; ++itr;
  *itr = command_set; ++itr;
  *itr = command; ++itr;

  return res;
}

/**
 * Uses \c EXPECT_* family macros to assert that \c size bytes from \c data
 * onwards match the bytes in \c arr byte-for-byte.
 */
template<size_t size>
void ExpectBytesEq(void* data, const std::array<unsigned char, size>& arr) {
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(*(reinterpret_cast<unsigned char*>(data) + i), arr[i]) 
      << "Bytes differ at: " << i;
  }
}

}  // namespace

using namespace roastery::command_packets;
using namespace roastery::command_packets::virtual_machine;

using std::get;

TEST(PacketTest, NoFields) {
  // We test this using the VirtualMachine Version command because it has no
  // out-going fields.
  VersionCommand packet;
  MockJdwpCon con;

  uint32_t id = packet.GetId();

  string encoded = packet.Serialize(con);

  ASSERT_EQ(encoded.length(), kHeaderLen);
  ExpectBytesEq(encoded.data(), MakeCommandPacketHeader(impl::kHeaderLen, id,
        static_cast<uint8_t>(JdwpFlags::kNone),
        static_cast<uint8_t>(commands::CommandSet::kVirtualMachine),
        static_cast<uint8_t>(commands::VirtualMachine::kVersion)));
}

TEST(PacketTest, OneField) {
  // We test this using the VirtualMachine ClassesBySignature command because
  // it has one out-going field.
  ClassesBySignatureCommand packet;

  MockJdwpCon con;

  uint32_t id = packet.GetId();
  auto& fields = packet.GetFields();
  JdwpString fieldContent;
  fieldContent << "Ljava/lang/String;";
  get<0>(fields) = fieldContent;

  string encoded = packet.Serialize(con);

  ASSERT_EQ(encoded.length(), kHeaderLen + fieldContent.Serialize(con).length());
  // Check that the header is correct
  ExpectBytesEq(encoded.data(), MakeCommandPacketHeader(encoded.length(), id,
        static_cast<uint8_t>(JdwpFlags::kNone),
        static_cast<uint8_t>(commands::CommandSet::kVirtualMachine),
        static_cast<uint8_t>(commands::VirtualMachine::kClassesBySignature)));
  // Check that the string survived
  JdwpString afterDeserialization;
  afterDeserialization.FromEncoded(encoded.substr(kHeaderLen), con);
  EXPECT_EQ(fieldContent.GetValue(), afterDeserialization.GetValue());
}

TEST(PacketTest, Vector) {
  // We test this using the VirtualMachine DisposeObjects command because it has
  // one field, a vector of tuples
  DisposeObjectsCommand packet;

  MockJdwpCon con;
  EXPECT_CALL(con, GetObjIdSizeImpl).WillRepeatedly(Return(8));

  uint32_t id = packet.GetId();
  auto& fields = packet.GetFields();
  JdwpInt num_reqs; size_t num_reqs_len = num_reqs.value_size;
  JdwpObjId obj_id; size_t obj_id_len = obj_id.value_size;
  JdwpInt ref_cnt; size_t ref_cnt_len = ref_cnt.value_size;
  num_reqs << 4;
  obj_id << 0xDEADBEEFCAFEF00Dull;
  ref_cnt << 1;
  for (int i = 0; i < num_reqs.GetValue(); i++) {
    get<0>(fields).push_back({ obj_id, ref_cnt });
  }

  string encoded = packet.Serialize(con);

  ASSERT_EQ(encoded.length(),
      kHeaderLen + num_reqs_len +
      (obj_id_len + ref_cnt_len) * num_reqs.GetValue());
  // Check the header
  ExpectBytesEq(encoded.data(), MakeCommandPacketHeader(encoded.length(), id,
        static_cast<uint8_t>(JdwpFlags::kNone),
        static_cast<uint8_t>(commands::CommandSet::kVirtualMachine),
        static_cast<uint8_t>(commands::VirtualMachine::kDisposeObjects)));
  // Check that each field survived
  JdwpInt new_num_reqs;
  new_num_reqs.FromEncoded(encoded.substr(kHeaderLen), con);
  EXPECT_EQ(num_reqs.GetValue(), new_num_reqs.GetValue());
}

using namespace event_request;

TEST(PacketTest, EventRequestSet) {
  SetCommand packet;

  MockJdwpCon con;
  EXPECT_CALL(con, GetObjIdSizeImpl).WillRepeatedly(Return(8));

  uint32_t id = packet.GetId();
  auto& fields = packet.GetFields();
  JdwpByte event_kind; event_kind << 1;
  JdwpByte suspend_policy; suspend_policy << 2;

  vector<SetCommand::Modifier> modifiers;

  SetCommand::Modifier mod1; 
  JdwpInt count; count << 0;
  mod1.emplace<0>(count);
  modifiers.push_back(mod1);

  SetCommand::Modifier mod2;
  JdwpReferenceTypeId ref; ref << 0xDEADBEEFCAFEF00Dull;
  JdwpBool caught; caught << true;
  JdwpBool uncaught; uncaught << false;
  mod2.emplace<7>(tuple<JdwpReferenceTypeId, JdwpBool, JdwpBool>(
        { ref, caught, uncaught }));
  modifiers.push_back(mod2);

  fields = { event_kind, suspend_policy, modifiers};

  string encoded = packet.Serialize(con);
  ExpectBytesEq(encoded.data(), MakeCommandPacketHeader(encoded.length(), id,
        static_cast<uint8_t>(JdwpFlags::kNone),
        static_cast<uint8_t>(commands::CommandSet::kEventRequest),
        static_cast<uint8_t>(commands::EventRequest::kSet)));

  // Make sure all the fields survived in the right order
  size_t curr_byte = kHeaderLen;

  JdwpByte new_event_kind;
  new_event_kind.FromEncoded(encoded.substr(curr_byte), con);
  EXPECT_EQ(new_event_kind.GetValue(), event_kind.GetValue());
  curr_byte += new_event_kind.value_size;
  
  JdwpByte new_suspend_policy;
  new_suspend_policy.FromEncoded(encoded.substr(curr_byte), con);
  EXPECT_EQ(new_suspend_policy.GetValue(), suspend_policy.GetValue());
  curr_byte += new_suspend_policy.value_size;

  JdwpInt modifier_len;
  modifier_len.FromEncoded(encoded.substr(curr_byte), con);
  EXPECT_EQ(static_cast<size_t>(modifier_len.GetValue()), modifiers.size());
  curr_byte += modifier_len.value_size;

  JdwpByte mod1kind;
  mod1kind.FromEncoded(encoded.substr(curr_byte), con);
  EXPECT_EQ(mod1kind.GetValue(), 1);
  curr_byte += mod1kind.value_size;

  JdwpInt new_count;
  new_count.FromEncoded(encoded.substr(curr_byte), con);
  EXPECT_EQ(new_count.GetValue(), count.GetValue());
  curr_byte += new_count.value_size;

  JdwpByte mod2kind;
  mod2kind.FromEncoded(encoded.substr(curr_byte), con);
  EXPECT_EQ(mod2kind.GetValue(), 8);
  curr_byte += mod2kind.value_size;

  JdwpReferenceTypeId new_ref;
  new_ref.FromEncoded(encoded.substr(curr_byte), con);
  EXPECT_EQ(new_ref.GetValue(), ref.GetValue());
  curr_byte += new_ref.value_size;

  JdwpBool new_caught;
  new_caught.FromEncoded(encoded.substr(curr_byte), con);
  EXPECT_EQ(new_caught.GetValue(), caught.GetValue());
  curr_byte += new_caught.value_size;

  JdwpBool new_uncaught;
  new_uncaught.FromEncoded(encoded.substr(curr_byte), con);
  EXPECT_EQ(new_uncaught.GetValue(), uncaught.GetValue());
  curr_byte += new_uncaught.value_size;
}

