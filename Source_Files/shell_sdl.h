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
 *  shell_sdl.cpp - Main game loop and input handling, SDL implementation
 *
 *  Written in 2000 by Christian Bauer
 *
 *  LP: Sept 6, 2001: Added Ian-Rickard-style button sounds
 *
 *  Mar 1, 2002 (Woody Zenfell):
 *      ripped w_levels out of here into sdl_widgets.*
 *
 *  May 16, 2002 (Woody Zenfell):
 *      support for Escape (in-game) and Alt+F4 (Windows only) as quit gestures
 *
 *  August 27, 2003 (Woody Zenfell):
 *	Created w_directory_browsing_list widget (like w_file_list but can navigate
 *	directories); now using that in FileSpecifier::ReadDialog()
 */

#ifndef __SHELL_SDL__
#define __SHELL_SDL__

#ifndef psp
#define psp
#endif

#ifdef __MVCPP__

#include "sdl_cseries.h"
#include "SDL_rwops.h"
#include "shell.h"
#include "world.h"
#include "XML_ParseTreeRoot.h"
#include "interface.h"
#include "preferences.h"
#include "screen.h"
#include "mysound.h"
#include "vbl.h"
#include "map.h"
#include "screen_drawing.h"
#include "computer_interface.h"
#include "fades.h"
#include "images.h"
#include "music.h"
#include "player.h"
#include "interface_menus.h"
#include "items.h"

static void main_event_loop(void);

extern bool CheatsActive;

extern int process_keyword_key(char key);

int new_menu_item;

void global_idle_proc(void);
void handle_keyword(int type_of_cheat);
void free_and_unlock_memory(void);

#endif

#include "XML_Loader_SDL.h"
#include "resource_manager.h"
#include "sdl_dialogs.h"
#include "sdl_fonts.h"
#include "sdl_widgets.h"
//#include "gp2x_joystick.h"
#include "SDL.h"

#include "TextStrings.h"

#ifdef HAVE_CONFIG_H
#include "confpaths.h"
#endif

#include <exception>
#include <algorithm>
#include <vector>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_OPENGL
# if defined (__APPLE__) && defined (__MACH__)
#  include <OpenGL/gl.h>
# else
#  include <GL/gl.h>
# endif
#endif

#ifdef HAVE_SDL_NET_H
#include <SDL_net.h>
#endif

#ifdef HAVE_SDL_SOUND_H
#include <SDL_sound.h>
#endif

#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef PSP
#include <pspctrl.h>
#include "pspdefines.h"
#endif

#include "SDL_syswm.h"

#include "alephversion.h"

#include "Logging.h"
#include "network.h"
#include "Console.h"

#ifdef PSP
#define MODULE_NAME "AlephOne"
#endif

extern "C" int main(int argc, char **argv);

/*extern gp2xkey gp2xkeybindings[14];
extern gp2xkey gkbdefault1[14];
extern gp2xkey gkbdefault2[14];
extern gp2xkey gkbdefault3[14];*/

int actions[18]={0,_looking_center,_looking_up,_looking_down,_turning_left,_turning_right,_moving_forward, \
	_moving_backward,_sidestepping_left,_sidestepping_right,_left_trigger_state,_right_trigger_state, \
	_cycle_weapons_forward,_cycle_weapons_backward,_action_trigger_state,_toggle_map,_toggle_look,_hold_look};

// Data directories
vector <DirectorySpecifier> data_search_path; // List of directories in which data files are searched for
DirectorySpecifier local_data_dir;    // Local (per-user) data file directory
DirectorySpecifier preferences_dir;   // Directory for preferences
DirectorySpecifier saved_games_dir;   // Directory for saved games
DirectorySpecifier recordings_dir;    // Directory for recordings (except film buffer, which is stored in local_data_dir)

// Command-line options
bool option_nogl = true;             // Disable OpenGL
bool option_nosound = false;          // Disable sound output
bool option_nogamma = false;	      // Disable gamma table effects (menu fades)
bool option_debug = false;
static bool force_fullscreen = false; // Force fullscreen mode
static bool force_windowed = false;   // Force windowed mode

int write_debug(char str[],int ret=1);


#ifdef __BEOS__
// From csfiles_beos.cpp
extern string get_application_directory(void);
extern string get_preferences_directory(void);
#endif

// From preprocess_map_sdl.cpp
extern bool get_default_music_spec(FileSpecifier &file);

// From vbl_sdl.cpp
void execute_timer_tasks(void);

// Prototypes
static void initialize_application(void);
static void shutdown_application(void);
static void initialize_marathon_music_handler(void);
static void process_event(const SDL_Event &event);
//static void process_gp2x_event(const unsigned long c);

#ifdef PSP

// PSP: Used to keep track of when to restore the old palette
//      (ex. after reading terminal)
bool 				PSP_must_restore_palette = false;

// PSP: Fullscreen or HUD
bool				PSP_display_hud = true;

// PSP: Palette to be restored
struct color_table	PSP_palette_to_restore; 

#endif


/*
 *  Main function
 */
/*
static void usage(const char *prg_name)
{
#ifdef __WIN32__
	MessageBox(NULL, "Command line switches:\n\n"
#else
	printf("\nUsage: %s\n"
#endif
	  "\t[-h | --help]          Display this help message\n"
	  "\t[-v | --version]       Display the game version\n"
	  "\t[-d | --debug]         Allow saving of core files\n"
	  "\t                       (by disabling SDL parachute)\n"
	  "\t[-f | --fullscreen]    Run the game fullscreen\n"
	  "\t[-w | --windowed]      Run the game in a window\n"
#ifdef HAVE_OPENGL
	  "\t[-g | --nogl]          Do not use OpenGL\n"
#endif
	  "\t[-s | --nosound]       Do not access the sound card\n"
	  "\t[-m | --nogamma]       Disable gamma table effects (menu fades)\n"
#if defined(unix) || defined(__BEOS__) || defined(__WIN32__)
	  "\nYou can use the ALEPHONE_DATA environment variable to specify\n"
	  "the data directory.\n"
#endif
#ifdef __WIN32__
	  , "Usage", MB_OK | MB_ICONINFORMATION
#else
	  , prg_name
#endif
	);
	exit(0);
}*/

#ifdef psp

void sdl_psp_exception_handler(PspDebugRegBlock *regs)
{
/*	pspDebugScreenInit();

	pspDebugScreenSetBackColor(0x00FF0000);
	pspDebugScreenSetTextColor(0xFFFFFFFF);
	pspDebugScreenClear();

	pspDebugScreenPrintf("I regret to inform you your psp has just crashed\n\n");
	pspDebugScreenPrintf("Exception Details:\n");
	pspDebugDumpException(regs);
	pspDebugScreenPrintf("\nThe offending routine may be identified with:\n\n"
		"\tpsp-addr2line -e target.elf -f -C 0x%x 0x%x 0x%x\n",
		regs->epc, regs->badvaddr, regs->r[31]);*/
		
	FILE* fCrash = fopen("Aleph One Log.txt","a");
	
	if (fCrash) {
		
		fprintf(fCrash,"PSP AlephOne Crashed:\n");
		fprintf(fCrash,"\nThe offending routine may be identified with:\n\n"
			"\tpsp-addr2line -e target.elf -f -C 0x%x 0x%x 0x%x\n",
			regs->epc, regs->badvaddr, regs->r[31]);
		fclose(fCrash);
	}
	sceKernelExitGame();
}

#endif


#ifdef psp 
int exit_callback(int arg1, int arg2, void *common) 
{ 
   printf("Processing Exit Request:\n");
   set_game_state(_quit_game);
   sceKernelExitGame();
   return 0; 
} 

int CallbackThread(SceSize args, void *argp) 
{ 
   int cbid; 

   cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL); 
   sceKernelRegisterExitCallback(cbid); 
   sceKernelSleepThreadCB(); 

   return 0; 
} 

int SetupCallbacks(void) 
{ 
   int thid = 0; 

   thid = sceKernelCreateThread("update_thread", CallbackThread, 
                 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0); 
   if(thid >= 0) 
   { 
      sceKernelStartThread(thid, 0, 0); 
   } 

   return thid; 
} 

int write_debug(char str[],int ret)
{
	/*	int len;
		char buffer[50];
		int file;
		
		file = sceIoOpen("DEBUG.txt", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
		if (ret) len = sprintf(buffer,"%s",str);	
			else len = sprintf(buffer,"%s",str);
				
		sceIoWrite(file, buffer ,sizeof(char)*len);	
		sceIoClose(file);*/
		
}


#endif

