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
	
Oct 13, 2000 (Loren Petrich)
	Converted the in-memory script data into Standard Template Library vectors

Nov 25, 2000 (Loren Petrich)
	Added support for specifying movies for levels, as Jesse Simko had requested.
	Also added end-of-game support.

Jul 31, 2002 (Loren Petrich):
	Added images.cpp/h accessor for TEXT resources,
	because it can support the M2-Win95 map format.
*/

#include <vector>
using namespace std;

#include "cseries.h"
#include "shell.h"
#include "game_wad.h"
#include "Music.h"
#include "ColorParser.h"
#include "XML_DataBlock.h"
#include "XML_LevelScript.h"
#include "XML_ParseTreeRoot.h"
#include "Random.h"
#include "images.h"
#include "lua_script.h"

#include "OGL_LoadScreen.h"


// The "command" is an instruction to process a file/resource in a certain sort of way
struct LevelScriptCommand
{
	// Types of commands
	enum {
		MML,
		Music,
		Movie,
#ifdef HAVE_LUA
		Lua,
#endif /* HAVE_LUA */
#ifdef HAVE_OPENGL
		LoadScreen
#endif
	};
	int Type;
	
	enum {
		UnsetResource = -32768
	};
	
	// Where to read the command data from:
	
	// This is a MacOS resource ID
	short RsrcID;
	
	bool RsrcPresent() {return RsrcID != UnsetResource;}
	
	// This is a Unix-style filespec, with form
	// <dirname>/<dirname>/<filename>
	// with the root directory being the map file's directory
	vector<char> FileSpec;
	
	// Additional data:
	
	// Relative size for a movie (negative means don't use)
	float Size;

	// Additional load screen information:
	short T, L, B, R;
	rgb_color Colors[2];
	bool Stretch;
	
	LevelScriptCommand(): RsrcID(UnsetResource), Size(NONE), L(0), T(0), R(0), B(0), Stretch(true) {
		memset(&Colors[0], 0, sizeof(rgb_color));
		memset(&Colors[1], 0xff, sizeof(rgb_color));
	}
};

struct LevelScriptHeader
{
	// Special pseudo-levels: restoration of previous parameters, defaults for this map file,
	// and the end of the game
	enum {
		Restore = -3,
		Default = -2,
		End = -1
	};
	int Level;
	
	vector<LevelScriptCommand> Commands;
	
	// Thanx to the Standard Template Library,
	// the copy constructor and the assignment operator will be automatically implemented
	
	// Whether the music files are to be played in random order;
	// it defaults to false (sequential order)
	bool RandomOrder;
	
	LevelScriptHeader(): Level(Default), RandomOrder(false) {}
};

// Scripts for current map file
static vector<LevelScriptHeader> LevelScripts;

// Current script for adding commands to and for running
static LevelScriptHeader *CurrScriptPtr = NULL;

// Map-file parent directory (where all map-related files are supposed to live)
static DirectorySpecifier MapParentDir;

#ifdef HAVE_LUA
// Same for Lua
static bool LuaFound = false;
#endif /* HAVE_LUA */

// Movie filespec and whether it points to a real file
static FileSpecifier MovieFile;
static bool MovieFileExists = false;
static float MovieSize = NONE;

// For selecting the end-of-game screens --
// what fake level index for them, and how many to display
// (resource numbers increasing in sequence) 
// (defaults from interface.cpp)
short EndScreenIndex = 99;
short NumEndScreens = 1;


// The level-script parsers are separate from the main MML ones,
// because they operate on per-level data.

static XML_DataBlock LSXML_Loader;
static XML_ElementParser LSRootParser("");
static XML_ElementParser LevelScriptSetParser("marathon_levels");

// Get parsers for other stuff
static XML_ElementParser *LevelScript_GetParser();
static XML_ElementParser *EndLevelScript_GetParser();
static XML_ElementParser *DefaultLevelScript_GetParser();
static XML_ElementParser *RestoreLevelScript_GetParser();
static XML_ElementParser *EndScreens_GetParser();


// This is for searching for a script and running it -- works for pseudo-levels
static void GeneralRunScript(int LevelIndex);

// Similar generic function for movies
static void FindMovieInScript(int LevelIndex);

// Defined in images.cpp and 
extern bool get_text_resource_from_scenario(int resource_number, LoadedResource& TextRsrc);

