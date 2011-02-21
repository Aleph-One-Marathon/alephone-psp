#include <cstdlib>
/*
GAME_WINDOW.C

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

Thursday, December 30, 1993 9:51:24 PM

Tuesday, May 24, 1994 6:42:32 PM
	functions beginning with underscores are mac-specific and always have a corresponding portable
	function which sets up game-specific information (see update_compass and _update_compass for
	an example).
Friday, June 10, 1994 3:56:57 AM
	gutted, rewritten.  Much cleaner now.
Sunday, September 4, 1994 6:32:05 PM
	made scroll_inventory() non-static so that shell.c can call it.
Saturday, September 24, 1994 10:33:19 AM
	fixed some things, twice...
Friday, October 7, 1994 3:13:22 PM
	New interface.  Draws panels from PICT resources for memory consumption.  Inventory is 
	different panels, which are switched to whenever you grab an item.  There is no scrolling.
Tuesday, June 6, 1995 3:37:50 PM
	Marathon II modifications.
Tuesday, August 29, 1995 4:02:15 PM
	Reestablishing a level of portablility...

Feb 4, 2000 (Loren Petrich):
	Added SMG display stuff
	
Apr 30, 2000 (Loren Petrich): Added XML parser object for all the screen_drawing data,
	and also essentiall all the data here.

May 28, 2000 (Loren Petrich): Added support for buffering the Heads-Up Display

Jul 2, 2000 (Loren Petrich):
	The HUD is now always buffered

Jul 16, 2001 (Loren Petrich):
	Using "temporary" as storage space for count_text and weapon_name;
	it is 256 bytes long, which should be more than enough for most text.
	This fixes the long-weapon-name bug.

Mar 08, 2002 (Woody Zenfell):
    SDL network microphone support
*/

#include "cseries.h"

#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef HAVE_OPENGL
# if defined (__APPLE__) && defined (__MACH__)
#   include <OpenGL/gl.h>
# elif defined mac
#   include <gl.h>
# else
#  include <GL/gl.h>
# endif
#endif

#include "HUDRenderer_SW.h"
#include "game_window.h"

// LP addition: color and font parsers
#include "ColorParser.h"
#include "FontHandler.h"
#include "screen.h"

#include    "network_sound.h"

#ifdef HAVE_OPENGL
# if defined (__APPLE__) && defined (__MACH__)
#   include <OpenGL/gl.h>
# elif defined mac
#   include <gl.h>
# else
#   include <GL/gl.h>
# endif
#endif


#ifdef env68k
	#pragma segment screen
#endif

extern void draw_panels(void);
extern void validate_world_window(void);
static void set_current_inventory_screen(short player_index, short screen);

/* --------- globals */

// LP addition: whether or not the motion sensor is active
bool MotionSensorActive = true;

struct interface_state_data interface_state;

#define NUMBER_OF_WEAPON_INTERFACE_DEFINITIONS	10
struct weapon_interface_data weapon_interface_definitions[NUMBER_OF_WEAPON_INTERFACE_DEFINITIONS] =
{
	/* Mac, the knife.. */
	{
		_i_knife,
		UNONE,
		433, 432,
		NONE, NONE,
		0, 0,
		false,
		{
			{ _unused_interface_data, 0, 0, 0, 0, 0, 0, UNONE, UNONE, true},
			{ _unused_interface_data, 0, 0, 0, 0, 0, 0, UNONE, UNONE, true}
		}
	},
	
	/* Harry, the .44 */
	{
		_i_magnum,
		BUILD_DESCRIPTOR(_collection_interface, _magnum_panel),
		432, 444,
		420, NONE,
		366, 517, 
		true,
		{
			{ _uses_bullets, 517, 412, 8, 1, 5, 14, BUILD_DESCRIPTOR(_collection_interface, _magnum_bullet), BUILD_DESCRIPTOR(_collection_interface, _magnum_casing), false},
			{ _uses_bullets, 452, 412, 8, 1, 5, 14, BUILD_DESCRIPTOR(_collection_interface, _magnum_bullet), BUILD_DESCRIPTOR(_collection_interface, _magnum_casing), true}
		}
	},

