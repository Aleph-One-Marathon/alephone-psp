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

	Created by Loren Petrich,
	Dec. 23, 2000
	Contains everything shared between screen.cpp and screen_sdl.cpp
	
Dec 29, 2000 (Loren Petrich):
	Added stuff for doing screen messages

Mar 19, 2001 (Loren Petrich):
	Added some even bigger screen resolutions

Jan 25, 2002 (Br'fin (Jeremy Parsons)):
	Added accessors for datafields now opaque in Carbon

 Aug 6, 2003 (Woody Zenfell):
	Minor tweaks to screen_printf() mechanism (safer; resets when screen_reset called)
*/

#include <stdarg.h>
#ifndef HAVE_SNPRINTF
#include "snprintf.h"	// for platforms that don't have it
// Maybe someone will work some autoconf/manual config.h magic so we can really only
// include snprintf.h (and snprintf.cpp in the build) when needed.
#endif

// MK: On the PSP different video modes don't make sense
/*#ifdef PSP
#define DESIRED_SCREEN_WIDTH 480
#define DESIRED_SCREEN_HEIGHT 272

// Biggest possible of those defined
#define MAXIMUM_WORLD_WIDTH 480
#define MAXIMUM_WORLD_HEIGHT 272

#define DEFAULT_WORLD_WIDTH 480
#define DEFAULT_WORLD_HEIGHT 272

#else*/
#define DESIRED_SCREEN_WIDTH 640
#define DESIRED_SCREEN_HEIGHT 480

// Biggest possible of those defined
#define MAXIMUM_WORLD_WIDTH 1900
#define MAXIMUM_WORLD_HEIGHT 1200

#define DEFAULT_WORLD_WIDTH 640
#define DEFAULT_WORLD_HEIGHT 320
//#endif

#include "Console.h"
#include "screen_drawing.h"

// LP addition: view sizes and display data


struct ViewSizeData
{
	short OverallWidth, OverallHeight;	// Of the display area, so as to properly center everything
	short MainWidth, MainHeight;		// Of the main 3D-rendered view
	short WithHUD, WithoutHUD;			// Corresponding entries that are with the HUD or without it
	short flags; // see below
};

enum { // view flags, has room for expansion
  _view_show_HUD = (1 << 0)
};

/*const*/ ViewSizeData ViewSizes[NUMBER_OF_VIEW_SIZES] =
{
	{ 640,  480,  320,  160,    _320_160_HUD,   _640_480, _view_show_HUD},    //   _320_160_HUD
	{ 640,  480,  480,  240,    _480_240_HUD,   _640_480, _view_show_HUD},	  //   _480_240_HUD
#ifdef PSP
	{480,  272, 480,  272, _480_152_HUD,  _480_272,   _view_show_HUD},
	{480,  272, 480,  272, _480_152_HUD,  _480_272,   0},
#endif
	{ 640,  480,  640,  320,    _640_320_HUD,   _640_480, _view_show_HUD},    //   _640_320_HUD
	{ 640,  480,  640,  480,    _640_320_HUD,   _640_480, 0},                 //   _640_480
	{ 800,  600,  800,  400,    _800_400_HUD,   _800_600, _view_show_HUD},    //   _800_400_HUD
	{ 800,  600,  800,  600,    _800_400_HUD,   _800_600, 0},                 //   _800_600
	{1024,  768, 1024,  512,   _1024_512_HUD,  _1024_768, _view_show_HUD},    //  _1024_512_HUD
	{1024,  768, 1024,  768,   _1024_512_HUD,  _1024_768, 0},                 //  _1024_768
	{1280, 1024, 1280,  640,   _1280_640_HUD, _1280_1024, _view_show_HUD},    //  _1280_640_HUD
	{1280, 1024, 1280, 1024,   _1280_640_HUD, _1280_1024, 0},                 //  _1280_1024
	{1600, 1200, 1600,  800,   _1600_800_HUD, _1600_1200, _view_show_HUD},    //  _1600_800_HUD
	{1600, 1200, 1600, 1200,   _1600_800_HUD, _1600_1200, 0},                 //  _1600_1200
	{1024,  640, 1024,  440,   _1024_440_HUD,  _1024_640, _view_show_HUD},    //  _1024_440_HUD
	{1024,  640, 1024,  640,   _1024_440_HUD,  _1024_640, 0},                 //  _1024_640
	{1280,  800, 1280,  600,   _1280_600_HUD,  _1280_800, _view_show_HUD},    //  _1280_600_HUD
	{1280,  800, 1280,  800,   _1280_600_HUD,  _1280_800, 0},                 //  _1280_800
	{1280,  854, 1280,  654, _1280_640WS_HUD,  _1280_854, _view_show_HUD},    //  _1280_640WS_HUD
	{1280,  854, 1280,  854, _1280_640WS_HUD,  _1280_854, _view_show_HUD},    //  _1280_854
	{1440,  900, 1440,  700,   _1440_700_HUD,  _1440_900, _view_show_HUD},    //  _1440_700_HUD
	{1440,  900, 1440,  700,   _1440_700_HUD,  _1440_900, 0},                 //  _1440_900
	{1680, 1050, 1680,  840,   _1680_840_HUD, _1680_1050, _view_show_HUD},    //  _1680_840_HUD
	{1680, 1050, 1680,  1050,  _1680_840_HUD, _1680_1050, 0},                 //  _1680_1050
	{1920, 1200, 1920,  950,   _1920_950_HUD, _1920_1200, _view_show_HUD},    //  _1900_950_HUD
	{1920, 1200, 1920,  1200,  _1920_950_HUD, _1920_1200, 0},                 //  _1900_1200
	{2560, 1600, 2560, 1280, _2560_1280_HUD, _2560_1600, _view_show_HUD},
	{2560, 1600, 2560, 1600, _2560_1280_HUD, _2560_1600, 0},
	{1280, 768, 1216, 608, _1280_768WS_HUD, _1280_768, _view_show_HUD},
	{1280, 768, 1280, 768, _1280_768WS_HUD, _1280_768, 0},
	{1280, 960, 1280, 640, _1280_640SD_HUD, _1280_960, _view_show_HUD},
	{1280, 960, 1280, 960, _1280_640SD_HUD, _1280_960, 0},
	{1280, 720, 1280, 520, _1280_720_HUD, _1280_720, _view_show_HUD},
	{1280, 720, 1280, 720, _1280_720_HUD, _1280_720, 0},
};

// Note: the overhead map will always fill all of the screen except for the HUD,
// and the terminal display will always have a size of 640*320.

/* ---------- globals */

struct color_table *uncorrected_color_table; /* the pristine color environment of the game (can be 16bit) */
struct color_table *world_color_table; /* the gamma-corrected color environment of the game (can be 16bit) */
struct color_table *interface_color_table; /* always 8bit, for mixed-mode (i.e., valkyrie) fades */
struct color_table *visible_color_table; /* the color environment the player sees (can be 16bit) */

struct view_data *world_view; /* should be static */

// Convenient package for the drawing target (contains dimensions and pixel-row pointers)
struct bitmap_definition *world_pixels_structure;

// LP change: added stuff for keeping track of screen sizes;
// this is for forcing the clearing of the screen when resizing.
// These are initialized to improbable values.
short PrevBufferWidth = INT16_MIN, PrevBufferHeight = INT16_MIN,
	PrevOffsetWidth = INT16_MIN, PrevOffsetHeight = INT16_MIN;

static struct screen_mode_data screen_mode;

#define FRAME_SAMPLE_SIZE 20
bool displaying_fps= false;
short frame_count, frame_index;
long frame_ticks[64];

// LP addition:
// whether to show one's position
bool ShowPosition = false;

// Whether rendering of the HUD has been requested
static bool HUD_RenderRequest = false;

static bool screen_initialized= false;

short bit_depth= NONE;
short interface_bit_depth= NONE;

// LP addition: this is defined in overhead_map.c
// It indicates whether to render the overhead map in OpenGL
extern bool OGL_MapActive;


// Current screen messages:
const int NumScreenMessages = 7;
struct ScreenMessage
{
	enum {
		Len = 256
	};

	int TimeRemaining;	// How many more engine ticks until the message expires?
	char Text[Len];		// Text to display
	
	ScreenMessage(): TimeRemaining(0) {Text[0] = 0;}
};

static int MostRecentMessage = NumScreenMessages-1;
static ScreenMessage Messages[NumScreenMessages];

/* SB */
static struct ScriptHUDElement {
	/* I don't like this convention, but I'll follow it. */
	enum {
		Len = 256
	};
	/* this needs optimized (sorry, making fun of my grandmother...) */
	/* it's char[4] instead of int32 to make the OpenGL support simpler to implement */
	unsigned char icon[1024];
	bool isicon;
	int color;
	char text[Len];
} ScriptHUDElements[MAXIMUM_NUMBER_OF_SCRIPT_HUD_ELEMENTS];
/* /SB */

/* ---------- private prototypes */

static void set_overhead_map_status(bool status);
static void set_terminal_status(bool status);

/* ---------- code */

/* SB */
namespace icon {
	
  static inline char nextc(const char*& p, size_t& rem) {
    if(rem == 0) throw "end of string";
    --rem;
    return *(p++);
  }
	
  // we can't use ctype for this because of locales and hexadecimal
  static inline bool isadigit(char p) {
    if(p >= '0' && p <= '9') return true;
    else if(p >= 'A' && p <= 'F') return true;
    else if(p >= 'a' && p <= 'f') return true;
    else return false;
  }
	
  static inline unsigned char digit(char p) {
    if(p >= '0' && p <= '9') return p - '0';
    else if(p >= 'A' && p <= 'F') return p - 'A' + 0xA;
    else if(p >= 'a' && p <= 'f') return p - 'a' + 0xA;
    else throw "invalid digit";
  }
	
  static inline unsigned char readuc(const char*& p, size_t& rem) {
    char a = nextc(p, rem), b;
    b = nextc(p, rem);
    return (digit(a) << 4) | digit(b);
  }
	
  static bool parseicon(const char* p, size_t rem,
			unsigned char palette[1024], int& numcolors,
			unsigned char graphic[256]) {
    char chars[256];
    try {
      char oc, c;
      size_t n, m;
      numcolors = 0;
      while(1) {
	c = nextc(p, rem);
	if(c >= '0' && c <= '9')
	  numcolors = numcolors * 10 + (c - '0');
	else break;
      }
      if(numcolors == 0) return 1;
      oc = c;
      do {
	c = nextc(p, rem);
      } while(c == oc);
      n = 0;
      while(n < numcolors) {
	chars[n] = c;
	palette[n * 4] = readuc(p, rem);
	palette[n * 4 + 1] = readuc(p, rem);
	palette[n * 4 + 2] = readuc(p, rem);
	c = nextc(p, rem); /* ignore a char, UNLESS... */
	if(isadigit(c)) {  /* ...it's a digit */
	  --p; ++rem; /* let readuc see it */
	  palette[n * 4 + 3] = readuc(p, rem);
	  nextc(p, rem); /* remember to ignore another char */
	}
	else
	  palette[n * 4 + 3] = 255;
	++n;
	c = nextc(p, rem);
      }
      n = 0;
      while(n < 256) {
	for(m = 0; m < numcolors; ++m) {
	  if(chars[m] == c) {
	    graphic[n++] = m;
	    break;
	  }
	}
	c = nextc(p, rem);
      }
    } catch(...) {
      return false;
    }
    return true;
  }
	
  void seticon(int idx, unsigned char palette[1024], unsigned char graphic[256]) {
    unsigned char* p1, *p2, px;
    int n;
    p1 = ScriptHUDElements[idx].icon;
    p2 = graphic;
    for(n = 0; n < 256; ++n) {
      px = *(p2++);
      *(p1++) = palette[px * 4 + 3];
      *(p1++) = palette[px * 4];
      *(p1++) = palette[px * 4 + 1];
      *(p1++) = palette[px * 4 + 2];
    }
    ScriptHUDElements[idx].isicon = true;
  }
	
}

void SetScriptHUDColor(int idx, int color) {
  idx %= MAXIMUM_NUMBER_OF_SCRIPT_HUD_ELEMENTS; /* o_o */
  ScriptHUDElements[idx].color = color % 8; /* O_O */
}

void SetScriptHUDText(int idx, const char* text) {
  idx %= MAXIMUM_NUMBER_OF_SCRIPT_HUD_ELEMENTS;
  if(!text) text = "";
  strncpy(ScriptHUDElements[idx].text, text, 255);
  ScriptHUDElements[idx].text[255] = 0;
}

bool SetScriptHUDIcon(int idx, const char* text, size_t rem) {
  unsigned char palette[1024], graphic[256];
  int numcolors;
  idx %= MAXIMUM_NUMBER_OF_SCRIPT_HUD_ELEMENTS;
  if(text) {
    if(!icon::parseicon(text, rem, palette, numcolors, graphic)) return false;
    icon::seticon(idx, palette, graphic);
  } else ScriptHUDElements[idx].isicon = false;
  return true;
}

void SetScriptHUDSquare(int idx, int _color) {
  unsigned char palette[4]; /* short, I KNOW. */
  unsigned char graphic[256];
  idx %= MAXIMUM_NUMBER_OF_SCRIPT_HUD_ELEMENTS;
  ScriptHUDElements[idx].color = _color % 8;
  memset(graphic, 0, 256);
  SDL_Color color;
  _get_interface_color(_color+_computer_interface_text_color, &color);
  palette[0] = color.r;
  palette[1] = color.g;
  palette[2] = color.b;
  palette[3] = 0xff;
  icon::seticon(idx, palette, graphic);
}
/* /SB */

void reset_messages()
{
	// ZZZ: reset screen_printf's
	for(int i = 0; i < NumScreenMessages; i++)
		Messages[i].TimeRemaining = 0;
	/* SB: reset HUD elements */
	for(int i = 0; i < MAXIMUM_NUMBER_OF_SCRIPT_HUD_ELEMENTS; i++) {
		ScriptHUDElements[i].color = 1;
		ScriptHUDElements[i].text[0] = 0;
		ScriptHUDElements[i].isicon = false;
	}
}

// LP addition: this resets the screen; useful when starting a game
void reset_screen()
{
	// Resetting cribbed from initialize_screen()
	world_view->overhead_map_scale= DEFAULT_OVERHEAD_MAP_SCALE;
	world_view->overhead_map_active= false;
	world_view->terminal_mode_active= false;
	world_view->horizontal_scale= 1, world_view->vertical_scale= 1;
	
	// LP change:
	ResetFieldOfView();

	reset_messages();
}

// LP change: resets field of view to whatever the player had had when reviving
void ResetFieldOfView()
{
	world_view->tunnel_vision_active = false;

	if (current_player->extravision_duration)
	{
		world_view->field_of_view = EXTRAVISION_FIELD_OF_VIEW;
		world_view->target_field_of_view = EXTRAVISION_FIELD_OF_VIEW;
	}
	else
	{
		world_view->field_of_view = NORMAL_FIELD_OF_VIEW;
		world_view->target_field_of_view = NORMAL_FIELD_OF_VIEW;
	}
}


bool zoom_overhead_map_out(
	void)
{
	bool Success = false;
	if (world_view->overhead_map_scale > OVERHEAD_MAP_MINIMUM_SCALE)
	{
		world_view->overhead_map_scale--;
		Success = true;
	}
	
	return Success;
}

bool zoom_overhead_map_in(
	void)
{
	bool Success = false;
	if (world_view->overhead_map_scale < OVERHEAD_MAP_MAXIMUM_SCALE)
	{
		world_view->overhead_map_scale++;
		Success = true;
	}
	
	return Success;
}

void start_teleporting_effect(
	bool out)
{
	if (View_DoFoldEffect())
		start_render_effect(world_view, out ? _render_effect_fold_out : _render_effect_fold_in);
}

void start_extravision_effect(
	bool out)
{
	// LP change: doing this by setting targets
	world_view->target_field_of_view = out ? EXTRAVISION_FIELD_OF_VIEW : NORMAL_FIELD_OF_VIEW;
}

// LP addition:
void start_tunnel_vision_effect(bool out);

//CP addition: returns the screen info
screen_mode_data *get_screen_mode(
	void)
{
	return &screen_mode;
}

// LP: gets a size ID's related size ID's that show or hide the HUD, respectively
short GetSizeWithHUD(short Size)
{
	assert(Size >= 0 && Size < NUMBER_OF_VIEW_SIZES);
	return ViewSizes[Size].WithHUD;
}

short GetSizeWithoutHUD(short Size)
{
	assert(Size >= 0 && Size < NUMBER_OF_VIEW_SIZES);
	return ViewSizes[Size].WithoutHUD;
}

/* These should be replaced with better preferences control functions */
// LP change: generalizing this
bool game_window_is_full_screen(
	void)
{
	short msize = screen_mode.size;
	assert(msize >= 0 && msize < NUMBER_OF_VIEW_SIZES);
	return (!TEST_FLAG(ViewSizes[msize].flags, _view_show_HUD));
}


void change_gamma_level(
	short gamma_level)
{
	screen_mode.gamma_level= gamma_level;
	gamma_correct_color_table(uncorrected_color_table, world_color_table, gamma_level);
	stop_fade();
	obj_copy(*visible_color_table, *world_color_table);
	assert_world_color_table(interface_color_table, world_color_table);
	change_screen_mode(&screen_mode, false);
	set_fade_effect(NONE);
}

/* ---------- private code */

// LP addition: routine for displaying text

// Globals for communicating with the SDL contents of DisplayText
static SDL_Surface *DisplayTextDest = NULL;
static font_info *DisplayTextFont = NULL;
static short DisplayTextStyle = 0;

/*static*/ void DisplayText(short BaseX, short BaseY, const char *Text, unsigned char r = 0xff, unsigned char g = 0xff, unsigned char b = 0xff)
{
#ifdef HAVE_OPENGL
	// OpenGL version:
	// activate only in the main view, and also if OpenGL is being used for the overhead map
	if((OGL_MapActive || !world_view->overhead_map_active) && !world_view->terminal_mode_active)
		if (OGL_RenderText(BaseX, BaseY, Text, r, g, b)) return;
#endif

	draw_text(DisplayTextDest, Text, BaseX+1, BaseY+1, SDL_MapRGB(world_pixels->format, 0x00, 0x00, 0x00), DisplayTextFont, DisplayTextStyle);
	draw_text(DisplayTextDest, Text, BaseX, BaseY, SDL_MapRGB(world_pixels->format, r, g, b), DisplayTextFont, DisplayTextStyle);	

}

uint16 DisplayTextWidth(const char *Text)
{
	return text_width(Text, DisplayTextFont, DisplayTextStyle);
}

static void update_fps_display(SDL_Surface *s)
{
	if (displaying_fps && !player_in_terminal_mode(current_player_index))
	{
		uint32 ticks = SDL_GetTicks();
		char fps[sizeof("120.00fps (10000 ms)")];
		char ms[sizeof("(10000 ms)")];
		
		frame_ticks[frame_index]= ticks;
		frame_index= (frame_index+1)%FRAME_SAMPLE_SIZE;
		if (frame_count<FRAME_SAMPLE_SIZE)
		{
			frame_count+= 1;
			strcpy(fps, "--");
		}
		else
		{
			float count = (FRAME_SAMPLE_SIZE * MACHINE_TICKS_PER_SECOND) / float(ticks-frame_ticks[frame_index]);
			int latency = NetGetLatency();
			if (latency > -1)
				sprintf(ms, "(%i ms)", latency);
			else
				ms[0] = '\0';
							
			if (count >= TICKS_PER_SECOND)
				sprintf(fps, "%lu%s %s",(unsigned long)TICKS_PER_SECOND,".00fps", ms);
			else
				sprintf(fps, "%3.2ffps %s", count, ms);
		}
		
		FontSpecifier& Font = GetOnScreenFont();
		
		DisplayTextDest = s;
		DisplayTextFont = Font.Info;
		DisplayTextStyle = Font.Style;
		short X0 = 0;
		short Y0 = s->h;

		// The line spacing is a generalization of "5" for larger fonts
		short Offset = Font.LineSpacing / 3;
		short X = X0 + Offset;
		short Y = Y0 - Offset;
		if (Console::instance()->input_active())
		{
			Y -= Font.LineSpacing;
		}
		DisplayText(X,Y,fps);
		
	}
	else
	{
		frame_count= frame_index= 0;
	}
}


static void DisplayPosition(SDL_Surface *s)
{
	if (!ShowPosition) return;
		
	FontSpecifier& Font = GetOnScreenFont();
	
	DisplayTextDest = s;
	DisplayTextFont = Font.Info;
	DisplayTextStyle = Font.Style;
	short X0 = 0;
	short Y0 = 0;
	
	short LineSpacing = Font.LineSpacing;
	short X = X0 + LineSpacing/3;
	short Y = Y0 + LineSpacing;
	const float FLOAT_WORLD_ONE = float(WORLD_ONE);
	const float AngleConvert = 360/float(FULL_CIRCLE);
	sprintf(temporary, "X       = %8.3f",world_view->origin.x/FLOAT_WORLD_ONE);
	DisplayText(X,Y,temporary);
	Y += LineSpacing;
	sprintf(temporary, "Y       = %8.3f",world_view->origin.y/FLOAT_WORLD_ONE);
	DisplayText(X,Y,temporary);
	Y += LineSpacing;
	sprintf(temporary, "Z       = %8.3f",world_view->origin.z/FLOAT_WORLD_ONE);
	DisplayText(X,Y,temporary);
	Y += LineSpacing;
	sprintf(temporary, "Polygon = %8d",world_view->origin_polygon_index);
	DisplayText(X,Y,temporary);
	Y += LineSpacing;
	short Angle = world_view->yaw;
	if (Angle > HALF_CIRCLE) Angle -= FULL_CIRCLE;
	sprintf(temporary, "Yaw     = %8.3f",AngleConvert*Angle);
	DisplayText(X,Y,temporary);
	Y += LineSpacing;
	Angle = world_view->pitch;
	if (Angle > HALF_CIRCLE) Angle -= FULL_CIRCLE;
	sprintf(temporary, "Pitch   = %8.3f",AngleConvert*Angle);
	DisplayText(X,Y,temporary);
	
}

static void DisplayInputLine(SDL_Surface *s)
{
  if (Console::instance()->input_active() && 
      !Console::instance()->displayBuffer().empty()) {
    FontSpecifier& Font = GetOnScreenFont();
    
  DisplayTextDest = s;
  DisplayTextFont = Font.Info;
  DisplayTextStyle = Font.Style;
  short X0 = 0;
  short Y0 = s->h;

  short Offset = Font.LineSpacing / 3;
  short X = X0 + Offset;
  short Y = Y0 - Offset;
  DisplayText(X, Y, Console::instance()->displayBuffer().c_str());
  }
}

static void DisplayMessages(SDL_Surface *s)
{	
	FontSpecifier& Font = GetOnScreenFont();
	
	DisplayTextDest = s;
	DisplayTextFont = Font.Info;
	DisplayTextStyle = Font.Style;
	short X0 = 0;
	short Y0 = 0;
	
	short LineSpacing = Font.LineSpacing;
	short X = X0 + LineSpacing/3;
	short Y = Y0 + LineSpacing;
	if (ShowPosition) Y += 6*LineSpacing;	// Make room for the position data
	/* SB */
	for(int i = 0; i < MAXIMUM_NUMBER_OF_SCRIPT_HUD_ELEMENTS; ++i) {
		if(ScriptHUDElements[i].text[0]) {
			short x2 = X, sk, ysk = 0;
			/* Yes, I KNOW this is the same i as above. I know what I'm doing. */
			for(i = 0; i < MAXIMUM_NUMBER_OF_SCRIPT_HUD_ELEMENTS; ++i) {
				if(!ScriptHUDElements[i].text[0]) continue;
				sk = SCRIPT_HUD_ELEMENT_SPACING;
				if(ScriptHUDElements[i].isicon) {
					ysk = 2;
#ifdef HAVE_OPENGL
					if(OGL_IsActive() && ((OGL_MapActive || !world_view->overhead_map_active) && !world_view->terminal_mode_active)) {
						GLuint tex;
						glPushAttrib(GL_ALL_ATTRIB_BITS);
						glEnable(GL_TEXTURE_2D);
						glDisable(GL_CULL_FACE);
						glEnable(GL_BLEND);
						glDisable(GL_ALPHA_TEST);
						glMatrixMode(GL_MODELVIEW);
						glPushMatrix();
						glLoadIdentity();
						glGenTextures(1, &tex);
						glTranslatef(x2-0.5f,Y+4.5f,0);
						glRasterPos2d(0, 0);
						glBindTexture(GL_TEXTURE_2D, tex);
#ifdef ALEPHONE_LITTLE_ENDIAN
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, ScriptHUDElements[i].icon);
#else
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, ScriptHUDElements[i].icon);
#endif
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
						glColor3f(1,1,1);
						glBegin(GL_TRIANGLE_FAN);
						glTexCoord2f(0.0f, 0.0f);
						glVertex2f(0.5f, -15.5f);
						glTexCoord2f(1.0f, 0.0f);
						glVertex2f(16.5f, -15.5f);
						glTexCoord2f(1.0f, 1.0f);
						glVertex2f(16.5f, 0.5f);
						glTexCoord2f(0.0f, 1.0f);
						glVertex2f(0.5f, 0.5f);
						glEnd();
						/*glDrawPixels(16, 16, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, ScriptHUDElements[i].icon);*/
						glDeleteTextures(1, &tex);
						glPopMatrix();
						glPopAttrib();				
					}
					else 
#endif 
					{
						SDL_Surface* srf;
						SDL_Rect rect;
						rect.x = x2;
						rect.y = Y - 11;
						rect.w = rect.h = 16;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
						srf = SDL_CreateRGBSurfaceFrom(ScriptHUDElements[i].icon, 16, 16, 32, 64, 0xFF<<8, 0xFF<<16, 0xFF<<24, 0xFF);
#else
						srf = SDL_CreateRGBSurfaceFrom(ScriptHUDElements[i].icon, 16, 16, 32, 64, 0xFF<<16, 0xFF<<8, 0xFF, 0xFF<<24);
#endif
						SDL_BlitSurface(srf, NULL, s, &rect);
						SDL_FreeSurface(srf);
					}
					x2 += 20;
					sk -= 20;
				}
				SDL_Color color;
				_get_interface_color(ScriptHUDElements[i].color+_computer_interface_text_color, &color);
				DisplayText(x2,Y + (ScriptHUDElements[i].isicon ? 2 : 0),ScriptHUDElements[i].text, color.r, color.g, color.b);
				x2 += sk;
			}
			Y += LineSpacing + ysk;
			break;
		}
	}
	/* /SB */
	//	for (int k=0; k<NumScreenMessages; k++)
	for (int k = NumScreenMessages - 1; k >= 0; k--)
	{
	  int Which = (MostRecentMessage+NumScreenMessages-k) % NumScreenMessages;
		while (Which < 0)
			Which += NumScreenMessages;
		ScreenMessage& Message = Messages[Which];
		if (Message.TimeRemaining <= 0) continue;
		Message.TimeRemaining--;
		
		DisplayText(X,Y,Message.Text);
		Y += LineSpacing;
	}

}

