#define PSP_SCREEN_W		480		// PSP Screen Width
#define PSP_SCREEN_H		272		// PSP Screen Height
#define	PSP_SCREEN_W_4_3	362		// PSP Screen Width for 4:3 ratio
#define PSP_SCREEN_H_4_3	272		// PSP Screen Height for 4:3 ratio
#define PSP_TERMINAL_LINES	14		// PSP Lines per terminal page
#define PSP_VS_COUNT		1		// PSP Number of View Sizes

#ifndef __PSPDEFINESH__
#define __PSPDEFINESH__

// PSP: Used to keep track of when to restore the old palette
//      (ex. after reading terminal)
extern bool					PSP_must_restore_palette;

// PSP: Keep track if we are using fullscreen or HUD
//		(must be integrated with screen.h)
extern bool					PSP_display_hud;

// PSP: Palette to be restored
extern struct color_table 	PSP_palette_to_restore; 

#endif
