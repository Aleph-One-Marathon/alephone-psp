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
	
	February 21, 2000 (Loren Petrich)
	
	Animated-textures-interface file.
	
	May 14, 2000 (Loren Petrich)
	
	Modified the internal architecture heavily,
	renamed one of the external calls,
	and added XML-configuration support
*/

#ifndef _ANIMATED_TEXTURES
#define _ANIMATED_TEXTURES

#include "shape_descriptors.h"
#include "XML_ElementParser.h"

// Updates the animated textures
void AnimTxtr_Update();

// Does animated-texture translation.
// Note: a shape_descriptor is really a short integer
shape_descriptor AnimTxtr_Translate(shape_descriptor Texture);

// XML-parser support
XML_ElementParser *AnimatedTextures_GetParser();

#endif
