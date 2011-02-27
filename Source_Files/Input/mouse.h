#ifndef __MOUSE_H
#define __MOUSE_H

/*
MOUSE.H

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

Tuesday, January 17, 1995 2:53:17 PM  (Jason')

    May 16, 2002 (Woody Zenfell):
        semi-hacky scheme in SDL to let mouse buttons simulate keypresses
*/

void enter_mouse(short type);
void test_mouse(short type, uint32 *action_flags, _fixed *delta_yaw, _fixed *delta_pitch, _fixed *delta_velocity);
void exit_mouse(short type);
void mouse_idle(short type);
void recenter_mouse(void);

// ZZZ: stuff of various hackiness levels to pretend mouse buttons are keys
#ifdef SDL
void mouse_buttons_become_keypresses(Uint8* ioKeyMap);
void mouse_scroll(bool up);

#define NUM_SDL_MOUSE_BUTTONS 8   // since SDL_GetMouseState() returns 8 bits
#define SDLK_BASE_MOUSE_BUTTON 65 // this is button 1's pseudo-keysym
#endif // SDL

#endif
