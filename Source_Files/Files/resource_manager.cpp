/*

	Copyright (C) 1991-2001 and beyond by Bungie Studios, Inc.
	and the "Aleph One" developers.
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

*/

/*
 *  resource_manager.cpp - MacOS resource handling for non-Mac platforms
 *
 *  Written in 2000 by Christian Bauer
 *
 *  Jan 16, 2003 (Woody Zenfell):
 *      Reworked stemmed-file opening logic; now using new Logging facility
 */

#include <SDL_endian.h>

#include "cseries.h"
#include "resource_manager.h"
#include "FileHandler.h"
#include "Logging.h"

#include <stdio.h>
#include <vector>
#include <list>
#include <map>
#include <cstdlib>

#ifndef NO_STD_NAMESPACE
using std::iostream;
using std::vector;
using std::list;
using std::map;
#endif


// From FileHandler_SDL.cpp
extern bool is_applesingle(SDL_RWops *f, bool rsrc_fork, long &offset, long &length);
extern bool is_macbinary(SDL_RWops *f, long &data_length, long &rsrc_length);

// From csfiles_beos.cpp
bool has_rfork_attribute(const char *file);
SDL_RWops *sdl_rw_from_rfork(const char *file, bool writable);


// Structure for open resource file
struct res_file_t {
	res_file_t() : f(NULL) {}
	res_file_t(SDL_RWops *file) : f(file) {}
	res_file_t(const res_file_t &other) {f = other.f;}
	~res_file_t() {}

	const res_file_t &operator=(const res_file_t &other)
	{
		if (this != &other)
			f = other.f;
		return *this;
	}

	bool read_map(void);
	size_t count_resources(uint32 type) const;
	void get_resource_id_list(uint32 type, vector<int> &ids) const;
	bool get_resource(uint32 type, int id, LoadedResource &rsrc) const;
	bool get_ind_resource(uint32 type, int index, LoadedResource &rsrc) const;
	bool has_resource(uint32 type, int id) const;

	SDL_RWops *f;		// Opened resource file

	typedef map<int, uint32> id_map_t;			// Maps resource ID to offset to resource data
	typedef map<uint32, id_map_t> type_map_t;	// Maps resource type to ID map

	type_map_t types;	// Map of all resource types found in file
};


// List of open resource files
static list<res_file_t *> res_file_list;
static list<res_file_t *>::iterator cur_res_file_t;


/*
 *  Find file in list of opened files
 */

static list<res_file_t *>::iterator find_res_file_t(SDL_RWops *f)
{
	list<res_file_t *>::iterator i, end = res_file_list.end();
	for (i=res_file_list.begin(); i!=end; i++) {
		res_file_t *r = *i;
		if (r->f == f)
			return i;
	}
	return res_file_list.end();
}


/*
 *  Initialize resource management
 */

void initialize_resources(void)
{
	// nothing to do
}


/*
 *  Read and parse resource map from file
 */

