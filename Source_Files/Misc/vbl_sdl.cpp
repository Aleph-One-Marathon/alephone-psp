#include <cstdlib>
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
 *  vbl_sdl.cpp - Input handling, SDL specific stuff
 *
 *  Written in 2000 by Christian Bauer
 *
 *  May 16, 2002 (Woody Zenfell):
 *      parse_keymap() now calls mouse_buttons_become_keypresses()
 *      in support of treating mouse button clicks like, well, keypresses.
 */

#include "cseries.h"
#include "FileHandler.h"
#include "resource_manager.h"

#include "map.h"
#include "shell.h"
#include "preferences.h"
#include "interface.h"
#include "player.h"
#include "mouse.h"
#include "key_definitions.h"
#include "tags.h" // for filetypes.
#include "computer_interface.h"
#include "Console.h"
//#include "gp2x_joystick.h"

#include "vbl.h"

#include "vbl_definitions.h"

extern gp2xkey gp2xkeybindings[16];

// Constants
#define MAXIMUM_FLAG_PERSISTENCE    15
#define DOUBLE_CLICK_PERSISTENCE    10
#define FILM_RESOURCE_TYPE          FOUR_CHARS_TO_INT('f', 'i', 'l', 'm')

#define NUMBER_OF_SPECIAL_FLAGS (sizeof(special_flags)/sizeof(struct special_flag_data))
static struct special_flag_data special_flags[]=
{
	{_double_flag, _look_dont_turn, _looking_center},
	{_double_flag, _run_dont_walk, _action_trigger_state},
	{_latched_flag, _action_trigger_state},
	{_latched_flag, _cycle_weapons_forward},
	{_latched_flag, _cycle_weapons_backward},
	{_latched_flag, _toggle_map}
};

/*
 *  Get FileDesc for replay, ask user if desired
 */

bool find_replay_to_use(bool ask_user, FileSpecifier &file)
{
	if (ask_user) {
		return file.ReadDialog(_typecode_film);
	} else
		return get_recording_filedesc(file);
}


/*
 *  Get FileDesc for default recording file
 */

bool get_recording_filedesc(FileSpecifier &File)
{
	File.SetToLocalDataDir();
	File += getcstr(temporary, strFILENAMES, filenameMARATHON_RECORDING);
	return File.Exists();
}


/*
 *  Save film buffer to user-selected file
 */

void move_replay(void)
{
	// Get source file specification
	FileSpecifier src_file, dst_file;
	if (!get_recording_filedesc(src_file))
		return;

	// Ask user for destination file
	char prompt[256], default_name[256];
	if (!dst_file.WriteDialog(_typecode_film, getcstr(prompt, strPROMPTS, _save_replay_prompt), getcstr(default_name, strFILENAMES, filenameMARATHON_RECORDING)))
		return;

	// Copy file
	dst_file.CopyContents(src_file);
	int error = dst_file.GetError();
	if (error)
		alert_user(infoError, strERRORS, fileError, error);
}


/*
 *  Poll keyboard and return action flags
 */