	/* Ripley, the plasma pistol. */
	{
		_i_plasma_pistol,
		BUILD_DESCRIPTOR(_collection_interface, _zeus_panel),
		431, 443,
		401, NONE,
		366, 475, 
		false,
		{
			{ _uses_energy, 414, 366, 20, 0, 38, 57, _energy_weapon_full_color, _energy_weapon_empty_color, true},
			{ _unused_interface_data, 450, 410, 50, 0, 62, 7, _energy_weapon_full_color, _energy_weapon_empty_color, true}
		}
	},
	
	/* Arnold, the assault rifle */	
	{
		_i_assault_rifle,
		BUILD_DESCRIPTOR(_collection_interface, _assault_panel),
		430, 452,
		439, NONE, //��
		366, 460, 
		false,
		{
			{ _uses_bullets, 391, 368, 13, 4, 4, 10, BUILD_DESCRIPTOR(_collection_interface, _assault_rifle_bullet), BUILD_DESCRIPTOR(_collection_interface, _assault_rifle_casing), true},
			{ _uses_bullets, 390, 413, 7, 1, 8, 12, BUILD_DESCRIPTOR(_collection_interface, _assault_rifle_grenade), BUILD_DESCRIPTOR(_collection_interface, _assault_rifle_grenade_casing), true},
		}
	},
		
	/* John R., the missile launcher */	
	{
		_i_missile_launcher,
		BUILD_DESCRIPTOR(_collection_interface, _missile_panel),
		433, 445,
		426, NONE,
		365, 419, 
		false,
		{
			{ _uses_bullets, 385, 376, 2, 1, 16, 49, BUILD_DESCRIPTOR(_collection_interface, _missile), BUILD_DESCRIPTOR(_collection_interface, _missile_casing), true},
			{ _unused_interface_data, 0, 0, 0, 0, 0, 0, UNONE, UNONE, true }
		}
	},

	/* ???, the flame thrower */	
	{
		_i_flamethrower,
		BUILD_DESCRIPTOR(_collection_interface, _flamethrower_panel),
		433, 445,
		398, NONE,
		363, 475, 
		false,
		{
			/* This weapon has 7 seconds of flamethrower carnage.. */
			{ _uses_energy, 427, 369, 7*TICKS_PER_SECOND, 0, 38, 57, _energy_weapon_full_color, _energy_weapon_empty_color, true},
			{ _unused_interface_data, 450, 410, 50, 0, 62, 7, _energy_weapon_full_color, _energy_weapon_empty_color, true}
		}
	},

	/* Predator, the alien shotgun */	
	{
		_i_alien_shotgun,
		BUILD_DESCRIPTOR(_collection_interface, _alien_weapon_panel),
		418, 445,
		395, 575,
		359, 400, 
		false,
		{
			{ _unused_interface_data, 425, 411, 50, 0, 96, 7, _energy_weapon_full_color, _energy_weapon_empty_color, true},
			{ _unused_interface_data, 450, 410, 50, 0, 62, 7, _energy_weapon_full_color, _energy_weapon_empty_color, true}
		}
	},

	/* Shotgun */
	{
		_i_shotgun,
		BUILD_DESCRIPTOR(_collection_interface, _single_shotgun),
		432, 444,
		420, NONE,
		373, 451, 
		true,
		{
			{ _uses_bullets, 483, 411, 2, 1, 12, 16, BUILD_DESCRIPTOR(_collection_interface, _shotgun_bullet), BUILD_DESCRIPTOR(_collection_interface, _shotgun_casing), true},
			{ _uses_bullets, 451, 411, 2, 1, 12, 16, BUILD_DESCRIPTOR(_collection_interface, _shotgun_bullet), BUILD_DESCRIPTOR(_collection_interface, _shotgun_casing), true}
		}
	},

	/* Ball */
	{
		_i_red_ball, // statistically unlikely to be valid (really should be SKULL)
		BUILD_DESCRIPTOR(_collection_interface, _skull),
		432, 444,
		402, NONE,
		366, 465, 
		false,
		{
			{ _unused_interface_data, 451, 411, 2, 1, 12, 16, BUILD_DESCRIPTOR(_collection_interface, _shotgun_bullet), BUILD_DESCRIPTOR(_collection_interface, _shotgun_casing), true},
			{ _unused_interface_data, 483, 411, 2, 1, 12, 16, BUILD_DESCRIPTOR(_collection_interface, _shotgun_bullet), BUILD_DESCRIPTOR(_collection_interface, _shotgun_casing), true}
		}
	},
	