bool res_file_t::read_map(void)
{
	SDL_RWseek(f, 0, SEEK_END);
	uint32 file_size = SDL_RWtell(f);
	SDL_RWseek(f, 0, SEEK_SET);
	uint32 fork_start = 0;

        if(file_size < 16) {
            if(file_size == 0)
                logNote("file has zero length");
            else
                logAnomaly1("file too small (%d bytes) to be valid", file_size);
            return false;
        }

	// Determine file type (AppleSingle and MacBinary II files are handled transparently)
	long offset, data_length, rsrc_length;
	if (is_applesingle(f, true, offset, rsrc_length)) {
                logTrace("file is_applesingle");
		fork_start = offset;
		file_size = offset + rsrc_length;
	} else if (is_macbinary(f, data_length, rsrc_length)) {
                logTrace("file is_macbinary");
		fork_start = 128 + ((data_length + 0x7f) & ~0x7f);
		file_size = fork_start + rsrc_length;
	}
        else
                logTrace("file is raw resource fork format");

	// Read resource header
	SDL_RWseek(f, fork_start, SEEK_SET);
	uint32 data_offset = SDL_ReadBE32(f) + fork_start;
	uint32 map_offset = SDL_ReadBE32(f) + fork_start;
	uint32 data_size = SDL_ReadBE32(f);
	uint32 map_size = SDL_ReadBE32(f);
        logDump4("resource header: data offset %d, map_offset %d, data_size %d, map_size %d", data_offset, map_offset, data_size, map_size);

	// Verify integrity of resource header
	if (data_offset >= file_size || map_offset >= file_size ||
	    data_offset + data_size > file_size || map_offset + map_size > file_size) {
		logAnomaly("file's resource header corrupt");
		return false;
	}

	// Read map header
	SDL_RWseek(f, map_offset + 24, SEEK_SET);
	uint32 type_list_offset = map_offset + SDL_ReadBE16(f);
	//uint32 name_list_offset = map_offset + SDL_ReadBE16(f);
	//printf(" type_list_offset %d, name_list_offset %d\n", type_list_offset, name_list_offset);

	// Verify integrity of map header
	if (type_list_offset >= file_size) {
		logAnomaly("file's resource map header corrupt");
		return false;
	}

	// Read resource type list
	SDL_RWseek(f, type_list_offset, SEEK_SET);
	int num_types = SDL_ReadBE16(f) + 1;
	for (int i=0; i<num_types; i++) {

		// Read type list item
		uint32 type = SDL_ReadBE32(f);
		int num_refs = SDL_ReadBE16(f) + 1;
		uint32 ref_list_offset = type_list_offset + SDL_ReadBE16(f);
		//printf("  type %c%c%c%c, %d refs\n", type >> 24, type >> 16, type >> 8, type, num_refs);

		// Verify integrity of item
		if (ref_list_offset >= file_size) {
			logAnomaly("file's resource type list corrupt");
			return false;
		}

		// Create ID map for this type
		id_map_t &id_map = types[type];

		// Read reference list
		uint32 cur = SDL_RWtell(f);
		SDL_RWseek(f, ref_list_offset, SEEK_SET);
		for (int j=0; j<num_refs; j++) {

			// Read list item
			int id = SDL_ReadBE16(f);
			SDL_RWseek(f, 2, SEEK_CUR);
			uint32 rsrc_data_offset = data_offset + (SDL_ReadBE32(f) & 0x00ffffff);
			//printf("   id %d, rsrc_data_offset %d\n", id, rsrc_data_offset);

			// Verify integrify of item
			if (rsrc_data_offset >= file_size) {
				logAnomaly("file's resource reference list corrupt");
				return false;
			}

			// Add ID to map
			id_map[id] = rsrc_data_offset;

			SDL_RWseek(f, 4, SEEK_CUR);
		}
		SDL_RWseek(f, cur, SEEK_SET);
	}

	return true;
}


/*
 *  Open resource file, set current file to the newly opened one
 */
 
static SDL_RWops*
open_res_file_from_rwops(SDL_RWops* f) {
    if (f) {
    	#ifdef PSP
    		printf("PSP - Resource File Opened\n");
    	#endif
    	
            // Successful, create res_file_t object and read resource map
            res_file_t *r = new res_file_t(f);
            if (r->read_map()) {

                    // Successful, add file to list of open files
                    res_file_list.push_back(r);
                    cur_res_file_t = --res_file_list.end();
                    
                    // ZZZ: this exists mostly to help the user understand (via logContexts) which of
                    // potentially several copies of a resource fork is actually being used.
                    logNote("success, using this resource data");

            } else {
    	#ifdef PSP
    				printf("PSP - Invalid Resource Map\n");
    	#endif
                    // Error reading resource map
                    SDL_RWclose(f);
                    return NULL;
            }
    }
    else
            logNote("file could not be opened");
    return f;
}

static SDL_RWops*
open_res_file_from_path(const char* inPath) {
    logContext1("trying path %s", inPath);
/*
    string theContextString("trying path ");
    theContextString += inPath;
    logContext(theContextString.c_str());
*/
    
    return open_res_file_from_rwops(SDL_RWFromFile(inPath, "rb"));
}

