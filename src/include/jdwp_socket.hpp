/* Provides an abstraction for a socket connected to a JDWP server
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

#ifndef ROASTERY_JDWP_SOCKET_H_
#define ROASTERY_JDWP_SOCKET_H_

#include <memory>
#include <string>

namespace roastery {

/**
 * Represents a socket connected to a JDWP server, ready to send/recieve
 * messages.
 */
class JdwpSocket {
  public:
    /**
     * Create a connection to \c localhost on \c port.
     *
     * @throws std::system_error if there is a system error creating a
     * connection to \c port.
     */
    explicit JdwpSocket(uint16_t port);
    /**
     * Create a connection to \c address on \c port.
     *
     * @throws std::system_error if there is a system error connecting to
     * \c address on \c port.
     */
    explicit JdwpSocket(const std::string& address, uint16_t port);
    
    // Disable copies/default constructor
    JdwpSocket() = delete;
    JdwpSocket(const JdwpSocket& copy) = delete;
    JdwpSocket& operator=(const JdwpSocket& other) = delete;

    // Moveable
    /**
     * Move \c other to \c this.
     */
    JdwpSocket(JdwpSocket&& other) noexcept;
    /**
     * Move \c other to \c this.
     */
    JdwpSocket& operator=(JdwpSocket&& other) noexcept;

    /**
     * Destroy \c this and clean up any associated resources.
     */
    ~JdwpSocket();

    /**
     * Writes \c data to the connected server.
     *
     * @param data The data to write.
     *
     * @throws roastery::JdwpException if the connection is closed.
     * @throws std::system_error if there is an error writing to the server,
     * other than a closed connection.
     * @throws std::logic_error if this socket is not currently connected.
     */
    void Write(const std::string& data);
    /**
     * Reads \c len bytes from the server and returns it as a string.
     *
     * @returns The data read.
     *
     * @throws roastery::JdwpException if the connection is closed.
     * @throws std::system_error if there is an error reading from the socket.
     * @throws std::logic_error if the socket is not currently connected.
     */
    std::string Read(size_t len);
  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif  // ROASTERY_JDWP_SOCKET_H_