	/* LP addition: SMG (clone of assault rifle) */	
	{
		_i_smg,
		BUILD_DESCRIPTOR(_collection_interface, _smg),
		430, 452,
		439, NONE, //��
		366, 460, 
		false,
		{
			{ _uses_bullets, 405, 382, 8, 4, 5, 10, BUILD_DESCRIPTOR(_collection_interface, _smg_bullet), BUILD_DESCRIPTOR(_collection_interface, _smg_casing), true},
			{ _unused_interface_data, 390, 413, 7, 1, 8, 12, BUILD_DESCRIPTOR(_collection_interface, _assault_rifle_grenade), BUILD_DESCRIPTOR(_collection_interface, _assault_rifle_grenade_casing), true},
		}
	},
};

// Is OpenGL rendering of the HUD currently active?
bool OGL_HUDActive = false;

// Software rendering
HUD_SW_Class HUD_SW;

/* --------- code */

void initialize_game_window(void)
{
	initialize_motion_sensor(
		BUILD_DESCRIPTOR(_collection_interface, _motion_sensor_mount),
		BUILD_DESCRIPTOR(_collection_interface, _motion_sensor_virgin_mount),
		BUILD_DESCRIPTOR(_collection_interface, _motion_sensor_alien),
		BUILD_DESCRIPTOR(_collection_interface, _motion_sensor_friend),
		BUILD_DESCRIPTOR(_collection_interface, _motion_sensor_enemy),
		BUILD_DESCRIPTOR(_collection_interface, _network_compass_shape_nw),
		MOTION_SENSOR_SIDE_LENGTH);
}

/* draws the entire interface */
void draw_interface(void)
{
	if (OGL_HUDActive)
		return;

	if (!game_window_is_full_screen())
	{
		/* draw the frame */
		draw_panels();
	}
		
	validate_world_window();
}

/* updates only what needs changing (time_elapsed==NONE means redraw everything no matter what,
	but skip the interface frame) */
void update_interface(short time_elapsed)
{
	if (OGL_HUDActive) {
		if (time_elapsed == NONE)
			reset_motion_sensor(current_player_index);
		else
			return;
	}

	if (!game_window_is_full_screen())
	{
		// LP addition: don't force an update unless explicitly requested
		bool force_update = (time_elapsed == NONE);

		ensure_HUD_buffer();

		// LP addition: added support for HUD buffer;
		_set_port_to_HUD();
		if (HUD_SW.update_everything(time_elapsed))
			force_update = true;
		_restore_port();
		
		// Draw the whole thing if doing so is requested
		// (may need some smart way of drawing only what has to be drawn)
		if (force_update)
			RequestDrawingHUD();
	}
}

void mark_interface_collections(bool loading)
{
	loading ? mark_collection_for_loading(_collection_interface) : 
		mark_collection_for_unloading(_collection_interface);
}

void mark_weapon_display_as_dirty(void)
{
	interface_state.weapon_is_dirty = true;
}

void mark_ammo_display_as_dirty(void)
{
	interface_state.ammo_is_dirty = true;
}

void mark_shield_display_as_dirty(void)
{
	interface_state.shield_is_dirty = true;
}

void mark_oxygen_display_as_dirty(void)
{
	interface_state.oxygen_is_dirty = true;
}

void mark_player_inventory_screen_as_dirty(short player_index, short screen)
{
	struct player_data *player= get_player_data(player_index);

	set_current_inventory_screen(player_index, screen);
	SET_INVENTORY_DIRTY_STATE(player, true);
}

void mark_player_inventory_as_dirty(short player_index, short dirty_item)
{
	struct player_data *player= get_player_data(player_index);

	/* If the dirty item is not NONE, then goto that item kind display.. */
	if(dirty_item != NONE)
	{
		short item_kind= get_item_kind(dirty_item);
		short current_screen= GET_CURRENT_INVENTORY_SCREEN(player);

		/* Don't change if it is a powerup, or you are in the network statistics screen */
		if(item_kind != _powerup && item_kind != current_screen) // && current_screen!=_network_statistics)
		{
			/* Goto that type of item.. */
			set_current_inventory_screen(player_index, item_kind);
		}
	}
	SET_INVENTORY_DIRTY_STATE(player, true);
}