static void SetupLSParseTree()
{
	// Don't set up more than once!
	static bool WasSetUp = false;
	if (WasSetUp)
		return;
	else
		WasSetUp = true;
	
	LSRootParser.AddChild(&LevelScriptSetParser);
	
	LevelScriptSetParser.AddChild(LevelScript_GetParser());
	LevelScriptSetParser.AddChild(EndLevelScript_GetParser());
	LevelScriptSetParser.AddChild(DefaultLevelScript_GetParser());
	LevelScriptSetParser.AddChild(RestoreLevelScript_GetParser());
	LevelScriptSetParser.AddChild(EndScreens_GetParser());
}


// Loads all those in resource 128 in a map file (or some appropriate equivalent)
void LoadLevelScripts(FileSpecifier& MapFile)
{
	// Because all the files are to live in the map file's parent directory
	MapFile.ToDirectory(MapParentDir);
	
	// Get rid of the previous level script
	// ghs: unless it's the first time, in which case we would be clearing
	// any external level scripts, so don't
	static bool FirstTime = true;
	if (FirstTime)
		FirstTime = false;
	else
		LevelScripts.clear();
	
	// Set these to their defaults before trying to change them
	EndScreenIndex = 99;
	NumEndScreens = 1;
	
	// Lazy setup of XML parsing definitions
	SetupLSParseTree();
	LSXML_Loader.CurrentElement = &LSRootParser;

	// OpenedResourceFile OFile;
	// if (!MapFile.Open(OFile)) return;
	
	// The script is stored at a special resource ID;
	// simply quit if it could not be found
	LoadedResource ScriptRsrc;
	
	// if (!OFile.Get('T','E','X','T',128,ScriptRsrc)) return;
	if (!get_text_resource_from_scenario(128,ScriptRsrc)) return;
	
	// Load the script
	LSXML_Loader.SourceName = "[Map Script]";
	LSXML_Loader.ParseData((char *)ScriptRsrc.GetPointer(),ScriptRsrc.GetLength());
}


// Runs a script for some level
// runs level-specific MML...
void RunLevelScript(int LevelIndex)
{
	static bool FirstTime = true;
	// None found just yet...
#ifdef HAVE_LUA
	LuaFound = false;
#endif /* HAVE_LUA */
	
	// For whatever previous music had been playing...
	Music::instance()->FadeOut(MACHINE_TICKS_PER_SECOND/2);
	
	// If no scripts were loaded or none of them had music specified,
	// then don't play any music
	Music::instance()->ClearLevelMusic();

#ifdef HAVE_OPENGL	
	OGL_LoadScreen::instance()->Clear();
#endif

	if (FirstTime)
		FirstTime = false; // first time, we already have base MML loaded
	else
	{
		// reset values to engine defaults first
		ResetAllMMLValues();
		// then load the base stuff (from Scripts folder and whatnot)
		LoadBaseMMLScripts();
	}

	GeneralRunScript(LevelScriptHeader::Default);
	GeneralRunScript(LevelIndex);
	
	Music::instance()->SeedLevelMusic();

}

// Intended to be run at the end of a game
void RunEndScript()
{
	GeneralRunScript(LevelScriptHeader::Default);
	GeneralRunScript(LevelScriptHeader::End);
}

// Intended for restoring old parameter values, because MML sets values at a variety
// of different places, and it may be easier to simply set stuff back to defaults
// by including those defaults in the script.
void RunRestorationScript()
{
	GeneralRunScript(LevelScriptHeader::Default);
	GeneralRunScript(LevelScriptHeader::Restore);
}

