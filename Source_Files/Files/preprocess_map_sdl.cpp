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
 *  preprocess_map_sdl.cpp - Save game routines, SDL implementation
 *
 *  Written in 2000 by Christian Bauer
 */

#include "cseries.h"
#include "FileHandler.h"

#include <vector>

#include "world.h"
#include "map.h"
#include "shell.h"
#include "interface.h"
#include "game_wad.h"
#include "game_errors.h"

// From shell_sdl.cpp
extern vector<DirectorySpecifier> data_search_path;


/*
 *  Get FileSpecifiers for default data files
 */ 
#ifdef psp
	#include <pspkernel.h>
	#include <pspdebug.h>
	#include <pspsdk.h>
	#include <pspnet.h>
	#include <pspnet_inet.h>
	#include <pspnet_apctl.h>
	#include <pspnet_resolver.h>
#endif

 #ifdef psp
	 int wppm_debug(char str[],int ret=1)
	{
	/*		int len;
			char buffer[50];
			int file;
			
			file = sceIoOpen("DEBUG.txt", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
			if (ret) len = sprintf(buffer,"%s",str);	
				else len = sprintf(buffer,"%s",str);
					
			sceIoWrite(file, buffer ,sizeof(char)*len);	
			sceIoClose(file);*/
			
	}
#endif


static bool get_default_spec(FileSpecifier &file, const string &name)
{
	#ifdef psp
		wppm_debug("get_default_spec:started\n");
	#endif
	vector<DirectorySpecifier>::const_iterator i = data_search_path.begin(), end = data_search_path.end();
	#ifdef psp
		wppm_debug("get_default_spec:iterating\n");
	#endif
	while (i != end) {
		file = *i + name;
		if (file.Exists())
			return true;
		i++;
	}
	wppm_debug("get_default_spec:iterations ended\n");
	return false;
}

static bool get_default_spec(FileSpecifier &file, int type)
{
	wppm_debug("get_default_spec2:started\n");
	char name[256];
	getcstr(name, strFILENAMES, type);
	wppm_debug("Name:",0);
	wppm_debug(name,1);
	wppm_debug("get_default_spec2:ended\n");
	return get_default_spec(file, name);
}

void get_default_map_spec(FileSpecifier &file)
{
	//wppm_debug("get_default_map_spec:started\n");
	if (!get_default_spec(file, filenameDEFAULT_MAP))
		alert_user(fatalError, strERRORS, badExtraFileLocations, -1);
	wppm_debug("get_default_map_spec:ended\n");
}

void get_default_physics_spec(FileSpecifier &file)
{
	wppm_debug("get_default_physics_spec:started\n");
	get_default_spec(file, filenamePHYSICS_MODEL);
	// don't care if it does not exist
}

void get_default_shapes_spec(FileSpecifier &file)
{
	wppm_debug("get_default_shapes_spec:started\n");
	if (!get_default_spec(file, filenameSHAPES8))
		alert_user(fatalError, strERRORS, badExtraFileLocations, -1);
	wppm_debug("get_default_shapes_spec:ended\n");
}

void get_default_sounds_spec(FileSpecifier &file)
{
	wppm_debug("get_default_sounds_spec:started\n");
	get_default_spec(file, filenameSOUNDS8);
	// don't care if it does not exist
}

bool get_default_music_spec(FileSpecifier &file)
{
	/*wppm_debug("get_default_music_spec:started\n");
	return get_default_spec(file, filenameMUSIC);*/
	return true;
}

bool get_default_theme_spec(FileSpecifier &file)
{
	wppm_debug("get_default_theme_spec:started\n");
	FileSpecifier theme = "Themes";
	theme += getcstr(temporary, strFILENAMES, filenameDEFAULT_THEME);
	wppm_debug("get_default_theme_spec:ended\n");
	return get_default_spec(file, theme.GetPath());
}


/*
 *  Choose saved game for loading
 */

bool choose_saved_game_to_load(FileSpecifier &saved_game)
{
	return saved_game.ReadDialog(_typecode_savegame);
}


/*
 *  Save game
 */

bool save_game(void)
{
	wppm_debug("save_game\n");
	pause_game();
	//show_cursor();

	// Translate the name
	FileSpecifier file;
	//get_current_saved_game_name(file);
	//char game_name[256];
	//file.GetName(game_name);

	// Display the dialog
	char prompt[256];
	//bool success = file.WriteDialogAsync(_typecode_savegame, getcstr(prompt, strPROMPTS, _save_game_prompt), game_name);
	bool success=file.WriteDialog(_typecode_savegame,getcstr(prompt, strPROMPTS, _save_game_prompt),"");

	// Save game
	if (success)
		success = save_game_file(file);

	hide_cursor();
	resume_game();

	return success;
}


/*
 *  Store additional data in saved game file
 */

void add_finishing_touches_to_save_file(FileSpecifier &file)
{
	// The MacOS version stores an overhead thumbnail and the level name
	// in resources. Do we want to imitate this?
}
