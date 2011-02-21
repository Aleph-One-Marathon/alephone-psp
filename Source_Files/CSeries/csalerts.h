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


#ifndef _CSERIES_ALERTS_
#define _CSERIES_ALERTS_

#if defined(__GNUC__)
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif

enum {
	infoError,
	fatalError
};

extern void alert_user(char *message, short severity = infoError);

extern void alert_user(
	short severity,
	short resid,
	short item,
	OSErr error);

extern void pause_debug(void);
extern void vpause(
	char *message);

extern void halt(void) NORETURN;
extern void vhalt(
	char *message) NORETURN;

extern void _alephone_assert(
	char *file,
	long line,
	char *what) NORETURN;
extern void _alephone_warn(
	char *file,
	long line,
	char *what);

#if TARGET_API_MAC_CARBON && !defined(SDL)
DialogItemIndex SimpleAlert(AlertType Type, const char *Message, const char *Explain = NULL);
#endif

#undef assert

#ifdef DEBUG
#define assert(what) ((what) ? (void)0 : _alephone_assert(__FILE__,__LINE__,"Assertion failed: " #what))
#define vassert(what,message) ((what) ? (void)0 : _alephone_assert(__FILE__,__LINE__,(message)))
#define warn(what) ((what) ? (void)0 : _alephone_warn(__FILE__,__LINE__,"Assertion failed: " #what))
#define vwarn(what,message) ((what) ? (void)0 : _alephone_warn(__FILE__,__LINE__,(message)))
#else
#define assert(what) ((void) 0)
#define vassert(what,message) ((void) 0)
#define warn(what) ((void) 0)
#define vwarn(what,message) ((void) 0)
#endif

#endif
