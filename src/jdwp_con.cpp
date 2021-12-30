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

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "jdwp_packet.hpp"
#include "jdwp_socket.hpp"

using std::lock_guard;
using std::map;
using std::mutex;
using std::queue;
using std::shared_ptr;
using std::thread;

namespace roastery {

// IJdwpCon Housekeeping
IJdwpCon::~IJdwpCon() = default;

/**
 * Implementation of \c JdwpCon.
 */
class JdwpCon::Impl : public IJdwpCon {
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
        socket(new JdwpSocket(address, port)),
        should_cancel(false),
        write_thread(&Impl::OutgoingMessageQueueHandler, this),
        read_thread(&Impl::IncomingMessageQueueHandler, this) { }

    // No copies/default constructor
    Impl() = delete;
    Impl(const Impl& copy) = delete;
    Impl& operator=(const Impl& other) = delete;

    // Not moveable
    Impl(Impl&& other) = delete;
    Impl& operator=(Impl&& other) = delete;

    ~Impl() override {
      this->should_cancel = true;
      if (write_thread.joinable()) {
        write_thread.join();
      }
      if (read_thread.joinable()) {
        read_thread.join();
      }
    }

  protected:
#warning Fix Get(type)IdSize to use messages once implemented
    /**
     * Returns the size of an \c objectID on the connected VM, in bytes.
     */
    uint8_t GetObjIdSizeImpl() override { return 0; }
    /**
     * Returns the size of a \c methodID on the connected VM, in bytes.
     */
    uint8_t GetMethodIdSizeImpl() override { return 0; }
    /**
     * Returns the size of a \c fieldID on the connected VM, in bytes.
     */
    uint8_t GetFieldIdSizeImpl() override { return 0; }
    /**
     * Returns the size of a \c frameID on the connected VM, in bytes.
     */
    uint8_t GetFrameIdSizeImpl() override { return 0; }

    /**
     * Registers the given \c handler, which will have the appropriate \c Handle
     * function invoked when an event packet is recieved.
     */
    void RegisterEventHandlerImpl(unique_ptr<Handler> handler) override {
      lock_guard<mutex> l(this->event_handlers_lck);
      this->event_handlers.push_back(move(handler));
    }

    /**
     * Queues the given message to be send to the JVM.
     *
     * @param message The message to send.
     */
    void SendMessageImpl(unique_ptr<IJdwpCommandPacket> message) override {
        lock_guard<mutex> l(this->outgoing_messages_lck);
        this->outgoing_messages.push(move(message));
    }
  private:
    unique_ptr<JdwpSocket> socket;

    std::atomic_bool should_cancel;
    thread write_thread, read_thread;
    queue<unique_ptr<IJdwpCommandPacket>> outgoing_messages;
    mutex outgoing_messages_lck;

    /**
     * Maps from message IDs to their replies.
     */
    std::map<uint32_t, shared_ptr<void>> incoming_messages;
    mutex incoming_messages_lck;

    /**
     * Hols all currently registered event handlers
     */
    vector<unique_ptr<Handler>> event_handlers;
    mutex event_handlers_lck;

    /**
     * Dispatches outgoing messages to \c socket.
     */
    void OutgoingMessageQueueHandler() {
      while (!this->should_cancel) {
        unique_ptr<IJdwpCommandPacket> next_packet;
        if (!this->outgoing_messages.empty()) {
          {
            lock_guard<mutex> l(outgoing_messages_lck);
            next_packet = move(this->outgoing_messages.front());
            this->outgoing_messages.pop();
          }
          this->socket->Write(next_packet->Serialize(*this));
        } else {
          std::this_thread::yield();
        }
      }
    }

    /**
     * Dispatches incoming messages to registered handlers.
     */
    void IncomingMessageQueueHandler() {
      while (!this->should_cancel) {
        if (this->socket->CanRead()) {
          string header = this->socket->Read(impl::kHeaderLen);
          uint32_t total_len = ntohl(
              // Read the first four bytes of the header as a length.
              *reinterpret_cast<uint32_t*>(header.data()));
          string body = this->socket->Read(total_len - impl::kHeaderLen);
          string packet = header + body;
          if (HeaderIsEvent(header)) {
            auto events = IJdwpEvent::FromComposite(packet, *this);
            for (auto& event : events) {
              for (auto& handler : event_handlers) {
                event->Dispatch(*handler);
              }
            }
          } else {
            lock_guard<mutex> l(incoming_messages_lck);
            // add new message to the map
          }
        } else {
          std::this_thread::yield();
        }
      }
    }
};

uint8_t IJdwpCon::GetObjIdSize() { return this->GetObjIdSizeImpl(); }
uint8_t IJdwpCon::GetMethodIdSize() { return this->GetMethodIdSizeImpl(); }
uint8_t IJdwpCon::GetFieldIdSize() { return this->GetFieldIdSizeImpl(); }
uint8_t IJdwpCon::GetFrameIdSize() { return this->GetFrameIdSizeImpl(); }
void IJdwpCon::RegisterEventHandler(unique_ptr<Handler> handler) {
  this->RegisterEventHandlerImpl(move(handler));
}
void IJdwpCon::SendMessage(unique_ptr<IJdwpCommandPacket> message) {
  this->SendMessageImpl(move(message));
}

JdwpCon::JdwpCon(uint16_t port) :
  pImpl(new JdwpCon::Impl(port)) { }
JdwpCon::JdwpCon(const string& address, uint16_t port) :
  pImpl(new JdwpCon::Impl(address, port)) { }

JdwpCon::JdwpCon(JdwpCon&& other) noexcept = default;
JdwpCon& JdwpCon::operator=(JdwpCon&& other) noexcept = default;

JdwpCon::~JdwpCon() = default;

uint8_t JdwpCon::GetObjIdSizeImpl() { return this->pImpl->GetObjIdSize(); }
uint8_t JdwpCon::GetMethodIdSizeImpl() {
  return this->pImpl->GetMethodIdSize();
}
uint8_t JdwpCon::GetFieldIdSizeImpl() { return this->pImpl->GetFieldIdSize(); }
uint8_t JdwpCon::GetFrameIdSizeImpl() { return this->pImpl->GetFrameIdSize(); }
void JdwpCon::RegisterEventHandlerImpl(unique_ptr<Handler> handler) {
  this->pImpl->RegisterEventHandler(move(handler));
}
void JdwpCon::SendMessageImpl(unique_ptr<IJdwpCommandPacket> p) {
  return this->pImpl->SendMessage(move(p));
}

}  // namespace roastery

