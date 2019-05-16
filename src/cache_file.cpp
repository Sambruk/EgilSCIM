/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
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
 *
 * Further development with groups and relations support
 * by Ola Mattsson - IT informa for Sambruk
 */

#ifdef _WIN32
#include <io.h>
#define close _close
#define read _read
#define access _access
#define open _open
#define F_OK 0

#pragma warning( disable : 4996 ) 

#else
#include <unistd.h>
#endif
#include <fcntl.h>

#include "cache_file.hpp"
#include "utility/simplescim_error_string.hpp"
#include "model/object_list.hpp"
#include "config_file.hpp"

cache_file::~cache_file() {
	// asserts on Windows...
	//close(cache_file_fd);
}

/**
 * Reads 'n' bytes into 'buf' from 'fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::read_n(uint8_t *buf, size_t n) {
	auto nread = read(cache_file_fd, buf, static_cast<unsigned int>(n)); // TODO Shouldn't cast n to unsigned int

	if (nread == -1) {
		simplescim_error_string_set_errno("read_n:read:%s", cache_file_filename.c_str());
		return -1;
	}

	if ((size_t) nread < n) {
		simplescim_error_string_set_prefix("read_n:read:%s", cache_file_filename.c_str());
		simplescim_error_string_set_message("unexpected end-of-file");
		return -1;
	}

	return 0;
}

/**
 * Reads a 64-bit value from 'fd' and stores it in 'buf'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::read_uint64(uint64_t *buf) {
	int err;

	err = read_n((uint8_t *) buf, sizeof(uint64_t));

	if (err == -1) {
		return -1;
	}

	return 0;
}

/**
 * Reads a value from 'fd' and stores it
 * in 'avp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::read_value(std::string *avp) {
	uint64_t av_len;
	int err;

	/* Read value length */
	err = read_uint64(&av_len);

	if (err == -1) {
		return -1;
	}

	/* Allocate value */
	auto av_val = static_cast<uint8_t *>(malloc(av_len + 1));

	if (av_val == nullptr) {
		simplescim_error_string_set_errno("read_value:"
		                                  "malloc");
		return -1;
	}

	/* Read value */
	err = read_n(av_val, av_len);

	if (err == -1) {
		free(av_val);
		return -1;
	}

	/* nullptr-terminate value */
	av_val[av_len] = '\0';

	*avp = std::string((const char *) av_val, av_len);
	free(av_val);

	return 0;
}

/**
 * Reads a simplescim_arbval_list from 'fd' and
 * stores it in 'alp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::read_value_list(string_vector *alp) {
	uint64_t al_len;
	uint64_t i;
	int err;

	/* Read number of values */
	err = read_uint64(&al_len);

	if (err == -1) {
		return -1;
	}

	/* Allocate 'al' */
	auto al = string_vector();

	/* Read all values */
	for (i = 0; i < al_len; ++i) {

		std::string value;
		err = read_value(&value);

		if (err == -1) {
			return -1;
		}

		/* Insert 'value' into 'al' */
		al.emplace_back(value);
	}

	*alp = al;

	return 0;
}

/**
 * Reads one object from 'fd' and stores it in 'userp', and
 * stores the object's unique identifier in 'uidp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
std::shared_ptr<base_object> cache_file::read_object(std::string *uidp) {
	std::string uid;
	int err;

	/** Read object's unique identifier */
	err = read_value(&uid);

	if (err == -1) {
		return nullptr;
	}
	/** read object type */
	std::string type;
	err = read_value(&type);

	if (err == -1) {
		return nullptr;
	}

	uint64_t n_attributes;
	err = read_uint64(&n_attributes);

	if (err == -1) {
		return nullptr;
	}

	auto object = std::make_shared<base_object>(type);

	/* Read attributes */
	for (uint64_t i = 0; i < n_attributes; ++i) {

		/* Read attribute name */
		std::string attribute;
		err = read_value(&attribute);

		if (err == -1) {
			return nullptr;
		}

		/* Read values */
		string_vector values;
		err = read_value_list(&values);

		if (err == -1) {
			return nullptr;
		}

		object->add_attribute(attribute, std::move(values));
	}

	*uidp = uid;
	return object;

}

/**
 * Reads objects from 'fd' and stores them in 'usersp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
std::shared_ptr<object_list> cache_file::read_objects() {
	std::string uid;
	uint64_t n_users;
	int err;

	/* Read n_users */
	err = read_uint64(&n_users);

	if (err == -1) {
		return std::make_shared<object_list>();
	}

	/* Allocate object list */
	auto objects = std::make_shared<object_list>();

	/* Read all objects */
	for (uint64_t i = 0; i < n_users; ++i) {

		std::shared_ptr<base_object> object = read_object(&uid);

		if (object == nullptr) {
			return std::make_shared<object_list>();
		}

		objects->add_object(uid, object);
	}

	return objects;
}

/**
 * Reads cache file specified in configuration file and
 * constructs a object list according to its contents.
 * On success, a pointer to the constructed object list is
 * returned. If the cache file doesn't exist, an empty object
 * list is returned. On error, nullptr is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
std::shared_ptr<object_list> cache_file::get_contents() {

	/* Get the cache file's name from configuration file */
	cache_file_filename = config_file::instance().get("cache-file");

	if (cache_file_filename.empty()) {
		simplescim_error_string_set("get_users", "required variable \"cache-file\" is missing");
		return nullptr;
	}
	if (cache_file_filename == "nocache")
		return std::make_shared<object_list>();

	return get_objects_from_file(cache_file_filename.c_str());

}