uint32 parse_keymap(void)
{
  uint32 flags = 0;

  if(get_keyboard_controller_status())
    {
      Uint8 *key_map;
      if (Console::instance()->input_active()) {
	static Uint8 chat_input_mode_keymap[SDLK_LAST];
	memset(&chat_input_mode_keymap, 0, sizeof(chat_input_mode_keymap));
	key_map = chat_input_mode_keymap;
      } else {
	key_map = SDL_GetKeyState(NULL);
      }

	unsigned long c;

//	c=gp2x_joystick_read();

	int i;
	bool templook=false;

/*	for(i=0;i<14;i++)
		if(joystate(i,c) && (gp2xkeybindings[i].set1==_toggle_look || gp2xkeybindings[i].set1==_hold_look)) {
			input_preferences->lookswitch=true;
			if(gp2xkeybindings[i].set1==_toggle_look)
				templook=true;
			else
				templook=false;
		}

	for(i=0;i<14;i++) {
		if(joystate(i,c)) {
			int state;

			if(input_preferences->lookswitch) {	
				state = gp2xkeybindings[i].set2;
				if(state==0) state=gp2xkeybindings[i].set1;
			} else {
				state = gp2xkeybindings[i].set1;
				if(state==0) state=gp2xkeybindings[i].set2;
			}
			
			if(state!=_toggle_look && state!=_hold_look) {
				if(state==_looking_down && input_preferences->invert)
					flags |= _looking_up;
				else if(state==_looking_up && input_preferences->invert)
					flags |= _looking_down;
				else
					flags |= state;
			}
		}
	}*/
	
	//MKTODO: Use This
	/*for ( i=0; i<SDL_JoystickNumButtons(joystick); ++i ) {
			if (SDL_JoystickGetButton(joystick, i) == SDL_PRESSED) {
			
			}
		}*/
	
#ifdef psp
	
	SDL_Joystick *joystick = SDL_JoystickOpen(0);
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_LEFT) == SDL_PRESSED &&
		SDL_JoystickGetButton(joystick,PSP_BUTTON_TRIANGLE) != SDL_PRESSED) {
		flags |= _turning_left;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_RIGHT) == SDL_PRESSED&&
		SDL_JoystickGetButton(joystick,PSP_BUTTON_TRIANGLE) != SDL_PRESSED) {
		flags |= _turning_right;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_UP) == SDL_PRESSED &&
		SDL_JoystickGetButton(joystick,PSP_BUTTON_TRIANGLE) != SDL_PRESSED) {
		flags |= _moving_forward;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_DOWN) == SDL_PRESSED &&
		SDL_JoystickGetButton(joystick,PSP_BUTTON_TRIANGLE) != SDL_PRESSED) {
		flags |= _moving_backward;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_CROSS) == SDL_PRESSED) {
		flags |= _left_trigger_state;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_SQUARE) == SDL_PRESSED) {
		flags |= _right_trigger_state;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_CIRCLE) == SDL_PRESSED) {
		flags |= _action_trigger_state;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_L) == SDL_PRESSED) {
		flags |= _sidestepping_left;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_R) == SDL_PRESSED) {
		flags |= _sidestepping_right;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_SELECT) == SDL_PRESSED) {
		flags |= _toggle_map;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_TRIANGLE) == SDL_PRESSED &&
			SDL_JoystickGetButton(joystick,PSP_BUTTON_LEFT) == SDL_PRESSED) {
		flags |= _cycle_weapons_backward;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_TRIANGLE) == SDL_PRESSED &&
			SDL_JoystickGetButton(joystick,PSP_BUTTON_RIGHT) == SDL_PRESSED) {
		flags |= _cycle_weapons_forward;
	}
	
	if (SDL_JoystickGetButton(joystick,PSP_BUTTON_L) == SDL_PRESSED &&
			SDL_JoystickGetButton(joystick,PSP_BUTTON_R) == SDL_PRESSED) {
		flags |= _swim;
	}
	
	/*
	 * PSP: Look only if not in overhead map as looking share the same
	 *		controls of zooming
	 */
	 
	if (!PLAYER_HAS_MAP_OPEN(current_player)) {
		if (SDL_JoystickGetButton(joystick,PSP_BUTTON_TRIANGLE) == SDL_PRESSED &&
				SDL_JoystickGetButton(joystick,PSP_BUTTON_UP) == SDL_PRESSED) {
			flags |= _looking_up;
		}
		
		if (SDL_JoystickGetButton(joystick,PSP_BUTTON_TRIANGLE) == SDL_PRESSED &&
				SDL_JoystickGetButton(joystick,PSP_BUTTON_DOWN) == SDL_PRESSED) {
			flags |= _looking_down;
		}
		
		if (SDL_JoystickGetButton(joystick,PSP_BUTTON_TRIANGLE) == SDL_PRESSED &&
				SDL_JoystickGetButton(joystick,PSP_BUTTON_CIRCLE) == SDL_PRESSED) {
			flags |= _looking_center;
		}
	}
	
	SDL_JoystickClose(joystick);
	
