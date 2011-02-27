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
	
	Overhead-Map OpenGL Class Implementation
	by Loren Petrich,
	August 3, 2000
	
	Subclass of OverheadMapClass for doing rendering with OpenGL
	Code originally from OGL_Map.c; font handling is still MacOS-specific.

[Notes from OGL_Map.c]
	This is for drawing the Marathon overhead map with OpenGL, because at large resolutions,
	the main CPU can be too slow for this.
	
	Much of this is cribbed from overhead_map_macintosh.c and translated into OpenGL form
	
July 9, 2000:

	Complete this OpenGL renderer. I had to add a font-info cache, so as to avoid
	re-generating the fonts for every frame. The font glyphs and offsets are stored
	as display lists, which appears to be very efficient.

July 16, 2000:

	Added begin/end pairs for line and polygon rendering; the purpose of these is to allow
	more efficient caching.

Jul 17, 2000:

	Paths now cached and drawn as a single line strip per path.
	Lines now cached and drawn in groups with the same width and color;
		that has yielded a significant performance improvement.
	Same for the polygons, but with relatively little improvement.
[End notes]

Aug 6, 2000 (Loren Petrich):
	Added perimeter drawing to drawing commands for the player object;
	this guarantees that this object will always be drawn reasonably correctly
	
Oct 13, 2000 (Loren Petrich)
	Converted the various lists into Standard Template Library vectors

