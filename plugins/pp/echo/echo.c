/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2020 Föreningen Sambruk
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.

 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pp_interface.h>
#include <stdlib.h>
#include <string.h>

extern int echo_init(int count, char **vars, char **values) {
    return 0;
}

extern int echo_include(const char *type) {
    return PP_PROCESS_TYPE;
}

extern int echo_process(const char *type, const char *input, char **output) {
    *output = strdup(input);
    return 0;
}

extern void echo_free(void *ptr) {
    free(ptr);
}

void echo_exit() {
}