// Search for level script and then run it
void GeneralRunScript(int LevelIndex)
{
	// Find the pointer to the current script
	CurrScriptPtr = NULL;
	for (vector<LevelScriptHeader>::iterator ScriptIter = LevelScripts.begin(); ScriptIter < LevelScripts.end(); ScriptIter++)
	{
		if (ScriptIter->Level == LevelIndex)
		{
			CurrScriptPtr = &(*ScriptIter);	// Iterator to pointer
			break;
		}
	}
	if (!CurrScriptPtr) return;
	
	// Insures that this order is the last order set
	Music::instance()->LevelMusicRandom(CurrScriptPtr->RandomOrder);
	
	// OpenedResourceFile OFile;
	// FileSpecifier& MapFile = get_map_file();
	// if (!MapFile.Open(OFile)) return;
	
	for (unsigned k=0; k<CurrScriptPtr->Commands.size(); k++)
	{
		LevelScriptCommand& Cmd = CurrScriptPtr->Commands[k];
		
		// Data to parse
		char *Data = NULL;
		size_t DataLen = 0;
		
		// First, try to load a resource (only for scripts)
		LoadedResource ScriptRsrc;
		switch(Cmd.Type)
		{
		case LevelScriptCommand::MML:
#ifdef HAVE_LUA
		case LevelScriptCommand::Lua:
#endif /* HAVE_LUA */
			// if (Cmd.RsrcPresent() && OFile.Get('T','E','X','T',Cmd.RsrcID,ScriptRsrc))
			if (Cmd.RsrcPresent() && get_text_resource_from_scenario(Cmd.RsrcID,ScriptRsrc))
			{
				Data = (char *)ScriptRsrc.GetPointer();
				DataLen = ScriptRsrc.GetLength();
			}
		}
		
		switch(Cmd.Type)
		{
		case LevelScriptCommand::MML:
			{
				// Skip if not loaded
				if (Data == NULL || DataLen <= 0) break;
				
				// Set to the MML root parser
				char ObjName[256];
				sprintf(ObjName,"[Map Rsrc %hd for Level %d]",Cmd.RsrcID,LevelIndex);
				LSXML_Loader.SourceName = ObjName;
				LSXML_Loader.CurrentElement = &RootParser;
				LSXML_Loader.ParseData(Data,DataLen);
			}
			break;
		
#ifdef HAVE_LUA
                        
			case LevelScriptCommand::Lua:
			{
				// Skip if not loaded
				if (Data == NULL || DataLen <= 0) break;
				
				// Load and indicate whether loading was successful
				if (LoadLuaScript(Data, DataLen))
					LuaFound = true;
			}
			break;
#endif /* HAVE_LUA */
		
		case LevelScriptCommand::Music:
			{
				FileSpecifier MusicFile;
				if (MusicFile.SetNameWithPath(&Cmd.FileSpec[0]))
					Music::instance()->PushBackLevelMusic(MusicFile);
			}
			break;
#ifdef HAVE_OPENGL
		case LevelScriptCommand::LoadScreen:
		{
			if (Cmd.FileSpec.size() > 0)
			{
				if (Cmd.L || Cmd.T || Cmd.R || Cmd.B)
				{
					OGL_LoadScreen::instance()->Set(Cmd.FileSpec, Cmd.Stretch, Cmd.L, Cmd.T, Cmd.R - Cmd.L, Cmd.B - Cmd.T);
					OGL_LoadScreen::instance()->Colors()[0] = Cmd.Colors[0];
					OGL_LoadScreen::instance()->Colors()[1] = Cmd.Colors[1];
				}
				else 
					OGL_LoadScreen::instance()->Set(Cmd.FileSpec, Cmd.Stretch);
			}
		}
#endif
		// The movie info is handled separately
			
		}
	}
}



// Search for level script and then run it
void FindMovieInScript(int LevelIndex)
{
	// Find the pointer to the current script
	CurrScriptPtr = NULL;
	for (vector<LevelScriptHeader>::iterator ScriptIter = LevelScripts.begin(); ScriptIter < LevelScripts.end(); ScriptIter++)
	{
		if (ScriptIter->Level == LevelIndex)
		{
			CurrScriptPtr = &(*ScriptIter);	// Iterator to pointer
			break;
		}
	}
	if (!CurrScriptPtr) return;
		
	for (unsigned k=0; k<CurrScriptPtr->Commands.size(); k++)
	{
		LevelScriptCommand& Cmd = CurrScriptPtr->Commands[k];
				
		switch(Cmd.Type)
		{
		case LevelScriptCommand::Movie:
			{
				MovieFileExists = MovieFile.SetNameWithPath(&Cmd.FileSpec[0]);

				// Set the size only if there was a movie file here
				if (MovieFileExists)
					MovieSize = Cmd.Size;
			}
			break;
		}
	}
}


// Movie functions

// Finds the level movie and the end movie, to be used in show_movie()
// The first is for some level,
// while the second is for the end of a game
void FindLevelMovie(short index)
{
	MovieFileExists = false;
	MovieSize = NONE;
	FindMovieInScript(LevelScriptHeader::Default);
	FindMovieInScript(index);
}


FileSpecifier *GetLevelMovie(float& Size)
{
	if (MovieFileExists)
	{
		// Set only if the movie-size value is positive
		if (MovieSize >= 0) Size = MovieSize;
		return &MovieFile;
	}
	else
		return NULL;
}


