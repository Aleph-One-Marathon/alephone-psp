/*
OVERHEAD_MAP.C

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

Friday, June 10, 1994 2:53:51 AM

Saturday, June 11, 1994 1:27:58 AM
	the portable parts of this file should be moved into RENDER.C 
Friday, August 12, 1994 7:57:00 PM
	invisible polygons and lines are never drawn.
Thursday, September 8, 1994 8:19:15 PM (Jason)
	changed behavior of landscaped lines
Monday, October 24, 1994 4:35:38 PM (Jason)
	only draw the checkpoint at the origin.
Monday, October 31, 1994 3:52:00 PM (Jason)
	draw name of map on overhead map, last.
Monday, August 28, 1995 1:44:43 PM  (Jason)
	toward portability; removed clip region from _render_overhead_map.

Feb 3, 2000 (Loren Petrich):
	Jjaro-goo color is the same as the sewage color

Feb 4, 2000 (Loren Petrich):
	Changed halt() to assert(false) for better debugging

Feb 18, 2000 (Loren Petrich):
	Made VacBobs display properly 

May 2, 2000 (Loren Petrich):
	Added XML setting of overhead-map-display parameters;
	also imported the number of paths for displaying them.
	
	Can display alien monsters, items, projectiles, and paths.

Jul 4, 2000 (Loren Petrich):
	Made XML map-display settings compatible with the map cheat.

Jul 8, 2000 (Loren Petrich):
	Added support for OpenGL rendering;
	in these routines, it's the global flag OGL_MapActive,
	which indicates whether to do so in the overhead map

Jul 16, 2000 (Loren Petrich):
	Added begin/end pairs for polygons and lines,
	so that caching of them can be more efficient (important for OpenGL)

[Loren Petrich: notes for this file moved here]
OVERHEAD_MAP_MAC.C
Monday, August 28, 1995 1:41:36 PM  (Jason)

Feb 3, 2000 (Loren Petrich):
	Jjaro-goo color is the same as the sewage color

Feb 4, 2000 (Loren Petrich):
	Changed halt() to assert(false) for better debugging

Jul 8, 2000 (Loren Petrich):
	Added support for OpenGL rendering, in the form of calls to OpenGL versions

Jul 16, 2000 (Loren Petrich):
	Added begin/end pairs for polygons and lines,
	so that caching of them can be more efficient (important for OpenGL)

Aug 3, 2000 (Loren Petrich):
	All the code here has been transferred to either OverheadMapRenderer.c/h or OverheadMap_QuickDraw.c/h
[End notes for overhead_map_macintosh.c]

Nov 12, 2000 (Loren Petrich):
	Added automap reset function and XML parsing
*/

#include "cseries.h"

#include "shell.h" // for _get_player_color

#include "map.h"
#include "monsters.h"
#include "overhead_map.h"
#include "player.h"
#include "render.h"
#include "flood_map.h"
#include "platforms.h"
#include "media.h"

// LP addition: to parse the colors:
#include "ColorParser.h"

// Object-oriented setup of overhead-map rendering
#ifdef mac
#include "OverheadMap_QD.h"
#else
#include "OverheadMap_SDL.h"
#endif
#include "OverheadMap_OGL.h"

#include <string.h>
#include <stdlib.h>

#ifdef env68k
#pragma segment shell
#endif

#ifdef DEBUG
//#define PATH_DEBUG
//#define RENDER_DEBUG
#endif

#ifdef RENDER_DEBUG
extern struct view_data *world_view;
#endif

// Constants moved out to OverheadMapRender.h
// Render flags now in OverheadMapRender.c


