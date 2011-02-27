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

	Font handler
	by Loren Petrich,
	December 17, 2000
	
	This is for specifying and working with text fonts

Dec 25, 2000 (Loren Petrich):
	Added OpenGL-rendering support

Dec 31, 2000 (Loren Petrich):
	Switched to a 32-bit intermediate GWorld, so that text antialiasing
	will work properly.

Jan 12, 2001 (Loren Petrich):
	Fixed MacOS version of TextWidth() -- uses current font
*/

#include "cseries.h"

#ifdef __MVCPP__
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

#include <math.h>
#include <string.h>
#include "FontHandler.h"

#include "shape_descriptors.h"
#include "screen_drawing.h"


// MacOS-specific: stuff that gets reused
// static CTabHandle Grays = NULL;

// Font-specifier equality and assignment:

bool FontSpecifier::operator==(FontSpecifier& F)
{
	if (strncmp(NameSet,F.NameSet,NameSetLen) != 0) return false;
	if (Size != F.Size) return false;
	if (Style != F.Style) return false;
	if (strncmp(File,F.File,NameSetLen) != 0) return false;
	return true;
}

FontSpecifier& FontSpecifier::operator=(FontSpecifier& F)
{
	memcpy(NameSet,F.NameSet,NameSetLen);
	Size = F.Size;
	Style = F.Style;
	memcpy(File,F.File,NameSetLen);
	return *this;
}

// Initializer: call before using because of difficulties in setting up a proper constructor:

void FontSpecifier::Init()
{
#ifdef SDL
	Info = NULL;
#endif
	Update();
#ifdef HAVE_OPENGL
	OGL_Texture = NULL;
#endif
}


// MacOS- and SDL-specific stuff
#if defined(mac)

// Cribbed from _get_font_line_spacing() and similar functions in screen_drawing.cpp

void FontSpecifier::Update()
{
	// csfonts -- push old font
	TextSpec OldFont;
	GetFont(&OldFont);
	
	// Parse the font spec to find the font ID;
	// if it is not set, then it is assumed to be the system font
	ID = 0;
	
	Str255 Name;
	
	char *NamePtr = FindNextName(NameSet);
	
	while(NamePtr)
	{
		char *NameEndPtr = FindNameEnd(NamePtr);
		
		// Make a Pascal string out of the name
		int NameLen = MIN(NameEndPtr - NamePtr, 255);
		Name[0] = NameLen;
		memcpy(Name+1,NamePtr,NameLen);
		Name[NameLen+1] = 0;
		// dprintf("Name, Len: <%s>, %d",Name+1,NameLen);
		
		// MacOS ID->name translation
		GetFNum(Name,&ID);
		// dprintf("ID = %d",ID);
		if (ID != 0) break;
		
		NamePtr = FindNextName(NameEndPtr);
	}
	
	// Get the other font features
	Use();
	FontInfo Info;
	GetFontInfo(&Info);
	
	Ascent = Info.ascent;
	Descent = Info.descent;
	Leading = Info.leading;
	Height = Ascent + Leading;
	LineSpacing = Ascent + Descent + Leading;

	for (int k=0; k<256; k++)
		Widths[k] = ::CharWidth(k);
	
	// pop old font
	SetFont(&OldFont);
}


void FontSpecifier::Use()
{
	// MacOS-specific:
	TextFont(ID);
	TextFace(Style);
	TextSize(Size);
}


int FontSpecifier::TextWidth(const char *Text)
{
	// csfonts -- push old font
	TextSpec OldFont;
	GetFont(&OldFont);
	
	// Set to use current font
	Use();
	
	int Len = MIN(strlen(Text),255);
	// MacOS-specific; note the :: for getting the top-level function instead of a member one
	return int(::TextWidth(Text,0,Len));
	
	// pop old font
	SetFont(&OldFont);
}

#elif defined(SDL)

void FontSpecifier::Update()
{
	// Clear away
	if (Info) {
		unload_font(Info);
		Info = NULL;
	}
		
	TextSpec Spec;
	// Simply implements format "#<value>"; may want to generalize this
	if (File[0] == '#') 
	{
		short ID;
		sscanf(File+1, "%hd", &ID);
		
		Spec.font = ID;
	}
	else
	{
		Spec.font = -1; // no way to fall back :(
		Spec.normal = File;
	}

	Spec.size = Size;
	Spec.style = Style;
	Spec.adjust_height = AdjustLineHeight;
	Info = load_font(Spec);
	
	if (Info) {
		Ascent = Info->get_ascent();
		Descent = Info->get_descent();
		Leading = Info->get_leading();
		Height = Ascent + Leading;
		LineSpacing = Ascent + Descent + Leading;
		for (int k=0; k<256; k++)
			Widths[k] = char_width(k, Info, Style);
	} else
		Ascent = Descent = Leading = Height = LineSpacing = 0;
}