std::shared_ptr<object_list> cache_file::get_objects_from_file(const char *filename) {

	cache_file_filename = filename;

	/* Check if cache file exists.
	   If not, return an empty object list. */
	if (access(filename, F_OK) == -1) {
		return std::make_shared<object_list>();
	}

	/* Open cache file */
	const auto open_flags = 
#ifdef _WIN32
		_O_RDONLY | _O_BINARY;
#else
		O_RDONLY;
#endif

	cache_file_fd = open(filename, open_flags);

	if (cache_file_fd == -1) {
		simplescim_error_string_set_errno("get_users:open:%s", filename);
		return nullptr;
	}

	/* Read object list from cache file */

	std::shared_ptr<object_list> list = read_objects();;


	close(cache_file_fd);

	return list;
}


/**
 * ******************************************************************************
 * ******************************************************************************
 * ***********************STORE IT**************************************************
 * ******************************************************************************
 * ******************************************************************************
 */

/**
 * Writes 'n' bytes from 'buf' to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::write_n(const uint8_t *buf, size_t n) {
	auto nwritten = write(cache_file::instance().cache_file_fd, buf, static_cast<unsigned int>(n)); // TODO Shouldn't cast n to unsigned int here...

	if (nwritten == -1) {
		simplescim_error_string_set_errno("simplescim_cache_file_write_n:write:%s",
		                                  cache_file_filename.c_str());
		return -1;
	}

	if ((size_t) nwritten < n) {
		simplescim_error_string_set_prefix("simplescim_cache_file_write_n:write:%s",
		                                   cache_file_filename.c_str());
		simplescim_error_string_set_message("could only write %ld B out of %lu B", nwritten, n);
		return -1;
	}

	return 0;
}

/**
 * Writes a 64-bit value from 'buf' to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::write_uint64(uint64_t n) {
	int err;

	err = cache_file::write_n((const uint8_t *) &n, sizeof(uint64_t));

	if (err == -1) {
		return -1;
	}

	return 0;
}

/**
 * Writes a simplescim_arbval from 'av' to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::write_value(const std::string &av) {
	int err;

	/* Write value_length */
	err = write_uint64(av.length());

	if (err == -1) {
		return -1;
	}

	/* Write value */
	err = write_n((const uint8_t *) av.c_str(), av.length());

	if (err == -1) {
		return -1;
	}

	return 0;
}

/**
 * Writes a simplescim_arbval_list from 'al' to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::write_value_list(const string_vector &al) {
	int err;

	/* Write n_values */
	err = write_uint64(al.size());

	if (err == -1) {
		return -1;
	}

	/* Write values */
	for (const auto& str : al) {
		err = write_value(str);

		if (err == -1) {
			return -1;
		}
	}

	return 0;
}

/**
 * Writes one object attribute and its values to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::write_attribute(const std::string &attribute, const string_vector &values) {
	uint64_t attribute_len;
	int err;

	/* Calculate attribute_name_length */
	attribute_len = attribute.length();

	/* Write attribute_name_length */
	err = write_uint64(attribute_len);

	if (err == -1) {
		return -1;
	}

	/* Write attribute_name */
	err = write_n((const uint8_t *) attribute.c_str(), attribute_len);

	if (err == -1) {
		return -1;
	}

	/* Write n_values and values */
	err = write_value_list(values);

	if (err == -1) {
		return -1;
	}

	return 0;
}

/**
 * Writes one object to 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::write_object(const std::string &uid, const base_object &object) {
	int err;

	/* Write unique_identifier_length and unique_identifier */
	err = write_value(uid);

	if (err == -1) {
		return -1;
	}

	std::string type = object.getSS12000type();
	if (type.empty()) {
		std::cerr << "trying to cache an object without type " << uid << std::endl;
		return -1;
	}

	err = write_value(type);

	if (err == -1) {
		return -1;
	}

	/* Write n_attributes */
	err = write_uint64(object.number_of_attributes());

	if (err == -1) {
		return -1;
	}

	/* Write attributes */

	for (auto &&attib : object.attributes) {
		err = write_attribute(attib.first, attib.second);
		if (err == -1)
			return err;
	}

	return err;
}

/**
 * Writes 'objects' to cache file specified in configuration
 * file.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int cache_file::save(std::shared_ptr<object_list> objects) {
	int err;

	/* Get cache file's name */
	cache_file_filename = config_file::instance().get("cache-file");

	if (cache_file_filename.empty()) {
		simplescim_error_string_set("simplescim_cache_file_save", "required variable \"cache-file\" is missing");
		return -1;
	}

	/* Open cache file for writing (mode 0664) */

	const auto open_flags =
#ifdef _WIN32
		_O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY;
#else
		O_WRONLY | O_CREAT | O_TRUNC;
#endif

	const auto pmode =
#ifdef _WIN32
		_S_IREAD | _S_IWRITE;
#else
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
#endif
	cache_file_fd = open(cache_file_filename.c_str(), open_flags, pmode);

	if (cache_file_fd == -1) {
		simplescim_error_string_set_errno("simplescim_cache_file_save:open:%s", cache_file_filename.c_str());
		return -1;
	}

	/* Write n_objects */
	err = cache_file::instance().write_uint64(objects->size());

	if (err == -1) {
		close(cache_file_fd);
		return -1;
	}

	for (auto &&item : objects->objects) {
		err = write_object(item.first, *item.second);
		if (err == -1)
			break;
	}

	close(cache_file_fd);

	if (err == -1) {
		return -1;
	}

	return 0;
}
