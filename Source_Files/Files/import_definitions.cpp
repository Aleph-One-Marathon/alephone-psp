/*
IMPORT_DEFINITIONS.C

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

Sunday, October 2, 1994 1:25:23 PM  (Jason')

Aug 12, 2000 (Loren Petrich):
	Using object-oriented file handler

Aug 31, 2000 (Loren Petrich):
	Added unpacking code for the physics models
*/

#include "cseries.h"
#include <string.h>

#include "tags.h"
#include "map.h"
#include "interface.h"
#include "game_wad.h"
#include "wad.h"
#include "game_errors.h"
#include "shell.h"
#include "preferences.h"
#include "FileHandler.h"
#include "shell.h"
#include "preferences.h"

// LP: get all the unpacker definitions
#include "monsters.h"
#include "effects.h"
#include "projectiles.h"
#include "player.h"
#include "weapons.h"
#include "physics_models.h"

/* ---------- globals */

#define IMPORT_STRUCTURE
#include "extensions.h"

/* ---------- local globals */
static FileSpecifier PhysicsFileSpec;

/* ---------- local prototype */
static struct wad_data *get_physics_wad_data(bool *bungie_physics);
static void import_physics_wad_data(struct wad_data *wad);

/* ---------- code */
void set_physics_file(FileSpecifier& File)
{
	PhysicsFileSpec = File;
}

void set_to_default_physics_file(
	void)
{
	get_default_physics_spec(PhysicsFileSpec);
//	dprintf("Set to: %d %d %.*s", physics_file.vRefNum, physics_file.parID, physics_file.name[0], physics_file.name+1);
}

void init_physics_wad_data()
{
	init_monster_definitions();
	init_effect_definitions();
	init_projectile_definitions();
	init_physics_constants();
	init_weapon_definitions();
}

void import_definition_structures(
	void)
{
	struct wad_data *wad;
	bool bungie_physics;

	init_physics_wad_data();

	wad= get_physics_wad_data(&bungie_physics);
	if(wad)
	{
		/* Actually load it in.. */		
		import_physics_wad_data(wad);
		
		free_wad(wad);
	}
}

void *get_network_physics_buffer(
	long *physics_length)
{
	void *data= get_flat_data(PhysicsFileSpec, false, 0);
	
	if(data)
	{
		*physics_length= get_flat_data_length(data);
	} else {
		*physics_length= 0;
	}
	
	return data;
}

void process_network_physics_model(
	void *data)
{
	init_physics_wad_data();

	if(data)
	{
		struct wad_header header;
		struct wad_data *wad;
	
		wad= inflate_flat_data(data, &header);
		if(wad)
		{
			import_physics_wad_data(wad);
			free_wad(wad); /* Note that the flat data points into the wad. */
		}
	}
}

/* --------- local code */
static struct wad_data *get_physics_wad_data(
	bool *bungie_physics)
{
	struct wad_data *wad= NULL;
	
//	dprintf("Open is: %d %d %.*s", physics_file.vRefNum, physics_file.parID, physics_file.name[0], physics_file.name+1);

	OpenedFile PhysicsFile;
	if(open_wad_file_for_reading(PhysicsFileSpec,PhysicsFile))
	{
		struct wad_header header;

		if(read_wad_header(PhysicsFile, &header))
		{
			if(header.data_version==BUNGIE_PHYSICS_DATA_VERSION || header.data_version==PHYSICS_DATA_VERSION)
			{
				wad= read_indexed_wad_from_file(PhysicsFile, &header, 0, true);
				if(header.data_version==BUNGIE_PHYSICS_DATA_VERSION)
				{
					*bungie_physics= true;
				} else {
					*bungie_physics= false;
				}
			}
		}

		close_wad_file(PhysicsFile);
	} 
	
	/* Reset any errors that might have occurred.. */
	set_game_error(systemError, errNone);

	return wad;
}

static void import_physics_wad_data(
	struct wad_data *wad)
{
	// LP: this code is copied out of game_wad.c
	size_t data_length;
	byte *data;
	size_t count;
	bool PhysicsModelLoaded = false;
	
	data= (unsigned char *)extract_type_from_wad(wad, MONSTER_PHYSICS_TAG, &data_length);
	count = data_length/SIZEOF_monster_definition;
	assert(count*SIZEOF_monster_definition == data_length);
	assert(count <= NUMBER_OF_MONSTER_TYPES);
	if (data_length > 0)
	{
		PhysicsModelLoaded = true;
		unpack_monster_definition(data,count);
	}
	
	data= (unsigned char *)extract_type_from_wad(wad, EFFECTS_PHYSICS_TAG, &data_length);
	count = data_length/SIZEOF_effect_definition;
	assert(count*SIZEOF_effect_definition == data_length);
	assert(count <= NUMBER_OF_EFFECT_TYPES);
	if (data_length > 0)
	{
		PhysicsModelLoaded = true;
		unpack_effect_definition(data,count);
	}
	
	data= (unsigned char *)extract_type_from_wad(wad, PROJECTILE_PHYSICS_TAG, &data_length);
	count = data_length/SIZEOF_projectile_definition;
	assert(count*SIZEOF_projectile_definition == data_length);
	assert(count <= NUMBER_OF_PROJECTILE_TYPES);
	if (data_length > 0)
	{
		PhysicsModelLoaded = true;
		unpack_projectile_definition(data,count);
	}
	
	data= (unsigned char *)extract_type_from_wad(wad, PHYSICS_PHYSICS_TAG, &data_length);
	count = data_length/SIZEOF_physics_constants;
	assert(count*SIZEOF_physics_constants == data_length);
	assert(count <= get_number_of_physics_models());
	if (data_length > 0)
	{
		PhysicsModelLoaded = true;
		unpack_physics_constants(data,count);
	}
	
	data= (unsigned char*) extract_type_from_wad(wad, WEAPONS_PHYSICS_TAG, &data_length);
	count = data_length/SIZEOF_weapon_definition;
	assert(count*SIZEOF_weapon_definition == data_length);
	assert(count <= get_number_of_weapon_types());
	if (data_length > 0)
	{
		PhysicsModelLoaded = true;
		unpack_weapon_definition(data,count);
	}
}