// The configuration data
static OvhdMap_CfgDataStruct OvhdMap_ConfigData = 
{
	// Polygon colors
	{
		{0, 12000, 0},				// Plain polygon
		{30000, 0, 0},				// Platform
		{14*256, 37*256, 63*256},	// Water
		{76*256, 27*256, 0},		// Lava
		{70*256, 90*256, 0},		// Sewage
		{70*256, 90*256, 0},		// JjaroGoo
		{137*256, 0, 137*256},		// PfhorSlime
		{0, 12000, 0},			// Hill
		{76*256, 27*256, 0},		// Minor Damage
		{137*256, 0, 137*256}		// Major Damage
	},
	// Line definitions (color, 4 widths)
	{
		{{0, 65535, 0}, {1, 2, 2, 4}},	// Solid
		{{0, 40000, 0}, {1, 1, 1, 2}},	// Elevation
		{{65535, 0, 0}, {1, 2, 2, 4}}	// Control-Panel
	},
	// Thing definitions (color, shape, 4 radii)
	{
		{{0, 0, 65535}, _rectangle_thing, {1, 2, 4, 8}}, /* civilian */
		{{65535, 0, 0}, _rectangle_thing, {1, 2, 4, 8}}, /* non-player monster */
		{{65535, 65535, 65535}, _rectangle_thing, {1, 2, 3, 4}}, /* item */
		{{65535, 65535, 0}, _rectangle_thing, {1, 1, 2, 3}}, /* projectiles */
		{{65535, 0, 0}, _circle_thing, {8, 16, 16, 16}}	// LP note: this is for checkpoint locations
	},
	// Live-monster type assignments
	{
		// Marine
		_civilian_thing,
		// Ticks
		_monster_thing,
		_monster_thing,
		_monster_thing,
		// S'pht
		_monster_thing,
		_monster_thing,
		_monster_thing,
		_monster_thing,
		// Pfhor
		_monster_thing,
		_monster_thing,
		_monster_thing,
		_monster_thing,
		// Bob
		_civilian_thing,
		_civilian_thing,
		_civilian_thing,
		_civilian_thing,
		// Drone
		_monster_thing,
		_monster_thing,
		_monster_thing,
		_monster_thing,
		_monster_thing,
		// Cyborg
		_monster_thing,
		_monster_thing,
		_monster_thing,
		_monster_thing,
		// Enforcer
		_monster_thing,
		_monster_thing,
		// Hunter
		_monster_thing,
		_monster_thing,
		// Trooper
		_monster_thing,
		_monster_thing,
		// Big Cyborg, Hunter
		_monster_thing,
		_monster_thing,
		// F'lickta
		_monster_thing,
		_monster_thing,
		_monster_thing,
		// S'pht'Kr
		_monster_thing,
		_monster_thing,
		// Juggernauts
		_monster_thing,
		_monster_thing,
		// Tiny ones
		_monster_thing,
		_monster_thing,
		_monster_thing,
		// VacBobs
		_civilian_thing,
		_civilian_thing,
		_civilian_thing,
		_civilian_thing,
	},
	// Dead-monster type assignments
	{
		NONE,				// Interface (what one sees in the HUD)
		NONE,				// Weapons in Hand
		
		_monster_thing,		// Juggernaut
		_monster_thing,		// Tick
		_monster_thing,		// Explosion effects
		_monster_thing,		// Hunter
		NONE,				// Player
	
		_monster_thing,		// Items
		_monster_thing,		// Trooper
		_monster_thing,		// Fighter
		_monster_thing,		// S'pht'Kr
		_monster_thing,		// F'lickta
		
		_civilian_thing,	// Bob
		_civilian_thing,	// VacBob
		_monster_thing,		// Enforcer
		_monster_thing,		// Drone
		_monster_thing,		// S'pht
		
		NONE,				// Water
		NONE,				// Lava
		NONE,				// Sewage
		NONE,				// Jjaro
		NONE,				// Pfhor
	
		NONE,				// Water Scenery
		NONE,				// Lava Scenery
		NONE,				// Sewage Scenery
		NONE,				// Jjaro Scenery
		NONE,				// Pfhor Scenery
		
		NONE,				// Day
		NONE,				// Night
		NONE,				// Moon
		NONE,				// Outer Space
		
		_monster_thing		// Cyborg
	},
	// Player-entity definition
	{16, 10, (7*NUMBER_OF_ANGLES)/20},
	// Annotations (color, 4 fonts)
	{
		{{0, 65535, 0},
		{
			{"Monaco",  5, styleBold, 0, "#4"},
			{"Monaco",  9, styleBold, 0, "#4"},
			{"Monaco", 12, styleBold, 0, "#4"},
			{"Monaco", 18, styleBold, 0, "#4"},
		}}
	},
	// Map name (color, font)
	{{0, 65535, 0}, {"Monaco", 18, styleNormal, 0, "#4"}, 25},
	// Path color
	{65535, 65535, 65535},
	// What to show (aliens, items, projectiles, paths)
	false, false, false, false
};
static bool MapFontsInited = false;

