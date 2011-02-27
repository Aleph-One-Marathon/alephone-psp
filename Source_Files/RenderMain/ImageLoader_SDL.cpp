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
 *  ImageLoader_SDL.cpp - Image file loading, SDL implementation
 *
 *  Written in 2001 by Christian Bauer
 */

#include "ImageLoader.h"
#include "FileHandler.h"

#ifdef HAVE_SDL_IMAGE_H
#include <SDL_image.h>
#endif


/*
 *  Load specified image file
 */

bool ImageDescriptor::LoadFromFile(FileSpecifier& File, int ImgMode, int flags, int actual_width, int actual_height, int maxSize)
{
	if (flags & ImageLoader_ImageIsAlreadyPremultiplied)
		PremultipliedAlpha = true;

	// Don't load opacity if there is no color component:
	switch(ImgMode) {
		case ImageLoader_Colors:
			if (LoadDDSFromFile(File, flags, actual_width, actual_height, maxSize)) return true;
			break;
		
		case ImageLoader_Opacity:
			if (!IsPresent())
				return false;
			break;
		
		default:
			vassert(false, csprintf(temporary,"Bad image mode for loader: %d",ImgMode));
	}

	// Load image to surface
#ifndef SDL_RFORK_HACK
#ifdef HAVE_SDL_IMAGE
	SDL_Surface *s = IMG_Load(File.GetPath());
#else
	SDL_Surface *s = SDL_LoadBMP(File.GetPath());
#endif
#else
SDL_Surface *s = NULL;
#endif
	if (s == NULL)
		return false;

	// Get image dimensions and set its size
	int Width = s->w, Height = s->h;
	int OriginalWidth = (actual_width) ? actual_width : Width;
	int OriginalHeight = (actual_height) ? actual_height : Height;
	if (flags & ImageLoader_ResizeToPowersOfTwo) {
		Width = NextPowerOfTwo(Width);
		Height = NextPowerOfTwo(Height);
	}
	switch (ImgMode) {
		case ImageLoader_Colors:
			Resize(Width, Height);
			VScale = ((double) OriginalWidth / (double) Width);
			UScale = ((double) OriginalHeight / (double) Height);
			MipMapCount = 0;
			break;

		case ImageLoader_Opacity:
			// If the wrong size, then bug out
			if (Width != this->Width || Height != this->Height || ((double) OriginalWidth / Width != VScale || ((double) OriginalHeight / Height != UScale))) {
				SDL_FreeSurface(s);
				return false;
			}
			break;
	}

	// Convert to 32-bit OpenGL-friendly RGBA surface
#ifdef ALEPHONE_LITTLE_ENDIAN
	SDL_Surface *rgba = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#else
	SDL_Surface *rgba = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#endif
	if (rgba == NULL) {
		SDL_FreeSurface(s);
		return false;
	}

	SDL_SetAlpha(s, 0, 0xff); // disable SDL_SRCALPHA
	SDL_BlitSurface(s, NULL, rgba, NULL);
	SDL_FreeSurface(s);

	// Convert surface to RGBA texture
	switch (ImgMode) {
		case ImageLoader_Colors:
			memcpy(GetPixelBasePtr(), rgba->pixels, Width * Height * 4);
			break;

		case ImageLoader_Opacity: {
			uint8 *p = (uint8 *)rgba->pixels;
			uint8 *q = (uint8 *)GetPixelBasePtr();
			for (int h=0; h<Height; h++) {
				for (int w=0; w<Width; w++) {
					// RGB to greyscale value, and then to the opacity
					float Red = float(*p++);
					float Green = float(*p++);
					float Blue = float(*p++);
					p++;
					float Opacity = (Red + Green + Blue) / 3.0F;
					q[3] = PIN(int(Opacity + 0.5), 0, 255);
					q += 4;
				}
			}
			break;
		}
	}

	SDL_FreeSurface(rgba);
	return true;
}
