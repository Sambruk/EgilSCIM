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

#ifndef EGILSCIM_INDENTED_LOGGER_HPP
#define EGILSCIM_INDENTED_LOGGER_HPP

#include <fstream>

/*
 * An idented logger writes messages to a file with
 * indentation (for logging e.g. recursive processes such as our
 * LDAP load).
 */
class indented_logger {
public:
    void open(const char* filename);

    bool is_open() const { return of.is_open(); }

    void log(const std::string& str);

    void indent();
    void unindent();

    // Class for scope based indentation.
    class indenter {
    public:
        indenter(indented_logger& l) : logger(l) {
            logger.indent();
        }

        ~indenter() {
            logger.unindent();
        }

    private:
        indented_logger& logger;
    };
    
private:
    std::ofstream of;
    int indentation = 0;

    const int INDENT = 2;
};

#endif // EGILSCIM_INDENTED_LOGGER_HPP