#ifdef HAVE_SDL_TTF
void fix_missing_overhead_map_fonts()
{
	strcpy(OvhdMap_ConfigData.map_name_data.Font.File, "mono");
	for (int k = 0; k < NUMBER_OF_ANNOTATION_DEFINITIONS; k++)
	{
		for (int s = 0; s <= OVERHEAD_MAP_MAXIMUM_SCALE-OVERHEAD_MAP_MINIMUM_SCALE; s++)
		{
			strcpy(OvhdMap_ConfigData.annotation_definitions[k].Fonts[s].File, "mono");
		}
	}
	
}
#endif

// Is OpenGL rendering of the map currently active?
// Set this from outside, because we want to make an OpenGL rendering for the main view,
// yet a software rendering for an in-terminal checkpoint view
bool OGL_MapActive = false;

// Software rendering
#ifdef mac
static OverheadMap_QD_Class OverheadMap_SW;
#else
static OverheadMap_SDL_Class OverheadMap_SW;
#endif
// OpenGL rendering
#ifdef HAVE_OPENGL
static OverheadMap_OGL_Class OverheadMap_OGL;
#endif

// Overhead-map-rendering mode
enum {
	OverheadMap_Normal,
	OverheadMap_CurrentlyVisible,
	OverheadMap_All,
	NUMBER_OF_OVERHEAD_MAP_MODES
};
static short OverheadMapMode = OverheadMap_Normal;


/* ---------- code */
// LP: most of it has been moved into OverheadMapRenderer.c

static void InitMapFonts()
{
	// Init the fonts the first time through
	if (!MapFontsInited)
	{
		for (int i=0; i<NUMBER_OF_ANNOTATION_DEFINITIONS; i++)
		{
			annotation_definition& NoteDef = OvhdMap_ConfigData.annotation_definitions[i];
			for (int j=0; j<NUMBER_OF_ANNOTATION_SIZES; j++)
				NoteDef.Fonts[j].Init();
		}
		OvhdMap_ConfigData.map_name_data.Font.Init();
		MapFontsInited = true;
	}
}

void _render_overhead_map(
	struct overhead_map_data *data)
{
	InitMapFonts();
		
	// Select which kind of rendering (OpenGL or software)
	OverheadMapClass *OvhdMapPtr;
#ifdef HAVE_OPENGL
	if (OGL_MapActive)
		OvhdMapPtr = &OverheadMap_OGL;
	else
#endif
		OvhdMapPtr = &OverheadMap_SW;
	
	// Do the rendering
	OvhdMapPtr->ConfigPtr = &OvhdMap_ConfigData;
	OvhdMapPtr->Render(*data);
}


// Call this from outside
void OGL_ResetMapFonts(bool IsStarting)
{
#ifdef HAVE_OPENGL
	InitMapFonts();
	for (int i=0; i<NUMBER_OF_ANNOTATION_DEFINITIONS; i++)
	{
		annotation_definition& NoteDef = OvhdMap_ConfigData.annotation_definitions[i];
		for (int j=0; j<NUMBER_OF_ANNOTATION_SIZES; j++)
			NoteDef.Fonts[j].OGL_Reset(IsStarting);
	}
	OvhdMap_ConfigData.map_name_data.Font.OGL_Reset(IsStarting);
#endif
}

void ResetOverheadMap()
{
	// Default: nothing (mapping is cumulative)
	switch(OverheadMapMode)
	{
	case OverheadMap_CurrentlyVisible:
		// No previous visibility is carried over
		memset(automap_lines, 0, (dynamic_world->line_count/8+((dynamic_world->line_count%8)?1:0)*sizeof(byte)));
		memset(automap_polygons, 0, (dynamic_world->polygon_count/8+((dynamic_world->polygon_count%8)?1:0)*sizeof(byte)));
		
		break;
		
	case OverheadMap_All:
		// Everything is assumed visible
		memset(automap_lines, 0xff, (dynamic_world->line_count/8+((dynamic_world->line_count%8)?1:0)*sizeof(byte)));
		memset(automap_polygons, 0xff, (dynamic_world->polygon_count/8+((dynamic_world->polygon_count%8)?1:0)*sizeof(byte)));
		
		break;
	};
}



// XML elements for parsing motion-sensor specification;
// this is a specification of what monster type gets what
// overhead-map display. It's by monster type for living monsters
// and by collection type for dead monsters.

