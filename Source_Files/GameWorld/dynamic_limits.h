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

Feb 10, 2000 (Loren Petrich):
	
	This is the interface for getting various limits on dynamically-set quantities
	such as number of objects, number of NPC's (monsters, aliens), number of projectiles, etc.
	
	Their initial values are gotten from a resource.
	
May 4, 2000
	Replaced resource-fork initialization with XML initialization
*/

#ifndef MARATHON_DYNAMIC_ENTITY_LIMITS
#define MARATHON_DYNAMIC_ENTITY_LIMITS

#include "XML_ElementParser.h"


// Limit types:
enum {
	_dynamic_limit_objects,				// Objects (every possible kind)
	_dynamic_limit_monsters,			// NPC's
	_dynamic_limit_paths,				// Paths for NPC's to follow (determines how many may be active)
	_dynamic_limit_projectiles,			// Projectiles
	_dynamic_limit_effects,				// Currently-active effects (blood splatters, explosions, etc.)
	_dynamic_limit_rendered,			// Number of objects to render
	_dynamic_limit_local_collision,		// [16] Local collision buffer (target visibility, NPC-NPC collisions, etc.)
	_dynamic_limit_global_collision,	// [64] Global collision buffer (projectiles with other objects) 
	NUMBER_OF_DYNAMIC_LIMITS
};


// XML-parser support
XML_ElementParser *DynamicLimits_GetParser();

// Accessor
uint16 get_dynamic_limit(int which);

#endif