#endif

	if(templook) input_preferences->lookswitch=false;

      // ZZZ: let mouse code simulate keypresses
      mouse_buttons_become_keypresses(key_map);
      
      // Parse the keymap
      key_definition *key = current_key_definitions;
      for (unsigned i=0; i<NUMBER_OF_STANDARD_KEY_DEFINITIONS; i++, key++)
	if (key_map[key->offset])
	  flags |= key->action_flag;
      
      // Post-process the keymap
      struct special_flag_data *special = special_flags;
      for (unsigned i=0; i<NUMBER_OF_SPECIAL_FLAGS; i++, special++) {
	if (flags & special->flag) {
	  switch (special->type) {
	  case _double_flag:
	    // If this flag has a double-click flag and has been hit within
	    // DOUBLE_CLICK_PERSISTENCE (but not at MAXIMUM_FLAG_PERSISTENCE),
	    // mask on the double-click flag */
	    if (special->persistence < MAXIMUM_FLAG_PERSISTENCE
		&&	special->persistence > MAXIMUM_FLAG_PERSISTENCE - DOUBLE_CLICK_PERSISTENCE)
	      flags |= special->alternate_flag;
	    break;
	    
	  case _latched_flag:
	    // If this flag is latched and still being held down, mask it out
	    if (special->persistence == MAXIMUM_FLAG_PERSISTENCE)
	      flags &= ~special->flag;
	    break;
	    
	  default:
	    assert(false);
	    break;
	  }
	  
	  special->persistence = MAXIMUM_FLAG_PERSISTENCE;
	} else
	  special->persistence = FLOOR(special->persistence-1, 0);
      }
      
      // Handle the selected input controller
      if (input_preferences->input_device != _keyboard_or_game_pad) {
	_fixed delta_yaw, delta_pitch, delta_velocity;
	test_mouse(input_preferences->input_device, &flags, &delta_yaw, &delta_pitch, &delta_velocity);
	flags = mask_in_absolute_positioning_information(flags, delta_yaw, delta_pitch, delta_velocity);
      }
      
      // Modify flags with run/walk and swim/sink
      bool do_interchange =
	(local_player->variables.flags & _HEAD_BELOW_MEDIA_BIT) ?
	(input_preferences->modifiers & _inputmod_interchange_swim_sink) != 0:
	(input_preferences->modifiers & _inputmod_interchange_run_walk) != 0;
      if (do_interchange)
	flags ^= _run_dont_walk;
      
      if (player_in_terminal_mode(local_player_index))
	flags = build_terminal_action_flags((char *)key_map);
    } // if(get_keyboard_controller_status())
  
  return flags;
}


/*
 *  Get random demo replay from map
 */

bool setup_replay_from_random_resource(uint32 map_checksum)
{
	// not supported in SDL version
	return false;
}


/*
 *  Set keys to match preferences
 */

void set_keys_to_match_preferences(void)
{
	set_keys(input_preferences->keycodes);
}


/*
 *  Periodic task management
 */

typedef bool (*timer_func)(void);

static timer_func tm_func = NULL;	// The installed timer task
static uint32 tm_period;			// Ticks between two calls of the timer task
static uint32 tm_last = 0, tm_accum = 0;

timer_task_proc install_timer_task(short tasks_per_second, timer_func func)
{
	// We only handle one task, which is enough
	tm_period = 1000 / tasks_per_second;
	tm_func = func;
	tm_last = SDL_GetTicks();
	tm_accum = 0;
	return (timer_task_proc)tm_func;
}

void remove_timer_task(timer_task_proc proc)
{
	tm_func = NULL;
}

void execute_timer_tasks(void)
{
	if (tm_func) {
		uint32 now = SDL_GetTicks();
		tm_accum += now - tm_last;
		tm_last = now;
		bool first_time = true;
		while (tm_accum >= tm_period) {
			tm_accum -= tm_period;
			if (first_time) {
				if(get_keyboard_controller_status())
					mouse_idle(input_preferences->input_device);

				first_time = false;
			}
			tm_func();
		}
	}
}
