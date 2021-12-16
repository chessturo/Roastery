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

#include "jdwp_con.hpp"

#include <cstdint>
#include <string>

#include "jdwp_socket.hpp"

namespace roastery {

// IJdwpCon Housekeeping
IJdwpCon::~IJdwpCon() { }

/**
 * Implementation of \c JdwpCon.
 */
class JdwpCon::Impl {
  public:
    /**
     * Creates a new \c JdwpCon::Impl, connected to \c localhost.
     *
     * @param port The port to connect on. Should be in host byte order.
     * 
     * @throws std::system_error if there is a system error
     * creating/reading/writing to the socket created.
     */
    explicit Impl(uint16_t port) : Impl("localhost", port) { }

    /**
     * Creates a new \c JdwpCon::Impl, connected to \c address.
     *
     * @param address The address to connect to. Should just be a host name,
     * not a URI.
     * @param port The port to connect on. Should be in host byte order.
     *
     * @throws std::system_error if there is a system error
     * creating/reading/writing to the socket created.
     */
    explicit Impl(const string& address, uint16_t port) :
        socket(new JdwpSocket(address, port)) { }

    // No copies/default constructor
    Impl() = delete;
    Impl(const Impl& copy) = delete;
    Impl& operator=(const Impl& other) = delete;

    // Not moveable
    Impl(Impl&& other) = delete;
    Impl& operator=(Impl&& other) = delete;

    ~Impl() = default;

#warning Fix Get(type)IdSize to use messages once implemented
    /**
     * Returns the size of an \c objectID on the connected VM, in bytes.
     */
    uint8_t GetObjIdSize() { return 0; }
    /**
     * Returns the size of a \c methodID on the connected VM, in bytes.
     */
    uint8_t GetMethodIdSize() { return 0; }
    /**
     * Returns the size of a \c fieldID on the connected VM, in bytes.
     */
    uint8_t GetFieldIdSize() { return 0; }
    /**
     * Returns the size of a \c frameID on the connected VM, in bytes.
     */
    uint8_t GetFrameIdSize() { return 0; }
  private:
    unique_ptr<JdwpSocket> socket;

};

JdwpCon::JdwpCon(uint16_t port) :
  pImpl(new JdwpCon::Impl(port)) { }
JdwpCon::JdwpCon(const string& address, uint16_t port) :
  pImpl(new JdwpCon::Impl(address, port)) { }

JdwpCon::JdwpCon(JdwpCon&& other) noexcept = default;
JdwpCon& JdwpCon::operator=(JdwpCon&& other) noexcept = default;

JdwpCon::~JdwpCon() = default;

uint8_t JdwpCon::GetObjIdSize() { return this->pImpl->GetObjIdSize(); }
uint8_t JdwpCon::GetMethodIdSize() { return this->pImpl->GetMethodIdSize(); }
uint8_t JdwpCon::GetFieldIdSize() { return this->pImpl->GetFieldIdSize(); }
uint8_t JdwpCon::GetFrameIdSize() { return this->pImpl->GetFrameIdSize(); }

}  // namespace roastery

