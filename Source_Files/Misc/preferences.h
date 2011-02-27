#ifndef __PREFERENCES_H
#define __PREFERENCES_H

/*
	preferences.h

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

	Tuesday, June 13, 1995 10:07:04 AM- rdm created.

Feb 10, 2000 (Loren Petrich):
	Added stuff for input modifiers: run/walk and swim/sink

Feb 25, 2000 (Loren Petrich):
	Set up persistent stuff for the chase cam and crosshairs

Mar 2, 2000 (Loren Petrich):
	Added chase-cam and crosshairs interfaces

Mar 14, 2000 (Loren Petrich):
	Added OpenGL stuff

Apr 27, 2000 (Loren Petrich):
	Added Josh Elsasser's "don't switch weapons" patch

Oct 22, 2001 (Woody Zenfell):
	Changed the player name in player_preferences_data back to a Pstring (was Cstring in SDL version)

May 16, 2002 (Woody Zenfell):
	New control option "don't auto-recenter view"

Apr 10, 2003 (Woody Zenfell):
	Join hinting and autogathering have Preferences entries now

May 22, 2003 (Woody Zenfell):
	Support for preferences for multiple network game protocols; configurable local game port.
 */

#include "interface.h"
#include "ChaseCam.h"
#include "Crosshairs.h"
#include "OGL_Setup.h"
#include "shell.h"
#include "SoundManager.h"


/* New preferences junk */
const float DEFAULT_MONITOR_REFRESH_FREQUENCY = 60;	// 60 Hz

enum {
	_sw_alpha_off,
	_sw_alpha_fast,
	_sw_alpha_nice,
};

struct graphics_preferences_data
{
	struct screen_mode_data screen_mode;
#ifdef mac
	GDSpec device_spec;
	// LP: converted this to floating-point for convenience
	float refresh_frequency;
#endif
	// LP change: added OpenGL support
	OGL_ConfigureData OGL_Configure;

	bool double_corpse_limit;

	int16 software_alpha_blending;

	bool hog_the_cpu;
};

struct serial_number_data
{
	bool network_only;
	byte long_serial_number[10];
	Str255 user_name;
	Str255 tokenized_serial_number;
};

enum {
	_network_game_protocol_ring,
	_network_game_protocol_star,
	NUMBER_OF_NETWORK_GAME_PROTOCOLS,

	_network_game_protocol_default = _network_game_protocol_star
};

struct network_preferences_data
{
	bool allow_microphone;
	bool game_is_untimed;
	int16 type; // look in network_dialogs.c for _ethernet, etc...
	int16 game_type;
	int16 difficulty_level;
	uint16 game_options; // Penalize suicide, etc... see map.h for constants
	int32 time_limit;
	int16 kill_limit;
	int16 entry_point;
	bool autogather;
	bool join_by_address;
	char join_address[256];
	uint16 game_port;	// TCP and UDP port number used for game traffic (not player-location traffic)
	uint16 game_protocol; // _network_game_protocol_star, etc.
	bool use_speex_encoder;
	bool use_netscript;
#ifdef mac
	FSSpec netscript_file;
#else
	char netscript_file[256];
#endif	
	uint16 cheat_flags;
	bool advertise_on_metaserver;
	bool attempt_upnp;
	bool check_for_updates;

	enum {
		kMetaserverLoginLength = 16
	};

	char metaserver_login[kMetaserverLoginLength];
	char metaserver_password[kMetaserverLoginLength];
	bool use_custom_metaserver_colors;
	rgb_color metaserver_colors[2];
	bool mute_metaserver_guests;
};

struct player_preferences_data
{
	unsigned char name[PREFERENCES_NAME_LENGTH+1];
	int16 color;
	int16 team;
	uint32 last_time_ran;
	int16 difficulty_level;
	bool background_music_on;
	struct ChaseCamData ChaseCam;
	struct CrosshairData Crosshairs;
};

// LP addition: input-modifier flags
// run/walk and swim/sink
// LP addition: Josh Elsasser's dont-switch-weapons patch
enum {
	_inputmod_interchange_run_walk = 0x0001,
	_inputmod_interchange_swim_sink = 0x0002,
	_inputmod_dont_switch_to_new_weapon = 0x0004,
	_inputmod_invert_mouse = 0x0008,
	_inputmod_use_button_sounds = 0x0010,
	_inputmod_dont_auto_recenter = 0x0020   // ZZZ addition
};

// shell keys
enum {
	_key_inventory_left,
	_key_inventory_right,
	_key_switch_view,
	_key_volume_up,
	_key_volume_down,
	_key_zoom_in,
	_key_zoom_out,
	_key_toggle_fps,
	_key_activate_console,
	NUMBER_OF_SHELL_KEYS
};

#define MAX_BUTTONS 5
struct input_preferences_data
{
	int16 input_device;
	int16 keycodes[NUMBER_OF_KEYS];
	// LP addition: input modifiers
	uint16 modifiers;
	// Mouse-sensitivity parameters (LP: originally ZZZ)
	_fixed sens_horizontal;
	_fixed sens_vertical;
	// SB: Option to get rid of the horrible, horrible, horrible mouse acceleration.
	bool mouse_acceleration;
	// mouse button actions
	int16 mouse_button_actions[MAX_BUTTONS];
	int16 shell_keycodes[NUMBER_OF_SHELL_KEYS];
};

// mouse button action types
enum {
	_mouse_button_does_nothing,
	_mouse_button_fires_left_trigger,
	_mouse_button_fires_right_trigger
};

#define MAXIMUM_PATCHES_PER_ENVIRONMENT (32)

struct environment_preferences_data
{
#ifdef mac
	FSSpec map_file;
	FSSpec physics_file;
	FSSpec shapes_file;
	FSSpec sounds_file;
#else
	char map_file[256];
	char physics_file[256];
	char shapes_file[256];
	char sounds_file[256];
#endif
	uint32 map_checksum;
	uint32 physics_checksum;
	TimeType shapes_mod_date;
	TimeType sounds_mod_date;
	uint32 patches[MAXIMUM_PATCHES_PER_ENVIRONMENT];
#ifdef SDL
	char theme_dir[256];
#endif
	// ZZZ: these aren't really environment preferences, but
	// preferences that affect the environment preferences dialog
	bool group_by_directory;	// if not, display popup as one giant flat list
	bool reduce_singletons;		// make groups of a single element part of a larger parent group

	// ghs: are themes part of the environment? they are now
	bool smooth_text;

	char solo_lua_file[256];
	bool use_solo_lua;
};

/* New preferences.. (this sorta defeats the purpose of this system, but not really) */
extern struct graphics_preferences_data *graphics_preferences;
extern struct serial_number_data *serial_preferences;
extern struct network_preferences_data *network_preferences;
extern struct player_preferences_data *player_preferences;
extern struct input_preferences_data *input_preferences;
//extern struct sound_manager_parameters *sound_preferences;
extern SoundManager::Parameters *sound_preferences;
extern struct environment_preferences_data *environment_preferences;

/* --------- functions */
void initialize_preferences(void);
void read_preferences();
void handle_preferences(void);
void write_preferences(void);

#endif
