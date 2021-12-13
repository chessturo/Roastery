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

#include <mutex>

#include "jdwp_packet.hpp"

namespace roastery {

namespace impl {

uint32_t nextId = 0;
std::mutex nextIdLck;
uint32_t GetNextId() {
  uint32_t res;
  nextIdLck.lock();
  res = nextId;
  nextId++;
  nextIdLck.unlock();
  return res;
}

}  // namespace impl

// Defining pure virtual dtors for house keeping
IJdwpCommandPacket::~IJdwpCommandPacket() { }

}  // namespace roastery

