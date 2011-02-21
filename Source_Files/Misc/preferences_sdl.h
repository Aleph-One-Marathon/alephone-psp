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
 *  preferences_sdl.cpp - Preferences handling, SDL specific stuff
 *
 *  Written in 2000 by Christian Bauer
 *
 *  Feb 27, 2002 (Woody Zenfell):
 *    Now allowing selection of OpenGL or software renderer, and doing separate config boxes for each.
 *
 *  Mar 08, 2002 (Woody Zenfell):
 *      Added UI/preferences elements to configure microphone key
 *
 *  May 16, 2002 (Woody Zenfell):
 *      Added UI/preferences elements for configurable mouse sensitivity
 *      Preventing assignment of SDLK_ESCAPE; it's now a quit gesture in-game
 *      Added UI for choosing "don't auto-recenter" option
 *      Added warning message about safeguards surrounding behavior modifiers
 */

#ifndef _SDL_PREFERNCES_
#define _SDL_PREFERNCES_

#ifdef __MVCPP__
#include "sdl_cseries.h"
#include "shape_descriptors.h"
#endif

#include "sdl_dialogs.h"
#include "sdl_fonts.h"
#include "sdl_widgets.h"
#include "screen.h"
#include "images.h"
#include "find_files.h"
#include "screen_drawing.h"
#include "mouse.h"
#include "preferences_widgets_sdl.h"
#include "map.h" // for difficulty levels

#include <string.h>
#include <vector>
#include <math.h>    // logf and expf, for sensitivity slider (ZZZ)
// JTP: GCC3.1 on OSX was lacking these functions in math.h
#ifndef logf
#define logf(x) ((float)log(x))
#endif
#ifndef expf
#define expf(x) ((float)exp(x))
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>	// for getlogin()
#endif

#ifdef __WIN32__
#include <windows.h> // for GetUserName()
#endif

#ifdef __MVCPP__

#include "world.h"
#include "shell.h"
#include "preferences.h"
#include "mysound.h"
#include "wad.h"

#endif


// Prototypes
static void player_dialog(void *arg);
static void opengl_dialog(void *arg);
static void graphics_dialog(void *arg);
static void sound_dialog(void *arg);
static void controls_dialog(void *arg);
static void environment_dialog(void *arg);
static void keyboard_dialog(void *arg);


/*
 *  Get user name
 */

static void get_name_from_system(unsigned char *outName)
{
    // Skipping usual string safety pickiness, I'm tired tonight.
    // Hope caller's buffer is big enough.
    char* name = (char*) outName;

#if defined(unix) || defined(__BEOS__) || (defined (__APPLE__) && defined (__MACH__))

	char *login = getlogin();
	strcpy(name, login ? login : "Bob User");

#elif defined(__WIN32__)

	char login[17];
	DWORD len = 17;

	bool hasName = (GetUserName((LPSTR)login, &len) == TRUE);
	if (hasName && strpbrk(login, "\\/:*?\"<>|") == NULL) // Ignore illegal names
		strcpy(name, login);
	else
		strcpy(name, "Bob User");

#elif defined(psp)
	
	strcpy(name, "ToXicSham");
	
#else
#error get_name_from_system() not implemented for this platform
#endif

    // In-place conversion to pstring
    a1_c2pstr(name);
}


/*
 *  Ethernet always available
 */

static bool ethernet_active(void)
{
	return true;
}


/*
 *  Main preferences dialog
 */

void handle_preferences(void)
{
	// Save the existing preferences, in case we have to reload them
	write_preferences();

	// Load sensible palette
	if (SDL_GetVideoSurface()->format->BitsPerPixel == 8) {
		struct color_table *system_colors = build_8bit_system_color_table();
		assert_world_color_table(system_colors, system_colors);
		delete system_colors;
	}

	// Create top-level dialog
	dialog d;
	d.add(new w_static_text("PREFERENCES", TITLE_FONT, TITLE_COLOR));
	d.add(new w_spacer());
	d.add(new w_button("PLAYER", player_dialog, &d));
	d.add(new w_button("GRAPHICS", graphics_dialog, &d));
	d.add(new w_button("SOUND", sound_dialog, &d));
	d.add(new w_button("CONTROLS", controls_dialog, &d));
	d.add(new w_button("ENVIRONMENT", environment_dialog, &d));
	d.add(new w_spacer());
	d.add(new w_button("RETURN", dialog_cancel, &d));

	// Clear menu screen
	clear_screen();

	// Run dialog
	d.run();

	// Redraw main menu
	display_main_menu();
}


