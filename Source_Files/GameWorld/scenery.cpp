/*
SCENERY.C

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

Thursday, December 1, 1994 11:56:43 AM  (Jason)

Friday, June 16, 1995 11:48:23 AM  (Jason)
	animated scenery; audible scenery.
Tuesday, October 10, 1995 10:30:58 AM  (Jason)
	destroyable scenery; new_scenery doesn�t bail on out-of-range scenery.

Jan 30, 2000 (Loren Petrich):
	Added some typecasts

Feb 15, 2000 (Loren Petrich):
	Added some code to load scenery definitions
	And to manage animated scenery with a growable list

May 17, 2000 (Loren Petrich):
	Added XML support

May 18, 2000 (Loren Petrich):
	If an object has destruction effect NONE, then don't create an effect.

May 26, 2000 (Loren Petrich):
	Added XML shapes support

Oct 13, 2000 (Loren Petrich):
	Converted list to STL;
	also modified get_scenery_definition to return NULL for out-of-range scenery
*/

#include "cseries.h"

#include <string.h>
#include <stdlib.h>
#include <vector>
#include "ShapesParser.h"

#include "cseries.h"
#include "map.h"
#include "render.h"
#include "interface.h"
#include "flood_map.h"
#include "effects.h"
#include "monsters.h"
#include "projectiles.h"
#include "player.h"
#include "platforms.h"
#include "scenery.h"

#ifdef env68k
#pragma segment objects
#endif

/* ---------- constants */

enum
{
	MAXIMUM_ANIMATED_SCENERY_OBJECTS= 20
};

/* ---------- globals */

#include "scenery_definitions.h"


// LP change: growable list of animated scenery objects
static vector<short> AnimatedSceneryObjects;


/* ---------- private prototypes */

struct scenery_definition *get_scenery_definition(short scenery_type);

/* ---------- code */

void initialize_scenery(
	void)
{
	// Will reserve some space for them
	AnimatedSceneryObjects.reserve(32);
}

/* returns object index if successful, NONE otherwise */
short new_scenery(
	struct object_location *location,
	short scenery_type)
{
	short object_index= NONE;
	
	// This get function will do the bounds checking
	struct scenery_definition *definition= get_scenery_definition(scenery_type);
	if (!definition) return NONE;
	
	object_index= new_map_object(location, definition->shape);
	if (object_index!=NONE)
	{
		struct object_data *object= get_object_data(object_index);
		
		SET_OBJECT_OWNER(object, _object_is_scenery);
		SET_OBJECT_SOLIDITY(object, (definition->flags&_scenery_is_solid) ? true : false);
		object->permutation= scenery_type;
	}

	return object_index;
}

void animate_scenery(
	void)
{
	for (unsigned i=0; i<AnimatedSceneryObjects.size(); ++i)
		animate_object(AnimatedSceneryObjects[i]);
}

void deanimate_scenery(short object_index)
{
	std::vector<short>::iterator it;
	for (it = AnimatedSceneryObjects.begin(); it != AnimatedSceneryObjects.end() && *it != object_index; it++);
	
	if (it != AnimatedSceneryObjects.end())
		AnimatedSceneryObjects.erase(it);
}

void randomize_scenery_shape(short object_index)
{
	object_data *object = get_object_data(object_index);
	scenery_definition *definition = get_scenery_definition(object->permutation);
	if (!definition) return;

	if (!randomize_object_sequence(object_index, definition->shape))
	{
		AnimatedSceneryObjects.push_back(object_index);
	}
}

void randomize_scenery_shapes(
	void)
{
	struct object_data *object;
	short object_index;
	
	AnimatedSceneryObjects.clear();
	
	for (object_index= 0, object= objects; object_index<MAXIMUM_OBJECTS_PER_MAP; ++object_index, ++object)
	{
		if (GET_OBJECT_OWNER(object)==_object_is_scenery)
		{
			struct scenery_definition *definition= get_scenery_definition(object->permutation);
			if (!definition) continue;
			
			if (!randomize_object_sequence(object_index, definition->shape))
			{
				AnimatedSceneryObjects.push_back(object_index);
			}
		}
	}
}

void get_scenery_dimensions(
	short scenery_type,
	world_distance *radius,
	world_distance *height)
{
	struct scenery_definition *definition= get_scenery_definition(scenery_type);
	if (!definition)
	{
		// Fallback
		*radius = 0;
		*height = 0;
		return;
	}

	*radius= definition->radius;
	*height= definition->height;
}

void damage_scenery(
	short object_index)
{
	struct object_data *object= get_object_data(object_index);
	struct scenery_definition *definition= get_scenery_definition(object->permutation);
	if (!definition) return;
	
	if (definition->flags&_scenery_can_be_destroyed)
	{
		object->shape= definition->destroyed_shape;
		// LP addition: don't create a destruction effect if the effect type is NONE
		if (definition->destroyed_effect != NONE)
			new_effect(&object->location, object->polygon, definition->destroyed_effect, object->facing);
		SET_OBJECT_OWNER(object, _object_is_normal);
	}
}

