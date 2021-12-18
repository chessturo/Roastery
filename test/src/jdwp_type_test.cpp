/* Provides tests for `jdwp_type.hpp` and `jdwp_type.cpp`
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

#include <array>
#include <sstream>
#include <vector>

#include "gtest/gtest.h"

#include "jdwp_con.hpp"
#include "jdwp_type.hpp"
#include "mock_jdwp_con.hpp"

using namespace roastery;
using namespace roastery::test;

using ::testing::Return;
using std::array;
using std::stringstream;

namespace {

const size_t kObjectIdSize = 8;
const array<unsigned char, kObjectIdSize> kObjIdNBO = { 0x0D, 0xF0, 0xFE, 0xCA,
  0xEF, 0xBE, 0xAD, 0xDE };
const array<unsigned char, kObjectIdSize> kObjIdHBO = { 0xDE, 0xAD, 0xBE, 0xEF,
  0xCA, 0xFE, 0xF0, 0x0D };

const size_t kMethodIdSize = 8;
const array<unsigned char, kMethodIdSize> kMethodIdNBO = { 0x42, 0x42, 0x42,
  0x42, 0x1E, 0x0D, 0xF0, 0x15 };
const array<unsigned char, kMethodIdSize> kMethodIdHBO = { 0x15, 0xF0, 0x0D,
  0x1E, 0x42, 0x42, 0x42, 0x42 };

stringstream& operator<<(stringstream& s, JdwpTag t) {
  s << static_cast<char>(t);
  return s;
}
stringstream& operator<<(stringstream& s, JdwpTypeTag t) {
  s << static_cast<char>(t);
  return s;
}

/**
 * Uses \c EXPECT_* family macros to assert that \c size bytes from \c data
 * onwards match the bytes in \c arr byte-for-byte.
 */
template<size_t size>
void ExpectBytesEq(void* data, const array<unsigned char, size>& arr) {
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(*(reinterpret_cast<unsigned char*>(data) + i), arr[i]) 
      << "Bytes differ at: " << i;
  }
}

template<typename T>
string Stringify(const T& arr) {
  return string(std::begin(arr), std::end(arr));
}

}  // namespace

TEST(TypeTest, JdwpPrimativesTest) {
  MockJdwpCon con;

  JdwpInt i;
  i << 0x12345678;

  EXPECT_EQ(i.GetValue(), 0x12345678);
  ExpectBytesEq(i.Serialize(con).data(),
      array<unsigned char, 4> { 0x12, 0x34, 0x56, 0x78 });

  JdwpDouble d;
}

TEST(TypeTest, JdwpTaggedObjectIdTest) {
  MockJdwpCon con;
  EXPECT_CALL(con, GetObjIdSizeImpl).WillRepeatedly(Return(kObjectIdSize));

  stringstream tagged_obj_id_data;
  tagged_obj_id_data << JdwpTag::kObject << Stringify(kObjIdNBO);

  JdwpTaggedObjectId jtoi;
  jtoi.FromEncoded(tagged_obj_id_data.str(), con);

  EXPECT_EQ(JdwpTag::kObject, jtoi.tag);
  auto obj_id = jtoi.obj_id.GetValue();
  ExpectBytesEq(&obj_id, kObjIdHBO);
  EXPECT_EQ(tagged_obj_id_data.str(), jtoi.Serialize(con));
}

TEST(TypeTest, JdwpLocationTest) {
  // Setup the location index NBO/HBO
  array<unsigned char, 8> loc_index_HBO = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC,
    0xDE, 0xFF };
  string loc_index_NBO = string(std::begin(loc_index_HBO),
      std::end(loc_index_HBO));
  reverse(loc_index_NBO.begin(), loc_index_NBO.end());

  MockJdwpCon con;
  EXPECT_CALL(con, GetObjIdSizeImpl).WillRepeatedly(Return(kObjectIdSize));
  EXPECT_CALL(con, GetMethodIdSizeImpl).WillRepeatedly(Return(kMethodIdSize));

  stringstream loc;
  loc << JdwpTypeTag::kClass << Stringify(kObjIdNBO) << Stringify(kMethodIdNBO)
    << loc_index_NBO;

  JdwpLocation location;
  location.FromEncoded(loc.str(), con);

  EXPECT_EQ(location.type, JdwpTypeTag::kClass);
  auto class_id = location.class_id.GetValue();
  auto method_id = location.method_id.GetValue();
  auto index = location.index;
  ExpectBytesEq(&class_id, kObjIdHBO);
  ExpectBytesEq(&method_id, kMethodIdHBO);
  ExpectBytesEq(&index, loc_index_HBO);

  EXPECT_EQ(loc.str(), location.Serialize(con));
}

TEST(TypeTest, JdwpStringTest) {
  MockJdwpCon con;

  array<unsigned char, 4> str_len_NBO = { 0x00, 0x00, 0x00, 0x08 };
  const string kStr = "roastery";

  stringstream jdwp_str;
  jdwp_str << Stringify(str_len_NBO) << kStr;

  JdwpString str;
  str.FromEncoded(jdwp_str.str(), con);

  EXPECT_EQ(str.GetValue(), kStr);
  EXPECT_EQ(str.Serialize(con), jdwp_str.str());
}