/*
 *  Player dialog
 */

static void player_dialog(void *arg)
{
	// Create dialog
	dialog d;
	d.add(new w_static_text("PLAYER SETTINGS", TITLE_FONT, TITLE_COLOR));
	d.add(new w_spacer());
	w_select *level_w = new w_select("Difficulty", player_preferences->difficulty_level, NULL /*level_labels*/);
	level_w->set_labels_stringset(kDifficultyLevelsStringSetID);
	d.add(level_w);
        
	d.add(new w_spacer());

	d.add(new w_static_text("Appearance"));

	w_text_entry *name_w = new w_text_entry("Name", PREFERENCES_NAME_LENGTH, "");
	name_w->set_identifier(iNAME);
	name_w->set_enter_pressed_callback(dialog_try_ok);
	name_w->set_value_changed_callback(dialog_disable_ok_if_empty);
	d.add(name_w);

	w_player_color *pcolor_w = new w_player_color("Color", player_preferences->color);
	d.add(pcolor_w);
	w_player_color *tcolor_w = new w_player_color("Team Color", player_preferences->team);
	d.add(tcolor_w);
	
	d.add(new w_spacer());
        
	w_left_button* ok_button = new w_left_button("ACCEPT", dialog_ok, &d);
	ok_button->set_identifier(iOK);
	d.add(ok_button);
	d.add(new w_right_button("CANCEL", dialog_cancel, &d));

	// We don't do this earlier because it (indirectly) invokes the name_typing callback, which needs iOK
	copy_pstring_to_text_field(&d, iNAME, player_preferences->name);

	// Clear screen
	clear_screen();

	// Run dialog
	if (d.run() == 0) {	// Accepted
		bool changed = false;

		const char *name = name_w->get_text();
		unsigned char theOldNameP[PREFERENCES_NAME_LENGTH+1];
		pstrncpy(theOldNameP, player_preferences->name, PREFERENCES_NAME_LENGTH+1);
		char *theOldName = a1_p2cstr(theOldNameP);
		if (strcmp(name, theOldName)) {
			copy_pstring_from_text_field(&d, iNAME, player_preferences->name);
			changed = true;
		}

		int16 level = static_cast<int16>(level_w->get_selection());
		assert(level >= 0);
		if (level != player_preferences->difficulty_level) {
			player_preferences->difficulty_level = level;
			changed = true;
		}

		int16 color = static_cast<int16>(pcolor_w->get_selection());
		assert(color >= 0);
		if (color != player_preferences->color) {
			player_preferences->color = color;
			changed = true;
		}

		int16 team = static_cast<int16>(tcolor_w->get_selection());
		assert(team >= 0);
		if (team != player_preferences->team) {
			player_preferences->team = team;
			changed = true;
		}

		if (changed)
			write_preferences();
	}
}


/*
 *  Handle graphics dialog
 */

static const char *depth_labels[3] = {
	"8 Bit", "16 Bit", NULL
};

static const char *resolution_labels[3] = {
	"Low", "High", NULL
};

static const char *size_labels[13] = {
	"320x160", "480x240", "640x320", "640x480 (no HUD)",
	"800x400", "800x600 (no HUD)", "1024x512", "1024x768 (no HUD)",
	"1280x640", "1280x1024 (no HUD)", "1600x800", "1600x1200 (no HUD)", NULL
};

static const char *gamma_labels[9] = {
	"Darkest", "Darker", "Dark", "Normal", "Light", "Really Light", "Even Lighter", "Lightest", NULL
};

static const char* renderer_labels[] = {
	"Software", "OpenGL", NULL
};

enum {
    iRENDERING_SYSTEM = 1000
};

