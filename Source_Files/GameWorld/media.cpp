/*
MEDIA.C

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

Sunday, March 26, 1995 1:13:11 AM  (Jason')

May 17, 2000 (Loren Petrich):
	Added XML support, including a damage parser

May 26, 2000 (Loren Petrich):
	Added XML shapes support

June 3, 2000 (Loren Petrich):
	Added some idiot-proofing: if a liquid type is not found, then it is assumed
	to be nonexistent. This is implemented by returning the null pointer.

July 1, 2000 (Loren Petrich):
	Inlined the media accessors

Aug 29, 2000 (Loren Petrich):
	Added packing and unpacking routines

Feb 8, 2001 (Loren Petrich):
	Fixed liquid-count bug in parallel with similar bug in map.cpp
*/

#include "cseries.h"

#include "map.h"
#include "media.h"
#include "effects.h"
#include "fades.h"
#include "lightsource.h"
#include "SoundManager.h"

#include "Packing.h"

// LP addition: XML parser for damage
#include "DamageParser.h"

#include <string.h>

#ifdef env68k
#pragma segment marathon
#endif

/* ---------- macros */

#define CALCULATE_MEDIA_HEIGHT(m) ((m)->low + FIXED_INTEGERAL_PART(((m)->high-(m)->low)*get_light_intensity((m)->light_index)))

/* ---------- globals */

// Turned the list of liquids into a variable array;
// took over their maximum number as how many of them

vector<media_data> MediaList;

// struct media_data *medias;

/* ---------- private prototypes */

static void update_one_media(size_t media_index, bool force_update);

/* ---------- globals */

#include "media_definitions.h"

static media_definition *get_media_definition(
	const short type);

/* ---------- code */

media_data *get_media_data(
	const size_t media_index)
{
	struct media_data *media = GetMemberWithBounds(medias,media_index,MAXIMUM_MEDIAS_PER_MAP);
	
	if (!media) return NULL;
	if (!(SLOT_IS_USED(media))) return NULL;
	
	return media;
}

// LP change: moved down here because it uses liquid definitions
media_definition *get_media_definition(
	const short type)
{
	return GetMemberWithBounds(media_definitions,type,NUMBER_OF_MEDIA_TYPES);
}


// light_index must be loaded
size_t new_media(
	struct media_data *initializer)
{
	struct media_data *media;
	size_t media_index;
	
	for (media_index= 0, media= medias; media_index<MAXIMUM_MEDIAS_PER_MAP; ++media_index, ++media)
	{
		if (SLOT_IS_FREE(media))
		{
			*media= *initializer;
			
			MARK_SLOT_AS_USED(media);
			
			media->origin.x= media->origin.y= 0;
			update_one_media(media_index, true);
			
			break;
		}
	}
	if (media_index==MAXIMUM_MEDIAS_PER_MAP) media_index= UNONE;
	
	return media_index;
}

bool media_in_environment(
	short media_type,
	short environment_code)
{
	// LP change: idiot-proofing
	struct media_definition *definition= get_media_definition(media_type);
	if (!definition) return false;
	
	return collection_in_environment(definition->collection, environment_code);
}

void update_medias(
	void)
{
	size_t media_index;
	struct media_data *media;
	
	for (media_index= 0, media= medias; media_index<MAXIMUM_MEDIAS_PER_MAP; ++media_index, ++media)
	{
		if (SLOT_IS_USED(media))
		{
			update_one_media(media_index, false);
			
			media->origin.x= WORLD_FRACTIONAL_PART(media->origin.x + ((cosine_table[media->current_direction]*media->current_magnitude)>>TRIG_SHIFT));
			media->origin.y= WORLD_FRACTIONAL_PART(media->origin.y + ((sine_table[media->current_direction]*media->current_magnitude)>>TRIG_SHIFT));
		}
	}
}