SDL_RWops *open_res_file(FileSpecifier &file)
{
    logContext1("opening resource file %s", file.GetPath());
/*
    string theContextString("trying to open resource file ");
    theContextString += file.GetPath();
    logContext(theContextString.c_str());
*/

#ifdef PSP
	printf("PSP - Opening Resource File Path %s\n",file.GetPath());
#endif

    string rsrc_file_name = file.GetPath();
    string resources_file_name = rsrc_file_name;
    string darwin_rsrc_file_name = rsrc_file_name;
    rsrc_file_name += ".rsrc";
    resources_file_name += ".resources";
    darwin_rsrc_file_name += "/rsrc";

    SDL_RWops* f = NULL;

    // Open file, try <name>.rsrc first, then <name>.resources, then <name>/rsrc then <name>
#ifdef __BEOS__
    // On BeOS, try MACOS:RFORK attribute first
    if (has_rfork_attribute(file.GetPath())) {
            f = sdl_rw_from_rfork(file.GetPath(), false);
            f = open_res_file_from_rwops(f);
    }
#endif

#ifdef PSP
	printf("PSP - Trying resource name:\n-%s\n-%s\n-%s\n-%s\n",
			rsrc_file_name.c_str(),resources_file_name.c_str(),
			darwin_rsrc_file_name.c_str(),file.GetPath());
#endif

    if (f == NULL)
            f = open_res_file_from_path(rsrc_file_name.c_str());
    if (f == NULL)
            f = open_res_file_from_path(resources_file_name.c_str());
    if (f == NULL)
	   f = open_res_file_from_path(darwin_rsrc_file_name.c_str());
    if (f == NULL)
	   f = open_res_file_from_path(file.GetPath());

#ifdef PSP
	if (f == NULL) printf("PSP - No Resource File Found\n");
#endif

    return f;
}



/*
 *  Close resource file
 */

void close_res_file(SDL_RWops *file)
{
	if (file == NULL)
		return;

	// Find file in list
	list<res_file_t *>::iterator i = find_res_file_t(file);
	if (i != res_file_list.end()) {

		// Remove it from the list, close the file and delete the res_file_t
		res_file_t *r = *i;
		SDL_RWclose(r->f);
		res_file_list.erase(i);
		delete r;

		cur_res_file_t = --res_file_list.end();
	}
}


/*
 *  Return current resource file
 */

SDL_RWops *cur_res_file(void)
{
	res_file_t *r = *cur_res_file_t;
	assert(r);
	return r->f;
}


/*
 *  Set current resource file
 */

void use_res_file(SDL_RWops *file)
{
	list<res_file_t *>::iterator i = find_res_file_t(file);
	assert(i != res_file_list.end());
	cur_res_file_t = i;
}


/*
 *  Count number of resources of given type
 */

size_t res_file_t::count_resources(uint32 type) const
{
	type_map_t::const_iterator i = types.find(type);
	if (i == types.end())
		return 0;
	else
		return i->second.size();
}

size_t count_1_resources(uint32 type)
{
	return (*cur_res_file_t)->count_resources(type);
}

size_t count_resources(uint32 type)
{
	size_t count = 0;
	list<res_file_t *>::const_iterator i = cur_res_file_t, begin = res_file_list.begin();
	while (true) {
		count += (*i)->count_resources(type);
		if (i == begin)
			break;
		i--;
	}
	return count;
}


/*
 *  Get list of id of resources of given type
 */

void res_file_t::get_resource_id_list(uint32 type, vector<int> &ids) const
{
	type_map_t::const_iterator i = types.find(type);
	if (i != types.end()) {
		id_map_t::const_iterator j, end = i->second.end();
		for (j=i->second.begin(); j!=end; j++)
			ids.push_back(j->first);
	}
}

void get_1_resource_id_list(uint32 type, vector<int> &ids)
{
	ids.clear();
	(*cur_res_file_t)->get_resource_id_list(type, ids);
}