int main(int argc, char **argv)
{
	
	//#ifdef psp
	/*	int fdout;
		int len;
		char buffer[50];
		
		//SetupCallbacks();
		
		fdout = sceIoOpen("DEBUG.txt", PSP_O_WRONLY | PSP_O_CREAT , 0777);
		write_debug(fdout,"Started!");
		sceIoClose(fdout);*/
		
	//#endif
	
	/*// Print banner (don't bother if this doesn't appear when started from a GUI)
	printf ("Aleph One " A1_VERSION_STRING "\n"
	  "http://source.bungie.org/\n\n"
	  "Original code by Bungie Software <http://www.bungie.com/>\n"
	  "Additional work by Loren Petrich, Chris Pruett, Rhys Hill et al.\n"
	  "TCP/IP networking by Woody Zenfell\n"
	  "Expat XML library by James Clark\n"
	  "SDL port by Christian Bauer <Christian.Bauer@uni-mainz.de>\n"
#if defined(__MACH__) && defined(__APPLE__)
	  "Mac OS X/SDL version by Chris Lovell, Alexander Strange, and Woody Zenfell\n"
#endif
	  "\nThis is free software with ABSOLUTELY NO WARRANTY.\n"
	  "You are welcome to redistribute it under certain conditions.\n"
	  "For details, see the file COPYING.\n"
#ifdef __BEOS__
	  // BeOS version is statically linked against SDL, so we have to include this:
	  "\nSimple DirectMedia Layer (SDL) Library included under the terms of the\n"
	  "GNU Library General Public License.\n"
	  "For details, see the file COPYING.SDL.\n"
#endif
#ifdef HAVE_SDL_NET
	  "\nBuilt with network play enabled.\n"
#endif
#ifdef HAVE_LUA
	  "\nBuilt with Lua scripting enabled.\n"
#endif
    );*/

/*	// Parse arguments
	char *prg_name = argv[0];
	argc--;
	argv++;
	while (argc > 0) {
		if (strcmp(*argv, "-h") == 0 || strcmp(*argv, "--help") == 0) {
			usage(prg_name);
		} else if (strcmp(*argv, "-v") == 0 || strcmp(*argv, "--version") == 0) {
			printf("Aleph One " A1_VERSION_STRING "\n");
			exit(0);
		} else if (strcmp(*argv, "-f") == 0 || strcmp(*argv, "--fullscreen") == 0) {
			force_fullscreen = true;
		} else if (strcmp(*argv, "-w") == 0 || strcmp(*argv, "--windowed") == 0) {
			force_windowed = true;
		} else if (strcmp(*argv, "-g") == 0 || strcmp(*argv, "--nogl") == 0) {
			option_nogl = true;
		} else if (strcmp(*argv, "-s") == 0 || strcmp(*argv, "--nosound") == 0) {
			option_nosound = true;
		} else if (strcmp(*argv, "-m") == 0 || strcmp(*argv, "--nogamma") == 0) {
			option_nogamma = true;
		} else if (strcmp(*argv, "-d") == 0 || strcmp(*argv, "--debug") == 0) {
		  option_debug = true;
		} else {
			printf("Unrecognized argument '%s'.\n", *argv);
			usage(prg_name);
		}
		argc--;
		argv++;
	}*/
/*
	try {

		// Initialize everything
		initialize_application();

		// Run the main loop
		main_event_loop();

	} catch (exception &e) {
		fprintf(stderr, "Unhandled exception: %s\n", e.what());
		exit(1);
	} catch (...) {
		fprintf (stderr, "Unknown exception\n");
		exit(1);
	}*/

	
	SetupCallbacks();
	
	#ifdef psp
		//int fdout;
		//int len;
		//char buffer[50];
		
		//fdout = sceIoOpen("DEBUG.txt", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0777);
		write_debug("Started! With Debug!");
		write_debug("Aleph One " A1_VERSION_STRING "\n");
		write_debug("Initializing...\n");
		//sceIoClose(fdout);
		
		pspDebugInstallErrorHandler(sdl_psp_exception_handler);
		
	#endif
	
	initialize_application();
	
	#ifdef psp
		write_debug("Initialized!!! FUCK!!\n");
		write_debug("Starting the game!\n");
	#endif
	
	main_event_loop();
	
	#ifdef PSP
					printf("Main event loop done\nExiting main..\n");
	#endif
	return 0;
}

static void initialize_application(void)
{
#if defined(__WIN32__) && defined(__MINGW__)
  if (option_debug) LoadLibrary("exchndl.dll");
#endif
	// Initialize SDL
	
	#ifdef psp
		write_debug("-----------------------");
		write_debug("Initializing SDL...");
	#endif
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER | 
		     (option_nosound ? 0 : SDL_INIT_AUDIO) |
		     (option_debug ? SDL_INIT_NOPARACHUTE : 0)
		     ) < 0) {
		fprintf(stderr, "Couldn't initialize SDL (%s)\n", SDL_GetError());
		exit(1);
	}
	#ifdef psp
		write_debug("OK\n");
	#endif
	//SDL_WM_SetCaption("Aleph One", "Aleph One");
	atexit(shutdown_application);

	//gp2x_joystick_init();
	#ifdef psp
		write_debug("SDL Joystick...");
	#endif
	//SDL_JoystickOpen(0);
	printf("Joystick Inited\n");
	#ifdef psp
		write_debug("OK\n");
	#endif
	
#ifdef HAVE_SDL_NET
	// Initialize SDL_net
	#ifdef psp
		write_debug("SDL Net...");
	#endif
	if (SDLNet_Init () < 0) {
		fprintf (stderr, "Couldn't initialize SDL_net (%s)\n", SDLNet_GetError());
		exit(1);
	}
	#ifdef psp
		write_debug("OK\n");
	#endif
#endif

#ifdef HAVE_SDL_SOUND
	// Initialize SDL_sound
	#ifdef psp
		write_debug("SDL Sound...");
	#endif
	if (Sound_Init () == 0) {
		fprintf (stderr, "Couldn't initialize SDL_sound (%s)\n", Sound_GetError());
		exit(1);
	}
	#ifdef psp
		write_debug("OK\n");
	#endif
#endif

	// Find data directories, construct search path
	DirectorySpecifier default_data_dir;

#if defined(unix)

	//default_data_dir = PKGDATADIR;
	//const char *home = getenv("HOME");
	//if (home)
	//	local_data_dir = home;
	//local_data_dir += ".alephone";
	default_data_dir = "./data";
	local_data_dir = "./data";
#elif defined(psp)

	default_data_dir = "./data";
	local_data_dir = "./data";

#elif defined(__APPLE__) && defined(__MACH__)
	extern char *bundle_name; // SDLMain.m
	DirectorySpecifier bundle_data_dir = bundle_name;
	bundle_data_dir += "Contents/Resources/DataFiles";

	default_data_dir = bundle_data_dir;
	const char *home = getenv("HOME");
	if (home)
	    local_data_dir = home;
	local_data_dir += "Library";
	local_data_dir += "Application Support";
	local_data_dir += "AlephOne";

#elif defined(__BEOS__)

	default_data_dir = get_application_directory();
	local_data_dir = get_preferences_directory();

#else
#error Data file paths must be set for this platform.
#endif

#if defined(__WIN32__)
#define LIST_SEP ';'
#else
#define LIST_SEP ':'
#endif

//	const char *data_env = getenv("ALEPHONE_DATA");
//	if (data_env) {
		// Read colon-separated list of directories
//		string path = data_env;
//		string::size_type pos;
//		while ((pos = path.find(LIST_SEP)) != string::npos) {
//			if (pos) {
//				string element = path.substr(0, pos);
//				data_search_path.push_back(element);
//			}
//			path.erase(0, pos + 1);
//		}
//		if (!path.empty())
//			data_search_path.push_back(path);
//	} else
#if defined(__APPLE__) && defined(__MACH__)
	data_search_path.push_back(".");
#endif
	data_search_path.push_back(default_data_dir);
	data_search_path.push_back(local_data_dir);

	// Subdirectories
	preferences_dir = local_data_dir;
	saved_games_dir = local_data_dir + "Saved Games";
	recordings_dir = local_data_dir + "Recordings";

	// Create local directories
	#ifdef psp
		write_debug("Create local directories...");
	#endif
	local_data_dir.CreateDirectory();
	saved_games_dir.CreateDirectory();
	recordings_dir.CreateDirectory();
#if defined(__APPLE__) && defined(__MACH__)
	DirectorySpecifier local_mml_dir = bundle_data_dir + "MML";
#else
	DirectorySpecifier local_mml_dir = local_data_dir + "MML";
#endif
	local_mml_dir.CreateDirectory();
#if defined(__APPLE__) && defined(__MACH__)
	DirectorySpecifier local_themes_dir = bundle_data_dir + "Themes";
#else
	DirectorySpecifier local_themes_dir = local_data_dir + "Themes";
#endif
	local_themes_dir.CreateDirectory();
	#ifdef psp
		write_debug("OK\n");
	#endif
	
	#ifdef psp
		write_debug("Initialize Resources...");
	#endif
	// Setup resource manager
	initialize_resources();
	#ifdef psp
		write_debug("OK\n");
	#endif

	// Parse MML files
	#ifdef psp
		write_debug("Setting up MML parse tree...");
	#endif
	
	SetupParseTree();
	
	#ifdef psp
		write_debug("OK\n");
	#endif
	
	#ifdef psp
		write_debug("Loading Base MML Scripts...");
	#endif
	
	LoadBaseMMLScripts();
	
	#ifdef psp
		write_debug("OK\n");
	#endif

	// Check for presence of strings
	if (!TS_IsPresent(strERRORS) || !TS_IsPresent(strFILENAMES)) {
		fprintf(stderr, "Can't find required text strings (missing MML?).\n");
		exit(1);
	}

	// Load preferences
	#ifdef psp
		write_debug("Initialize preferences...\n");
	#endif
	initialize_preferences();
	#ifdef psp
		write_debug("InitPref:OK\n");
	#endif
	
//#ifndef HAVE_OPENGL
	graphics_preferences->screen_mode.acceleration = _no_acceleration;
//#endif
	if (force_fullscreen)
		graphics_preferences->screen_mode.fullscreen = true;
	/*if (force_windowed)		// takes precedence over fullscreen because windowed is safer
		graphics_preferences->screen_mode.fullscreen = false;*/
	write_preferences();

// ZZZ: disable gamma fading in Mac OS X software rendering
#if defined(__APPLE__) && defined(__MACH__)
        if(graphics_preferences->screen_mode.acceleration == _no_acceleration)
            option_nogamma = true;