extern short local_player_index;
extern bool game_is_networked;

static void DisplayNetMicStatus(SDL_Surface *s)
{
	if (!game_is_networked) return;

	// the net mic status is a message, and a colored text "icon"
	string icon;
	string status;
	SDL_Color iconColor;

	if (!current_netgame_allows_microphone())
	{
		if (dynamic_world->speaking_player_index == local_player_index)
		{
			status = "disabled";
			icon = "  x";
			iconColor.r = 0xff;
			iconColor.g = iconColor.b = 0x0;

		}
	}
	else if (dynamic_world->speaking_player_index == local_player_index)
	{
		if (GET_GAME_OPTIONS() & _force_unique_teams)
			status = "all";
		else
			status = "team";
		icon = "<!>";

		player_data *player = get_player_data(dynamic_world->speaking_player_index);
		if (GET_GAME_OPTIONS() & _force_unique_teams)
			_get_interface_color(PLAYER_COLOR_BASE_INDEX + player->color, &iconColor);
		else
			_get_interface_color(PLAYER_COLOR_BASE_INDEX + player->team, &iconColor);
	} 
	else if (dynamic_world->speaking_player_index != NONE)
	{
		// find the name and color of the person who is speaking
		player_data *player = get_player_data(dynamic_world->speaking_player_index);
		status = player->name;
		_get_interface_color(PLAYER_COLOR_BASE_INDEX + player->color, &iconColor);
		icon = ">!<";
	}

	FontSpecifier& Font = GetOnScreenFont();
	short Y = s->h - Font.LineSpacing / 3;
	if (Console::instance()->input_active())
	{
		Y -= Font.LineSpacing;
	}
	short Xicon = s->w - DisplayTextWidth(icon.c_str()) - Font.LineSpacing / 3;
	short Xstatus = Xicon - DisplayTextWidth(" ") - DisplayTextWidth(status.c_str());

	DisplayText(Xicon, Y, icon.c_str(), iconColor.r, iconColor.g, iconColor.b);
	DisplayText(Xstatus, Y, status.c_str());
}