void get_resource_id_list(uint32 type, vector<int> &ids)
{
	ids.clear();
	list<res_file_t *>::const_iterator i = cur_res_file_t, begin = res_file_list.begin();
	while (true) {
		(*i)->get_resource_id_list(type, ids);
		if (i == begin)
			break;
		i--;
	}
}


/*
 *  Get resource data (must be freed with free())
 */

bool res_file_t::get_resource(uint32 type, int id, LoadedResource &rsrc) const
{
	rsrc.Unload();

	// Find resource in map
	type_map_t::const_iterator i = types.find(type);
	if (i != types.end()) {
		id_map_t::const_iterator j = i->second.find(id);
		if (j != i->second.end()) {

			// Found, read data size
			SDL_RWseek(f, j->second, SEEK_SET);
			uint32 size = SDL_ReadBE32(f);

			// Allocate memory and read data
			void *p = malloc(size);
			if (p == NULL)
				return false;
			SDL_RWread(f, p, 1, size);
			rsrc.p = p;
			rsrc.size = size;

			//printf("get_resource type %c%c%c%c, id %d -> data %p, size %d\n", type >> 24, type >> 16, type >> 8, type, id, p, size);
			return true;
		}
	}
	return false;
}

bool get_1_resource(uint32 type, int id, LoadedResource &rsrc)
{
	return (*cur_res_file_t)->get_resource(type, id, rsrc);
}

bool get_resource(uint32 type, int id, LoadedResource &rsrc)
{
	list<res_file_t *>::const_iterator i = cur_res_file_t, begin = res_file_list.begin();
	while (true) {
		bool found = (*i)->get_resource(type, id, rsrc);
		if (found)
			return true;
		if (i == begin)
			break;
		i--;
	}
	return false;
}


/*
 *  Get resource data by index (must be freed with free())
 */

bool res_file_t::get_ind_resource(uint32 type, int index, LoadedResource &rsrc) const
{
	rsrc.Unload();

	// Find resource in map
	type_map_t::const_iterator i = types.find(type);
	if (i != types.end()) {
		if (index < 1 || index > int(i->second.size()))
			return false;
		id_map_t::const_iterator j = i->second.begin();
		for (int k=1; k<index; k++)
			++j;

		// Read data size
		SDL_RWseek(f, j->second, SEEK_SET);
		uint32 size = SDL_ReadBE32(f);

		// Allocate memory and read data
		void *p = malloc(size);
		if (p == NULL)
			return false;
		SDL_RWread(f, p, 1, size);
		rsrc.p = p;
		rsrc.size = size;

		//printf("get_ind_resource type %c%c%c%c, index %d -> data %p, size %d\n", type >> 24, type >> 16, type >> 8, type, index, p, size);
		return true;
	}
	return false;
}

bool get_1_ind_resource(uint32 type, int index, LoadedResource &rsrc)
{
	return (*cur_res_file_t)->get_ind_resource(type, index, rsrc);
}

bool get_ind_resource(uint32 type, int index, LoadedResource &rsrc)
{
	list<res_file_t *>::const_iterator i = cur_res_file_t, begin = res_file_list.begin();
	while (true) {
		bool found = (*i)->get_ind_resource(type, index, rsrc);
		if (found)
			return true;
		if (i == begin)
			break;
		i--;
	}
	return false;
}


/*
 *  Check if resource is present
 */

bool res_file_t::has_resource(uint32 type, int id) const
{
	type_map_t::const_iterator i = types.find(type);
	if (i != types.end()) {
		id_map_t::const_iterator j = i->second.find(id);
		if (j != i->second.end())
			return true;
	}
	return false;
}

bool has_1_resource(uint32 type, int id)
{
	return (*cur_res_file_t)->has_resource(type, id);
}

bool has_resource(uint32 type, int id)
{
	list<res_file_t *>::const_iterator i = cur_res_file_t, begin = res_file_list.begin();
	while (true) {
		if ((*i)->has_resource(type, id))
			return true;
		if (i == begin)
			break;
		i--;
	}
	return false;
}