static void software_rendering_options_dialog(void* arg)
{
	// Create dialog
	dialog d;
	d.add(new w_static_text("SOFTWARE RENDERING OPTIONS", TITLE_FONT, TITLE_COLOR));
	d.add(new w_spacer());

    w_toggle *depth_w = new w_toggle("Color Depth", graphics_preferences->screen_mode.bit_depth == 16, depth_labels);
	d.add(depth_w);
	w_toggle *resolution_w = new w_toggle("Resolution", graphics_preferences->screen_mode.high_resolution, resolution_labels);
	d.add(resolution_w);

	d.add(new w_spacer());
	d.add(new w_left_button("ACCEPT", dialog_ok, &d));
	d.add(new w_right_button("CANCEL", dialog_cancel, &d));

	// Clear screen
	clear_screen();

	// Run dialog
	if (d.run() == 0) {	// Accepted
		bool changed = false;

		int depth = (depth_w->get_selection() ? 16 : 8);
		if (depth != graphics_preferences->screen_mode.bit_depth) {
			graphics_preferences->screen_mode.bit_depth = depth;
			changed = true;
			// don't change mode now; it will be changed when the game starts
		}

		bool hi_res = resolution_w->get_selection() != 0;
		if (hi_res != graphics_preferences->screen_mode.high_resolution) {
			graphics_preferences->screen_mode.high_resolution = hi_res;
			changed = true;
		}

		if (changed)
			write_preferences();
	}
}

// ZZZ addition: bounce to correct renderer-config box based on selected rendering system.
static void rendering_options_dialog_demux(void* arg)
{
	int theSelectedRenderer = get_selection_control_value((dialog*) arg, iRENDERING_SYSTEM) - 1;

	switch(theSelectedRenderer) {
		case _no_acceleration:
			software_rendering_options_dialog(arg);
			break;

		case _opengl_acceleration:
			opengl_dialog(arg);
			break;

		default:
			assert(false);
			break;
	}
}

static void graphics_dialog(void *arg)
{
	dialog *parent = (dialog *)arg;

	// Create dialog
	dialog d;
	d.add(new w_static_text("GRAPHICS SETUP", TITLE_FONT, TITLE_COLOR));
	d.add(new w_spacer());

    w_select* renderer_w = new w_select("Rendering System", graphics_preferences->screen_mode.acceleration, renderer_labels);
    renderer_w->set_identifier(iRENDERING_SYSTEM);
#ifndef HAVE_OPENGL
    renderer_w->set_selection(_no_acceleration);
    renderer_w->set_enabled(false);
#endif
    d.add(renderer_w);

	w_select *size_w = new w_select("Screen Size", graphics_preferences->screen_mode.size, size_labels);
	d.add(size_w);
	w_toggle *fullscreen_w = new w_toggle("Fullscreen", graphics_preferences->screen_mode.fullscreen);
	d.add(fullscreen_w);
	w_select *gamma_w = new w_select("Brightness", graphics_preferences->screen_mode.gamma_level, gamma_labels);
	d.add(gamma_w);

    d.add(new w_spacer());
    d.add(new w_button("RENDERING OPTIONS", rendering_options_dialog_demux, &d));
    d.add(new w_spacer());
#ifdef HAVE_OPENGL
    d.add(new w_static_text("Warning: switching renderers is currently a bit buggy."));
    d.add(new w_static_text("If you switch, consider quitting and restarting Aleph One."));
#else
    d.add(new w_static_text("This copy of Aleph One was built without OpenGL support."));
#endif
    d.add(new w_spacer());

	d.add(new w_left_button("ACCEPT", dialog_ok, &d));
	d.add(new w_right_button("CANCEL", dialog_cancel, &d));

	// Clear screen
	clear_screen();

	// Run dialog
	if (d.run() == 0) {	// Accepted
		bool changed = false;

		bool fullscreen = fullscreen_w->get_selection() != 0;
		if (fullscreen != graphics_preferences->screen_mode.fullscreen) {
			graphics_preferences->screen_mode.fullscreen = fullscreen;
			// This is the only setting that has an immediate effect
			toggle_fullscreen(fullscreen);
			parent->draw();
			changed = true;
		}

        short renderer = static_cast<short>(renderer_w->get_selection());
		assert(renderer >= 0);
        if(renderer != graphics_preferences->screen_mode.acceleration) {
            graphics_preferences->screen_mode.acceleration = renderer;

            // Disable fading under Mac OS X software rendering
#if defined(__APPLE__) && defined(__MACH__) && defined(HAVE_OPENGL)
            extern bool option_nogamma;
            option_nogamma = true;
#endif

            changed = true;
        }

		short size = static_cast<short>(size_w->get_selection());
		assert(size >= 0);
		if (size != graphics_preferences->screen_mode.size) {
			graphics_preferences->screen_mode.size = size;
			changed = true;
			// don't change mode now; it will be changed when the game starts
		}

		short gamma = static_cast<short>(gamma_w->get_selection());
		if (gamma != graphics_preferences->screen_mode.gamma_level) {
			graphics_preferences->screen_mode.gamma_level = gamma;
			changed = true;
		}

		if (changed) {
			write_preferences();
			parent->draw();		// DirectX seems to need this
		}
	}
}