// Defined in screen_drawing_sdl.cpp
extern int8 char_width(uint8 c, const sdl_font_info *font, uint16 style);

int FontSpecifier::TextWidth(const char *text)
{
	int width = 0;
	char c;
	while ((c = *text++) != 0)
		width += Widths[c];
	return width;
}

#endif

#ifdef HAVE_OPENGL
// Reset the OpenGL fonts; its arg indicates whether this is for starting an OpenGL session
// (this is to avoid texture and display-list memory leaks and other such things)
void FontSpecifier::OGL_Reset(bool IsStarting)
{
	// Don't delete these if there is no valid texture;
	// that indicates that there are no valid texture and display-list ID's.
	if (!IsStarting && !OGL_Texture)
	{
		glDeleteTextures(1,&TxtrID);
		glDeleteLists(DispList,256);
	}

	// Invalidates whatever texture had been present
	if (OGL_Texture)
	{
		delete[]OGL_Texture;
		OGL_Texture = NULL;
	}
	
	// Put some padding around each glyph so as to avoid clipping it
	const int Pad = 1;
	int ascent_p = Ascent + Pad, descent_p = Descent + Pad;
	int widths_p[256];
	for (int i=0; i<256; i++) {
	  widths_p[i] = Widths[i] + 2*Pad;
	}
	// Now for the totals and dimensions
	int TotalWidth = 0;
	for (int k=0; k<256; k++)
		TotalWidth += widths_p[k];
	
	// For an empty font, clear out
	if (TotalWidth <= 0) return;
	
	int GlyphHeight = ascent_p + descent_p;
	
	int EstDim = int(sqrt(static_cast<float>(TotalWidth*GlyphHeight)) + 0.5);
	TxtrWidth = MAX(128, NextPowerOfTwo(EstDim));
	
	// Find the character starting points and counts
	unsigned char CharStarts[256], CharCounts[256];
	int LastLine = 0;
	CharStarts[LastLine] = 0;
	CharCounts[LastLine] = 0;
	short Pos = 0;
	for (int k=0; k<256; k++)
	{
		// Over the edge? If so, then start a new line
		short NewPos = Pos + widths_p[k];
		if (NewPos > TxtrWidth)
		{
			LastLine++;
			CharStarts[LastLine] = k;
			Pos = widths_p[k];
			CharCounts[LastLine] = 1;
		} else {
			Pos = NewPos;
			CharCounts[LastLine]++;
		}
	}
	TxtrHeight = MAX(128, NextPowerOfTwo(GlyphHeight*(LastLine+1)));
	
#if defined(mac)
		
	// MacOS-specific: render the font glyphs onto a GWorld,
	// and use it as the source of the font texture.
	Rect ImgRect;
	SetRect(&ImgRect,0,0,TxtrWidth,TxtrHeight);
	GWorldPtr FTGW;
	OSErr Err = NewGWorld(&FTGW,32,&ImgRect,0,0,0);
	if (Err != noErr) return;
	PixMapHandle Pxls = GetGWorldPixMap(FTGW);
	LockPixels(Pxls);
	
	CGrafPtr OrigPort;
	GDHandle OrigDevice;
	GetGWorld(&OrigPort,&OrigDevice);
 	SetGWorld(FTGW,0);
	
	BackColor(blackColor);
	ForeColor(whiteColor);
 	EraseRect(&ImgRect);
 	
 	Use();	// The GWorld needs its font set for it
 	
 	for (int k=0; k<=LastLine; k++)
 	{
 		char Which = CharStarts[k];
 		int VPos = k*GlyphHeight + ascent_p;
 		int HPos = Pad;
 		for (int m=0; m<CharCounts[k]; m++)
 		{
 			MoveTo(HPos,VPos);
 			DrawChar(Which);
 			HPos += widths_p[(unsigned char) (Which++)];
 		}
 	}

#elif defined(SDL)
	// Render the font glyphs into the SDL surface
	SDL_Surface *FontSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, TxtrWidth, TxtrHeight, 32, 0xff0000, 0x00ff00, 0x0000ff, 0);
	if (FontSurface == NULL)
		return;

	// Set background to black
	SDL_FillRect(FontSurface, NULL, SDL_MapRGB(FontSurface->format, 0, 0, 0));
	Uint32 White = SDL_MapRGB(FontSurface->format, 0xFF, 0xFF, 0xFF);
	
	// Copy to surface
	for (int k = 0; k <= LastLine; k++)
	{
		char Which = CharStarts[k];
		int VPos = (k * GlyphHeight) + ascent_p;
		int HPos = Pad;
		for (int m = 0; m < CharCounts[k]; m++)
		{
		  
		  ::draw_text(FontSurface, &Which, 1, HPos, VPos, White, Info, Style);
		  HPos += widths_p[(unsigned char) (Which++)];
		}
	}