void get_media_detonation_effect(
	short media_index,
	short type,
	short *detonation_effect)
{
	struct media_data *media= get_media_data(media_index);
	// LP change: idiot-proofing
	if (!media) return;
	
	struct media_definition *definition= get_media_definition(media->type);
	if (!definition) return;

	if (type!=NONE)
	{
		if (!(type>=0 && type<NUMBER_OF_MEDIA_DETONATION_TYPES)) return;
		
		if (definition->detonation_effects[type]!=NONE) *detonation_effect= definition->detonation_effects[type];
	}
}

short get_media_sound(
	short media_index,
	short type)
{
	struct media_data *media= get_media_data(media_index);
	// LP change: idiot-proofing
	if (!media) return NONE;
	
	struct media_definition *definition= get_media_definition(media->type);
	if (!definition) return NONE;

	if (!(type>=0 && type<NUMBER_OF_MEDIA_SOUNDS)) return NONE;

	return definition->sounds[type];
}

struct damage_definition *get_media_damage(
	short media_index,
	_fixed scale)
{
	struct media_data *media= get_media_data(media_index);
	// LP change: idiot-proofing
	if (!media) return NULL;
	
	struct media_definition *definition= get_media_definition(media->type);
	if (!definition) return NULL;
	
	struct damage_definition *damage= &definition->damage;

	damage->scale= scale;
		
	return (damage->type==NONE || (dynamic_world->tick_count&definition->damage_frequency)) ?
		(struct damage_definition *) NULL : damage;
}

short get_media_submerged_fade_effect(
	short media_index)
{
	struct media_data *media= get_media_data(media_index);
	// LP change: idiot-proofing
	if (!media) return NONE;
	
	struct media_definition *definition= get_media_definition(media->type);
	if (!definition) return NONE;
	
	return definition->submerged_fade_effect;
}

bool get_media_collection(short media_index, short& collection)
{
	media_data *media = get_media_data(media_index);

	if (!media) return false;

	media_definition *definition = get_media_definition(media->type);
	if (!definition) return false;

	collection = definition->collection;
	return true;
}

// LP addition:
bool IsMediaDangerous(short media_index)
{
	media_definition *definition = get_media_definition(media_index);
	if (!definition) return false;
	
	struct damage_definition *damage= &definition->damage;
	if (damage->type == NONE) return false;
	else return (damage->base > 0);
}

/* ---------- private code */

static void update_one_media(
	size_t media_index,
	bool force_update)
{
	struct media_data *media= get_media_data(media_index);
	// LP change: idiot-proofing
	if (!media) return;
	struct media_definition *definition= get_media_definition(media->type);
	if (!definition) return;

	/* update height */
	media->height= (media->low + FIXED_INTEGERAL_PART((media->high-media->low)*get_light_intensity(media->light_index)));

	/* update texture */	
	media->texture= BUILD_DESCRIPTOR(definition->collection, definition->shape);
	media->transfer_mode= definition->transfer_mode;

#if 0
if (force_update || !(dynamic_world->tick_count&definition->shape_frequency))
	{
		shape_descriptor texture;
		
		do
		{
			texture= BUILD_DESCRIPTOR(definition->collection, definition->shape + global_random()%definition->shape_count);
		}
		while (definition->shape_count>1 && texture==media->texture);
		
		media->texture= BUILD_DESCRIPTOR(definition->collection, definition->shape);
		media->transfer_mode= definition->transfer_mode;
	}
#else
	(void)force_update;
#endif
}

// LP addition: count number of media types used,
// for better Infinity compatibility when saving games.
// Fixed countdown bug in parallel with similar bug in map.cpp
size_t count_number_of_medias_used()
{
	size_t number_used = 0; // Take care of the case of no slots being used
	for (int media_index=((int)MAXIMUM_MEDIAS_PER_MAP)-1; media_index>=0; media_index--)
	{
		// Look for the last used one rather than the last unused one!
		if (SLOT_IS_USED(medias + media_index))
		{
			number_used = (size_t)(media_index + 1);
			break;
		}
	}
	return number_used;	
}