// Generalized parser for level-script commands; they all use the same format,
// but will be distinguished by their names
class XML_LSCommandParser: public XML_ElementParser
{
	// Need to get an object ID (either resource ID or filename)
	bool ObjectWasFound;
	
	// New script command to compose	
	LevelScriptCommand Cmd;
	
public:
	bool Start();
	bool End();

	// For grabbing the level ID
	bool HandleAttribute(const char *Tag, const char *Value);

	XML_LSCommandParser(const char *_Name, int _CmdType): XML_ElementParser(_Name) {Cmd.Type = _CmdType;}
};

bool XML_LSCommandParser::Start()
{
	ObjectWasFound = false;
	Color_SetArray(Cmd.Colors, 2);
	return true;
}


bool XML_LSCommandParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"resource"))
	{
		short RsrcID;
		if (ReadBoundedInt16Value(Value,RsrcID,0,SHRT_MAX))
		{
			Cmd.RsrcID = RsrcID;
			ObjectWasFound = true;
			return true;
		}
		else
			return false;
	}
	else if (StringsEqual(Tag,"file"))
	{
		size_t vlen = strlen(Value) + 1;
		Cmd.FileSpec.resize(vlen);
		memcpy(&Cmd.FileSpec[0],Value,vlen);
		ObjectWasFound = true;
		return true;
	}
	else if (StringsEqual(Tag, "stretch"))
	{
		return ReadBooleanValueAsBool(Value, Cmd.Stretch);
	}
	else if (StringsEqual(Tag,"size"))
	{
		return ReadFloatValue(Value,Cmd.Size);
	}
	else if (StringsEqual(Tag, "progress_top"))
	{
		return ReadInt16Value(Value, Cmd.T);
	}
	else if (StringsEqual(Tag, "progress_bottom"))
	{
		return ReadInt16Value(Value, Cmd.B);
	}
	else if (StringsEqual(Tag, "progress_left"))
	{
		return ReadInt16Value(Value, Cmd.L);
	}
	else if (StringsEqual(Tag, "progress_right"))
	{
		return ReadInt16Value(Value, Cmd.R);
	}
	UnrecognizedTag();
	return false;
}

bool XML_LSCommandParser::End()
{
	if (!ObjectWasFound) return false;

	
	assert(CurrScriptPtr);
	CurrScriptPtr->Commands.push_back(Cmd);
	return true;
}

static XML_LSCommandParser MMLParser("mml",LevelScriptCommand::MML);
static XML_LSCommandParser MusicParser("music",LevelScriptCommand::Music);
static XML_LSCommandParser MovieParser("movie",LevelScriptCommand::Movie);
#ifdef HAVE_LUA
static XML_LSCommandParser LuaParser("lua",LevelScriptCommand::Lua);
#endif /* HAVE_LUA */
#ifdef HAVE_OPENGL
static XML_LSCommandParser LoadScreenParser("load_screen", LevelScriptCommand::LoadScreen);
#endif

class XML_RandomOrderParser: public XML_ElementParser
{
public:
	bool HandleAttribute(const char *Tag, const char *Value)
	{
		if (StringsEqual(Tag,"on"))
		{
			bool RandomOrder;
			bool Success = ReadBooleanValueAsBool(Value,RandomOrder);
			if (Success)
			{
				assert(CurrScriptPtr);
				CurrScriptPtr->RandomOrder = RandomOrder;
			}
			return Success;
		}
		UnrecognizedTag();
		return false;
	}
	
	XML_RandomOrderParser(): XML_ElementParser("random_order") {}
};


static XML_RandomOrderParser RandomOrderParser;

	
static void AddScriptCommands(XML_ElementParser& ElementParser)
{
	ElementParser.AddChild(&MMLParser);
	ElementParser.AddChild(&MusicParser);
	ElementParser.AddChild(&RandomOrderParser);
	ElementParser.AddChild(&MovieParser);
#ifdef HAVE_LUA
        ElementParser.AddChild(&LuaParser);
#endif /* HAVE_LUA */
#ifdef HAVE_OPENGL
	ElementParser.AddChild(&LoadScreenParser);
	LoadScreenParser.AddChild(Color_GetParser());
#endif
}


// Generalized parser for level scripts; for also parsing default and restoration scripts

class XML_GeneralLevelScriptParser: public XML_ElementParser
{
protected:
	// Tell the level script and the parser for its contents what level we are currently doing
	void SetLevel(short Level);
	
public:
	