#endif
 	
 	// Non-MacOS-specific: allocate the texture buffer
 	// Its format is LA 88, where L is the luminosity and A is the alpha channel
 	// The font value will go into A.
 	OGL_Texture = new uint8[2*GetTxtrSize()];
	
#if defined(mac)
	// Now copy from the GWorld into the OpenGL texture	
	uint8 *PixBase = (byte *)GetPixBaseAddr(Pxls);
 	int Stride = int((**Pxls).rowBytes & 0x7fff);
 	
 	for (int k=0; k<TxtrHeight; k++)
 	{
 		uint8 *SrcPxl = PixBase + k*Stride + 1;	// Use red channel
 		uint8 *DstPxl = OGL_Texture + 2*k*TxtrWidth;
 		for (int m=0; m<TxtrWidth; m++)
 		{
 			*(DstPxl++) = 0xff;	// Base color: white (will be modified with glColorxxx())
 			*(DstPxl++) = *SrcPxl;
 			SrcPxl += 4;
 		}
 	}
 	
 	UnlockPixels(Pxls);
 	SetGWorld(OrigPort,OrigDevice);
 	DisposeGWorld(FTGW);

#elif defined(SDL)
	// Copy the SDL surface into the OpenGL texture
	uint8 *PixBase = (uint8 *)FontSurface->pixels;
	int Stride = FontSurface->pitch;
 	
 	for (int k=0; k<TxtrHeight; k++)
 	{
 		uint8 *SrcPxl = PixBase + k*Stride + 1;	// Use one of the middle channels (red or green or blue)
 		uint8 *DstPxl = OGL_Texture + 2*k*TxtrWidth;
 		for (int m=0; m<TxtrWidth; m++)
 		{
 			*(DstPxl++) = 0xff;	// Base color: white (will be modified with glColorxxx())
 			*(DstPxl++) = *SrcPxl;
 			SrcPxl += 4;
  		}
 	}
	
	// Clean up
	SDL_FreeSurface(FontSurface);
#endif
	
	// OpenGL stuff starts here 	
 	// Load texture
 	glGenTextures(1,&TxtrID);
 	glBindTexture(GL_TEXTURE_2D,TxtrID);
 	
 	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, TxtrWidth, TxtrHeight,
		0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, OGL_Texture);
 	
 	// Allocate and create display lists of rendering commands
 	DispList = glGenLists(256);
 	GLfloat TWidNorm = GLfloat(1)/TxtrWidth;
 	GLfloat THtNorm = GLfloat(1)/TxtrHeight;
 	for (int k=0; k<=LastLine; k++)
 	{
 		unsigned char Which = CharStarts[k];
 		GLfloat Top = k*(THtNorm*GlyphHeight);
 		GLfloat Bottom = (k+1)*(THtNorm*GlyphHeight);
 		int Pos = 0;
 		for (int m=0; m<CharCounts[k]; m++)
 		{
 			short Width = widths_p[Which];
 			int NewPos = Pos + Width;
 			GLfloat Left = TWidNorm*Pos;
 			GLfloat Right = TWidNorm*NewPos;
 			
 			glNewList(DispList + Which, GL_COMPILE);
 			
 			// Move to the current glyph's (padded) position
 			glTranslatef(-Pad,0,0);
 			
 			// Draw the glyph rectangle
 			// Due to a bug in MacOS X Classic OpenGL, glVertex2s() was changed to glVertex2f()
 			glBegin(GL_POLYGON);
 			
 			glTexCoord2f(Left,Top);
  			glVertex2d(0,-ascent_p);
  			
 			glTexCoord2f(Right,Top);
  			glVertex2d(Width,-ascent_p);
  			
 			glTexCoord2f(Right,Bottom);
 			glVertex2d(Width,descent_p);
 			
 			glTexCoord2f(Left,Bottom);
			glVertex2d(0,descent_p);
			
			glEnd();
			
			// Move to the next glyph's position
			glTranslated(Width-Pad,0,0);
			
 			glEndList();
 			
 			// For next one
 			Pos = NewPos;
 			Which++;
 		}
 	}
}