// Parser for living monsters
class XML_LiveAssignParser: public XML_ElementParser
{
	bool IsPresent[2];
	short Monster, Type;

public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
		
	XML_LiveAssignParser(): XML_ElementParser("assign_live") {}
};

bool XML_LiveAssignParser::Start()
{
	for (int k=0; k<2; k++)
		IsPresent[k] = false;
	return true;
}

bool XML_LiveAssignParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"monster"))
	{
		if (ReadBoundedInt16Value(Value,Monster,0,NUMBER_OF_MONSTER_TYPES-1))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"type"))
	{
		// The permissible values, -1, 0, and 1, are
		// NONE
		// _civilian_thing
		// _monster_thing
		if (ReadBoundedInt16Value(Value,Type,short(-1),short(1)))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_LiveAssignParser::AttributesDone()
{
	// Verify...
	bool AllPresent = IsPresent[0] && IsPresent[1];
	
	if (!AllPresent)
	{
		AttribsMissing();
		return false;
	}
	
	// Put into place
	OvhdMap_ConfigData.monster_displays[Monster] = Type;
		
	return true;
}

static XML_LiveAssignParser LiveAssignParser;


// Parser for dead monsters
class XML_DeadAssignParser: public XML_ElementParser
{
	bool IsPresent[2];
	short Coll, Type;

public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
		
	XML_DeadAssignParser(): XML_ElementParser("assign_dead") {}
};

bool XML_DeadAssignParser::Start()
{
	for (int k=0; k<2; k++)
		IsPresent[k] = false;
	return true;
}

bool XML_DeadAssignParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"coll"))
	{
		if (ReadBoundedInt16Value(Value,Coll,0,NUMBER_OF_COLLECTIONS-1))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"type"))
	{
		// The permissible values, -1, 0, and 1, are
		// NONE
		// _civilian_thing
		// _monster_thing
		if (ReadBoundedInt16Value(Value,Type,short(-1),short(1)))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_DeadAssignParser::AttributesDone()
{
	// Verify...
	bool AllPresent = IsPresent[0] && IsPresent[1];
	
	if (!AllPresent)
	{
		AttribsMissing();
		return false;
	}
		
	OvhdMap_ConfigData.dead_monster_displays[Coll] = Type;
	
	return true;
}

static XML_DeadAssignParser DeadAssignParser;


// Boolean-attribute parser: for switching stuff on and off
class XML_OvhdMapBooleanParser: public XML_ElementParser
{
	bool IsPresent;

public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	
	// Out here so it can be changed
	short IsOn;
	// Whether this element got used or not
	short GotUsed;
		
	XML_OvhdMapBooleanParser(const char *_Name): XML_ElementParser(_Name) {}
};

bool XML_OvhdMapBooleanParser::Start()
{
	IsPresent = false;
	return true;
}

bool XML_OvhdMapBooleanParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"on"))
	{
		if (ReadBooleanValueAsInt16(Value,IsOn))
		{
			IsPresent = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_OvhdMapBooleanParser::AttributesDone()
{
	// Verify...
	if (!IsPresent)
	{
		AttribsMissing();
		return false;
	}
	GotUsed = true;
	return true;
}

static XML_OvhdMapBooleanParser
	ShowAliensParser("aliens"),
	ShowItemsParser("items"),
	ShowProjectilesParser("projectiles"),
	ShowPathsParser("paths");


// Parser for living monsters
class XML_LineWidthParser: public XML_ElementParser
{
	bool IsPresent[3];
	short Type, Scale, Width;

public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
		
	XML_LineWidthParser(): XML_ElementParser("line") {}
};

bool XML_LineWidthParser::Start()
{
	for (int k=0; k<3; k++)
		IsPresent[k] = false;
	return true;
}

bool XML_LineWidthParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"type"))
	{
		// The permissible values, 0, 1, and 2, are line, elevation, and control panel
		if (ReadBoundedInt16Value(Value,Type,0,NUMBER_OF_LINE_DEFINITIONS-1))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"scale"))
	{
		if (ReadBoundedInt16Value(Value,Scale,0,OVERHEAD_MAP_MAXIMUM_SCALE-OVERHEAD_MAP_MINIMUM_SCALE))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"width"))
	{
		if (ReadInt16Value(Value,Width))
		{
			IsPresent[2] = true;
			return true;
		}
		else return false;
	}
	UnrecognizedTag();
	return false;
}