#endif

	#ifdef psp
		write_debug("Initialize the rest...\n");
	#endif

	// Initialize everything
	#ifdef psp
		write_debug(" Initialize Mytm...");
	#endif
	mytm_initialize();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Fonts..");
	#endif
	initialize_fonts();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Sound Manager...");
	#endif
	initialize_sound_manager(sound_preferences);
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Marathon Music Handler...");
	#endif
	initialize_marathon_music_handler();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Keyboard Controller...");
	#endif
	initialize_keyboard_controller();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Screen...");
	#endif
	initialize_screen(&graphics_preferences->screen_mode, false);
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Marathon...");
	#endif
	initialize_marathon();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Screen Drawing...");
	#endif
	initialize_screen_drawing();
	#ifdef psp
		write_debug("OK\n");
	#endif
	FileSpecifier theme = environment_preferences->theme_dir;
	#ifdef psp
		write_debug("	Initialize Dialogs...");
	#endif
	initialize_dialogs(theme);
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Terminal Manager...");
	#endif
	initialize_terminal_manager();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Shape Handler...");
	#endif
	initialize_shape_handler();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Fades...");
	#endif
	initialize_fades();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Images Manager...");
	#endif
	initialize_images_manager();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Environment Form Preferences...");
	#endif
	load_environment_from_preferences();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("	Initialize Game State...");
	#endif
	initialize_game_state();
	#ifdef psp
		write_debug("OK\n");
	#endif
	#ifdef psp
		write_debug("Initialize:OK\n");
	#endif
}


static void shutdown_application(void)
{
        // ZZZ: seem to be having weird recursive shutdown problems esp. with fullscreen modes...
        static bool already_shutting_down = false;
        if(already_shutting_down)
                return;
        
        already_shutting_down = true;

	//gp2x_joystick_deinit();
        
#ifdef HAVE_SDL_NET
	SDLNet_Quit();
#endif
#ifdef HAVE_SDL_SOUND
	Sound_Quit();
#endif
	SDL_Quit();

	//chdir("/usr/gp2x");
	//execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
}


bool networking_available(void)
{
#ifdef HAVE_SDL_NET
	return true;
#else
	return false;
#endif
}


static void initialize_marathon_music_handler(void)
{
	FileSpecifier file;
	if (get_default_music_spec(file))
		initialize_music_handler(file);
}


bool quit_without_saving(void)
{
	dialog d;
	d.add (new w_static_text("Are you sure you wish to"));
	d.add (new w_static_text("cancel the game in progress?"));
	d.add (new w_spacer());
	d.add (new w_left_button("YES", dialog_ok, &d));
	d.add (new w_right_button("NO", dialog_cancel, &d));
	return d.run() == 0;
}


// ZZZ: moved level-numbers widget into sdl_widgets for a wider audience.

const int32 AllPlayableLevels = _single_player_entry_point | _multiplayer_carnage_entry_point | _multiplayer_cooperative_entry_point;

short get_level_number_from_user(void)
{
	// Get levels
	vector<entry_point> levels;
	if (!get_entry_points(levels, AllPlayableLevels)) {
		entry_point dummy;
		dummy.level_number = 0;
		strcpy(dummy.level_name, "Untitled Level");
		levels.push_back(dummy);
	}

	// Create dialog
	dialog d;
	if (vidmasterStringSetID != -1 && TS_IsPresent(vidmasterStringSetID) && TS_CountStrings(vidmasterStringSetID) > 0) {
		// if we there's a stringset present for it, load the message from there
		int num_lines = TS_CountStrings(vidmasterStringSetID);

		for (size_t i = 0; i < num_lines; i++) {
			bool message_font_title_color = true;
			char *string = TS_GetCString(vidmasterStringSetID, i);
			if (!strncmp(string, "[QUOTE]", 7)) {
				string = string + 7;
				message_font_title_color = false;
			}
			if (!strlen(string))
				d.add(new w_spacer());
			else if (message_font_title_color)
				d.add(new w_static_text(string, MESSAGE_FONT, TITLE_COLOR));
			else
				d.add(new w_static_text(string));
		}

	} else {
		// no stringset or no strings in stringset - use default message
		d.add(new w_static_text("Before proceeding any further, you", MESSAGE_FONT, TITLE_COLOR));
		d.add(new w_static_text ("must take the oath of the vidmaster:", MESSAGE_FONT, TITLE_COLOR));
		d.add(new w_spacer());
		d.add(new w_static_text("\xd2I pledge to punch all switches,"));
		d.add(new w_static_text("to never shoot where I could use grenades,"));
		d.add(new w_static_text("to admit the existence of no level"));
		d.add(new w_static_text("except Total Carnage,"));
		d.add(new w_static_text("to never use Caps Lock as my \xd4run\xd5 key,"));
		d.add(new w_static_text("and to never, ever, leave a single Bob alive.\xd3"));
	}

	d.add(new w_spacer());
	d.add(new w_static_text("Start at level:", MESSAGE_FONT, TITLE_COLOR));

	w_levels *level_w = new w_levels(levels, &d);
	d.add(level_w);
	d.add(new w_spacer());
	d.add(new w_button("CANCEL", dialog_cancel, &d));

	// Run dialog
	short level;
	if (d.run() == 0)		// OK
		// Should do noncontiguous map files OK
		level = levels[level_w->get_selection()].level_number;
	else
		level = NONE;

	// Redraw main menu
	update_game_window();
	return level;
}


// ZZZ: Filesystem browsing list that lets user actually navigate directories...
class w_directory_browsing_list : public w_list<dir_entry>
{
public:
	w_directory_browsing_list(const FileSpecifier& inStartingDirectory, dialog* inParentDialog)
	: w_list<dir_entry>(entries, 400, 15, 0), parent_dialog(inParentDialog), current_directory(inStartingDirectory)
	{
		refresh_entries();
	}


	w_directory_browsing_list(const FileSpecifier& inStartingDirectory, dialog* inParentDialog, const string& inStartingFile)
	: w_list<dir_entry>(entries, 400, 15, 0), parent_dialog(inParentDialog), current_directory(inStartingDirectory)
	{
		refresh_entries();
		if(entries.size() != 0)
			select_entry(inStartingFile, false);
	}


	void set_directory_changed_callback(action_proc inCallback, void* inArg = NULL)
	{
		directory_changed_proc = inCallback;
		directory_changed_proc_arg = inArg;
	}


	void draw_item(vector<dir_entry>::const_iterator i, SDL_Surface *s, int16 x, int16 y, uint16 width, bool selected) const
	{
		y += font->get_ascent();
		set_drawing_clip_rectangle(0, x, s->h, x + width);

		if(i->is_directory)
		{
			string theName = i->name + "/";
			draw_text(s, theName.c_str (), x, y, selected ? get_dialog_color (ITEM_ACTIVE_COLOR) : get_dialog_color (ITEM_COLOR), font, style);
		}
		else
			draw_text(s, i->name.c_str (), x, y, selected ? get_dialog_color (ITEM_ACTIVE_COLOR) : get_dialog_color (ITEM_COLOR), font, style);

		set_drawing_clip_rectangle(SHRT_MIN, SHRT_MIN, SHRT_MAX, SHRT_MAX);
	}


	bool can_move_up_a_level()
	{
		string base;
		string part;
		current_directory.SplitPath(base, part);
		return (part != string());
	}


	void move_up_a_level()
	{
		string base;
		string part;
		current_directory.SplitPath(base, part);
		if(part != string())
		{
			FileSpecifier parent_directory(base);
			if(parent_directory.Exists())
			{
				current_directory = parent_directory;
				refresh_entries();
				select_entry(part, true);
				announce_directory_changed();
			}
		}
	}


	void item_selected(void)
	{
		current_directory.AddPart(entries[selection].name);

		if(entries[selection].is_directory)
		{
			refresh_entries();
			announce_directory_changed();
		}
		else
			parent_dialog->quit(0);
	}


	const FileSpecifier& get_file() { return current_directory; }
	
	
private:
	vector<dir_entry>	entries;
	dialog*			parent_dialog;
	FileSpecifier 		current_directory;
	action_proc		directory_changed_proc;
	void*			directory_changed_proc_arg;
	
	void refresh_entries()
	{
		if(current_directory.ReadDirectory(entries))
		{
			sort(entries.begin(), entries.end());
			num_items = entries.size();
			new_items();
		}
	}

	void select_entry(const string& inName, bool inIsDirectory)
	{
		dir_entry theEntryToFind(inName, NONE, inIsDirectory);
		vector<dir_entry>::iterator theEntry = lower_bound(entries.begin(), entries.end(), theEntryToFind);
		if(theEntry != entries.end())
			set_selection(theEntry - entries.begin());
	}

	void announce_directory_changed()
	{
		if(directory_changed_proc != NULL)
			directory_changed_proc(directory_changed_proc_arg);
	}
};



class w_file_list : public w_list<dir_entry> {
public:
	w_file_list(const vector<dir_entry> &items) : w_list<dir_entry>(items, 400, 15, 0) {}

	void draw_item(vector<dir_entry>::const_iterator i, SDL_Surface *s, int16 x, int16 y, uint16 width, bool selected) const
	{
		y += font->get_ascent();
		set_drawing_clip_rectangle(0, x, s->h, x + width);
		draw_text(s, i->name.c_str (), x, y, selected ? get_dialog_color (ITEM_ACTIVE_COLOR) : get_dialog_color (ITEM_COLOR), font, style);
		set_drawing_clip_rectangle(SHRT_MIN, SHRT_MIN, SHRT_MAX, SHRT_MAX);
	}
};