// Renders a C-style string in OpenGL.
// assumes screen coordinates and that the left baseline point is at (0,0).
// Alters the modelview matrix so that the next characters will be drawn at the proper place.
// One can surround it with glPushMatrix() and glPopMatrix() to remember the original.
void FontSpecifier::OGL_Render(const char *Text)
{
	// Bug out if no texture to render
	if (!OGL_Texture) return;
	
	glPushAttrib(GL_ENABLE_BIT);
	
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glBindTexture(GL_TEXTURE_2D,TxtrID);
	
	size_t Len = MIN(strlen(Text),255);
	for (size_t k=0; k<Len; k++)
	{
		unsigned char c = Text[k];
		glCallList(DispList+c);
	}
	
	glPopAttrib();
}


// Renders text a la _draw_screen_text() (see screen_drawing.h), with
// alignment and wrapping. Modelview matrix is unaffected.
void FontSpecifier::OGL_DrawText(const char *text, const screen_rectangle &r, short flags)
{
	// Copy the text to draw
	char text_to_draw[256];
	strncpy(text_to_draw, text, 256);
	text_to_draw[255] = 0;

	// Check for wrapping, and if it occurs, be recursive
	if (flags & _wrap_text) {
		int last_non_printing_character = 0, text_width = 0;
		unsigned count = 0;
		while (count < strlen(text_to_draw) && text_width < RECTANGLE_WIDTH(&r)) {
			text_width += CharWidth(text_to_draw[count]);
			if (text_to_draw[count] == ' ')
				last_non_printing_character = count;
			count++;
		}
		
		if( count != strlen(text_to_draw)) {
			char remaining_text_to_draw[256];
			
			// If we ever have to wrap text, we can't also center vertically. Sorry.
			flags &= ~_center_vertical;
			flags |= _top_justified;
			
			// Pass the rest of it back in, recursively, on the next line
			memcpy(remaining_text_to_draw, text_to_draw + last_non_printing_character + 1, strlen(text_to_draw + last_non_printing_character + 1) + 1);
	
			screen_rectangle new_destination = r;
			new_destination.top += LineSpacing;
			OGL_DrawText(remaining_text_to_draw, new_destination, flags);
	
			// Now truncate our text to draw
			text_to_draw[last_non_printing_character] = 0;
		}
	}

	// Truncate text if necessary
	int t_width = TextWidth(text_to_draw);
	if (t_width > RECTANGLE_WIDTH(&r)) {
		int width = 0;
		int num = 0;
		char c, *p = text_to_draw;
		while ((c = *p++) != 0) {
			width += CharWidth(c);
			if (width > RECTANGLE_WIDTH(&r))
				break;
			num++;
		}
		text_to_draw[num] = 0;
		t_width = TextWidth(text_to_draw);
	}


	// Horizontal positioning
	int x, y;
	if (flags & _center_horizontal)
		x = r.left + ((RECTANGLE_WIDTH(&r) - t_width) / 2);
	else if (flags & _right_justified)
		x = r.right - t_width;
	else
		x = r.left;

	// Vertical positioning
	if (flags & _center_vertical) {
		if (Height > RECTANGLE_HEIGHT(&r))
			y = r.top;
		else {
			y = r.bottom;
			int offset = RECTANGLE_HEIGHT(&r) - Height;
			y -= (offset / 2) + (offset & 1) + 1;
		}
	} else if (flags & _top_justified) {
		if (Height > RECTANGLE_HEIGHT(&r))
			y = r.bottom;
		else
			y = r.top + Height;
	} else
		y = r.bottom;

	// Draw text
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslated(x, y, 0);
	OGL_Render(text_to_draw);
	glPopMatrix();
}
#endif // def HAVE_OPENGL

	
// Given a pointer to somewhere in a name set, returns the pointer
// to the start of the next name, or NULL if it did not find any
char *FontSpecifier::FindNextName(char *NamePtr)
{
	char *NextNamePtr = NamePtr;
	char c;
	
	// Continue as long as one's in the string
	while((c = *NextNamePtr) != 0)
	{
		// dprintf("Nxt %c",c);
		// Check for whitespace and commas and semicolons
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',' || c == ';')
		{
			NextNamePtr++;
			continue;
		}
		return NextNamePtr;
	}
	return NULL;
}

