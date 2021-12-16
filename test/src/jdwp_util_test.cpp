/* Provides tests for misc utility functions.
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

#include <tuple>
#include <vector>

#include "gtest/gtest.h"

#include "jdwp_packet.hpp"

using std::tuple;
using std::vector;

using namespace roastery;
using namespace roastery::impl;


TEST(UtilTest, TupleForEachEmpty) {
  vector<string> res;
  tuple<> t = tuple();
  TupleForEach(t, [&](auto s) {
    static_cast<void>(s);
    FAIL() << "Empty tuple shouldn't call functor";
  });
}

TEST(UtilTest, TupleForEachOneElt) {
  vector<string> res;
  tuple<string> t = tuple<string>("test");
  TupleForEach(t, [&](auto s) {
    res.push_back(s);
  });
  EXPECT_EQ(std::get<0>(t), res[0]);
}

TEST(UtilTest, TupleForEachManyElt) {
  vector<string> res;
  tuple<string, string, string> t = { "test0", "test1", "test2" };
  TupleForEach(t, [&](auto s) {
    res.push_back(s);
  });
  EXPECT_EQ(std::get<0>(t), "test0");
  EXPECT_EQ(std::get<1>(t), "test1");
  EXPECT_EQ(std::get<2>(t), "test2");
}

TEST(UtilTest, IsVectorTest) {
  EXPECT_TRUE(IsVector<vector<int>>::value);
  EXPECT_TRUE(IsVector<vector<vector<int>>>::value);
  EXPECT_TRUE(IsVector<vector<string>>::value);
  EXPECT_FALSE(IsVector<int>::value);
  EXPECT_FALSE(IsVector<string>::value);
  EXPECT_FALSE(IsVector<JdwpString>::value);
}

