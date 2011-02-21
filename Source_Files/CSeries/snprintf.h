/*
 *  snprintf.h - crude, unsafe imitation of the real snprintf() and vsnprintf()

	Copyright (C) 2003 and beyond by Woody Zenfell, III
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


    Jan. 17, 2003 (Woody Zenfell): Created.

*/

#ifndef SNPRINTF_H
#define SNPRINTF_H

#include <stdarg.h>
#include <streambuf> // Fix MSVC7 strangeness

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_SNPRINTF
extern int snprintf(char* inBuffer, size_t inBufferSize, const char* inFormat, ...);
#endif

#ifndef HAVE_VSNPRINTF
extern int vsnprintf(char* inBuffer, size_t inBufferSize, const char* inFormat, va_list inArgs);
#endif

#endif // SNPRINTF_H