class w_read_file_list : public w_file_list {
public:
	w_read_file_list(const vector<dir_entry> &items, dialog *d) : w_file_list(items), parent(d) {}

	void item_selected(void)
	{
		parent->quit(0);
	}

private:
	dialog *parent;
};

static void
bounce_up_a_directory_level(void* inWidget)
{
	w_directory_browsing_list* theBrowser = static_cast<w_directory_browsing_list*>(inWidget);
	theBrowser->move_up_a_level();
}

enum
{
	iDIRBROWSE_UP_BUTTON = 100,
	iDIRBROWSE_DIR_NAME,
	iDIRBROWSE_BROWSER
};

static void
respond_to_directory_changed(void* inArg)
{
	// Get dialog and its directory browser
	dialog* theDialog = static_cast<dialog*>(inArg);
	w_directory_browsing_list* theBrowser = dynamic_cast<w_directory_browsing_list*>(theDialog->get_widget_by_id(iDIRBROWSE_BROWSER));
	
	// Update static text listing current directory
	w_static_text* theDirectoryName = dynamic_cast<w_static_text*>(theDialog->get_widget_by_id(iDIRBROWSE_DIR_NAME));
	theBrowser->get_file().GetName(temporary);
	theDirectoryName->set_text(temporary);
	
	// Update enabled state of Up button
	w_button* theUpButton = dynamic_cast<w_button*>(theDialog->get_widget_by_id(iDIRBROWSE_UP_BUTTON));
	theUpButton->set_enabled(theBrowser->can_move_up_a_level());
}

bool FileSpecifier::ReadDialog(Typecode type, char *prompt)
{
	// Set default prompt
	if (prompt == NULL) {
		switch (type) {
			case _typecode_savegame:
				prompt = "CONTINUE SAVED GAME";
				break;
			case _typecode_film:
				prompt = "REPLAY SAVED FILM";
				break;
			default:
				prompt = "OPEN FILE";
				break;
		}
	}

	// Read directory
	FileSpecifier dir;
	string filename;
	switch (type) {
		case _typecode_savegame:
			dir.SetToSavedGamesDir();
			break;
		case _typecode_film:
			dir.SetToRecordingsDir();
			break;
	case _typecode_scenario:
	  dir.SetToFirstDataDir();
	  break;
		case _typecode_netscript:
		{
			// Go to most recently-used directory
			DirectorySpecifier theDirectory;
			SplitPath(theDirectory, filename);
			dir.FromDirectory(theDirectory);
			if(!dir.Exists())
				dir.SetToLocalDataDir();
			break;
		}
		default:
			dir.SetToLocalDataDir();
			break;
	}

      SDL_Surface *world_pixels = SDL_GetVideoSurface();
      short X0 = world_pixels->w;
      short Y0 = world_pixels->h;
	int ichosen=0;
	long lasttime=0;
	bool avail=false;

	SDL_Flip(world_pixels);
	SDL_FillRect(world_pixels,NULL,0);
      FontSpecifier& Font = GetOnScreenFont();
      _set_port_to_screen_window();

	int i;
	bool ok=false;
	bool done=false;

	{
		long d=SDL_GetTicks();
		while(SDL_GetTicks()-d<1000) {}
	}

	//SDL_Rect zoom;
	//zoom.x=160;
	//zoom.y=0;
	//zoom.w=320;
	//zoom.h=240;

//	SDL_GP2X_Display(&zoom);
	
	//for(i=0;i<10;i++) sprintf(menutext[i],"Save %i",i);
#ifdef PSP
	
	/*struct color_table *system_colors = build_8bit_system_color_table();
	assert_world_color_table(system_colors, system_colors);
	delete system_colors;*/
	
	i = 0;
	
	SDL_Color colors[6*6*6];
	for (int red=0; red<6; red++) {
		for (int green=0; green<6; green++) {
			for (int blue=0; blue<6; blue++) {
				colors[i].r = red * 0x33;
				colors[i].g = green * 0x33;
				colors[i].b = blue * 0x33;
				i++;
			}
		}
	}
	
	SDL_SetColors(SDL_GetVideoSurface(),colors,0,6*6*6);
		
	printf("PSP Load Game dialog opened\n");

	while(!done) {
		int availsaves=0;
		int Y=20;
		for(i=0;i<10;i++) {
			char menutext[256];
			char filename[7];
			char strdate[256];
			bool exists=false;

			sprintf(filename,"Save %i",i+1);

			FileSpecifier FileToLoad;

			FileToLoad.SetToSavedGamesDir();
			FileToLoad.AddPart(filename);
			if(FileToLoad.Exists()) {
				
				TimeType savetime = FileToLoad.GetDate();
				struct tm * timeinfo = localtime(&savetime);
				
				availsaves++;
				strftime(strdate,sizeof(strdate),"%d/%m/%y %T",timeinfo);
				sprintf(menutext,"Save %i - (%s)",(i+1),strdate);
				exists=true;
			} else {
				sprintf(menutext,"Save %i - Empty",(i+1));
				exists=false;
			}

      		short RightJustOffset = Font.TextWidth(menutext);
      		short X = (X0 - RightJustOffset)/2;

			if(exists) {
				if(ichosen!=i)
					draw_text(world_pixels, menutext, X, Y, SDL_MapRGB(world_pixels->format, 255, 255, 255), Font.Info, Font.Style);
				else {
					avail=true;
					draw_text(world_pixels, menutext, X, Y, SDL_MapRGB(world_pixels->format, 255, 0, 0), Font.Info, Font.Style);
				}
			} else {
				if(ichosen!=i)
					draw_text(world_pixels, menutext, X, Y, SDL_MapRGB(world_pixels->format, 200, 200, 200), Font.Info, Font.Style);
				else {
					avail=false;
					draw_text(world_pixels, menutext, X, Y, SDL_MapRGB(world_pixels->format, 200, 0, 0), Font.Info, Font.Style);
				}
			}
			Y+=15;
		}

		if(availsaves==0) {
			char menutext[255];
			sprintf(menutext,"no saves found");

	      	short RightJustOffset = Font.TextWidth(menutext);
    		  	short X = (X0 - RightJustOffset)/2;

			draw_text(world_pixels, menutext, X, 100, SDL_MapRGB(world_pixels->format, 255, 255, 255), Font.Info, Font.Style);
		}
//TODO
	//SDL_Joystick *joystick = SDL_JoystickOpen(0);
		SceCtrlData pad;
		sceCtrlSetSamplingCycle(0);
		sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
	
		if(SDL_GetTicks()-lasttime>200) {
			sceCtrlReadBufferPositive(&pad, 1); 
			if (pad.Buttons != 0) 	{
				if (pad.Buttons & PSP_CTRL_UP) ichosen--;
				if (pad.Buttons & PSP_CTRL_DOWN) ichosen++;
					
				if(ichosen<0) ichosen=0;
				if(ichosen>9) ichosen=9;
					
				if((pad.Buttons & PSP_CTRL_CROSS) && avail) {
					printf("PSP Save Game chosen\n");
					done=true;
					if(availsaves==0)
						ok=false;
					else
						ok=true;
					break;
				} else if(pad.Buttons & PSP_CTRL_SQUARE) {
					printf("PSP Exiting Load Game dialog\n");
					done=true;
					ok=false;
					break;
				}
			}			
			lasttime=SDL_GetTicks();
		}
		SDL_Flip(SDL_GetVideoSurface());
	}
	
	if (ok) printf("PSP Save Game is ok\n");
	else printf("PSP Save Game not available\n");
	
#endif
      _restore_port();

	if(ok) {
		char filename[7];
		sprintf(filename,"Save %i",ichosen+1);

		dir.AddPart(filename);
		
		*this=dir;
	}

	return ok;

	//return result;
}

class w_file_name : public w_text_entry {
public:
	w_file_name(const char *name, dialog *d, const char *initial_name = NULL) : w_text_entry(name, 31, initial_name), parent(d) {}
	~w_file_name() {}

	void event(SDL_Event & e)
	{
		// Return = close dialog
		if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN)
			parent->quit(0);
		w_text_entry::event(e);
	}

private:
	dialog *parent;
};

class w_write_file_list : public w_file_list {
public:
	w_write_file_list(const vector<dir_entry> &items, const char *selection, dialog *d, w_file_name *w) : w_file_list(items), parent(d), name_widget(w)
	{
		if (selection) {
			vector<dir_entry>::const_iterator i, end = items.end();
			size_t num = 0;
			for (i = items.begin(); i != end; i++, num++) {
				if (i->name == selection) {
					set_selection(num);
					break;
				}
			}
		}
	}

	void item_selected(void)
	{
		name_widget->set_text(items[selection].name.c_str());
		parent->quit(0);
	}

private:
	dialog *parent;
	w_file_name *name_widget;
};

static bool confirm_save_choice(FileSpecifier & file);