void mark_player_network_stats_as_dirty(short player_index)
{
	if (GET_GAME_OPTIONS()&_live_network_stats)
	{
		struct player_data *player= get_player_data(player_index);
	
		set_current_inventory_screen(player_index, _network_statistics);
		SET_INVENTORY_DIRTY_STATE(player, true);
	}
}

void set_interface_microphone_recording_state(bool state)
{
#if !defined(DISABLE_NETWORKING)
	set_network_microphone_state(state);
#endif // !defined(DISABLE_NETWORKING)
}

void scroll_inventory(short dy)
{
	short mod_value, index, current_inventory_screen, section_count, test_inventory_screen = 0;
	short section_items[NUMBER_OF_ITEMS];
	short section_counts[NUMBER_OF_ITEMS];
	
	current_inventory_screen= GET_CURRENT_INVENTORY_SCREEN(current_player);

	if(dynamic_world->player_count>1)
	{
		mod_value= NUMBER_OF_ITEM_TYPES+1;
	} else {
		mod_value= NUMBER_OF_ITEM_TYPES;
	}

	if(dy>0)
	{
		for(index= 1; index<mod_value; ++index)
		{
			test_inventory_screen= (current_inventory_screen+index)%mod_value;
			
			assert(test_inventory_screen>=0 && test_inventory_screen<NUMBER_OF_ITEM_TYPES+1);			
			if(test_inventory_screen != NUMBER_OF_ITEM_TYPES)
			{
				calculate_player_item_array(current_player_index, test_inventory_screen,
					section_items, section_counts, &section_count);
				if(section_count) break; /* Go tho this one! */
			} else {
				/* Network statistics! */
				break; 
			}
		}
		
		current_inventory_screen= test_inventory_screen;
	} else {
		/* Going down.. */
		for(index= mod_value-1; index>0; --index)
		{
			test_inventory_screen= (current_inventory_screen+index)%mod_value;

			assert(test_inventory_screen>=0 && test_inventory_screen<NUMBER_OF_ITEM_TYPES+1);			
			if(test_inventory_screen != NUMBER_OF_ITEM_TYPES)
			{
				calculate_player_item_array(current_player_index, test_inventory_screen,
					section_items, section_counts, &section_count);
				if(section_count) break; /* Go tho this one! */
			} else {
				/* Network statistics! */
				break; 
			}
		}		

		current_inventory_screen= test_inventory_screen;
	}
	set_current_inventory_screen(current_player_index, current_inventory_screen);

	SET_INVENTORY_DIRTY_STATE(current_player, true);	
}

static void set_current_inventory_screen(
	short player_index,
	short screen)
{
	struct player_data *player= get_player_data(player_index);
	
	assert(screen>=0 && screen<7);
	
	player->interface_flags&= ~INVENTORY_MASK_BITS;
	player->interface_flags|= screen;
	player->interface_decay= 5*TICKS_PER_SECOND;
}


/* MML parser for the 'vidmaster' tag */
extern short vidmasterStringSetID; // shell.cpp
class XML_VidmasterParser: public XML_ElementParser
{
	short StringSetIndex;

public:

	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();

	XML_VidmasterParser(): XML_ElementParser("vidmaster") {}
};


bool XML_VidmasterParser::Start()
{
	StringSetIndex = -1;
	return true;
}

bool XML_VidmasterParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag, "stringset_index"))
		return ReadBoundedInt16Value(Value, StringSetIndex, -1, SHRT_MAX);
	/* Ideas for other attributes: enabled="{boolean}" (if not, no go-to-level for you) keys="shift,control"? */
	UnrecognizedTag();
	return false;
}

bool XML_VidmasterParser::AttributesDone()
{
	vidmasterStringSetID = StringSetIndex;
	return true;
}

static XML_VidmasterParser VidmasterParser;


/*
 *  MML parser for interface elements
 */

class XML_AmmoDisplayParser: public XML_ElementParser
{
	// Intended one to replace
	short Index;
	// Ammo data
	weapon_interface_ammo_data Data;
	
	// What is present?
	bool IndexPresent;
	enum {NumberOfValues = 10};
	bool IsPresent[NumberOfValues];
	
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();

	// Pointer to list of ammo data types, from the weapon parser
	weapon_interface_ammo_data *OrigAmmo;
	
	XML_AmmoDisplayParser(): XML_ElementParser("ammo") {OrigAmmo=0;}
};