	XML_GeneralLevelScriptParser(const char *_Name): XML_ElementParser(_Name) {}
};

void XML_GeneralLevelScriptParser::SetLevel(short Level)
{
	// Scan for current level
	CurrScriptPtr = NULL;
	for (vector<LevelScriptHeader>::iterator ScriptIter = LevelScripts.begin(); ScriptIter < LevelScripts.end(); ScriptIter++)
	{
		if (ScriptIter->Level == Level)
		{
			CurrScriptPtr = &(*ScriptIter);	// Iterator to pointer
			break;
		}
	}
	
	// Not found, so add it
	if (!CurrScriptPtr)
	{
		LevelScriptHeader NewHdr;
		NewHdr.Level = Level;
		LevelScripts.push_back(NewHdr);
		CurrScriptPtr = &LevelScripts.back();
	}
}


// For setting up scripting for levels in general
class XML_LevelScriptParser: public XML_GeneralLevelScriptParser
{
	// Need to get a level ID
	bool LevelWasFound;
	
public:
	bool Start() {LevelWasFound = false; return true;}
	bool AttributesDone() {return LevelWasFound;}

	// For grabbing the level ID
	bool HandleAttribute(const char *Tag, const char *Value);
	
	XML_LevelScriptParser(): XML_GeneralLevelScriptParser("level") {}
};


bool XML_LevelScriptParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"index"))
	{
		short Level;
		if (ReadBoundedInt16Value(Value,Level,0,SHRT_MAX))
		{
			SetLevel(Level);
			LevelWasFound = true;
			return true;
		}
		else
			return false;
	}
	UnrecognizedTag();
	return false;
}

static XML_LevelScriptParser LevelScriptParser;


// For setting up scripting for special pseudo-levels: the default and the restore
class XML_SpecialLevelScriptParser: public XML_GeneralLevelScriptParser
{
	short Level;
	
public:
	bool Start() {SetLevel(Level); return true;}
	
	XML_SpecialLevelScriptParser(char *_Name, short _Level): XML_GeneralLevelScriptParser(_Name), Level(_Level) {}
};

static XML_SpecialLevelScriptParser
	EndScriptParser("end",LevelScriptHeader::End),
	DefaultScriptParser("default",LevelScriptHeader::Default),
	RestoreScriptParser("restore",LevelScriptHeader::Restore);

static  XML_SpecialLevelScriptParser ExternalDefaultScriptParser("default_levels", LevelScriptHeader::Default);

// For setting up end-screen control
class XML_EndScreenParser: public XML_ElementParser
{
	// Need to get a level ID
	bool LevelWasFound;
	
public:
	bool HandleAttribute(const char *Tag, const char *Value);
	
	XML_EndScreenParser(): XML_ElementParser("end_screens") {}
};


bool XML_EndScreenParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"index"))
	{
		return ReadInt16Value(Value,EndScreenIndex);
	}
	else if (StringsEqual(Tag,"count"))
	{
		return ReadBoundedInt16Value(Value,NumEndScreens,0,SHRT_MAX);
	}
	UnrecognizedTag();
	return false;
}

static XML_EndScreenParser EndScreenParser;


// XML-parser support
XML_ElementParser *LevelScript_GetParser()
{
	AddScriptCommands(LevelScriptParser);

	return &LevelScriptParser;
}
XML_ElementParser *EndLevelScript_GetParser()
{
	AddScriptCommands(EndScriptParser);

	return &EndScriptParser;
}
XML_ElementParser *DefaultLevelScript_GetParser()
{
	AddScriptCommands(DefaultScriptParser);

	return &DefaultScriptParser;
}

XML_ElementParser *ExternalDefaultLevelScript_GetParser()
{
#ifdef HAVE_OPENGL
	ExternalDefaultScriptParser.AddChild(&LoadScreenParser);
	LoadScreenParser.AddChild(Color_GetParser());
#endif
	ExternalDefaultScriptParser.AddChild(&MusicParser);
	ExternalDefaultScriptParser.AddChild(&RandomOrderParser);

	return &ExternalDefaultScriptParser;
}

XML_ElementParser *RestoreLevelScript_GetParser()
{
	AddScriptCommands(RestoreScriptParser);

	return &RestoreScriptParser;
}
XML_ElementParser *EndScreens_GetParser()
{
	return &EndScreenParser;
}