bool FileSpecifier::WriteDialog(Typecode type, char *prompt, char *default_name)
{
	// Set default prompt
	if (prompt == NULL) {
		switch (type) {
			case _typecode_savegame:
				prompt = "SAVE GAME";
				break;
			case _typecode_film:
				prompt = "SAVE FILM";
				break;
			default:
				prompt = "SAVE FILE";
				break;
		}
	}

	// Read directory
	FileSpecifier dir;
	switch (type) {
		case _typecode_savegame:
			dir.SetToSavedGamesDir();
			break;
		case _typecode_film:
			dir.SetToRecordingsDir();
			break;
		default:
			dir.SetToLocalDataDir();
			break;
	}

      SDL_Surface *world_pixels = SDL_GetVideoSurface();
      short X0 = world_pixels->w;
      short Y0 = world_pixels->h;
	int ichosen=0;
	long lasttime=0;

	/*SDL_Rect zoom;
	zoom.x=220;
	zoom.y=0;
	zoom.w=320;
	zoom.h=240;*/

//	SDL_GP2X_Display(&zoom);

	SDL_Flip(world_pixels);
	SDL_FillRect(world_pixels,NULL,0);
      FontSpecifier& Font = GetOnScreenFont();
      _set_port_to_screen_window();

	int i;
	bool ok=false;
	bool done=false;

	{
		long d=SDL_GetTicks();
		while(SDL_GetTicks()-d<1000) {}
	}

	//for(i=0;i<10;i++) sprintf(menutext[i],"Save %i",i);
#ifdef PSP
	printf("PSP Save Game dialog opened\n");

	while(!done) {
		int Y=20;
		for(i=0;i<10;i++) {
			char menutext[256];
			char strdate[256];
			char filename[7];

			sprintf(filename,"Save %i",(i+1));

			FileSpecifier FileToLoad;

			FileToLoad.SetToSavedGamesDir();
			FileToLoad.AddPart(filename);
			if(FileToLoad.Exists()) {
				TimeType savetime = FileToLoad.GetDate();
				struct tm * timeinfo = localtime(&savetime);
				
				strftime(strdate,sizeof(strdate),"%d/%m/%y %T",timeinfo);
				sprintf(menutext,"Save %i - (%s)",(i+1),strdate);
			}
			else
				sprintf(menutext,"Save %i - Empty",(i+1));
			
      		short RightJustOffset = Font.TextWidth(menutext);
      		short X = (X0 - RightJustOffset)/2;

			if(ichosen!=i)
				draw_text(world_pixels, menutext, X, Y, SDL_MapRGB(world_pixels->format, 255, 255, 255), Font.Info, Font.Style);
			else {
				draw_text(world_pixels, menutext, X, Y, SDL_MapRGB(world_pixels->format, 255, 0, 0), Font.Info, Font.Style);
			}
			Y+=15;
		}
		
		SceCtrlData pad;
		sceCtrlSetSamplingCycle(0);
		sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
	
		if(SDL_GetTicks()-lasttime>200) {
			sceCtrlReadBufferPositive(&pad, 1); 
			if (pad.Buttons != 0) 	{
				if (pad.Buttons & PSP_CTRL_UP) ichosen--;
				if (pad.Buttons & PSP_CTRL_DOWN) ichosen++;
					
				if(ichosen<0) ichosen=0;
				if(ichosen>9) ichosen=9;
					
				if(pad.Buttons & PSP_CTRL_CROSS) {
					printf("PSP Save Game chosen\n");
					done=true;
					ok=true;
					break;
				} else if(pad.Buttons & PSP_CTRL_SQUARE) {
					printf("PSP Exiting Save Game dialog\n");
					done=true;
					ok=false;
					break;
				}
			}			
			lasttime=SDL_GetTicks();
		}
		
		SDL_Flip(SDL_GetVideoSurface());
	
	}
#endif
      _restore_port();

	if(ok) {
		char filename[7];
		sprintf(filename,"Save %i",(ichosen+1));

		dir.AddPart(filename);
		
		*this=dir;
	}

	/*zoom.x=0;
	zoom.y=0;
	zoom.w=640;
	zoom.h=480;

	SDL_GP2X_Display(&zoom);*/

	return ok;
}

bool FileSpecifier::WriteDialogAsync(Typecode type, char *prompt, char *default_name)
{
	return FileSpecifier::WriteDialog(type, prompt, default_name);
}

static bool confirm_save_choice(FileSpecifier & file)
{
	// If the file doesn't exist, everything is alright
	if (!file.Exists())
		return true;

	// Construct message
	char name[256];
	file.GetName(name);
	char message[512];
	sprintf(message, "'%s' already exists.", name);

	// Create dialog
	dialog d;
	d.add(new w_static_text(message));
	d.add(new w_static_text("Ok to overwrite?"));
	d.add(new w_spacer());
	d.add(new w_left_button("YES", dialog_ok, &d));
	d.add(new w_right_button("NO", dialog_cancel, &d));

	// Run dialog
	return d.run() == 0;
}


/*
 *  Main event loop
 */

const uint32 TICKS_BETWEEN_EVENT_POLL = 167; // 6 Hz

static void main_event_loop(void)
{
	uint32 last_event_poll = 0;
	while (get_game_state() != _quit_game) {

		bool yield_time = false;
		bool poll_event = false;

		switch (get_game_state()) {
			case _game_in_progress:
			case _change_level:
			  if (Console::instance()->input_active() || SDL_GetTicks() - last_event_poll >= TICKS_BETWEEN_EVENT_POLL) {
					poll_event = true;
					last_event_poll = SDL_GetTicks();
				} else
					SDL_PumpEvents ();	// This ensures a responsive keyboard control
				break;

			case _display_intro_screens:
			case _display_main_menu:
			case _display_chapter_heading:
			case _display_prologue:
			case _display_epilogue:
			case _begin_display_of_epilogue:
			case _display_credits:
			case _display_intro_screens_for_demo:
			case _display_quit_screens:
			case _displaying_network_game_dialogs:
				yield_time = interface_fade_finished();
				poll_event = true;
				break;

			case _close_game:
			case _switch_demo:
			case _revert_game:
				yield_time = poll_event = true;
				break;
		}

		if (poll_event) {
			global_idle_proc();

			while (true) {
				SDL_Event event;
				event.type = SDL_NOEVENT;
				SDL_PollEvent(&event);
				
				if (yield_time) {

					// The game is not in a "hot" state, yield time to other
					// processes by calling SDL_Delay() but only try for a maximum
					// of 30ms
					int num_tries = 0;
					while (event.type == SDL_NOEVENT && num_tries < 3) {
						SDL_Delay(10);
						SDL_PollEvent(&event);
						num_tries++;
					}
					yield_time = false;
				} else if (event.type == SDL_NOEVENT)
					break;

				process_event(event);
			}
		}

		/*unsigned long c = gp2x_joystick_read();

		if(c&GP2X_LEFT||c&GP2X_RIGHT||c&GP2X_UP||c&GP2X_DOWN||c&GP2X_A||c&GP2X_B||c&GP2X_X||c&GP2X_Y||c&GP2X_L||c&GP2X_R \
			|| c&GP2X_SELECT || c&GP2X_START || c&GP2X_VOL_UP || c&GP2X_VOL_DOWN || c&GP2X_CLICK) {
				SDL_Event event;
				event.type = SDL_JOYBUTTONDOWN;
				process_event(event);
		}*/
		
		execute_timer_tasks();
		idle_game_state();
	}
}


/*
 *  Process SDL event
 */

static bool has_cheat_modifiers(void)
{
	SDLMod m = SDL_GetModState();
	return (m & KMOD_SHIFT) && (m & KMOD_CTRL) && !(m & KMOD_ALT) && !(m & KMOD_META);
}

static void process_screen_click(const SDL_Event &event)
{
	portable_process_screen_click(event.button.x, event.button.y, has_cheat_modifiers());
}

