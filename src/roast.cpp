/* Provides constants that are useful across the codebase
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

#include <cstdlib>
#include <iostream>

#include "jdwp.hpp"


int main(int argc, char *argv[]) {
  auto r = roastery::JdwpCon("127.0.0.1", 3262);
  return EXIT_SUCCESS;
}