bool get_scenery_collection(short scenery_type, short& collection)
{
	struct scenery_definition *definition = get_scenery_definition(scenery_type);
	if (!definition) return false;

	collection = GET_DESCRIPTOR_COLLECTION(definition->shape);
	return true;
}

bool get_damaged_scenery_collection(short scenery_type, short& collection)
{
	struct scenery_definition *definition = get_scenery_definition(scenery_type);
	if (!definition || !(definition->flags & _scenery_can_be_destroyed))
		return false;

	collection = GET_DESCRIPTOR_COLLECTION(definition->destroyed_shape);
	return true;
}

/* ---------- private code */

struct scenery_definition *get_scenery_definition(
	short scenery_type)
{
	return GetMemberWithBounds(scenery_definitions,scenery_type,NUMBER_OF_SCENERY_DEFINITIONS);
}


// For being more specific about the shapes -- either normal or destroyed
class XML_SceneryShapesParser: public XML_ElementParser
{
	public:
	shape_descriptor *DescPtr;
	
	bool Start()
	{
		Shape_SetPointer(DescPtr);
		return true;
	}
	
	XML_SceneryShapesParser(const char *_Name): XML_ElementParser(_Name) {}
};

static XML_SceneryShapesParser SceneryNormalParser("normal"), SceneryDestroyedParser("destroyed");

struct scenery_definition *original_scenery_definitions = NULL;
class XML_SceneryObjectParser: public XML_ElementParser
{
	short Index;
	scenery_definition Data;
	
	// What is present?
	bool IndexPresent;
	enum {NumberOfValues = 4};
	bool IsPresent[NumberOfValues];
	
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	bool ResetValues();

	XML_SceneryObjectParser(): XML_ElementParser("object") {}
};

bool XML_SceneryObjectParser::Start()
{
	// back up old values first
	if (!original_scenery_definitions) {
		original_scenery_definitions = (struct scenery_definition *) malloc(sizeof(struct scenery_definition) * NUMBER_OF_SCENERY_DEFINITIONS);
		assert(original_scenery_definitions);
		for (unsigned i = 0; i < NUMBER_OF_SCENERY_DEFINITIONS; i++)
			original_scenery_definitions[i] = scenery_definitions[i];
	}

	IndexPresent = false;
	for (int k=0; k<NumberOfValues; k++)
		IsPresent[k] = false;
	
	return true;
}

bool XML_SceneryObjectParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"index"))
	{
		if (ReadBoundedInt16Value(Value,Index,0,NUMBER_OF_SCENERY_DEFINITIONS-1))
		{
			IndexPresent = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"flags"))
	{
		if (ReadUInt16Value(Value,Data.flags))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"radius"))
	{
		if (ReadInt16Value(Value,Data.radius))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"height"))
	{
		if (ReadInt16Value(Value,Data.height))
		{
			IsPresent[2] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"destruction"))
	{
		if (ReadBoundedInt16Value(Value,Data.destroyed_effect,NONE,NUMBER_OF_EFFECT_TYPES-1))
		{
			IsPresent[3] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_SceneryObjectParser::AttributesDone()
{
	// Verify...
	if (!IndexPresent)
	{
		AttribsMissing();
		return false;
	}
	scenery_definition& OrigData = scenery_definitions[Index];
	
	if (IsPresent[0]) OrigData.flags = Data.flags;
	if (IsPresent[1]) OrigData.radius = Data.radius;
	if (IsPresent[2]) OrigData.height = Data.height;
	if (IsPresent[3]) OrigData.destroyed_effect = Data.destroyed_effect;
	
	SceneryNormalParser.DescPtr = &OrigData.shape;
	SceneryDestroyedParser.DescPtr = &OrigData.destroyed_shape;
			
	return true;
}

bool XML_SceneryObjectParser::ResetValues()
{
	if (original_scenery_definitions) {
		for (unsigned i = 0; i < NUMBER_OF_SCENERY_DEFINITIONS; i++)
			scenery_definitions[i] = original_scenery_definitions[i];
		free(original_scenery_definitions);
		original_scenery_definitions = NULL;
	}
	return true;
}

static XML_SceneryObjectParser SceneryObjectParser;


static XML_ElementParser SceneryParser("scenery");


// XML-parser support
XML_ElementParser *Scenery_GetParser()
{
	SceneryNormalParser.AddChild(Shape_GetParser());
	SceneryDestroyedParser.AddChild(Shape_GetParser());
	SceneryObjectParser.AddChild(&SceneryNormalParser);
	SceneryObjectParser.AddChild(&SceneryDestroyedParser);
	SceneryParser.AddChild(&SceneryObjectParser);
	
	return &SceneryParser;
}

