/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2025 Föreningen Sambruk
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

#ifndef EGILSCIM_ADVISORY_FILE_LOCK_HPP
#define EGILSCIM_ADVISORY_FILE_LOCK_HPP

#include <memory>
#include <string>
#include <boost/interprocess/sync/named_mutex.hpp>

/** This is an advisory file lock for the cache file.
  * Internally this uses boost's named_mutex to coordinate
  * access to the cache file, but in a pragmatic way to
  * suit our needs for the cache file.
  * 
  * Motivation:
  * 
  * 1. We really want to avoid being blocked from writing the new cache file.
  * 2. Under normal circumstances there should be little risk of having to 
  *    wait long for access to the file.
  * 3. There could be other processes accessing the file (such as a backup script)
  *    which won't respect our advisory locking anyway, so the writing to the
  *    cache file will anyway need to implement a retry in case the filesystem
  *    blocks the rename when we're done writing.
  * 4. boost's named_mutex can throw an interprocess_error, if the interprocess
  *    implementation isn't working for some reason we don't want that to be
  *    enough of a reason not to write the cache file.
  * 5. If we fail to get the lock within a reasonable time, the most likely explanation
  *    is not that others are accessing it, but that the named_mutex hasn't been properly
  *    released (for instance it the process is hard killed while holding the mutex).
  * 
  * All this put together leads to an implementation which tries to take the
  * named mutex but with a timeout. If we fail to take the mutex (either by timeout or
  * a problem with the interprocess system), we'll carry on as if we have the mutex
  * (and remove it from the system so we won't need to wait next time).
  * 
  * In other words, under normal circumstances this will block multiple instances of
  * EgilSCIMClient from accessing the file at once, but it's not a 100% guarantee.
  * We're ok with that since we're anyway retrying the writes.
  */
class AdvisoryFileLock {
public:
    /*
     * path is the file to lock.
     * timeout is given in seconds.
     */
    AdvisoryFileLock(const std::string& path, int timeout);
    virtual ~AdvisoryFileLock();

private:
    std::unique_ptr<boost::interprocess::named_mutex> m_pmtx;
    int m_timeout;
};

#endif // EGILSCIM_ADVISORY_FILE_LOCK_HPP
