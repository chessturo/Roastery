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

using namespace roastery;

using impl::kHeaderLen;

namespace {

using std::array;
using std::ostringstream;
using std::string;

array<unsigned char, kHeaderLen> MakeCommandPacketHeader(uint32_t len, uint32_t id,
    uint8_t flags, uint8_t command_set, uint8_t command) {
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

TEST(PacketTest, NoFields) {
  // We test this using the VirtualMachine Version command because it has no
  // out-going fields.
  VirtualMachineVersionCommandPacket packet;
  uint32_t id = packet.GetId();

  string encoded = packet.Serialize();

  ASSERT_EQ(encoded.length(), kHeaderLen);
  ExpectBytesEq(encoded.data(), MakeCommandPacketHeader(impl::kHeaderLen, id,
        static_cast<uint8_t>(JdwpFlags::kNone), 0x01, 0x01));
}

TEST(PacketTest, OneField) {
  // We test this using the VirtualMachine ClassesBySignature command because
  // it has one out-going field.
  VirtualMachineClassesBySignatureCommandPacket packet;
  uint32_t id = packet.GetId();
  auto& fields = packet.GetFields();
  JdwpString fieldContent = JdwpString::fromHost("Ljava/lang/String;");
  std::get<0>(fields) = fieldContent;

  string encoded = packet.Serialize();

  ASSERT_EQ(encoded.length(), kHeaderLen + fieldContent.Serialize().length());
  // Check that the header is correct
  ExpectBytesEq(encoded.data(), MakeCommandPacketHeader(encoded.length(), id,
        static_cast<uint8_t>(JdwpFlags::kNone), 0x01, 0x02));
  // Check that the string survived
  JdwpString afterDeserialization =
    JdwpString::fromSerialized(encoded.substr(kHeaderLen));
  EXPECT_EQ(fieldContent.data, afterDeserialization.data);
}

