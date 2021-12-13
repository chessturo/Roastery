/* Specifies an abstraction for Java Debug Wire Protocol messaging
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

#ifndef ROASTERY_JDWP_PACKET_H_
#define ROASTERY_JDWP_PACKET_H_

#include <arpa/inet.h>

#include <cstdint>
#include <sstream>
#include <vector>

#include "jdwp_con.hpp"
#include "jdwp_exception.hpp"
#include "jdwp_type.hpp"

namespace roastery {

using std::string;

/**
 * Holds flag values for JDWP.
 */
enum class JdwpFlags : uint8_t {
  kNone = 0x00,
  kReply = 0x80,
};

/**
 * Holds a JDWP command packet. May be a command packet that originates from the
 * debugger, or one that originates from the JVM (an event packet).
 */
class IJdwpCommandPacket {
  public:
    virtual ~IJdwpCommandPacket() = 0;
};

// Template metaprogramming helpers
namespace impl {

using std::forward;
using std::get;
using std::tuple_size;

template <size_t rem>
class TupleForEachHelper {
  public:
    template <typename Tuple, typename Func>
    static void Exec(Tuple&& tuple, Func&& func) {
      constexpr size_t tuple_siz = tuple_size<std::decay_t<Tuple>>::value;
      static_assert(rem <= tuple_siz, "Bad size in TupleForEachHelper");
      constexpr size_t idx = tuple_siz - rem;
      func(get<idx>(tuple));
      TupleForEachHelper<rem - 1>
        ::Exec(forward<Tuple>(tuple), forward<Func>(func));
    }
};

// template recursion base case: 0 elements remaining
template <>
class TupleForEachHelper<0> {
  public:
    template <typename Tuple, typename Func>
    static void Exec(Tuple&& tuple, Func&& func) {
      static_cast<void>(tuple);
      static_cast<void>(func);
    }
};

/**
 * Runs a functor on each value in a \c std::tuple.
 *
 * @param tuple The tuple to iterate over
 * @param func The functor to execute on each value in \c tuple.
 */
template <typename Tuple, typename Func>
void TupleForEach(Tuple&& tuple, Func&& func) {
  TupleForEachHelper
    <tuple_size<std::decay_t<Tuple>>::value>
    ::Exec(forward<Tuple>(tuple), forward<Func>(func));
}

/**
 * Returns the next free ID for an outgoing packet.
 */
uint32_t GetNextId();

const uint32_t kHeaderLen = 11;
/**
 * Provides a base for types representing JDWP command packets.
 */
template <uint8_t command_set, uint8_t command, typename Fields>
class CommandPacketBase : public roastery::IJdwpCommandPacket {
  public:
    CommandPacketBase() : id(GetNextId()) { }

    Fields& GetFields() { return fields; }
    const Fields& GetFields() const { return fields; }

    uint32_t GetId() { return id; }

#warning TODO find a way to deserialize events into CommandPackets

    /**
     * Returns \c this serialized for transmission over JDWP.
     */
    string Serialize() const { return this->SerializeImpl(); }
    virtual ~CommandPacketBase() = 0;
  protected:
    /**
     * Generates a JDWP header for this command.
     *
     * @param body_len The length of the body/data in this JDWP packet. Should
     * not include the length of a JDWP command header. Should be in host
     * byte-order.
     * @param id The ID to be used in the header. Should be in host byte-order.
     */
    string ProduceHeader(size_t body_len, uint32_t id) const {
      if (body_len > UINT32_MAX - 11) {
        throw JdwpException("Body too long");
      }

      uint32_t len_nbo = htonl(static_cast<uint32_t>(body_len) + kHeaderLen);
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

    /**
     * Provides an implementation of \c Serialize(). Can be overriden by derived
     * classes for customization. By default, calls the \c Serialize() method
     * of each item in \c Fields and concatenates them.
     */
    virtual string SerializeImpl() const {
      std::stringstream res;
      TupleForEach(fields, [&res](auto s) {
        res << s.Serialize();
      });
      string body = res.str();

      return ProduceHeader(body.length(), this->id) + body;
    }
  private:
    uint32_t id;
    Fields fields;
};

template <uint8_t command_set, uint8_t command, typename Fields>
CommandPacketBase<command_set, command, Fields>::~CommandPacketBase() { }

}  // namespace impl

// Virtual Machine command set
using VirtualMachineVersionPacketFields = std::tuple<>;
class VirtualMachineVersionCommandPacket :
    public impl::CommandPacketBase<0x01, 0x01,
      VirtualMachineVersionPacketFields> { };

using VirtualMachineClassesBySignaturePacketFields =
  std::tuple<JdwpString>;
class VirtualMachineClassesBySignatureCommandPacket :
    public impl::CommandPacketBase<0x01, 0x02,
      VirtualMachineClassesBySignaturePacketFields> { };

}  // namespace roastery

#endif  // ROASTERY_JDWP_PACKET_H_

