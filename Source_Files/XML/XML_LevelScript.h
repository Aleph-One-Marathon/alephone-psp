#ifndef _XML_LEVEL_SCRIPT_
#define _XML_LEVEL_SCRIPT_
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

	Support for XML scripts in map files
	by Loren Petrich,
	April 16, 2000

	The reason for a separate object is that it will be necessary to execute certain commands
	only on certain levels.

Nov 25, 2000 (Loren Petrich)
	Added support for specifying movies for levels, as Jesse Simko had requested.
*/


#include "FileHandler.h"

// Loads all those in resource 128 in a map file (or some appropriate equivalent)
void LoadLevelScripts(FileSpecifier& MapFile);

// Runs a script for some level; loads Pfhortran,
// runs level-specific MML...
void RunLevelScript(int LevelIndex);

// Intended to be run at the end of a game
void RunEndScript();

// Intended for restoring old parameter values, because MML sets values at a variety
// of different places, and it may be easier to simply set stuff back to defaults
// by including those defaults in the script.
void RunRestorationScript();

// Indicates whether a level is active; this is for telling music.cpp not to loop the music
bool IsLevelMusicActive();

// When leaving the game for the main menu; indicates that there will be no script
// to use for music files
void StopLevelMusic();

// Gets the next song file to play music from, as a pointer to the file specifier.
// A NULL pointer means no music to play
FileSpecifier *GetLevelMusic();

// Finds the level movie and the end movie, to be used in show_movie()
// The first is for some level,
// while the second is for the end of a game
void FindLevelMovie(short index);
void FindEndMovie();

// Gets the pointer of a movie to play at a level, as a pointer to the file specifier.
// A NULL pointer means no movie to play.
// Its arg is the playback size, which will not be changed if not specified explicitly.
FileSpecifier *GetLevelMovie(float& PlaybackSize);

// For selecting the end-of-game screens --
// what fake level index for them, and how many to display
// (resource numbers increasing in sequence) 
extern short EndScreenIndex;
extern short NumEndScreens;

#endif