uint8 *unpack_media_data(uint8 *Stream, media_data* Objects, size_t Count)
{
	uint8* S = Stream;
	media_data* ObjPtr = Objects;
	
	for (size_t k = 0; k < Count; k++, ObjPtr++)
	{
		StreamToValue(S,ObjPtr->type);
		StreamToValue(S,ObjPtr->flags);
		
		StreamToValue(S,ObjPtr->light_index);
		
		StreamToValue(S,ObjPtr->current_direction);
		StreamToValue(S,ObjPtr->current_magnitude);
		
		StreamToValue(S,ObjPtr->low);
		StreamToValue(S,ObjPtr->high);
		
		StreamToValue(S,ObjPtr->origin.x);
		StreamToValue(S,ObjPtr->origin.y);
		StreamToValue(S,ObjPtr->height);
		
		StreamToValue(S,ObjPtr->minimum_light_intensity);
		StreamToValue(S,ObjPtr->texture);
		StreamToValue(S,ObjPtr->transfer_mode);
		
		S += 2*2;
	}
	
	assert((S - Stream) == static_cast<ptrdiff_t>(Count*SIZEOF_media_data));
	return S;
}

uint8 *pack_media_data(uint8 *Stream, media_data* Objects, size_t Count)
{
	uint8* S = Stream;
	media_data* ObjPtr = Objects;
	
	for (size_t k = 0; k < Count; k++, ObjPtr++)
	{
		ValueToStream(S,ObjPtr->type);
		ValueToStream(S,ObjPtr->flags);
		
		ValueToStream(S,ObjPtr->light_index);
		
		ValueToStream(S,ObjPtr->current_direction);
		ValueToStream(S,ObjPtr->current_magnitude);
		
		ValueToStream(S,ObjPtr->low);
		ValueToStream(S,ObjPtr->high);
		
		ValueToStream(S,ObjPtr->origin.x);
		ValueToStream(S,ObjPtr->origin.y);
		ValueToStream(S,ObjPtr->height);
		
		ValueToStream(S,ObjPtr->minimum_light_intensity);
		ValueToStream(S,ObjPtr->texture);
		ValueToStream(S,ObjPtr->transfer_mode);
		
		S += 2*2;
	}
	
	assert((S - Stream) == static_cast<ptrdiff_t>(Count*SIZEOF_media_data));
	return S;
}


// Parses effect indices
// NOTE: this does not need a ResetValues() method because XML_LiquidParser (parent) does it already.
class XML_LqEffectParser: public XML_ElementParser
{
	short Type;
	short Which;

	enum {NumberOfValues = 2};
	bool IsPresent[NumberOfValues];

public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	
	short *EffectList;
	
	XML_LqEffectParser(): XML_ElementParser("effect") {}
};


bool XML_LqEffectParser::Start()
{
	for (int k=0; k<NumberOfValues; k++)
		IsPresent[k] = false;
	
	return true;
}

