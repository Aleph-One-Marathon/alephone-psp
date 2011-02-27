/*

	Copyright (C) 1991-2001 and beyond by Bo Lindbergh
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
#ifndef _CSERIES_MISC_
#define _CSERIES_MISC_

#ifdef mac
#define MACINTOSH_TICKS_PER_SECOND 60
#define MACHINE_TICKS_PER_SECOND MACINTOSH_TICKS_PER_SECOND
#elif defined(SDL)
#define MACHINE_TICKS_PER_SECOND 1000
#else
#error MACHINE_TICKS_PER_SECOND not defined for this platform
#endif

extern uint32 machine_tick_count(void);
extern bool wait_for_click_or_keypress(
	uint32 ticks);

#ifdef env68k

#pragma parameter __D0 get_a0

extern long get_a0(void)
	= {0x2008};

#pragma parameter __D0 get_a5

extern long get_a5(void)
	= {0x200D};

#pragma parameter __D0 set_a5(__D0)

extern long set_a5(
	long a5)
	= {0xC18D};

#endif

extern void kill_screen_saver(void);

#ifdef DEBUG
extern void initialize_debugger(bool on);
#endif

#endif