static void handle_game_key(const SDL_Event &event)
{
	SDLKey key = event.key.keysym.sym;
	bool changed_screen_mode = false;
	bool changed_prefs = false;

	if (!game_is_networked && (event.key.keysym.mod & KMOD_CTRL) && CheatsActive) {
		int type_of_cheat = process_keyword_key(key);
		if (type_of_cheat != NONE)
			handle_keyword(type_of_cheat);
	}
	if (Console::instance()->input_active()) {
	  switch(key) {
	  case SDLK_RETURN:
	    Console::instance()->enter();
	    break;
	  case SDLK_ESCAPE:
	    Console::instance()->abort();
	    break;
	  case SDLK_BACKSPACE:
	  case SDLK_DELETE:
	    Console::instance()->backspace();
	    break;
	  default:
	    if (event.key.keysym.unicode > 0 && (event.key.keysym.unicode & 0xFF80) == 0) {
	      Console::instance()->key(event.key.keysym.unicode & 0x7F);
	    }
	  }
	}
	else
	{
	
	//TODO VOLUME 
	/*unsigned long c = gp2x_joystick_read();
	if(c&GP2X_L && c&GP2X_VOL_UP) // vol up
		key=SDLK_PERIOD;
	if(c&GP2X_L && c&GP2X_VOL_DOWN) // vol down
		key=SDLK_COMMA;*/

	/* if(c&GP2X_SELECT && c&GP2X_START) // quit
		do_menu_item_command(mGame, iQuitGame, false);

	if(c&GP2X_R && c&GP2X_VOL_UP) // screen mode up
		key=SDLK_F2;
	if(c&GP2X_R && c&GP2X_VOL_DOWN) // screen mode down
		key=SDLK_F1;
	if(c&GP2X_R && c&GP2X_L && c&GP2X_CLICK) // screenshot
		key=SDLK_F9;
	if(c&GP2X_SELECT && c&GP2X_VOL_UP) // gamma up
		key=SDLK_F11;
	if(c&GP2X_SELECT && c&GP2X_VOL_DOWN) // gamma down
		key=SDLK_F12;
	if(c&GP2X_START) // pause
		do_menu_item_command(mGame, iPause, false); */

#ifdef PSP

	SceCtrlData pad;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
	
	sceCtrlReadBufferPositive(&pad, 1); 
	if (pad.Buttons != 0) 	{
		
		//PSP : Take screenshot with L+R+CIRCLE
		
		if ((pad.Buttons & PSP_CTRL_LTRIGGER) && 
			(pad.Buttons & PSP_CTRL_RTRIGGER) && 
			(pad.Buttons & PSP_CTRL_CIRCLE)) 
				key = SDLK_F9;
		
		//PSP : Zoom overhead map with TRIANGLE + UP or DOWN
		
		if ((pad.Buttons & PSP_CTRL_TRIANGLE) &&
			(pad.Buttons & PSP_CTRL_UP) &&
			PLAYER_HAS_MAP_OPEN(current_player)) {
				if (zoom_overhead_map_in())
					PlayInterfaceButtonSound(Sound_ButtonSuccess());
			}
			
		if ((pad.Buttons & PSP_CTRL_TRIANGLE) &&
			(pad.Buttons & PSP_CTRL_DOWN) &&
			PLAYER_HAS_MAP_OPEN(current_player)) {
				if (zoom_overhead_map_out())
					PlayInterfaceButtonSound(Sound_ButtonSuccess());
			}
			
		//PSP : Tolge high resolution mode with L+R+TRIANGLE	
			
		if ((pad.Buttons & PSP_CTRL_LTRIGGER) && 
			(pad.Buttons & PSP_CTRL_RTRIGGER) && 
			(pad.Buttons & PSP_CTRL_TRIANGLE)) 
				key = SDLK_F3;
				
		//PSP : Increase and decrease screen resolution
				
		if ((pad.Buttons & PSP_CTRL_TRIANGLE) && 
			(pad.Buttons & PSP_CTRL_CIRCLE) && 
			(pad.Buttons & PSP_CTRL_UP)) 
				PSP_display_hud = true;
				
		if ((pad.Buttons & PSP_CTRL_TRIANGLE) && 
			(pad.Buttons & PSP_CTRL_CIRCLE) && 
			(pad.Buttons & PSP_CTRL_DOWN)) 
				PSP_display_hud = false;
	}

#endif

	switch (key) {
        case SDLK_ESCAPE:   // (ZZZ) Quit gesture (now safer)
            if(!player_controlling_game())
                do_menu_item_command(mGame, iQuitGame, false);
            else {
                if(get_ticks_since_local_player_in_terminal() > 1 * TICKS_PER_SECOND) {
                    if(!game_is_networked) {
                        do_menu_item_command(mGame, iQuitGame, false);
                    }
                    else {
                        screen_printf("If you wish to quit, press Alt+Q.");
                    }
                }
            }
            break;

		case SDLK_PERIOD:		// Sound volume up
			changed_prefs = adjust_sound_volume_up(sound_preferences, _snd_adjust_volume);
			break;
		case SDLK_COMMA:		// Sound volume down
			changed_prefs = adjust_sound_volume_down(sound_preferences, _snd_adjust_volume);
			break;

		case SDLK_BACKSPACE:	// switch player view
			walk_player_list();
			render_screen(NONE);
			break;

		case SDLK_EQUALS:
			if (zoom_overhead_map_in())
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
			else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
			break;
		case SDLK_MINUS:
			if (zoom_overhead_map_out())
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
			else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
			break;

		case SDLK_LEFTBRACKET:
			if (player_controlling_game()) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				scroll_inventory(-1);
			} else
				decrement_replay_speed();
			break;
		case SDLK_RIGHTBRACKET:
			if (player_controlling_game()) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				scroll_inventory(1);
			} else
				increment_replay_speed();
			break;

		case SDLK_QUESTION: 
		  PlayInterfaceButtonSound(Sound_ButtonSuccess());
			extern bool displaying_fps;
			displaying_fps = !displaying_fps;
			break;
	case SDLK_SLASH:
	  if (event.key.keysym.mod & KMOD_SHIFT) {
	    PlayInterfaceButtonSound(Sound_ButtonSuccess());
	    extern bool displaying_fps;
	    displaying_fps = !displaying_fps;
	  }
	  break;
	case SDLK_BACKSLASH:
	  if (game_is_networked) {
#if !defined(DISABLE_NETWORKING)
	    Console::instance()->activate_input(InGameChatCallbacks::SendChatMessage, InGameChatCallbacks::prompt());
#endif
	    PlayInterfaceButtonSound(Sound_ButtonSuccess());
	  } else
	    PlayInterfaceButtonSound(Sound_ButtonFailure());
	  break;

		case SDLK_F1:		// Decrease screen size
			if (graphics_preferences->screen_mode.size > 0) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.size--;
				changed_screen_mode = changed_prefs = true;
			} else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
			break;

		case SDLK_F2:		// Increase screen size
			if (graphics_preferences->screen_mode.size < 3) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.size++;
				changed_screen_mode = changed_prefs = true;
			} else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
			break;

		case SDLK_F3:		// Resolution toggle
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.high_resolution = !graphics_preferences->screen_mode.high_resolution;
				changed_screen_mode = changed_prefs = true;
			break;

		case SDLK_F4:		// Reset OpenGL textures
#ifdef HAVE_OPENGL
			if (OGL_IsActive()) {
				// Play the button sound in advance to get the full effect of the sound
				PlayInterfaceButtonSound(Sound_OGL_Reset());
				OGL_ResetTextures();
			} else
#endif
				PlayInterfaceButtonSound(Sound_ButtonInoperative());
			break;

		case SDLK_F5:		// Make the chase cam switch sides
			if (ChaseCam_IsActive())
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
			else
				PlayInterfaceButtonSound(Sound_ButtonInoperative());
			ChaseCam_SwitchSides();
			break;

		case SDLK_F6:		// Toggle the chase cam
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			ChaseCam_SetActive(!ChaseCam_IsActive());
			break;

		case SDLK_F7:		// Toggle tunnel vision
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			SetTunnelVision(!GetTunnelVision());
			break;

		case SDLK_F8:		// Toggle the crosshairs
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			Crosshairs_SetActive(!Crosshairs_IsActive());
			break;

		case SDLK_F9:		// Screen dump
			dump_screen();
			break;

		case SDLK_F10:		// Toggle the position display
			PlayInterfaceButtonSound(Sound_ButtonSuccess());
			{
				extern bool ShowPosition;
				ShowPosition = !ShowPosition;
			}
			break;

		case SDLK_F11:		// Decrease gamma level
			if (graphics_preferences->screen_mode.gamma_level) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.gamma_level--;
				change_gamma_level(graphics_preferences->screen_mode.gamma_level);
				changed_prefs = true;
			} else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
			break;

		case SDLK_F12:		// Increase gamma level
			if (graphics_preferences->screen_mode.gamma_level < NUMBER_OF_GAMMA_LEVELS - 1) {
				PlayInterfaceButtonSound(Sound_ButtonSuccess());
				graphics_preferences->screen_mode.gamma_level++;
				change_gamma_level(graphics_preferences->screen_mode.gamma_level);
				changed_prefs = true;
			} else
				PlayInterfaceButtonSound(Sound_ButtonFailure());
			break;

		default:
			if (get_game_controller() == _demo)
				set_game_state(_close_game);
			break;
	}
	}

	if (changed_screen_mode) {
	  screen_mode_data temp_screen_mode = graphics_preferences->screen_mode;
	  temp_screen_mode.fullscreen = get_screen_mode()->fullscreen;
	  change_screen_mode(&temp_screen_mode, true);
	  render_screen(0);
	}

	if (changed_prefs)
		write_preferences();
}

int getoptnum(int opt, int add) {
	int i;
	//int actions[18]={0,_looking_center,_looking_up,_looking_down,_turning_left,_turning_right,_moving_forward, \
	//	_moving_backward,_sidestepping_left,_sidestepping_right,_left_trigger_state,_right_trigger_state, \
	//	_cycle_weapons_forward,_cycle_weapons_backward,_action_trigger_state,_toggle_map,_toggle_look,_hold_look};

	for(i=0;i<18;i++)
		if(actions[i]==opt) break;

	i+=add;
	if(i>17) i=0;
	if(i<0) i=17;

	return i;
}