bool XML_LineWidthParser::AttributesDone()
{
	// Verify...
	bool AllPresent = true;
	for (int k=0; k<3; k++)
		AllPresent = AllPresent && IsPresent[k];
	
	if (!AllPresent)
	{
		AttribsMissing();
		return false;
	}
	
	// Put into place
	OvhdMap_ConfigData.line_definitions[Type].pen_sizes[Scale] = Width;
		
	return true;
}

static XML_LineWidthParser LineWidthParser;
	

// Subclassed to set the color objects appropriately
const int TOTAL_NUMBER_OF_COLORS =
	NUMBER_OF_POLYGON_COLORS + NUMBER_OF_LINE_DEFINITIONS +
	NUMBER_OF_THINGS + NUMBER_OF_ANNOTATION_DEFINITIONS + 2;

const int TOTAL_NUMBER_OF_FONTS = 
	NUMBER_OF_ANNOTATION_DEFINITIONS*(OVERHEAD_MAP_MAXIMUM_SCALE-OVERHEAD_MAP_MINIMUM_SCALE + 1) + 1;

class XML_OvhdMapParser: public XML_ElementParser
{
	// Extras are:
	// annotation color
	// map-title color
	// path color
	rgb_color Colors[TOTAL_NUMBER_OF_COLORS];
	FontSpecifier Fonts[TOTAL_NUMBER_OF_FONTS];
	
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool End();
	
	XML_OvhdMapParser(): XML_ElementParser("overhead_map") {}
};

bool XML_OvhdMapParser::Start()
{
	ShowAliensParser.GotUsed = false;
	ShowItemsParser.GotUsed = false;
	ShowProjectilesParser.GotUsed = false;
	ShowPathsParser.GotUsed = false;
	
	// Copy in the colors
	rgb_color *ColorPtr = Colors;
	
	rgb_color *PolyColorPtr = OvhdMap_ConfigData.polygon_colors;
	for (int k=0; k<NUMBER_OF_OLD_POLYGON_COLORS; k++)
		*(ColorPtr++) = *(PolyColorPtr++);

	line_definition *LineDefPtr = OvhdMap_ConfigData.line_definitions;
	for (int k=0; k<NUMBER_OF_LINE_DEFINITIONS; k++)
		*(ColorPtr++) = (LineDefPtr++)->color;
	
	thing_definition *ThingDefPtr = OvhdMap_ConfigData.thing_definitions;
	for (int k=0; k<NUMBER_OF_THINGS; k++)
		*(ColorPtr++) = (ThingDefPtr++)->color;
	
	annotation_definition *NoteDefPtr = OvhdMap_ConfigData.annotation_definitions;
	for (int k=0; k<NUMBER_OF_ANNOTATION_DEFINITIONS; k++)
		*(ColorPtr++) = (NoteDefPtr++)->color;
	
	*(ColorPtr++) = OvhdMap_ConfigData.map_name_data.color;
	*(ColorPtr++) = OvhdMap_ConfigData.path_color;
	
	for (int k=NUMBER_OF_OLD_POLYGON_COLORS; k<NUMBER_OF_POLYGON_COLORS; k++)
		*(ColorPtr++) = *(PolyColorPtr++);
	
	assert(ColorPtr == Colors + TOTAL_NUMBER_OF_COLORS);
	
	Color_SetArray(Colors,TOTAL_NUMBER_OF_COLORS);
	
	// Copy in the fonts
	FontSpecifier *FontPtr = Fonts;
	
	NoteDefPtr = OvhdMap_ConfigData.annotation_definitions;
	for (int k=0; k<NUMBER_OF_ANNOTATION_DEFINITIONS; k++)
	{
		FontSpecifier *NoteFontPtr = NoteDefPtr[k].Fonts;
		for (int s=0; s<=OVERHEAD_MAP_MAXIMUM_SCALE-OVERHEAD_MAP_MINIMUM_SCALE; s++)
			*(FontPtr++) = *(NoteFontPtr++);
	}
	*(FontPtr++) = OvhdMap_ConfigData.map_name_data.Font;
	
	assert(FontPtr == Fonts + TOTAL_NUMBER_OF_FONTS);
	
	Font_SetArray(Fonts,TOTAL_NUMBER_OF_FONTS);
	
	return true;
}

