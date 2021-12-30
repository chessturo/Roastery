/* Implements an abstraction for a socket connected to a JDWP server
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

#include "jdwp_socket.hpp"

#include <arpa/inet.h>
#include <fstream>
#include <netdb.h>
#include <poll.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>

#include "jdwp_exception.hpp"

namespace {

using std::string;

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
int Connect(const string& address, uint16_t portHost) {
  using std::system_error;
  using std::generic_category;

  port_t port = htons(portHost);

  int sock_fd = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
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

}  // namespace

using std::mutex;
using std::lock_guard;

namespace roastery {

/**
 * Implementation of \c JdwpSocket. \c Read and \c Write are thread-safe, and
 * can be used simultaneously by multiple threads.
 */
class JdwpSocket::Impl {
  public:
    explicit Impl(uint16_t port) : Impl("localhost", port) { }
    explicit Impl(const string& address, uint16_t port) :
        sock_fd(Connect(address, port)) {
      this->Write(kJdwpHandshake);
      string reply = this->Read(kJdwpHandshake.length());
      if (reply != kJdwpHandshake) {
        close(this->sock_fd);
        throw roastery::JdwpException("Bad handshake reply");
      }
    }

    // No copies/default constructor
    Impl() = delete;
    Impl(const Impl& copy) = delete;
    Impl& operator=(const Impl& other) = delete;

    // Not moveable
    Impl(Impl&& other) = delete;
    Impl& operator=(Impl&& other) = delete;

    /**
     * Destroy \c this and clean up any associated resources.
     */
    ~Impl() {
      close(this->sock_fd);
    }

    /**
     * Writes \c data to the connected server. Uses \c MSG_NOSIGNAL to avoid
     * \c SIGPIPE, but this is non-portable. \c SIGPIPE should be ignored
     * elsewhere to ensure the program doesn't crash if a connection is closed
     * unexpectedly.
     *
     * This method is thread-safe, it can be invoked concurrently by multiple
     * callers.
     *
     * @param data The data to write.
     *
     * @throws roastery::JdwpException if the connection is closed.
     * @throws std::system_error if there is an error writing to the server,
     * other than a closed connection.
     * @throws std::logic_error if this socket is not currently connected.
     */
    void Write(const string& data) {
      if (this->sock_fd < 0)
        throw std::logic_error("Cannot write while not connected");

      size_t bytes_written = 0;
      ssize_t written_this_call = 0;
      {  // critical segment, acquire lock_guard
        lock_guard<mutex> lck(write_lock);
        while (bytes_written < data.length()) {
          written_this_call = send(this->sock_fd, data.c_str() + bytes_written,
              data.length() - bytes_written, MSG_NOSIGNAL);
          if (written_this_call < 0) {
            if (errno == EPIPE) {
              this->Close();
              throw roastery::JdwpException("Connection closed");
            }
            if (errno != EAGAIN && errno != EINTR)
              throw std::system_error(errno, std::generic_category());
          }

          bytes_written += written_this_call;
        }
      }
    }

    /**
     * Returns whether or not there is data available to be read on this socket.
     *
     * @throws std::system_error if there is an issue polling the file
     * descriptor.
     * @throws std::logic_error if the socket is not currently connected.
     */
    bool CanRead() {
      if (this->sock_fd < 0)
        throw std::logic_error("Cannot poll while not connected");

      struct pollfd query = {
        .fd = this->sock_fd,
        .events = POLLIN,
        .revents = 0,
      };

      int res = poll(&query, 1, 0);
      if (res < 0 || query.revents & POLLERR) {
        throw std::system_error(errno, std::generic_category());
      }

      return query.revents & POLLIN;
    }

    /**
     * Reads \c len bytes from the server and returns it as a \c string.
     *
     * This method is thread-safe, it can be invoked concurrently by multiple
     * callers. This method will block until \c len bytes of data have been
     * read, the connection closes, or there is an IO error. This means that
     * if many bytes of data are requested, the call may block until the VM
     * writes a large amount of data.
     *
     * @param len The number of bytes to read.
     *
     * @throws roastery::JdwpException if the connection is closed.
     * @throws std::system_error if there is an error reading from the server.
     * @throws std::logic_error if the socket is not currently connected.
     *
     * @returns The data read.
     */
    string Read(size_t len) {
      if (this->sock_fd < 0)
        throw std::logic_error("Cannot read while not connected");

      size_t bytes_read = 0;
      ssize_t read_this_call = 0;
      string out = "";
      char buf[BUFSIZ + 1];
      {  // critical segment, acquire lock_guard
        lock_guard<mutex> lck(read_lock);
        while (bytes_read < len) {
          read_this_call = read(this->sock_fd, buf,
              std::min(static_cast<size_t>(BUFSIZ), len - bytes_read));
          if (read_this_call < 0 && errno != EAGAIN && errno != EINTR)
            throw std::system_error(errno, std::generic_category());
          if (read_this_call == 0) {
            this->Close();
            throw roastery::JdwpException("Connection closed");
          }
          out.append(buf, read_this_call);
          bytes_read += read_this_call;
        }
      }

      return out;
    }
  protected:
    /**
     * Closes the socket associated with \c this.
     */
    void Close() {
      close(this->sock_fd);
      this->sock_fd = -1;
    }

  private:
    mutable mutex read_lock;
    mutable mutex write_lock;
    const string kJdwpHandshake = "JDWP-Handshake";
    int sock_fd;
};

JdwpSocket::JdwpSocket(uint16_t port) : pImpl(new Impl(port)) { }
JdwpSocket::JdwpSocket(const string& address, uint16_t port) :
    pImpl(new Impl(address, port)) { }

JdwpSocket::JdwpSocket(JdwpSocket&& other) noexcept = default;
JdwpSocket& JdwpSocket::operator=(JdwpSocket&& other) noexcept = default;

JdwpSocket::~JdwpSocket() = default;

void JdwpSocket::Write(const string& data) { this->pImpl->Write(data); }
bool JdwpSocket::CanRead() { return this->pImpl->CanRead(); }
string JdwpSocket::Read(size_t len) { return this->pImpl->Read(len); }

}