int do_options_menu() {
	char actiontext[18][255]={":Nothing",":Center",":Look Up",":Look Down",":Turn Left",":Turn Right",":Forward", \
		":Backward",":Sidestep Left",":Sidestep Right",":Primary Fire",":Secondary Fire", \
		":Weapons+",":Weapons-",":Action",":Toggle Map",":Toggle Look",":Hold Look"};

	//change_screen(320,240,16,false);
	/*SDL_Rect zoom;
	zoom.x=0;
	zoom.y=0;
	zoom.w=320;
	zoom.h=240;
	SDL_GP2X_Display(&zoom);*/

      SDL_Surface *world_pixels = SDL_GetVideoSurface();
      short X0 = world_pixels->w;
      short Y0 = world_pixels->h;
	char menutext[16][255]={"Control Scheme","Invert Y Axis","UP","DOWN","LEFT","RIGHT","SELECT","CLICK","VOL-","VOL+","A","B","X","Y","L","R"};
	char schemetext[4][255]={":Default",":Default Swap",":Quake2X",":Custom"};
//	gp2xkey actionkey[14];

	int ichosen=0;
	long lasttime=0;
	int ret=-1;
	Uint16 currentscheme=input_preferences->controlsetup;
	bool invert=input_preferences->invert;

	SDL_FillRect(world_pixels,NULL,0);
      FontSpecifier& Font = GetOnScreenFont();
      _set_port_to_screen_window();

	int i;
	bool ok=false;

	{
		long d=SDL_GetTicks();
		while(SDL_GetTicks()-d<500) {}
	}

//	for(i=0;i<14;i++) 
		//actionkey[i]=gp2xkeybindings[i];

	while(1) {
		for(i=0;i<16;i++) {
			short RightJustOffset = Font.TextWidth(menutext[1])+10;
			short X = 20;
			Uint32 colour;

			if(ichosen!=i)
				if(currentscheme<3 && i>1)
					colour = SDL_MapRGB(world_pixels->format, 155, 155, 155);
				else
					colour = SDL_MapRGB(world_pixels->format, 255, 255, 255);
			else
					colour = colour = SDL_MapRGB(world_pixels->format, 255, 0, 0);

			draw_text(world_pixels, menutext[i], X, (i*12)+20, colour, Font.Info, Font.Style);
			if(i==0) draw_text(world_pixels, schemetext[currentscheme], X+RightJustOffset, (i*12)+20, colour, Font.Info, Font.Style);
			if(i==1 && invert) draw_text(world_pixels, ":Y", X+RightJustOffset, (i*12)+20, colour, Font.Info, Font.Style);
			if(i==1 && !invert) draw_text(world_pixels, ":N", X+RightJustOffset, (i*12)+20, colour, Font.Info, Font.Style);
			/*
			if(i>1) {
				char mentext[255];
				char mentext2[255];

				if(currentscheme==0) {
					sprintf(mentext,"%s",actiontext[getoptnum(gkbdefault1[i-2].set1,0)]);
					sprintf(mentext2,"%s",actiontext[getoptnum(gkbdefault1[i-2].set2,0)]);
				}
				if(currentscheme==1) {
					sprintf(mentext,"%s",actiontext[getoptnum(gkbdefault2[i-2].set1,0)]);
					sprintf(mentext2,"%s",actiontext[getoptnum(gkbdefault2[i-2].set2,0)]);
				}
				if(currentscheme==2) {
					sprintf(mentext,"%s",actiontext[getoptnum(gkbdefault3[i-2].set1,0)]);
					sprintf(mentext2,"%s",actiontext[getoptnum(gkbdefault3[i-2].set2,0)]);
				}
				if(currentscheme==3) {
					sprintf(mentext,"%s",actiontext[getoptnum(actionkey[i-2].set1,0)]);
					sprintf(mentext2,"%s",actiontext[getoptnum(actionkey[i-2].set2,0)]);
				}
				draw_text(world_pixels, mentext, X+(RightJustOffset/2), (i*12)+20, colour, Font.Info, Font.Style);
				draw_text(world_pixels, mentext2, (X+(RightJustOffset/2))+130, (i*12)+20, colour, Font.Info, Font.Style);
			}
			*/
		}
		

	/*	if(SDL_GetTicks()-lasttime>100) {
			unsigned long c = gp2x_joystick_read();
			int option=-1,option2=-1;
			
			//TODO MENU
			
			//if(c&GP2X_UP) ichosen--;
			//if(c&GP2X_DOWN) ichosen++;

			if(ichosen<0) ichosen=0;
			if(currentscheme<3 && ichosen>1) ichosen=1;
			if(ichosen>15) ichosen=15;

			if(ichosen>1) {
				option=getoptnum(actionkey[ichosen-2].set1,0);
				option2 = getoptnum(actionkey[ichosen-2].set2,0);
			}

			if(c&GP2X_LEFT) {
				SDL_FillRect(world_pixels,NULL,0);
				if(ichosen==0) currentscheme--;
				if(currentscheme<0) currentscheme=0;
				if(ichosen==1)
					if(invert) invert=false; else invert=true;
				if(ichosen>1) {
					option-=1;
				}
			} else if(c&GP2X_RIGHT) {
				SDL_FillRect(world_pixels,NULL,0);
				if(ichosen==0) currentscheme++;
				if(currentscheme>3) currentscheme=3;
				if(ichosen==1)
					if(invert) invert=false; else invert=true;
				if(ichosen>1) {
					option+=1;
				}
			} else if(c&GP2X_L) {
				if(ichosen>1) {
					SDL_FillRect(world_pixels,NULL,0);
					option2-=1;
				}				
			} else if(c&GP2X_R) {
				if(ichosen>1) {
					SDL_FillRect(world_pixels,NULL,0);
					option2+=1;
			}				

			}

			if(option<0) option=17;
			if(option>17) option=0;
			if(option2<0) option2=17;
			if(option2>17) option2=0;

			actionkey[ichosen-2].set1=actions[option];
			actionkey[ichosen-2].set2=actions[option2];

			if(c&GP2X_X) {
				// Need to run through and save the key config
				// Somehow
				if(currentscheme==0) {
					for(i=0;i<14;i++) {
						gp2xkeybindings[i].set1=gkbdefault1[i].set1;
						gp2xkeybindings[i].set2=gkbdefault1[i].set2;
					}
				} else if(currentscheme==1) {
					for(i=0;i<14;i++) {
						gp2xkeybindings[i].set1=gkbdefault2[i].set1;
						gp2xkeybindings[i].set2=gkbdefault2[i].set2;
					}
				} else if(currentscheme==2) {
					for(i=0;i<14;i++) {
						gp2xkeybindings[i].set1=gkbdefault3[i].set1;
						gp2xkeybindings[i].set2=gkbdefault3[i].set2;
					}
				} else {
					for(i=0;i<14;i++) {
						gp2xkeybindings[i].set1=actionkey[i].set1;
						gp2xkeybindings[i].set2=actionkey[i].set2;
					}
				}

				input_preferences->invert=invert;
				input_preferences->controlsetup=currentscheme;

				FILE *f;

				if ((f = fopen("./gp2x.conf","wb")) == NULL) {
				} else
				if (fwrite(&currentscheme,sizeof(uint16),1,f) != 1) {
				} else
				if (fwrite(&invert,sizeof(bool),1,f) != 1) {
				} else
					fwrite(gp2xkeybindings,sizeof(gp2xkey),16,f);


				fclose(f);

				ret=1;
				break;
			} else if(c&GP2X_START) {
				ret=0;
				break;
			}
			lasttime=SDL_GetTicks();
		}*/
	}

      _restore_port();
	//change_screen(640,480,16,false);
/*	zoom.x=160;
	zoom.y=0;
	zoom.w=320;
	zoom.h=240;
	SDL_GP2X_Display(&zoom);*/
	SDL_FillRect(world_pixels,NULL,0);
	SDL_Flip(world_pixels);

	return ret;
}

int do_ingame_menu() {
	//change_screen(320,240,16,false);
	/*SDL_Rect zoom;
	
	zoom.x=160;
	zoom.y=0;
	zoom.w=320;
	zoom.h=240;

	SDL_GP2X_Display(&zoom);
*/
    SDL_Surface *world_pixels = SDL_GetVideoSurface();
    short X0 = world_pixels->w;
    short Y0 = world_pixels->h;
	//char menutext[4][255]={"Save Game","Load Game","Options","Quit Game"};
	char menutext[2][255]={"Resume Game","Quit Game"};
	int ichosen=0;
	long lasttime=0;
	int ret=-1;

	SDL_FillRect(world_pixels,NULL,0);
      FontSpecifier& Font = GetOnScreenFont();
      _set_port_to_screen_window();

	int i;
	bool ok=false;

	{
		long d=SDL_GetTicks();
		while(SDL_GetTicks()-d<500) {}
	}

#ifdef PSP
	printf("PSP - Displaying PSP Ingame Menu\n");
#endif

	while(true) {
		for(i=0;i<2;i++) {
			short RightJustOffset = Font.TextWidth(menutext[i]);
			short X = (X0 - RightJustOffset)/2;

			if(ichosen!=i)
				draw_text(world_pixels, menutext[i], X, (i*20)+20, SDL_MapRGB(world_pixels->format, 255, 255, 255), Font.Info, Font.Style);
			else
				draw_text(world_pixels, menutext[i], X, (i*20)+20, SDL_MapRGB(world_pixels->format, 255, 0, 0), Font.Info, Font.Style);
		}


		//TODO
		/*if(SDL_GetTicks()-lasttime>200) {
			unsigned long c = gp2x_joystick_read();
			
			if(c&GP2X_UP) ichosen--;
			if(c&GP2X_DOWN) ichosen++;

			if(ichosen<0) ichosen=0;
			if(ichosen>3) ichosen=3;

			if(c&GP2X_X) {
				if(ichosen==2) { // Options
					do_options_menu();
				} else {
					switch(ichosen) {
						case 0: // Save Game
						ret=iSave;
						break;
						case 1: // L
						ret=iLoadGame;
						break;
						case 2:
						ret=0;
						break;
						case 3:
						ret=iCloseGame;
					}
					break;
				}
			} else if(c&GP2X_START) {
				ret=0;
				break;
			}
			lasttime=SDL_GetTicks();
		}*/
#ifdef PSP
		if(SDL_GetTicks()-lasttime>200) {
			SceCtrlData pad;
			sceCtrlSetSamplingCycle(0);
			sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
		
			sceCtrlReadBufferPositive(&pad, 1); 
			if (pad.Buttons != 0) 	{
				
				if (pad.Buttons & PSP_CTRL_UP) ichosen--;
				if (pad.Buttons & PSP_CTRL_DOWN) ichosen++;

				if (ichosen<0) ichosen=0;
				if (ichosen>1) ichosen=1;
				
				if (pad.Buttons & PSP_CTRL_CROSS) {
					//if(ichosen==2) { // Options
					//	do_options_menu();
					//} else {
						switch(ichosen) {
							case 0: // Save Game
							ret=0;
							break;
							case 1: // L
							ret=iCloseGamePSP;
							break;
					/*		case 2:
							ret=0;
							break;
							case 3:
							ret=iCloseGamePSP;*/
						}
						break;
				}
				
				if (pad.Buttons & PSP_CTRL_START) {
					ret=0;
					break;
				}
			}
			
			lasttime=SDL_GetTicks();
		}
	 }

#endif

      _restore_port();
/*	zoom.x=0;
	zoom.y=0;
	zoom.w=640;
	zoom.h=480;

	SDL_GP2X_Display(&zoom);*/
	clear_screen();
	recenter_mouse();
	//update_interface(NONE);
	return ret;
}