Jan 25, 2002 (Br'fin (Jeremy Parsons)):
	Added TARGET_API_MAC_CARBON for AGL.h
*/

#include <math.h>
#include <string.h>

#include "cseries.h"
#include "OverheadMap_OGL.h"
#include "map.h"

#ifdef HAVE_OPENGL

#ifdef HAVE_OPENGL
# if defined (__APPLE__) && defined (__MACH__)
#   include <OpenGL/gl.h>
#   include <OpenGL/glu.h>
# elif defined mac
#   include <gl.h>
#   include <glu.h>
# else
#   include <GL/gl.h>
#   include <GL/glu.h>
# endif
#endif

#ifdef mac
# if defined(EXPLICIT_CARBON_HEADER)
#  include <AGL/agl.h>
# else
#  include <agl.h>
# endif
#endif

#define USE_VERTEX_ARRAYS


// rgb_color straight to OpenGL
static inline void SetColor(rgb_color& Color) {glColor3usv((unsigned short *)(&Color));}

// Need to test this so as to find out when the color changes
static inline bool ColorsEqual(rgb_color& Color1, rgb_color& Color2)
{
	return
		((Color1.red == Color2.red) &&
			(Color1.green == Color2.green) &&
				(Color1.blue == Color2.blue));
}

#ifdef mac
// Render context for aglUseFont(); defined in OGL_Render.c
extern AGLContext RenderContext;
#endif


// For marking out the area to be blanked out when starting rendering;
// these are defined in OGL_Render.cpp
extern short ViewWidth, ViewHeight;

void OverheadMap_OGL_Class::begin_overall()
{

	// Blank out the screen
	// Do that by painting a black polygon
	
	glColor3f(0,0,0);
	glBegin(GL_POLYGON);
	glVertex2f(0,0);
	glVertex2f(0,ViewHeight);
	glVertex2f(ViewWidth,ViewHeight);
	glVertex2f(ViewWidth,0);
	glEnd();
	
/*
#ifndef mac
	glEnable(GL_SCISSOR_TEST);	// Don't erase the HUD
#endif
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
#ifndef mac
	glDisable(GL_SCISSOR_TEST);
#endif
*/
	
	// Here's for the overhead map
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	glLineWidth(1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void OverheadMap_OGL_Class::end_overall()
{
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	// Reset line width
	glLineWidth(1);
}


void OverheadMap_OGL_Class::begin_polygons()
{
	// Polygons are rendered before lines, and use the endpoint array,
	// so both of them will have it set here. Using the compiled-vertex extension,
	// however, makes everything the same color :-P
#if defined(USE_VERTEX_ARRAYS)
	// CB: Vertex arrays crash both Mesa and NVidia's GL implementation
	// (reason still to be determined)

	// ghs: this is because the texture coord array was enabled, I think
	//      if there's still trouble, #undef USE_VERTEX_ARRAYS
	glVertexPointer(2,GL_SHORT,GetVertexStride(),GetFirstVertex());
#endif

	// Reset color defaults
	SavedColor.red = SavedColor.green = SavedColor.blue = 0;
	SetColor(SavedColor);
	
	// Reset cache to zero length
	PolygonCache.clear();
}

void OverheadMap_OGL_Class::draw_polygon(
	short vertex_count,
	short *vertices,
	rgb_color& color)
{
	// Test whether the polygon parameters have changed
	bool AreColorsEqual = ColorsEqual(color,SavedColor);
	
	// If any change, then draw the cached lines with the *old* parameters,
	// Set the new parameters
	if (!AreColorsEqual)
	{
		DrawCachedPolygons();
		SavedColor = color;
		SetColor(SavedColor);
	}
	
	// Implement the polygons as triangle fans
#if defined(USE_VERTEX_ARRAYS)
	for (int k=2; k<vertex_count; k++)
	{
		PolygonCache.push_back(vertices[0]);
		PolygonCache.push_back(vertices[k-1]);
		PolygonCache.push_back(vertices[k]);
	}
#else
	glBegin(GL_POLYGON);
	for (int k=0; k<vertex_count; k++)
	{
		world_point2d v = GetVertex(vertices[k]);
		glVertex2i(v.x, v.y);
	}
	glEnd();
#endif
	
	// glDrawElements(GL_POLYGON,vertex_count,GL_UNSIGNED_SHORT,vertices);
}

void OverheadMap_OGL_Class::end_polygons()
{
	DrawCachedPolygons();
}

void OverheadMap_OGL_Class::DrawCachedPolygons()
{
#if defined(USE_VERTEX_ARRAYS)
	glDrawElements(GL_TRIANGLES, PolygonCache.size(),
		GL_UNSIGNED_SHORT, &PolygonCache.front());
#endif
	PolygonCache.clear();
}

void OverheadMap_OGL_Class::begin_lines()
{
	// Vertices already set
	
	// Reset color and pen size to defaults
	SetColor(SavedColor);
	SavedPenSize = 1;
	glLineWidth(SavedPenSize);
	
	// Reset cache to zero length
	LineCache.clear();
}


void OverheadMap_OGL_Class::draw_line(
	short *vertices,
	rgb_color& color,
	short pen_size)
{
	// Test whether the line parameters have changed
	bool AreColorsEqual = ColorsEqual(color,SavedColor);
	bool AreLinesEquallyWide = (pen_size == SavedPenSize);
	
	// If any change, then draw the cached lines with the *old* parameters
	if (!AreColorsEqual || !AreLinesEquallyWide) DrawCachedLines();
	
	// Set the new parameters
	if (!AreColorsEqual)
	{
		SavedColor = color;
		SetColor(SavedColor);
	}
	
	if (!AreLinesEquallyWide)
	{
		SavedPenSize = pen_size;
		glLineWidth(SavedPenSize);		
	}
	
#if defined(USE_VERTEX_ARRAYS)
	// Add the line's points to the cached line		
	LineCache.push_back(vertices[0]);
	LineCache.push_back(vertices[1]);
#else
	glBegin(GL_LINES);
	world_point2d v = GetVertex(vertices[0]);
	glVertex2i(v.x, v.y);
	v = GetVertex(vertices[1]);
	glVertex2i(v.x, v.y);
	glEnd();
#endif
}

void OverheadMap_OGL_Class::end_lines()
{
	DrawCachedLines();
}

void OverheadMap_OGL_Class::DrawCachedLines()
{
#if defined(USE_VERTEX_ARRAYS)
	glDrawElements(GL_LINES,LineCache.size(),
		GL_UNSIGNED_SHORT,&LineCache.front());
#endif
	LineCache.clear();
}


void OverheadMap_OGL_Class::draw_thing(
	world_point2d& center,
	rgb_color& color,
	short shape,
	short radius)
{
	SetColor(color);
	
	// The rectangle is a square
	const int NumRectangleVertices = 4;
	const GLfloat RectangleShape[NumRectangleVertices][2] =
	{
		{-0.75,-0.75},
		{-0.75,0.75},
		{0.75,0.75},
		{0.75,-0.75}
	};
	// The circle is here an octagon for convenience
	const int NumCircleVertices = 8;
	const GLfloat CircleShape[NumCircleVertices][2] =
	{
		{-0.75F,-0.3F},
		{-0.75F,0.3F},
		{-0.3F,0.75F},
		{0.3F,0.75F},
		{0.75F,0.3F},
		{0.75F,-0.3F},
		{0.3F,-0.75F},
		{-0.3F,-0.75F}
	};
	
	// Let OpenGL do the transformation work
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(center.x,center.y,0);
	glScalef(radius,radius,1);

	switch(shape)
	{
	case _rectangle_thing:
		glVertexPointer(2,GL_FLOAT,0,RectangleShape[0]);
		glDrawArrays(GL_POLYGON,0,NumRectangleVertices);
		break;
	case _circle_thing:
		glLineWidth(2);
		glVertexPointer(2,GL_FLOAT,0,CircleShape[0]);
		glDrawArrays(GL_LINE_LOOP,0,NumCircleVertices);
		break;
	default:
		break;
	}
	glPopMatrix();
}

void OverheadMap_OGL_Class::draw_player(
	world_point2d& center,
	angle facing,
	rgb_color& color,
	short shrink,
	short front,
	short rear,
	short rear_theta)
{
	SetColor(color);
	
	// The player is a simple triangle
	GLfloat PlayerShape[3][2];
	
	double rear_theta_rads = rear_theta*(8*atan(1.0)/FULL_CIRCLE);
	float rear_x = (float)(rear*cos(rear_theta_rads));
	float rear_y = (float)(rear*sin(rear_theta_rads));
	PlayerShape[0][0] = front;
	PlayerShape[0][1] = 0;
	PlayerShape[1][0] = rear_x;
	PlayerShape[1][1] = - rear_y;
	PlayerShape[2][0] = rear_x;
	PlayerShape[2][1] = rear_y;
	
	// Let OpenGL do the transformation work
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(center.x,center.y,0);
	glRotatef(facing*(360.0F/FULL_CIRCLE),0,0,1);
	float scale = 1/float(1 << shrink);
	glScalef(scale,scale,1);
	glVertexPointer(2,GL_FLOAT,0,PlayerShape[0]);
	glDrawArrays(GL_POLYGON,0,3);
	glLineWidth(1);					// LP: need only 1-pixel thickness of line
	glDrawArrays(GL_LINE_LOOP,0,3);	// LP addition: perimeter drawing makes small version easier to see
	glPopMatrix();
}

	
// Text justification: 0=left, 1=center
void OverheadMap_OGL_Class::draw_text(
	world_point2d& location,
	rgb_color& color,
	char *text,
	FontSpecifier& FontData,
	short justify)
{	
	// Find the left-side location
	world_point2d left_location = location;
	switch(justify)
	{
	case _justify_left:
		break;
		
	case _justify_center:
		left_location.x -= (FontData.TextWidth(text)>>1);
		break;
		
	default:
		return;
	}
	
	// Set color and location	
	SetColor(color);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(left_location.x,left_location.y,0);	
	FontData.OGL_Render(text);
	glPopMatrix();
}
	
void OverheadMap_OGL_Class::set_path_drawing(rgb_color& color)
{
	SetColor(color);
	glLineWidth(1);
}

void OverheadMap_OGL_Class::draw_path(
	short step,	// 0: first point
	world_point2d &location)
{
	// At first step, reset the length
	if (step <= 0) PathPoints.clear();
	
	// Add the point
	PathPoints.push_back(location);
}

void OverheadMap_OGL_Class::finish_path()
{
	glVertexPointer(2,GL_SHORT,sizeof(world_point2d),&PathPoints.front());
	glDrawArrays(GL_LINE_STRIP,0,(GLsizei)(PathPoints.size()));
}
#endif // def HAVE_OPENGL