TEST(TypeTest, JdwpValueTestObject) {
  MockJdwpCon con;
  EXPECT_CALL(con, GetObjIdSizeImpl).WillRepeatedly(Return(kObjectIdSize));

  stringstream jdwp_object;
  jdwp_object << static_cast<char>(JdwpTag::kObject) << Stringify(kObjIdNBO);

  JdwpValue object;
  object.FromEncoded(jdwp_object.str(), con);

  EXPECT_EQ(object.tag, JdwpTag::kObject);
  uint64_t obj_id = std::get<JdwpObjId>(object.value).GetValue();
  ExpectBytesEq(&obj_id, kObjIdHBO);

  EXPECT_EQ(object.Serialize(con), jdwp_object.str());
}

TEST(TypeTest, JdwpValueTestVoid) {
  MockJdwpCon con;

  stringstream jdwp_void;
  jdwp_void << static_cast<char>(JdwpTag::kVoid);

  JdwpValue vvoid;
  vvoid.FromEncoded(jdwp_void.str(), con);

  EXPECT_EQ(vvoid.tag, JdwpTag::kVoid);
  EXPECT_EQ(vvoid.Serialize(con), jdwp_void.str());
}

TEST(TypeTest, JdwpValueTestInt) {
  array<unsigned char, 4> int_NBO = { 0x78, 0x56, 0x34, 0x12 };
  array<unsigned char, 4> int_HBO; 
  std::reverse_copy(int_NBO.begin(), int_NBO.end(), int_HBO.begin());
  MockJdwpCon con;

  stringstream jdwp_int;
  jdwp_int << JdwpTag::kInt << Stringify(int_NBO);

  JdwpValue iint;
  iint.FromEncoded(jdwp_int.str(), con);

  EXPECT_EQ(iint.tag, JdwpTag::kInt);
  uint32_t int_val = std::get<JdwpInt>(iint.value).GetValue();
  ExpectBytesEq(&int_val, int_HBO);

  EXPECT_EQ(iint.Serialize(con), jdwp_int.str());
}

TEST(TypeTest, JdwpArrayRegionObjectTest) {
  array<unsigned char, 4> len_NBO = { 0x00, 0x00, 0x00, 0x04 };

  MockJdwpCon con;
  EXPECT_CALL(con, GetObjIdSizeImpl).WillRepeatedly(Return(kObjectIdSize));

  stringstream jdwp_arr_region;
  jdwp_arr_region << static_cast<char>(JdwpTag::kObject) <<
    Stringify(len_NBO) <<
      static_cast<char>(JdwpTag::kObject) << Stringify(kObjIdNBO) <<
      static_cast<char>(JdwpTag::kObject) << Stringify(kObjIdNBO) <<
      static_cast<char>(JdwpTag::kObject) << Stringify(kObjIdNBO) <<
      static_cast<char>(JdwpTag::kObject) << Stringify(kObjIdNBO);

  JdwpArrayRegion arr_region;
  arr_region.FromEncoded(jdwp_arr_region.str(), con);

  EXPECT_EQ(arr_region.tag, JdwpTag::kObject);
  ASSERT_EQ(arr_region.values->size(), (size_t)4);
  for (const unique_ptr<JdwpValue>& v : *arr_region.values) {
    EXPECT_EQ(v->tag, JdwpTag::kObject);
    uint64_t obj_id = std::get<JdwpObjId>(v->value).GetValue();
    ExpectBytesEq(&obj_id, kObjIdHBO);
  }
  EXPECT_EQ(arr_region.Serialize(con), jdwp_arr_region.str());
}

TEST(TypeTest, JdwpArrayRegionPrimativeTest) {
  array<unsigned char, 4> len_NBO = { 0x00, 0x00, 0x00, 0x04 };
  array<unsigned char, 4> int_NBO = { 0x78, 0x56, 0x34, 0x12 };
  array<unsigned char, 4> int_HBO; 
  std::reverse_copy(int_NBO.begin(), int_NBO.end(), int_HBO.begin());

  MockJdwpCon con;
  stringstream jdwp_arr_region;
  jdwp_arr_region << static_cast<char>(JdwpTag::kInt) <<
    Stringify(len_NBO) << Stringify(int_NBO) << Stringify(int_NBO) <<
    Stringify(int_NBO) << Stringify(int_NBO);

  JdwpArrayRegion arr_region;
  arr_region.FromEncoded(jdwp_arr_region.str(), con);

  EXPECT_EQ(arr_region.tag, JdwpTag::kInt);
  ASSERT_EQ(arr_region.values->size(), (size_t)4);
  for (const unique_ptr<JdwpValue>& v : *arr_region.values) {
    EXPECT_EQ(v->tag, JdwpTag::kInt);
    int32_t int_val = std::get<JdwpInt>(v->value).GetValue();
    ExpectBytesEq(&int_val, int_HBO);
  }
  EXPECT_EQ(arr_region.Serialize(con), jdwp_arr_region.str());
}

