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

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <string>
#include <system_error>

namespace {

/**
 * Holds a port number in <em>network byte order</em>.
 */
typedef uint16_t port_t;

/**
 * Create a TCP connection to \c address on \c port.
 *
 * @param address the hostname to connect to.
 * @param port the port to connect on. Should be in host byte order.
 *
 * @return The file descriptor of the created socket.
 */
int Connect(string& address, uint16_t portHost) {
  using std::system_error;
  using std::generic_category;

  port_t port = htons(portHost);

  int sock_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if (sock_fd == -1) {
    throw system_error(errno, generic_category(), "Could not create a socket");
  }

  struct addrinfo *res;
  struct addrinfo hints = {
    // Meaningful values
    .ai_flags = AI_V4MAPPED,
    .ai_family = AF_INET6,
    .ai_socktype = SOCK_STREAM,
    .ai_protocol = IPPROTO_TCP,
    // Zero the rest
    .ai_addrlen = 0,
    .ai_addr = 0,
    .ai_canonname = 0,
    .ai_next = 0,
  };
  int gai_err = getaddrinfo(address.c_str(), nullptr, &hints, &res);
  if (gai_err != 0) {
    if (gai_err != EAI_SYSTEM) {
      throw system_error(gai_err, generic_category(), gai_strerror(gai_err));
    } else {
      throw system_error(errno, generic_category());
    }
  }

  int connect_result = -1;
  for (struct addrinfo* curr = res;
      connect_result == -1 && curr != nullptr;
      curr = curr->ai_next) {
    // Since we set hints to only have GAI return AF_INET6, this cast is always
    // fine; either we get struct sockaddr_in6 or GAI would've failed.
    reinterpret_cast<struct sockaddr_in6*>(curr->ai_addr)->sin6_port = port;
    connect_result = connect(sock_fd, curr->ai_addr, curr->ai_addrlen);
  }
  freeaddrinfo(res);
  // If we fail, just throw the errno from the last addr tried
  if (connect_result == -1) {
    throw system_error(errno, generic_category(),
        "Could not connect to " + address);
  }

  return sock_fd;
}

/**
 * Writes \c data to \c fd.
 *
 * @param fd The file descriptor to write to.
 * @param data The data to write.
 *
 * @throws std::invalid_argument if \c fd is less than \c 0.
 * @throws std::system_error if there is an error writing to \c fd.
 */
void WrappedWrite(int fd, const string& data) {
  if (fd < 0) throw std::invalid_argument("Negative file descriptor");

  size_t bytes_written = 0;
  ssize_t written_this_call = 0;
  while (bytes_written < data.length()) {
    written_this_call = write(fd, data.c_str() + bytes_written,
        data.length() - bytes_written);
    if (written_this_call < 0 && errno != EAGAIN && errno != EINTR) {
      throw std::system_error(errno, std::generic_category());
    }
    bytes_written += written_this_call;
  }
}

/**
 * Reads \c len bytes from \c fd.
 *
 * @param fd The file descriptor to read from.
 * @param len The number of bytes to read.
 *
 * @throws std::invalid_argument if \c fd is less than \c 0.
 * @throws std::system_error if there is an error writing to \c fd.
 */
string WrappedRead(int fd, size_t len) {
  if (fd < 0) throw std::invalid_argument("Negative file descriptor");

  size_t bytes_read = 0;
  ssize_t read_this_call = 0;
  string out = "";
  char *buf = new char[BUFSIZ + 1];
  while (bytes_read < len) {
    read_this_call = read(fd, buf, BUFSIZ);
    if (read_this_call < 0 && errno != EAGAIN && errno != EINTR) {
      throw std::system_error(errno, std::generic_category());
    }
    buf[read_this_call] = '\0';
    out.append(buf);
    bytes_read += read_this_call;
  }

  return out;
}

}  // namespace

namespace roastery {

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
    explicit Impl(string address, uint16_t port) :
        sock_fd(Connect(address, port)) {
      DoHandshake();
    }

    // No copies/default constructor
    Impl() = delete;
    Impl(const Impl& copy) = delete;
    Impl& operator=(const Impl& other) = delete;

    // Not moveable
    Impl(Impl&& other) = delete;
    Impl& operator=(Impl&& other) = delete;

    ~Impl() {
      if (this->sock_fd != -1) close(this->sock_fd);
    }
  private:
    /**
     * File descriptor for the socket connected to the server.
     *
     * Set to \c -1 when not connected.
     */
    int sock_fd;

    const string kJdwpHandshake = "JDWP-Handshake";

    /**
     * Does the JDWP handshake over \c this->sock_fd.
     *
     * @throws std::logic_error if \c this->sock_fd \c == \c -1.
     * @throws roastery::JdwpException if an unexpected reply is recieved.
     */
    void DoHandshake() {
      if (this->sock_fd == -1) {
        throw std::logic_error("Cannot do handshake while not connected");
      }
      WrappedWrite(this->sock_fd, kJdwpHandshake);
      string reply = WrappedRead(this->sock_fd, kJdwpHandshake.length());
      if (reply != kJdwpHandshake) {
        throw roastery::JdwpException("Bad handshake reply");
      }
    }
};

JdwpCon::JdwpCon(uint16_t port) :
  pImpl(new JdwpCon::Impl(port)) { }

JdwpCon::JdwpCon(string address, uint16_t port) :
  pImpl(new JdwpCon::Impl(address, port)) { }

JdwpCon::JdwpCon(JdwpCon&& other) noexcept {
  this->pImpl = std::move(other.pImpl);
}

JdwpCon& JdwpCon::operator=(JdwpCon&& other) noexcept {
  this->pImpl = std::move(other.pImpl);
  return *this;
}

JdwpCon::~JdwpCon() { }

// JdwpException
using std::runtime_error;

JdwpException::JdwpException(const string& what_arg) :
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

}  // namespace Roastery

