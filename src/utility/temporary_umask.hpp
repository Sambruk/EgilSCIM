/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2021 Föreningen Sambruk
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

#ifndef EGILSCIM_TEMPORARY_UMASK_HPP
#define EGILSCIM_TEMPORARY_UMASK_HPP


// When possible, we use umask to limit access to the cache file.
#ifdef __unix__
#include <sys/types.h>
#include <sys/stat.h>

class temporary_umask {
public:
    temporary_umask(mode_t new_mask) {
        old_mask = umask(new_mask);
    }
    
    ~temporary_umask() {
        umask(old_mask);
    }

private:
    mode_t old_mask;
};

#else // Non-unix systems have a no-op as temporary_umask

class temporary_umask {
public:
    temporary_umask(int) {}
    ~temporary_umask() {}
};
#endif // __unix__

#endif // EGILSCIM_TEMPORARY_UMASK_HPP