bool XML_AmmoDisplayParser::Start()
{
	IndexPresent = false;
	for (int k=0; k<NumberOfValues; k++)
		IsPresent[k] = false;
	return true;
}

bool XML_AmmoDisplayParser::HandleAttribute(const char *Tag, const char *Value)
{
	// Won't be doing the item ID or the shape ID;
	// just the stuff necessary for good placement of weapon and name graphics
	if (StringsEqual(Tag,"index"))
	{
		if (ReadBoundedInt16Value(Value,Index,0,int(NUMBER_OF_WEAPON_INTERFACE_ITEMS)-1))
		{
			IndexPresent = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"type"))
	{
		if (ReadInt16Value(Value,Data.type))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"left"))
	{
		if (ReadInt16Value(Value,Data.screen_left))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"top"))
	{
		if (ReadInt16Value(Value,Data.screen_top))
		{
			IsPresent[2] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"across"))
	{
		if (ReadInt16Value(Value,Data.ammo_across))
		{
			IsPresent[3] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"down"))
	{
		if (ReadInt16Value(Value,Data.ammo_down))
		{
			IsPresent[4] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"delta_x"))
	{
		if (ReadInt16Value(Value,Data.delta_x))
		{
			IsPresent[5] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"delta_y"))
	{
		if (ReadInt16Value(Value,Data.delta_y))
		{
			IsPresent[6] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"bullet_shape"))
	{
		if (ReadUInt16Value(Value,Data.bullet))
		{
			IsPresent[7] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"empty_shape"))
	{
		if (ReadUInt16Value(Value,Data.empty_bullet))
		{
			IsPresent[8] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"right_to_left"))
	{
		if (ReadBooleanValueAsBool(Value,Data.right_to_left))
		{
			IsPresent[9] = true;
			return true;
		}
		else return false;
	}
	
	UnrecognizedTag();
	return false;
}

bool XML_AmmoDisplayParser::AttributesDone()
{
	if (!IndexPresent)
	{
		AttribsMissing();
		return false;
	}
	assert(OrigAmmo);
	weapon_interface_ammo_data& OrigData = OrigAmmo[Index];
	if (IsPresent[0]) OrigData.type = Data.type;
	if (IsPresent[1]) OrigData.screen_left = Data.screen_left;
	if (IsPresent[2]) OrigData.screen_top = Data.screen_top;
	if (IsPresent[3]) OrigData.ammo_across = Data.ammo_across;
	if (IsPresent[4]) OrigData.ammo_down = Data.ammo_down;
	if (IsPresent[5]) OrigData.delta_x = Data.delta_x;
	if (IsPresent[6]) OrigData.delta_y = Data.delta_y;
	if (IsPresent[7]) OrigData.bullet = Data.bullet;
	if (IsPresent[8]) OrigData.empty_bullet = Data.empty_bullet;
	if (IsPresent[9]) OrigData.right_to_left = Data.right_to_left;
		
	return true;
}

static XML_AmmoDisplayParser AmmoDisplayParser;


struct weapon_interface_data *original_weapon_interface_definitions = NULL;

class XML_WeaponDisplayParser: public XML_ElementParser
{
	// Intended one to replace
	short Index;
	// Weapon data
	weapon_interface_data Data;
	
	// What is present?
	bool IndexPresent;
	enum {NumberOfValues = 8};
	bool IsPresent[NumberOfValues];
	
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	bool ResetValues();

	XML_WeaponDisplayParser(): XML_ElementParser("weapon") {}
};

bool XML_WeaponDisplayParser::Start()
{
	// back up old values first
	if (!original_weapon_interface_definitions) {
		original_weapon_interface_definitions = (struct weapon_interface_data *) malloc(sizeof(struct weapon_interface_data) * NUMBER_OF_WEAPON_INTERFACE_DEFINITIONS);
		assert(original_weapon_interface_definitions);
		for (unsigned i = 0; i < NUMBER_OF_WEAPON_INTERFACE_DEFINITIONS; i++)
			original_weapon_interface_definitions[i] = weapon_interface_definitions[i];
	}

	IndexPresent = false;
	for (int k=0; k<NumberOfValues; k++)
		IsPresent[k] = false;
	return true;
}