static void process_game_key(const SDL_Event &event)
{
//	unsigned long c = gp2x_joystick_read();

#ifdef PSP

	SceCtrlData pad;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
	int item = -1;
	
#endif

	switch (get_game_state()) {
		case _game_in_progress:
			/*if(c&GP2X_START) {
				int item = -1;
				pause_game();
				item=do_ingame_menu();
				{
					long d=SDL_GetTicks();
					while(SDL_GetTicks()-d<500) {}
				}
				resume_game();
				if (item > 0)
					do_menu_item_command(mGame, item, has_cheat_modifiers());
				break;
			}*/
#ifdef PSP
	
	sceCtrlReadBufferPositive(&pad, 1); 
	if (pad.Buttons != 0) 	{
		if (pad.Buttons & PSP_CTRL_START) {
			//pause_game();
			item = do_ingame_menu();
			printf("PSP - Choosen %d\n",item);
			//resume_game();
			if (item > 0) 
				do_menu_item_command(mGamePSP, item, has_cheat_modifiers());
			break;
		}
	}

#endif
			if ((event.key.keysym.mod & KMOD_ALT) || (event.key.keysym.mod & KMOD_META)) {
				int item = -1;
				switch (event.key.keysym.sym) {
					case SDLK_p:
						item = iPause;
						break;
					case SDLK_s:
						item = iSave;
						break;
					case SDLK_r:
						item = iRevert;
						break;
					case SDLK_c:
						item = iCloseGame;
						break;
// ZZZ: Alt+F4 is also a quit gesture in Windows
#ifdef __WIN32__
                    case SDLK_F4:
#endif
					case SDLK_q:
						item = iQuitGame;
						break;
				case SDLK_RETURN:
				  item = 0;
				  toggle_fullscreen();
				  break;
				default:
				  break;
				}
				if (item > 0)
#ifdef PSP
					do_menu_item_command(mGamePSP, item, has_cheat_modifiers());
#else
					do_menu_item_command(mGame, item, has_cheat_modifiers());
#endif
				else if (item != 0)
					handle_game_key(event);
			} else
				handle_game_key(event);
			break;

		case _display_intro_screens:
		case _display_chapter_heading:
		case _display_prologue:
		case _display_epilogue:
		case _display_credits:
		case _display_quit_screens:
			if (interface_fade_finished())
				force_game_state_change();
			else
				stop_interface_fade();
			break;

		case _display_intro_screens_for_demo:
			stop_interface_fade();
			display_main_menu();
			break;

		case _quit_game:
		case _close_game:
		case _revert_game:
		case _switch_demo:
		case _change_level:
		case _begin_display_of_epilogue:
		case _displaying_network_game_dialogs:
			break;

		case _display_main_menu: {
			if (!interface_fade_finished())
				stop_interface_fade();
			int item = -1;

			switch (event.key.keysym.sym) {
				case SDLK_n:
					item = iNewGame;
					break;
				case SDLK_o:
					item = iLoadGame;
					break;
				case SDLK_g:
					item = iGatherGame;
					break;
				case SDLK_j:
					item = iJoinGame;
					break;
				case SDLK_p:
					item = iPreferences;
					break;
				case SDLK_r:
					item = iReplaySavedFilm;
					break;
				case SDLK_c:
					item = iCredits;
					break;
// ZZZ: Alt+F4 is also a quit gesture in Windows
#ifdef __WIN32__
                case SDLK_F4:
#endif
				case SDLK_q:
					item = iQuit;
					break;
				case SDLK_F9:
					dump_screen();
					break;
			case SDLK_RETURN:
			  if ((event.key.keysym.mod & KMOD_META) || (event.key.keysym.mod & KMOD_ALT)) {
			    toggle_fullscreen();
			  }
			  break;
				default:
					break;
			}
			
				
			item = iNewGame;
			
			/*if(c&GP2X_START && c&GP2X_L && c&GP2X_R) {
				item=iQuit;
			} else if(c&GP2X_START)
				item=iNewGame;*/

			/*if(c&GP2X_UP || c&GP2X_DOWN) {
				short curmenu = getmenuitem();
			
				if(c&GP2X_DOWN) curmenu++;
				if(c&GP2X_UP) curmenu--;

				if(curmenu<0) curmenu=0;
				if(curmenu>3) curmenu=3;
				
				setmenuitem(curmenu);
				drawmainmenu();
			}*/
			/*
			if(c&GP2X_X) {
				switch(getmenuitem()) {
					case 0:
					item=iNewGame;
					//change_screen(640,480,16,false);
					SDL_Rect zoom;
	
					zoom.x=0;
					zoom.y=0;
					zoom.w=640;
					zoom.h=480;

					SDL_GP2X_Display(&zoom);
					break;
					case 1:
					item=iLoadGame;
					break;
					case 2:
					// Options
					do_options_menu();
					item=0;
					c=0;
					display_main_menu();
					
					//change_screen(320,240,16,false);
					break;
					case 3:
					item=iQuit;
				}
			}*/

			if (item > 0) {
				//draw_menu_button_for_command(item);
#ifdef PSP
				printf("PSP - Process Game Key Item : %d\n",item);
				do_menu_item_command(mInterfacePSP, item, has_cheat_modifiers());
#else
				do_menu_item_command(mInterface, item, has_cheat_modifiers());
#endif
			}
			break;
		}
	}
}

static void process_system_event(const SDL_Event &event)
{
	// In order to get music in Windows, we need to process
	// system events. DirectShow notifies about events through
	// window messages. 
#ifdef WIN32
#ifndef WIN32_DISABLE_MUSIC
	switch (event.syswm.msg->msg) {
		case WM_DSHOW_GRAPH_NOTIFY:
			process_music_event_win32(event);
			break;
	}
#endif
#endif
}

static void process_event(const SDL_Event &event)
{
	switch (event.type) {
		case SDL_JOYBUTTONDOWN:
			process_game_key(event);
			break;
		case SDL_MOUSEBUTTONDOWN:
			process_screen_click(event);
			break;

		case SDL_KEYDOWN:
			process_game_key(event);
			break;

		case SDL_SYSWMEVENT:
			process_system_event(event);
			break;

		case SDL_QUIT:
			set_game_state(_quit_game);
			break;

	case SDL_ACTIVEEVENT:
	  if (event.active.state & SDL_APPACTIVE) {
	    if (event.active.gain) {
	      update_game_window();
	    }
	  }
	}

/*	unsigned long c=gp2x_joystick_read();

	if(c&GP2X_LEFT || c&GP2X_RIGHT || c&GP2X_UP || c&GP2X_DOWN || c&GP2X_X || c&GP2X_Y \
			|| c&GP2X_A || c&GP2X_B || c&GP2X_L || c&GP2X_R || c&GP2X_SELECT \
			|| c&GP2X_VOL_UP || c&GP2X_VOL_DOWN)
		process_game_key(event);*/
}

/*
 *  Save screen dump
 */

void dump_screen(void)
{
	// Find suitable file name
	FileSpecifier file;
	int i = 0;
	do {
		char name[256];
		sprintf(name, "Screenshot_%04d.bmp", i);
		file = local_data_dir + name;
		i++;
	} while (file.Exists());

	// Without OpenGL, dumping the screen is easy
	SDL_Surface *video = SDL_GetVideoSurface();
	if (!(video->flags & SDL_OPENGL)) {
		SDL_SaveBMP(SDL_GetVideoSurface(), file.GetPath());
		return;
	}

#ifdef HAVE_OPENGL
	// Otherwise, allocate temporary surface...
	SDL_Surface *t = SDL_CreateRGBSurface(SDL_SWSURFACE, video->w, video->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	  0x000000ff, 0x0000ff00, 0x00ff0000, 0);
#else
	  0x00ff0000, 0x0000ff00, 0x000000ff, 0);
#endif
	if (t == NULL)
		return;

	// ...and pixel buffer
	void *pixels = malloc(video->w * video->h * 3);
	if (pixels == NULL) {
		SDL_FreeSurface(t);
		return;
	}

	// Read OpenGL frame buffer
	glReadPixels(0, 0, video->w, video->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	// Copy pixel buffer (which is upside-down) to surface
	for (int y = 0; y < video->h; y++)
		memcpy((uint8 *)t->pixels + t->pitch * y, (uint8 *)pixels + video->w * 3 * (video->h - y - 1), video->w * 3);
	free(pixels);

	// Save surface
	SDL_SaveBMP(t, file.GetPath());
	SDL_FreeSurface(t);
#endif
}


void LoadBaseMMLScripts()
{
	XML_Loader_SDL loader;
	loader.CurrentElement = &RootParser;
	{
		vector <DirectorySpecifier>::const_iterator i = data_search_path.begin(), end = data_search_path.end();
		while (i != end) {
			DirectorySpecifier path = *i + "MML";
			loader.ParseDirectory(path);
			path = *i + "Scripts";
			loader.ParseDirectory(path);
			i++;
		}
	}
}

#endif