bool XML_LqEffectParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"type"))
	{
		if (ReadBoundedInt16Value(Value,Type,0,NUMBER_OF_MEDIA_DETONATION_TYPES-1))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"which"))
	{
		if (ReadBoundedInt16Value(Value,Which,NONE,NUMBER_OF_EFFECT_TYPES-1))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_LqEffectParser::AttributesDone()
{
	// Verify...
	bool AllPresent = true;
	for (int k=0; k<NumberOfValues; k++)
		AllPresent &= IsPresent[k];
	if (!AllPresent)
	{
		AttribsMissing();
		return false;
	}
	
	EffectList[Type] = Which;
			
	return true;
}

static XML_LqEffectParser LqEffectParser;


// Parses sound indices
// NOTE: this does not need a ResetValues() method because XML_LiquidParser (parent) does it already.
class XML_LqSoundParser: public XML_ElementParser
{
	short Type;
	short Which;

	enum {NumberOfValues = 2};
	bool IsPresent[NumberOfValues];

public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	
	short *SoundList;
	
	XML_LqSoundParser(): XML_ElementParser("sound") {}
};


bool XML_LqSoundParser::Start()
{
	for (int k=0; k<NumberOfValues; k++)
		IsPresent[k] = false;
	
	return true;
}

bool XML_LqSoundParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"type"))
	{
		if (ReadBoundedInt16Value(Value,Type,0,NUMBER_OF_MEDIA_SOUNDS-1))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"which"))
	{
		if (ReadBoundedInt16Value(Value,Which,NONE,SHRT_MAX))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_LqSoundParser::AttributesDone()
{
	// Verify...
	bool AllPresent = true;
	for (int k=0; k<NumberOfValues; k++)
		AllPresent &= IsPresent[k];
	if (!AllPresent)
	{
		AttribsMissing();
		return false;
	}
	
	SoundList[Type] = Which;
			
	return true;
}

static XML_LqSoundParser LqSoundParser;


struct media_definition *original_media_definitions = NULL;
class XML_LiquidParser: public XML_ElementParser
{
	short Index;
	media_definition Data;
	
	// What is present?
	bool IndexPresent;
	enum {NumberOfValues = 5};
	bool IsPresent[NumberOfValues];
	
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	bool ResetValues();

	XML_LiquidParser(): XML_ElementParser("liquid") {}
};

bool XML_LiquidParser::Start()
{
	// back up old values first
	if (!original_media_definitions) {
		original_media_definitions = (struct media_definition *) malloc(sizeof(struct media_definition) * NUMBER_OF_MEDIA_TYPES);
		assert(original_media_definitions);
		for (int i = 0; i < NUMBER_OF_MEDIA_TYPES; i++)
			original_media_definitions[i] = media_definitions[i];
	}

	IndexPresent = false;
	for (int k=0; k<NumberOfValues; k++)
		IsPresent[k] = false;
	
	return true;
}

bool XML_LiquidParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"index"))
	{
		if (ReadBoundedInt16Value(Value,Index,0,NUMBER_OF_MEDIA_TYPES-1))
		{
			IndexPresent = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"coll"))
	{
		if (ReadBoundedInt16Value(Value,Data.collection,0,NUMBER_OF_COLLECTIONS-1))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"frame"))
	{
		if (ReadBoundedInt16Value(Value,Data.shape,0,MAXIMUM_SHAPES_PER_COLLECTION-1))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"transfer"))
	{
		if (ReadBoundedInt16Value(Value,Data.transfer_mode,0,NUMBER_OF_TRANSFER_MODES-1))
		{
			IsPresent[2] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"damage_freq"))
	{
		if (ReadInt16Value(Value,Data.damage_frequency))
		{
			IsPresent[3] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"submerged"))
	{
		if (ReadBoundedInt16Value(Value,Data.submerged_fade_effect,0,NUMBER_OF_FADE_EFFECT_TYPES-1))
		{
			IsPresent[4] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_LiquidParser::AttributesDone()
{
	// Verify...
	if (!IndexPresent)
	{
		AttribsMissing();
		return false;
	}
	media_definition& OrigData = media_definitions[Index];
	
	if (IsPresent[0]) OrigData.collection = Data.collection;
	if (IsPresent[1]) OrigData.shape = Data.shape;
	if (IsPresent[2]) OrigData.transfer_mode = Data.transfer_mode;
	if (IsPresent[3]) OrigData.damage_frequency = Data.damage_frequency;
	if (IsPresent[4]) OrigData.submerged_fade_effect = Data.submerged_fade_effect;
	
	LqEffectParser.EffectList = OrigData.detonation_effects;
	LqSoundParser.SoundList = OrigData.sounds;
	Damage_SetPointer(&OrigData.damage);
	
	return true;
}

bool XML_LiquidParser::ResetValues()
{
	if (original_media_definitions) {
		for (int i = 0; i < NUMBER_OF_MEDIA_TYPES; i++)
			media_definitions[i] = original_media_definitions[i];
		free(original_media_definitions);
		original_media_definitions = NULL;
	}
	return true;
}

static XML_LiquidParser LiquidParser;


static XML_ElementParser LiquidsParser("liquids");


// XML-parser support
XML_ElementParser *Liquids_GetParser()
{
	LiquidParser.AddChild(&LqSoundParser);
	LiquidParser.AddChild(&LqEffectParser);
	LiquidParser.AddChild(Damage_GetParser());
	LiquidsParser.AddChild(&LiquidParser);
	
	return &LiquidsParser;
}