static void set_overhead_map_status( /* it has changed, this is the new status */
	bool status)
{
	world_view->overhead_map_active= status;
}

static void set_terminal_status( /* It has changed, this is the new state.. */
	bool status)
{
	bool restore_effect= false;
	short effect = 0, phase = 0;
	
	if(!status)
	{
		if(world_view->effect==_render_effect_fold_out)
		{
			effect= world_view->effect;
			phase= world_view->effect_phase;
			restore_effect= true;
		}
	}
	world_view->terminal_mode_active= status;
	
	if(restore_effect)
	{
		world_view->effect= effect;
		world_view->effect_phase= phase;
	}

	/* Dirty the view.. */
	dirty_terminal_view(current_player_index);
}

// For getting and setting tunnel-vision mode
bool GetTunnelVision() {return world_view->tunnel_vision_active;}
bool SetTunnelVision(bool TunnelVisionOn)
{
	// LP: simplifying tunnel-vision-activation/deactivation behavior
	world_view->tunnel_vision_active = TunnelVisionOn;
	start_tunnel_vision_effect(TunnelVisionOn);
	return world_view->tunnel_vision_active;
}

// This is for requesting the drawing of the Heads-Up Display;
// this is done because its drawing is now done when the main display is drawn
void RequestDrawingHUD()
{
	HUD_RenderRequest = true;
}

// ZZZ: I feel bad doing this, but ... not sure what best way to handle it is.
// #ifdef __MWERKS__
// using std::vsnprintf;
// #endif

// LP addition: display message on the screen;
// this really puts the current message into a buffer
// Code cribbed from csstrings
void screen_printf(const char *format, ...)
{
	MostRecentMessage = (MostRecentMessage + 1) % NumScreenMessages;
	while (MostRecentMessage < 0)
		MostRecentMessage += NumScreenMessages;
	ScreenMessage& Message = Messages[MostRecentMessage];
	
	Message.TimeRemaining = 7*TICKS_PER_SECOND;

	va_list list;

	va_start(list,format);
	// ZZZ: [v]sprintf is evil, generally: hard to guarantee you don't overflow target buffer
	// using [v]snprintf instead
	vsnprintf(Message.Text,sizeof(Message.Text),format,list);
	va_end(list);
}
