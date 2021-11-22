/* Implements an exception related to JDWP operations.
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

#include "jdwp_exception.hpp"

#include <stdexcept>
#include <string>

namespace roastery {

// JdwpException
using std::runtime_error;

JdwpException::JdwpException(const std::string& what_arg) :
  runtime_error(what_arg) { }

JdwpException::JdwpException(const char* what_arg) :
  runtime_error(what_arg) { }

JdwpException::JdwpException(const JdwpException& other) :
  runtime_error(other) { }

JdwpException& JdwpException::operator=(const JdwpException& other) {
  if (this != &other) {
    runtime_error::operator=(other);
  }
  return *this;
}

const char* JdwpException::what() const noexcept {
  return runtime_error::what();
}

}  // namespace roastery

