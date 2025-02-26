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

#include "advisory_file_lock.hpp"
#include "utility/utils.hpp"
#include <filesystem>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace {
    /** Construct a name for the mutex based on the path.
    *   We'll canonicalize the path in case it has been given differently
    *   to different processes. We'll then generate a UUID based on that
    *   since path names are not valid mutex names (at least on Windows).
    */
    std::string mutex_name(const std::string& path) {
        std::string str;
        try {
            str = std::filesystem::weakly_canonical(std::filesystem::path(path)).string();
        }
        catch (const std::exception&) {
            str = path;
        }
        return uuid_util::instance().generate(str);
    }
}

AdvisoryFileLock::AdvisoryFileLock(const std::string& path, int timeout)
 : m_timeout(timeout) {
    // Creating or locking the mutex might throw in exotic situations,
    // we don't want that to block us from writing the cache file.
    try {
        const auto mtx_name = mutex_name(path);
        m_pmtx = std::make_unique<boost::interprocess::named_mutex>(boost::interprocess::open_or_create_t{}, mtx_name.c_str());
        auto locked = m_pmtx->timed_lock(boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(m_timeout));
        if (!locked) {
            // There really shouldn't be any congestion for this mutex, if we fail to lock
            // the most likely explanation is a problem with the mutex (perhaps a process that held
            // the mutex was hard killed before it could release the mutex).
            m_pmtx = nullptr;
            boost::interprocess::named_mutex::remove(mtx_name.c_str());
        }
    }
    catch (std::exception&) {
        m_pmtx = nullptr;
    }

}

AdvisoryFileLock::~AdvisoryFileLock() {
    if (m_pmtx != nullptr) {
        m_pmtx->unlock();
    }
}