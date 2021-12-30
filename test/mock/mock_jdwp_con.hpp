/* Provides a mock implementation of a `IJdwpCon`
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

#include <cstdint>

#include "gmock/gmock.h"

#include "jdwp_con.hpp"
#include "jdwp_packet.hpp"

namespace roastery {

namespace test {

class MockJdwpCon : public IJdwpCon {
  public:
    MOCK_METHOD(uint8_t, GetObjIdSizeImpl, (), (override));
    MOCK_METHOD(uint8_t, GetMethodIdSizeImpl, (), (override));
    MOCK_METHOD(uint8_t, GetFieldIdSizeImpl, (), (override));
    MOCK_METHOD(uint8_t, GetFrameIdSizeImpl, (), (override));
    MOCK_METHOD(void, RegisterEventHandlerImpl, (unique_ptr<Handler>),
        (override));
    MOCK_METHOD(void, SendMessageImpl, (unique_ptr<IJdwpCommandPacket>),
        (override));
};

}  // namespace test

}  // namespace roastery