// Given a pointer to the beginning of a name, finds the pointer to the character
// just after the end of that name
char *FontSpecifier::FindNameEnd(char *NamePtr)
{
	char *NameEndPtr = NamePtr;
	char c;
	
	// Continue as long as one's in the string
	while((c = *NameEndPtr) != 0)
	{
		// dprintf("End %c",c);
		// Delimiter: comma or semicolon
		if (c == ',' || c == ';') break;
		NameEndPtr++;
	}
	return NameEndPtr;
}


// Font-parser object:
class XML_FontParser: public XML_ElementParser
{
	FontSpecifier TempFont;
	short Index;
	bool IsPresent[5];

public:
	FontSpecifier *FontList;
	int NumFonts;
	
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	
	XML_FontParser(): XML_ElementParser("font"), FontList(NULL) {}
};

bool XML_FontParser::Start()
{
	for (int k=0; k<5; k++)
		IsPresent[k] = false;
	return true;
}

bool XML_FontParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (NumFonts > 0)
	{
	if (StringsEqual(Tag,"index"))
	{
		if (ReadBoundedInt16Value(Value,Index,0,NumFonts-1))
		{
			IsPresent[3] = true;
			return true;
		}
		else return false;
	}
	}
	if (StringsEqual(Tag,"name"))
	{
		IsPresent[0] = true;
		strncpy(TempFont.NameSet,Value,FontSpecifier::NameSetLen);
		TempFont.NameSet[FontSpecifier::NameSetLen-1] = 0;	// For making the set always a C string
		return true;
	}
	else if (StringsEqual(Tag,"size"))
	{
		if (ReadInt16Value(Value,TempFont.Size))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"style"))
	{
		if (ReadInt16Value(Value,TempFont.Style))
		{
			IsPresent[2] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"file"))
	{
		IsPresent[4] = true;
		strncpy(TempFont.File,Value,FontSpecifier::NameSetLen);
		TempFont.File[FontSpecifier::NameSetLen-1] = 0;	// For making the set always a C string
		return true;
	}
	UnrecognizedTag();
	return false;
}

bool XML_FontParser::AttributesDone()
{
	// Verify...
	//bool AllPresent = true;
	if (NumFonts <= 0)
	{
		IsPresent[3] = true;	// Convenient fakery: no index -- always present
		Index = 0;
	}	
	if (!IsPresent[3])
	{
		AttribsMissing();
		return false;
	}
	
	// Put into place and update if necessary
	assert(FontList);
	bool DoUpdate = false;
	if (IsPresent[0])
	{
		strncpy(FontList[Index].NameSet,TempFont.NameSet,FontSpecifier::NameSetLen);
		DoUpdate = true;
	}
	if (IsPresent[1])
	{
		FontList[Index].Size = TempFont.Size;
		DoUpdate = true;
	}
	if (IsPresent[2])
	{
		FontList[Index].Style = TempFont.Style;
		DoUpdate = true;
	}
	if (IsPresent[4])
	{
		strncpy(FontList[Index].File,TempFont.File,FontSpecifier::NameSetLen);
		DoUpdate = true;
	}
	if (DoUpdate) FontList[Index].Update();
	return true;
}

static XML_FontParser FontParser;



// Returns a parser for the colors;
// several elements may have colors, so this ought to be callable several times.
XML_ElementParser *Font_GetParser() {return &FontParser;}

// This sets the array of colors to be read into.
// Its args are the pointer to that array and the number of colors in it.
// If that number is <= 0, then the color value is assumed to be non-indexed,
// and no "index" attribute will be searched for.
void Font_SetArray(FontSpecifier *FontList, int NumFonts)
{FontParser.FontList = FontList; FontParser.NumFonts = NumFonts;}
