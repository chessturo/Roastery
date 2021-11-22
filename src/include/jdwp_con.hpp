/* Provides an abstraction for the Java Debug Wire Protocol
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

#ifndef ROASTERY_JDWP_CON_H_
#define ROASTERY_JDWP_CON_H_

#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

using std::unique_ptr;
using std::string;

namespace roastery {

/**
 * Represents a connection to a JDWP server (e.g., a JVM).
 */
class JdwpCon {
  public:
    /**
     * Create a JDWP connection with \c localhost on \c port.
     *
     * @throws std::system_error if there is a system error creating a
     * connection to \c port.
     */
    explicit JdwpCon(uint16_t port);
    /**
     * Create a JDWP connection with \c address on \c port.
     *
     * @throws std::system_error if there is a system error connecting to
     * \c address on \c port.
     */
    explicit JdwpCon(const string& address, uint16_t port);

    // No copies/default constructor
    JdwpCon() = delete;
    JdwpCon(const JdwpCon& copy) = delete;
    JdwpCon& operator=(const JdwpCon& other) = delete;

    // Moveable
    /**
     * Move \c other to \c this.
     */
    JdwpCon(JdwpCon&& other) noexcept;
    /**
     * Move \c other to \c this.
     */
    JdwpCon& operator=(JdwpCon&& other) noexcept;

    /**
     * Destroy \c this and clean up any associated resources.
     */
    ~JdwpCon();
  private:
    class Impl;
    unique_ptr<Impl> pImpl;
};

}  // namespace roastery

#endif  // ROASTERY_JDWP_CON_H_

