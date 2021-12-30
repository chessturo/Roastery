/* Provides an entry point and CLI interactions
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

#include "roast.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

#include "jdwp_con.hpp"
#include "jdwp_packet.hpp"

using namespace roastery;

class PrintHandler : public Handler {
  public:
    using Handler::Handle;
    void Handle(IJdwpEvent& event) override {
      std::cout << "Event kind: " << std::hex <<
        static_cast<uint8_t>(event.GetKind()) << std::endl;
    }
};

int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);
  auto r = JdwpCon("127.0.0.1", 3262);
  r.RegisterEventHandler(std::make_unique<PrintHandler>());
  r.SendMessage(
      std::make_unique<command_packets::virtual_machine::VersionCommand>());
  std::cout << "Press enter to exit" << std::endl;
  std::cin.get();
  return EXIT_SUCCESS;
}