bool XML_OvhdMapParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"mode"))
	{
		return ReadBoundedInt16Value(Value,OverheadMapMode,0,NUMBER_OF_OVERHEAD_MAP_MODES-1);
	}
	else if (StringsEqual(Tag,"title_offset"))
	{
		return ReadInt16Value(Value,OvhdMap_ConfigData.map_name_data.offset_down);
	}
	UnrecognizedTag();
	return false;
}

bool XML_OvhdMapParser::End()
{
	if (ShowAliensParser.GotUsed)
		OvhdMap_ConfigData.ShowAliens = (ShowAliensParser.IsOn != 0);
	if (ShowItemsParser.GotUsed)
		OvhdMap_ConfigData.ShowItems = (ShowItemsParser.IsOn != 0);
	if (ShowProjectilesParser.GotUsed)
		OvhdMap_ConfigData.ShowProjectiles = (ShowProjectilesParser.IsOn != 0);
	if (ShowPathsParser.GotUsed)
		OvhdMap_ConfigData.ShowPaths = (ShowPathsParser.IsOn != 0);
		
	// Copy out the colors
	rgb_color *ColorPtr = Colors;
	
	rgb_color *PolyColorPtr = OvhdMap_ConfigData.polygon_colors;
	for (int k=0; k<NUMBER_OF_OLD_POLYGON_COLORS; k++)
		*(PolyColorPtr++) = *(ColorPtr++);

	line_definition *LineDefPtr = OvhdMap_ConfigData.line_definitions;
	for (int k=0; k<NUMBER_OF_LINE_DEFINITIONS; k++)
		(LineDefPtr++)->color = *(ColorPtr++);

	thing_definition *ThingDefPtr = OvhdMap_ConfigData.thing_definitions;
	for (int k=0; k<NUMBER_OF_THINGS; k++)
		(ThingDefPtr++)->color = *(ColorPtr++);

	annotation_definition *NoteDefPtr = OvhdMap_ConfigData.annotation_definitions;
	for (int k=0; k<NUMBER_OF_ANNOTATION_DEFINITIONS; k++)
		(NoteDefPtr++)->color = *(ColorPtr++);
	
	OvhdMap_ConfigData.map_name_data.color = *(ColorPtr++);
	OvhdMap_ConfigData.path_color = *(ColorPtr++);
	
	for (int k=NUMBER_OF_OLD_POLYGON_COLORS; k<NUMBER_OF_POLYGON_COLORS; k++)
		*(PolyColorPtr++) = *(ColorPtr++);
	
	assert(ColorPtr == Colors + TOTAL_NUMBER_OF_COLORS);
	
	// Copy out the fonts
	FontSpecifier *FontPtr = Fonts;

	NoteDefPtr = OvhdMap_ConfigData.annotation_definitions;
	for (int k=0; k<NUMBER_OF_ANNOTATION_DEFINITIONS; k++)
	{
		FontSpecifier *NoteFontPtr = NoteDefPtr[k].Fonts;
		for (int s=0; s<=OVERHEAD_MAP_MAXIMUM_SCALE-OVERHEAD_MAP_MINIMUM_SCALE; s++)
			*(NoteFontPtr++) = *(FontPtr++);
	}
	OvhdMap_ConfigData.map_name_data.Font = *(FontPtr++);
	
	assert(FontPtr == Fonts + TOTAL_NUMBER_OF_FONTS);

	return true;
}

static XML_OvhdMapParser OvhdMapParser;


// LP change: added overhead-map-parser export
XML_ElementParser *OverheadMap_GetParser()
{
	OvhdMapParser.AddChild(&LiveAssignParser);
	OvhdMapParser.AddChild(&DeadAssignParser);
	OvhdMapParser.AddChild(&ShowAliensParser);
	OvhdMapParser.AddChild(&ShowItemsParser);
	OvhdMapParser.AddChild(&ShowProjectilesParser);
	OvhdMapParser.AddChild(&ShowPathsParser);
	OvhdMapParser.AddChild(&LineWidthParser);
	OvhdMapParser.AddChild(Color_GetParser());
	OvhdMapParser.AddChild(Font_GetParser());
	
	return &OvhdMapParser;
}
