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

#include "rendered_cache_file.hpp"
#include "utility/temporary_umask.hpp"
#include "advisory_file_lock.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;

namespace rendered_cache_file {

const uint64_t MAGIC_NUMBER = 0xFFEEDDCCFEDCFEDC;
const uint8_t CURRENT_VERSION = 1;
const size_t HEADER_SIZE = sizeof(MAGIC_NUMBER) + sizeof(CURRENT_VERSION);

const int FILE_LOCK_TIMEOUT = 30; // seconds

template<typename T>
T read(ifstream& ifs) {
    T buff;
    ifs.read(reinterpret_cast<char*>(&buff), sizeof(T));

    auto nread = ifs.gcount();

    if (!ifs) {
        throw runtime_error("bad stream after read");
    }

    if ((size_t)nread < sizeof(T)) {
        throw runtime_error("read too few bytes");
    }

    return buff;
}

template<>
string read(ifstream& ifs) {
    auto len = read<uint64_t>(ifs);
    vector<char> buff(len);
    ifs.read(&buff[0], len);

    auto nread = ifs.gcount();

    if (!ifs) {
        throw runtime_error("bad stream after read");
    }

    if (nread < 0 || (uint64_t)nread < len) {
        throw runtime_error("read too few bytes");
    }

    return string(buff.begin(), buff.end());
}

template<typename T>
void write(ofstream& ofs, const T& value) {
    ofs.write((char*)&value, sizeof(T));

    if (!ofs) {
        throw runtime_error("failed to write to file");
    }
}

template<>
void write(ofstream& ofs, const string& value) {
    write<uint64_t>(ofs, value.size());
    ofs.write(value.c_str(), value.size());
    if (!ofs) {
        throw runtime_error("failed to write to file");
    }
}

size_t string_size(const string& value) {
    return sizeof(uint64_t) + value.size();
}

shared_ptr<rendered_object> read_object(ifstream& ifs) {
    auto id = read<string>(ifs);
    auto type = read<string>(ifs);
    auto json = read<string>(ifs);
    return make_shared<rendered_object>(id, type, json);
}

shared_ptr<rendered_object_list> read_objects(ifstream& ifs) {
    auto n_objects = read<uint64_t>(ifs);

    auto objects = make_shared<rendered_object_list>();

    for (uint64_t i = 0; i < n_objects; ++i) {
        shared_ptr<rendered_object> object = read_object(ifs);
        objects->add_object(object);
    }

    return objects;    
}


shared_ptr<rendered_object_list> get_contents(const string& path) {
    AdvisoryFileLock afl(path, FILE_LOCK_TIMEOUT);

    if (!std::filesystem::exists(path)) {
        return make_shared<rendered_object_list>();
    }

    ifstream ifs(path, ios_base::in | ios_base::binary);

    if (!ifs) {
        throw runtime_error(string("failed to open file: ") + path);
    }

    uint64_t magic = read<uint64_t>(ifs);

    if (magic != MAGIC_NUMBER) {
        throw bad_format();
    }

    uint8_t version = read<uint8_t>(ifs);

    if (version > CURRENT_VERSION) {
        throw std::runtime_error("version number of cache file is too high");
    }

    return read_objects(ifs);
}

void write_object(ofstream& ofs, std::shared_ptr<rendered_object> object) {
    write<string>(ofs, object->get_id());
    write<string>(ofs, object->get_type());
    write<string>(ofs, object->get_json());
}

size_t object_size(std::shared_ptr<rendered_object> object) {
    return 
        string_size(object->get_id()) + 
        string_size(object->get_type()) + 
        string_size(object->get_json());
}

void write_objects(ofstream& ofs, std::shared_ptr<rendered_object_list> objects) {
    write<uint64_t>(ofs, objects->size());

    for (const auto& obj : *objects) {
        write_object(ofs, obj.second);
    }
}

size_t estimate_objects(const rendered_object_list& current_objects, const rendered_object_list& cached) {
    size_t total = sizeof(uint64_t); // The size of the number of objects

    // Go through all current objects, if there's a cached object use the bigger size of the two
    // (because we might fail to update the object)
    for (const auto& obj : current_objects) {
        size_t size = object_size(obj.second);
        auto cached_object = cached.get_object(obj.second->get_id());
        if (cached_object) {
            auto cached_size = object_size(cached_object);
            if (cached_size > size) {
                size = cached_size;
            }
        }
        total += size;
    }

    // Also include all cached objects which aren't in current.
    // (In case the object isn't in current because it failed to render,
    // we will keep the cached version, or in case we fail to delete them 
    // we also need to keep them in the cache)
    for (const auto& obj : cached) {
        bool not_in_current = current_objects.get_object(obj.second->get_id()) == nullptr;
        if (not_in_current) {
            total += object_size(obj.second);
        }
    }

    return total;
}

std::string temp_file_for(const string& path) {
    return path + ".tmp";
}

void begin_rendered_cache_file(const string& path, size_t size_to_pre_allocate, std::ofstream& ofs) {
    {
        temporary_umask umask(0077);
        /* Open cache file for writing */
        ofs.open(temp_file_for(path), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    }

    if (!ofs) {
        throw runtime_error("failed to open cache file for writing");
    }

    // Pre-allocate disk space by simply writing data to the file.
    // There are more efficient ways of doing this, but this needs to work
    // both in a platform independent way and also for filesystems that
    // support sparse files. Simply writing 1's should be safest and we're
    // not so concerned with performance here.
    const size_t BLOCK_SIZE = 1024;
    char block[BLOCK_SIZE];
    std::fill_n(block, BLOCK_SIZE, 0xff);

    std::streampos pos = 0;
    ofs.seekp(pos);
    while (pos < streampos(size_to_pre_allocate)) {
        ofs.write(block, BLOCK_SIZE);
        if (!ofs) {
            throw std::runtime_error("failed to pre-allocate cache file (not enough disk space?)");
        }
        pos = ofs.tellp();
        if (pos == -1 || !ofs) {
            throw std::runtime_error("failed to get file position while pre-allocating cache file size");
        }
    }
    ofs.seekp(0);
}

void finalize_rendered_cache_file(ofstream& ofs, const string& path) {
    streampos size = -1;
    try {
        size = ofs.tellp();
    }
    catch (const std::exception&) {
        // We just won't resize the file then...
    }
    ofs.close();
    if (!ofs) {
        throw runtime_error("failed to close cache file");
    }

    // Get rid of the extra bytes we allocated in begin_rendered_cache_file
    if (size > 0) {
        std::error_code ec;
        std::filesystem::resize_file(temp_file_for(path), size, ec);
    }

    AdvisoryFileLock afl(path, FILE_LOCK_TIMEOUT);

    const int RENAME_RETRIES = 5;
    for (int i = 0; i < RENAME_RETRIES; ++i) {
        try {
            std::filesystem::rename(temp_file_for(path), path);
            return;
        }
        catch (const std::exception& e) {
            auto msg = std::string("failed to overwrite old cache file with new: ") + e.what();
            if (i + 1 >= RENAME_RETRIES) {
                throw std::runtime_error(msg);
            }
            else {
                std::cerr << msg << std::endl;
                std::cerr << "will retry in " << (i + 1)*10 << " seconds" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds((i + 1) * 10));
            }
        }
    }
}

void save(ofstream& ofs, shared_ptr<rendered_object_list> objects) {
    write<uint64_t>(ofs, MAGIC_NUMBER);
    write<uint8_t>(ofs, CURRENT_VERSION);

    write_objects(ofs, objects);
}

size_t size_estimate(const rendered_object_list& current_objects, const rendered_object_list& cached) {
    size_t total = HEADER_SIZE;
    total += estimate_objects(current_objects, cached);
    return total;
}

}
