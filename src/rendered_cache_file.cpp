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
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
#include <fstream>

using namespace std;
using namespace std::experimental;

namespace rendered_cache_file {

const uint64_t MAGIC_NUMBER = 0xFFEEDDCCFEDCFEDC;
const uint8_t CURRENT_VERSION = 1;

template<typename T>
T read(ifstream& ifs) {
    T buff;
    ifs.read(reinterpret_cast<char*>(&buff), sizeof(T));

    auto nread = ifs.gcount();

    if (!ifs) {
        throw std::runtime_error("bad stream after read");
    }

    if ((size_t)nread < sizeof(T)) {
        throw std::runtime_error("read too few bytes");
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
        throw std::runtime_error("bad stream after read");
    }

    if (nread < 0 || (uint64_t)nread < len) {
        throw std::runtime_error("read too few bytes");
    }

    return std::string(buff.begin(), buff.end());
}

template<typename T>
void write(ofstream& ofs, const T& value) {
    ofs.write((char*)&value, sizeof(T));

    if (!ofs) {
        throw std::runtime_error("failed to write to file");
    }
}

template<>
void write(ofstream& ofs, const string& value) {
    write<uint64_t>(ofs, value.size());
    ofs.write(value.c_str(), value.size());
}

std::shared_ptr<rendered_object> read_object(ifstream& ifs) {
    auto id = read<string>(ifs);
    auto type = read<string>(ifs);
    auto json = read<string>(ifs);
    return make_shared<rendered_object>(id, type, json);
}

shared_ptr<rendered_object_list> read_objects(ifstream& ifs) {
    auto n_objects = read<uint64_t>(ifs);

    auto objects = std::make_shared<rendered_object_list>();

    for (uint64_t i = 0; i < n_objects; ++i) {
        std::shared_ptr<rendered_object> object = read_object(ifs);
        objects->add_object(object);
    }

    return objects;    
}

shared_ptr<rendered_object_list> get_contents(const string& path) {
    if (!std::experimental::filesystem::exists(path)) {
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

void write_objects(ofstream& ofs, std::shared_ptr<rendered_object_list> objects) {
    write<uint64_t>(ofs, objects->size());

    for (const auto obj : *objects) {
        write_object(ofs, obj.second);
    }
}

void save(const string& path, std::shared_ptr<rendered_object_list> objects) {
    ofstream ofs;
    {
        temporary_umask umask(0077);
        /* Open cache file for writing */
        ofs.open(path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    }

    if (!ofs) {
        throw runtime_error("failed to open cache file for writing");
    }

    write<uint64_t>(ofs, MAGIC_NUMBER);
    write<uint8_t>(ofs, CURRENT_VERSION);

    write_objects(ofs, objects);
}

}
