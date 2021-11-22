/* Provides an exception related to JDWP operations.
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

#include <stdexcept>
#include <string>

namespace roastery {

/**
 * Represents an exceptional state related to processing a JDWP connection.
 */
class JdwpException : public std::runtime_error {
  public:
    /**
     * Constructs a \c JdwpException with the given explanatory message.
     */
    JdwpException(const std::string& what_arg);
    /**
     * Constructs a \c JdwpException with the given explanatory message.
     */
    JdwpException(const char* what_arg);

    /**
     * Creates a copy of \c other.
     */
    JdwpException(const JdwpException& other);
    /**
     * Assigns the contents of \c this to be the contents \c other.
     */
    JdwpException& operator=(const JdwpException& other);

    /**
     * Returns an explanatory string.
     */
    virtual const char* what() const noexcept override;
};

}  // namespace roastery
