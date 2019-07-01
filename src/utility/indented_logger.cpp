/**
 * This file is part of EgilSCIM.
 *
 * EgilSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EgilSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with EgilSCIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "indented_logger.hpp"

void indented_logger::open(const char* filename) {
    of.open(filename, std::ios_base::out | std::ios_base::trunc);
}

void indented_logger::log(const std::string& str) {
    of << std::string(indentation, ' ') << str << "\n";
}

void indented_logger::indent() {
    indentation += INDENT;
}
void indented_logger::unindent() {
    indentation -= INDENT;
}