/*
 *  OpenGL dialog
 */

static void opengl_dialog(void *arg)
{

}


/*
 *  Sound dialog
 */

class w_toggle *stereo_w, *dynamic_w;

class w_stereo_toggle : public w_toggle {
public:
	w_stereo_toggle(const char *name, bool selection) : w_toggle(name, selection) {}

	void selection_changed(void)
	{
		// Turning off stereo turns off dynamic tracking
		w_toggle::selection_changed();
		if (selection == false)
			dynamic_w->set_selection(false);
	}
};

class w_dynamic_toggle : public w_toggle {
public:
	w_dynamic_toggle(const char *name, bool selection) : w_toggle(name, selection) {}

	void selection_changed(void)
	{
		// Turning on dynamic tracking turns on stereo
		w_toggle::selection_changed();
		if (selection == true)
			stereo_w->set_selection(true);
	}
};

static const char *channel_labels[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", NULL};

class w_volume_slider : public w_slider {
public:
	w_volume_slider(const char *name, int vol) : w_slider(name, NUMBER_OF_SOUND_VOLUME_LEVELS, vol) {}
	~w_volume_slider() {}

	void item_selected(void)
	{
		test_sound_volume(selection, _snd_adjust_volume);
	}
};

static void sound_dialog(void *arg)
{
	// Create dialog
	dialog d;
	d.add(new w_static_text("SOUND SETUP", TITLE_FONT, TITLE_COLOR));
	d.add(new w_spacer());
	static const char *quality_labels[3] = {"8 Bit", "16 Bit", NULL};
	w_toggle *quality_w = new w_toggle("Quality", TEST_FLAG(sound_preferences->flags, _16bit_sound_flag), quality_labels);
	d.add(quality_w);
	stereo_w = new w_stereo_toggle("Stereo", sound_preferences->flags & _stereo_flag);
	d.add(stereo_w);
	dynamic_w = new w_dynamic_toggle("Active Panning", TEST_FLAG(sound_preferences->flags, _dynamic_tracking_flag));
	d.add(dynamic_w);
	w_toggle *ambient_w = new w_toggle("Ambient Sounds", TEST_FLAG(sound_preferences->flags, _ambient_sound_flag));
	d.add(ambient_w);
	w_toggle *more_w = new w_toggle("More Sounds", TEST_FLAG(sound_preferences->flags, _more_sounds_flag));
	d.add(more_w);
	w_toggle *button_sounds_w = new w_toggle("Interface Button Sounds", TEST_FLAG(input_preferences->modifiers, _inputmod_use_button_sounds));
	d.add(button_sounds_w);
	w_select *channels_w = new w_select("Channels", sound_preferences->channel_count, channel_labels);
	d.add(channels_w);
	w_volume_slider *volume_w = new w_volume_slider("Volume", sound_preferences->volume);
	d.add(volume_w);
	d.add(new w_spacer());
	d.add(new w_left_button("ACCEPT", dialog_ok, &d));
	d.add(new w_right_button("CANCEL", dialog_cancel, &d));

	// Clear screen
	clear_screen();

	// Run dialog
	if (d.run() == 0) {	// Accepted
		bool changed = false;

		uint16 flags = 0;
		if (quality_w->get_selection()) flags |= _16bit_sound_flag;
		if (stereo_w->get_selection()) flags |= _stereo_flag;
		if (dynamic_w->get_selection()) flags |= _dynamic_tracking_flag;
		if (ambient_w->get_selection()) flags |= _ambient_sound_flag;
		if (more_w->get_selection()) flags |= _more_sounds_flag;

		if (flags != sound_preferences->flags) {
			sound_preferences->flags = flags;
			changed = true;
		}

		flags = input_preferences->modifiers & ~_inputmod_use_button_sounds;
		if (button_sounds_w->get_selection()) flags |= _inputmod_use_button_sounds;
		if (flags != input_preferences->modifiers) {
			input_preferences->modifiers = flags;
			changed = true;
		}

		int16 channel_count = static_cast<int16>(channels_w->get_selection());
		if (channel_count != sound_preferences->channel_count) {
			sound_preferences->channel_count = channel_count;
			changed = true;
		}

		int volume = volume_w->get_selection();
		if (volume != sound_preferences->volume) {
			sound_preferences->volume = volume;
			changed = true;
		}

		if (changed) {
			set_sound_manager_parameters(sound_preferences);
			write_preferences();
		}
	}
}


/*
 *  Controls dialog
 */

static w_toggle *mouse_w;

static void controls_dialog(void *arg)
{
	// Create dialog
	dialog d;
	d.add(new w_static_text("CONTROLS", TITLE_FONT, TITLE_COLOR));
	d.add(new w_spacer());
	mouse_w = new w_toggle("Mouse Control", input_preferences->input_device != 0);
	d.add(mouse_w);
	w_toggle *invert_mouse_w = new w_toggle("Invert Mouse", TEST_FLAG(input_preferences->modifiers, _inputmod_invert_mouse));
	d.add(invert_mouse_w);

    const float kMinSensitivityLog = -3.0f;
    const float kMaxSensitivityLog = 3.0f;
    const float kSensitivityLogRange = kMaxSensitivityLog - kMinSensitivityLog;

	// LP: split this into horizontal and vertical sensitivities
	float theSensitivity, theSensitivityLog;
	
	theSensitivity = ((float) input_preferences->sens_vertical) / FIXED_ONE;
	if (theSensitivity <= 0.0f) theSensitivity = 1.0f;
	theSensitivityLog = logf(theSensitivity);
	int theVerticalSliderPosition =
		(int) ((theSensitivityLog - kMinSensitivityLog) * (1000.0f / kSensitivityLogRange));

    w_slider* sens_vertical_w = new w_slider("Mouse Vertical Sensitivity", 1000, theVerticalSliderPosition);
    d.add(sens_vertical_w);
    
	theSensitivity = ((float) input_preferences->sens_horizontal) / FIXED_ONE;
	if (theSensitivity <= 0.0f) theSensitivity = 1.0f;
	theSensitivityLog = logf(theSensitivity);
	int theHorizontalSliderPosition =
		(int) ((theSensitivityLog - kMinSensitivityLog) * (1000.0f / kSensitivityLogRange));

    w_slider* sens_horizontal_w = new w_slider("Mouse Horizontal Sensitivity", 1000, theHorizontalSliderPosition);
    d.add(sens_horizontal_w);
/*
    float   theSensitivity = ((float) input_preferences->sensitivity) / FIXED_ONE;
    // avoid nasty math problems
    if (theSensitivity <= 0.0f)
        theSensitivity = 1.0f;
    float   theSensitivityLog = logf(theSensitivity);
    int     theSliderPosition = (int) ((theSensitivityLog - kMinSensitivityLog) * (1000.0f / kSensitivityLogRange));

    w_slider* sensitivity_w = new w_slider("Mouse Sensitivity", 1000, theSliderPosition);
    d.add(sensitivity_w);
*/

    d.add(new w_spacer);

	w_toggle *always_run_w = new w_toggle("Always Run", input_preferences->modifiers & _inputmod_interchange_run_walk);
	d.add(always_run_w);
	w_toggle *always_swim_w = new w_toggle("Always Swim", TEST_FLAG(input_preferences->modifiers, _inputmod_interchange_swim_sink));
	d.add(always_swim_w);
	w_toggle *weapon_w = new w_toggle("Auto-Switch Weapons", !(input_preferences->modifiers & _inputmod_dont_switch_to_new_weapon));
	d.add(weapon_w);
    w_toggle* auto_recenter_w = new w_toggle("Auto-Recenter View", !(input_preferences->modifiers & _inputmod_dont_auto_recenter));
    d.add(auto_recenter_w);

    d.add(new w_spacer());
    d.add(new w_spacer);
	d.add(new w_button("CONFIGURE KEYBOARD", keyboard_dialog, &d));
	d.add(new w_spacer());
    d.add(new w_static_text("Warning: Auto-Switch Weapons and Auto-Recenter View"));
    d.add(new w_static_text("are always ON in network play.  Turning either one OFF"));
    d.add(new w_static_text("will also disable film recording for single-player games."));
    d.add(new w_static_text("Future versions of Aleph One may lift these restrictions."));
    d.add(new w_spacer);
	d.add(new w_left_button("ACCEPT", dialog_ok, &d));
	d.add(new w_right_button("CANCEL", dialog_cancel, &d));

	// Clear screen
	clear_screen();

	// Run dialog
	if (d.run() == 0) {	// Accepted
		bool changed = false;

		int16 device = static_cast<int16>(mouse_w->get_selection());
		if (device != input_preferences->input_device) {
			input_preferences->input_device = device;
			changed = true;
		}

		// LP: split the sensitivity into vertical and horizontal sensitivities
		float theNewSensitivityLog;
		int theNewSliderPosition;
		
		theNewSliderPosition = sens_vertical_w->get_selection();
        if(theNewSliderPosition != theVerticalSliderPosition) {
            theNewSensitivityLog = kMinSensitivityLog + ((float) theNewSliderPosition) * (kSensitivityLogRange / 1000.0f);
            input_preferences->sens_vertical = _fixed(expf(theNewSensitivityLog) * FIXED_ONE);
            changed = true;
        }
		
		theNewSliderPosition = sens_horizontal_w->get_selection();
        if(theNewSliderPosition != theHorizontalSliderPosition) {
            theNewSensitivityLog = kMinSensitivityLog + ((float) theNewSliderPosition) * (kSensitivityLogRange / 1000.0f);
            input_preferences->sens_horizontal = _fixed(expf(theNewSensitivityLog) * FIXED_ONE);
            changed = true;
        }

/*
        int theNewSliderPosition = sensitivity_w->get_selection();
        if(theNewSliderPosition != theSliderPosition) {
            float theNewSensitivityLog = kMinSensitivityLog + ((float) theNewSliderPosition) * (kSensitivityLogRange / 1000.0f);
            input_preferences->sensitivity = _fixed(expf(theNewSensitivityLog) * FIXED_ONE);
            changed = true;
        }
*/
		uint16 flags = input_preferences->modifiers & _inputmod_use_button_sounds;
		if (always_run_w->get_selection()) flags |= _inputmod_interchange_run_walk;
		if (always_swim_w->get_selection()) flags |= _inputmod_interchange_swim_sink;
		if (!(weapon_w->get_selection())) flags |= _inputmod_dont_switch_to_new_weapon;
		if (!(auto_recenter_w->get_selection())) flags |= _inputmod_dont_auto_recenter;
		if (invert_mouse_w->get_selection()) flags |= _inputmod_invert_mouse;

		if (flags != input_preferences->modifiers) {
			input_preferences->modifiers = flags;
			changed = true;
		}

		if (changed)
			write_preferences();
	}
}


/*
 *  Keyboard dialog
 */

const int NUM_KEYS = 21;

static const char *action_name[NUM_KEYS] = {
	"Move Forward", "Move Backward", "Turn Left", "Turn Right", "Sidestep Left", "Sidestep Right",
	"Glance Left", "Glance Right", "Look Up", "Look Down", "Look Ahead",
	"Previous Weapon", "Next Weapon", "Trigger", "2nd Trigger",
	"Sidestep", "Run/Swim", "Look",
	"Action", "Auto Map", "Microphone"
};

static SDLKey default_keys[NUM_KEYS] = {
	SDLK_KP8, SDLK_KP5, SDLK_KP4, SDLK_KP6,		// moving/turning
	SDLK_z, SDLK_x,								// sidestepping
	SDLK_a, SDLK_s,								// horizontal looking
	SDLK_d, SDLK_c, SDLK_v,						// vertical looking
	SDLK_KP7, SDLK_KP9,							// weapon cycling
	SDLK_SPACE, SDLK_LALT,						// weapon trigger
	SDLK_LSHIFT, SDLK_LCTRL, SDLK_LMETA,		// modifiers
	SDLK_TAB,									// action trigger
	SDLK_m,										// map
    SDLK_BACKQUOTE                              // microphone (ZZZ)
};

static SDLKey default_mouse_keys[NUM_KEYS] = {
	SDLK_w, SDLK_x, SDLK_LEFT, SDLK_RIGHT,		// moving/turning
	SDLK_a, SDLK_d,								// sidestepping
	SDLK_q, SDLK_e,								// horizontal looking
	SDLK_UP, SDLK_DOWN, SDLK_KP0,				// vertical looking
	SDLK_c, SDLK_z,								// weapon cycling
	SDLKey(SDLK_BASE_MOUSE_BUTTON), SDLKey(SDLK_BASE_MOUSE_BUTTON+2), // weapon trigger
	SDLK_RSHIFT, SDLK_LSHIFT, SDLK_LCTRL,		// modifiers
	SDLK_s,										// action trigger
	SDLK_TAB,									// map
    SDLK_BACKQUOTE                              // microphone (ZZZ)
};

class w_prefs_key;

static w_prefs_key *key_w[NUM_KEYS];

class w_prefs_key : public w_key {
public:
	w_prefs_key(const char *name, SDLKey key) : w_key(name, key) {}

	void set_key(SDLKey new_key)
	{
		// Key used for in-game function?
		int error = NONE;
		switch (new_key) {
			case SDLK_PERIOD:		// Sound volume up/down
			case SDLK_COMMA:
				error = keyIsUsedForSound;
				break;

			case SDLK_EQUALS:		// Map zoom
			case SDLK_MINUS:
				error = keyIsUsedForMapZooming;
				break;

			case SDLK_LEFTBRACKET:	// Inventory scrolling
			case SDLK_RIGHTBRACKET:
				error = keyIsUsedForScrolling;
				break;

			case SDLK_BACKSPACE:
			case SDLK_BACKSLASH:
		case SDLK_QUESTION:
			case SDLK_F1:
			case SDLK_F2:
			case SDLK_F3:
			case SDLK_F4:
			case SDLK_F5:
			case SDLK_F6:
			case SDLK_F7:
			case SDLK_F8:
			case SDLK_F9:
			case SDLK_F10:
			case SDLK_F11:
			case SDLK_F12:
            case SDLK_ESCAPE: // (ZZZ: for quitting)
				error = keyIsUsedAlready;
				break;

			default:
				break;
		}
		if (error != NONE) {
			alert_user(infoError, strERRORS, error, 0);
			return;
		}

		w_key::set_key(new_key);
		if (new_key == SDLK_UNKNOWN)
			return;

		// Remove binding to this key from all other widgets
		for (int i=0; i<NUM_KEYS; i++) {
			if (key_w[i] && key_w[i] != this && key_w[i]->get_key() == new_key) {
				key_w[i]->set_key(SDLK_UNKNOWN);
				key_w[i]->dirty = true;
			}
		}
	}
};

static void load_default_keys(void *arg)
{
	// Load default keys, depending on state of "Mouse control" widget
	dialog *d = (dialog *)arg;
	SDLKey *keys = (mouse_w->get_selection() ? default_mouse_keys : default_keys);
	for (int i=0; i<NUM_KEYS; i++)
		key_w[i]->set_key(keys[i]);
	d->draw();
}

static void keyboard_dialog(void *arg)
{
	// Clear array of key widgets (because w_prefs_key::set_key() scans it)
	for (int i=0; i<NUM_KEYS; i++)
		key_w[i] = NULL;

	// Create dialog
	dialog d;
	d.add(new w_static_text("CONFIGURE KEYBOARD", TITLE_FONT, TITLE_COLOR));
	d.add(new w_spacer());
	for (int i=0; i<NUM_KEYS; i++) {
		key_w[i] = new w_prefs_key(action_name[i], SDLKey(input_preferences->keycodes[i]));
		d.add(key_w[i]);
	}
	d.add(new w_spacer());
	d.add(new w_button("DEFAULTS", load_default_keys, &d));
	d.add(new w_spacer());
	d.add(new w_left_button("ACCEPT", dialog_ok, &d));
	d.add(new w_right_button("CANCEL", dialog_cancel, &d));

	// Clear screen
	clear_screen();

	// Run dialog
	if (d.run() == 0) {	// Accepted
		bool changed = false;

		for (int i=0; i<NUM_KEYS; i++) {
			SDLKey key = key_w[i]->get_key();
			if (key != input_preferences->keycodes[i]) {
				input_preferences->keycodes[i] = short(key);
				changed = true;
			}
		}

		set_keys(input_preferences->keycodes);
		if (changed)
			write_preferences();
	}
}


/*
 *  Environment dialog
 */

static void environment_dialog(void *arg)
{
	dialog *parent = (dialog *)arg;

	// Create dialog
	dialog d;
	d.add(new w_static_text("ENVIRONMENT SETTINGS", TITLE_FONT, TITLE_COLOR));
	d.add(new w_spacer());
	w_env_select *map_w = new w_env_select("Map", environment_preferences->map_file, "AVAILABLE MAPS", _typecode_scenario, &d);
	d.add(map_w);
	w_env_select *physics_w = new w_env_select("Physics", environment_preferences->physics_file, "AVAILABLE PHYSICS MODELS", _typecode_physics, &d);
	d.add(physics_w);
	w_env_select *shapes_w = new w_env_select("Shapes", environment_preferences->shapes_file, "AVAILABLE SHAPES", _typecode_shapes, &d);
	d.add(shapes_w);
	w_env_select *sounds_w = new w_env_select("Sounds", environment_preferences->sounds_file, "AVAILABLE SOUNDS", _typecode_sounds, &d);
	d.add(sounds_w);
	w_env_select *theme_w = new w_env_select("Theme", environment_preferences->theme_dir, "AVAILABLE THEMES", _typecode_theme, &d);
	d.add(theme_w);
	d.add(new w_spacer());
	d.add(new w_left_button("ACCEPT", dialog_ok, &d));
	d.add(new w_right_button("CANCEL", dialog_cancel, &d));

	// Clear screen
	clear_screen();

	// Run dialog
	bool theme_changed = false;
	if (d.run() == 0) {	// Accepted
		bool changed = false;

		const char *path = map_w->get_path();
		if (strcmp(path, environment_preferences->map_file)) {
			strcpy(environment_preferences->map_file, path);
			environment_preferences->map_checksum = read_wad_file_checksum(map_w->get_file_specifier());
			changed = true;
		}

		path = physics_w->get_path();
		if (strcmp(path, environment_preferences->physics_file)) {
			strcpy(environment_preferences->physics_file, path);
			environment_preferences->physics_checksum = read_wad_file_checksum(physics_w->get_file_specifier());
			changed = true;
		}

		path = shapes_w->get_path();
		if (strcmp(path, environment_preferences->shapes_file)) {
			strcpy(environment_preferences->shapes_file, path);
			environment_preferences->shapes_mod_date = shapes_w->get_file_specifier().GetDate();
			changed = true;
		}

		path = sounds_w->get_path();
		if (strcmp(path, environment_preferences->sounds_file)) {
			strcpy(environment_preferences->sounds_file, path);
			environment_preferences->sounds_mod_date = sounds_w->get_file_specifier().GetDate();
			changed = true;
		}
		
		path = theme_w->get_path();
		if (strcmp(path, environment_preferences->theme_dir)) {
			strcpy(environment_preferences->theme_dir, path);
			changed = theme_changed = true;
		}

		if (changed)
			load_environment_from_preferences();

		if (theme_changed) {
			FileSpecifier theme = environment_preferences->theme_dir;
			load_theme(theme);
		}

		if (changed || theme_changed)
			write_preferences();
	}

	// Redraw parent dialog
	if (theme_changed)
		parent->quit(0);	// Quit the parent dialog so it won't draw in the old theme
}

#endif