bool XML_WeaponDisplayParser::HandleAttribute(const char *Tag, const char *Value)
{
	// Won't be doing the item ID or the shape ID;
	// just the stuff necessary for good placement of weapon and name graphics
	if (StringsEqual(Tag,"index"))
	{
		if (ReadBoundedInt16Value(Value,Index,0,int(MAXIMUM_WEAPON_INTERFACE_DEFINITIONS)-1))
		{
			IndexPresent = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"shape"))
	{
		if (ReadUInt16Value(Value,Data.weapon_panel_shape))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"start_y"))
	{
		if (ReadInt16Value(Value,Data.weapon_name_start_y))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"end_y"))
	{
		if (ReadInt16Value(Value,Data.weapon_name_end_y))
		{
			IsPresent[2] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"start_x"))
	{
		if (ReadInt16Value(Value,Data.weapon_name_start_x))
		{
			IsPresent[3] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"end_x"))
	{
		if (ReadInt16Value(Value,Data.weapon_name_end_x))
		{
			IsPresent[4] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"top"))
	{
		if (ReadInt16Value(Value,Data.standard_weapon_panel_top))
		{
			IsPresent[5] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"left"))
	{
		if (ReadInt16Value(Value,Data.standard_weapon_panel_left))
		{
			IsPresent[6] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"multiple"))
	{
		if (ReadBooleanValueAsBool(Value,Data.multi_weapon))
		{
			IsPresent[7] = true;
			return true;
		}
		else return false;
	}
	
	UnrecognizedTag();
	return false;
}

bool XML_WeaponDisplayParser::AttributesDone()
{
	if (!IndexPresent)
	{
		AttribsMissing();
		return false;
	}
	
	weapon_interface_data& OrigData = weapon_interface_definitions[Index];
	
	if (IsPresent[0]) OrigData.weapon_panel_shape = Data.weapon_panel_shape;
	if (IsPresent[1]) OrigData.weapon_name_start_y = Data.weapon_name_start_y;
	if (IsPresent[2]) OrigData.weapon_name_end_y = Data.weapon_name_end_y;
	if (IsPresent[3]) OrigData.weapon_name_start_x = Data.weapon_name_start_x;
	if (IsPresent[4]) OrigData.weapon_name_end_x = Data.weapon_name_end_x;
	if (IsPresent[5]) OrigData.standard_weapon_panel_top = Data.standard_weapon_panel_top;
	if (IsPresent[6]) OrigData.standard_weapon_panel_left = Data.standard_weapon_panel_left;
	if (IsPresent[7]) OrigData.multi_weapon = Data.multi_weapon;
	
	AmmoDisplayParser.OrigAmmo = OrigData.ammo_data;
	
	return true;
}

bool XML_WeaponDisplayParser::ResetValues()
{
	if (original_weapon_interface_definitions) {
		for (unsigned i = 0; i < NUMBER_OF_WEAPON_INTERFACE_DEFINITIONS; i++)
			weapon_interface_definitions[i] = original_weapon_interface_definitions[i];
		free(original_weapon_interface_definitions);
		original_weapon_interface_definitions = NULL;
	}
	return true;
}

static XML_WeaponDisplayParser WeaponDisplayParser;

class XML_InterfaceParser: public XML_ElementParser
{
public:
	bool Start()
	{
		SetColorFontParserToScreenDrawing();
		return true;
	}
	bool HandleAttribute(const char *Tag, const char *Value)
	{
		if (StringsEqual(Tag,"motion_sensor"))
		{
			return ReadBooleanValueAsBool(Value, MotionSensorActive);
		}
		UnrecognizedTag();
		return false;
	}

	XML_InterfaceParser(): XML_ElementParser("interface") {}
};


static XML_InterfaceParser InterfaceParser;

// Makes a pointer to a the interface-data parser
XML_ElementParser *Interface_GetParser()
{

	// Add all subobjects:
	// weapon display, rectangles, and colors
	WeaponDisplayParser.AddChild(&AmmoDisplayParser);
	InterfaceParser.AddChild(&WeaponDisplayParser);
	InterfaceParser.AddChild(InterfaceRectangles_GetParser());
	InterfaceParser.AddChild(Color_GetParser());
	InterfaceParser.AddChild(Font_GetParser());
	// add the vidmaster parser too
	InterfaceParser.AddChild(&VidmasterParser);
	
	return &InterfaceParser;
}
