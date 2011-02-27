/*

	Copyright (C) 1991-2001 and beyond by Bo Lindbergh
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


Sept-Nov 2001 (Woody Zenfell): approximate emulations of originally Mac OS-only routines for
    the SDL dialog system, lets us share more code.  Using API that's a bit more specific, so
    we can split what were originally single functions into several related ones.  Mac OS
    implementation of these "split" functions is still handled by the original function.

Feb 27, 2002 (Br'fin (Jeremy Parsons)):
	Added utility routine GetListBoxListHandle for Carbon
*/

#include <string> /* Prefix header doesn't do this? */
#include <vector>

#ifndef _CSERIES_DIALOGS_
#define _CSERIES_DIALOGS_

#define iOK					1
#define iCANCEL				2

#ifdef SDL_RFORK_HACK
//AS thinks this is a great hack
#define dialog CHEESEOFDEATH
#define DialogPtr mDialogPtr
#include <Dialogs.h>
#undef dialog
#undef DialogPtr
#endif 

#if defined(mac)

#ifdef USES_NIBS
typedef WindowRef DialogPTR;
#define	CONTROL_INACTIVE	0
#define	CONTROL_ACTIVE		1
#else
typedef DialogPtr DialogPTR;
#define CONTROL_INACTIVE	kControlInactivePart
#define CONTROL_ACTIVE		kControlNoPart
#endif

#else //!mac

class dialog;

typedef	dialog*	DialogPtr;
typedef dialog* DialogPTR;

#define	CONTROL_INACTIVE	0
#define	CONTROL_ACTIVE		1

#endif //!mac

// (jkvw) Prototypes for all platforms, to (hopefully?) get ourselves a usable common interface.

extern bool QQ_control_exists (DialogPTR dlg, int item);
extern void QQ_set_control_activity (DialogPTR dlg, int item, bool active);
extern void QQ_hide_control (DialogPTR dlg, int item);
extern void QQ_show_control (DialogPTR dlg, int item);

extern bool QQ_get_boolean_control_value (DialogPTR dlg, int item);
extern void QQ_set_boolean_control_value (DialogPTR dlg, int item, bool value);

extern int QQ_get_selector_control_value (DialogPTR dlg, int item);
extern void QQ_set_selector_control_value (DialogPTR dlg, int item, int value);
extern void QQ_set_selector_control_labels (DialogPTR dlg, int item, const std::vector<std::string> labels);
extern void QQ_set_selector_control_labels_from_stringset (DialogPTR dlg, int item, int stringset_id);

extern const std::string QQ_copy_string_from_text_control (DialogPTR dlg, int item);
extern void QQ_copy_string_to_text_control (DialogPTR dlg, int item, const std::string &s);
extern long QQ_extract_number_from_text_control (DialogPTR dlg, int item);
extern void QQ_insert_number_into_text_control (DialogPTR dlg, int item, long number);


// (ZZZ:) Prototypes for both platforms.  I think I wrote some of these (on both platforms)
// so we could use a common API for common operations.  Others I think already existed with
// Mac OS implementations, so I just wrote SDL implementations.
extern long extract_number_from_text_item(
	DialogPtr dlg,
	short item);

extern void insert_number_into_text_item(
	DialogPtr dlg,
	short item,
	long number);

extern void copy_pstring_from_text_field(
        DialogPtr dialog,
        short item,
        unsigned char* pstring);

extern void copy_pstring_to_text_field(
        DialogPtr dialog,
        short item,
        const unsigned char* pstring);

extern void copy_pstring_to_static_text(DialogPtr dialog, short item, const unsigned char* pstring);



// (ZZZ:) For Macs only (some more non-Mac stuff later, read on)
#ifdef mac
#define SCROLLBAR_WIDTH	16

enum {
	centerRect
};

extern void AdjustRect(
	Rect const *frame,
	Rect const *in,
	Rect *out,
	short how);

extern void get_window_frame(
	WindowPtr win,
	Rect *frame);

extern DialogPtr myGetNewDialog(
	short id,
	void *storage,
	WindowPtr before,
	long refcon);

extern pascal Boolean general_filter_proc(
	DialogPtr dlg,
	EventRecord *event,
	short *hit);
extern ModalFilterUPP get_general_filter_upp(void);

extern void set_dialog_cursor_tracking(
	bool tracking);

extern bool hit_dialog_button(
	DialogPtr dlg,
	short item);

extern short get_dialog_control_value(
        DialogPtr dialog,
        short which_control);

extern void modify_control(
	DialogPtr dlg,
	short item,
	short hilite,
	short value);
        
extern void modify_radio_button_family(
	DialogPtr dlg,
	short firstItem,
	short lastItem,
	short activeItem);

typedef void (*dialog_header_proc_ptr)(
	DialogPtr dialog,
	Rect *frame);

extern void set_dialog_header_proc(
	dialog_header_proc_ptr proc);

#endif//mac



// (ZZZ:) Now, some functions I "specialized" for SDL are simply forwarded to the original, more
// general versions on classic Mac...
// These more specific names show better what manipulation is desired.  Also, more importantly,
// they let us patch up some differences between the way the Mac OS handles things and the way
// Christian's dialog code handles things (e.g., a Mac OS selection control (popup) is numbered
// from 1, whereas a boolean control (checkbox) is numbered 0 or 1.  In Christian's code, all
// selection controls (w_select and subclasses, including the boolean control w_toggle) are
// indexed from 0).
// These functions use the Mac OS numbering scheme.
#ifdef mac
__inline__ void modify_selection_control(
	DialogPtr dlg,
	short item,
	short hilite,
	short value) {modify_control(dlg, item, hilite, value); }

__inline__ void modify_control_enabled(
	DialogPtr dlg,
	short item,
	short hilite) {modify_control(dlg, item, hilite, NONE); }

__inline__ void modify_boolean_control(
	DialogPtr dlg,
	short item,
	short hilite,
	short value) {modify_control(dlg, item, hilite, value); }
        
__inline__ short get_selection_control_value(
        DialogPtr dialog,
        short which_control) {return get_dialog_control_value(dialog, which_control); }
        
__inline__ bool get_boolean_control_value(
        DialogPtr dialog,
        short which_control) {return get_dialog_control_value(dialog, which_control); }

#else//!mac

// (ZZZ: Here are the prototypes for the specialized SDL versions.)
extern void modify_selection_control(
	DialogPtr dlg,
	short item,
	short hilite,
	short value);

extern void modify_control_enabled(
	DialogPtr dlg,
	short item,
	short hilite);

extern void modify_boolean_control(
	DialogPtr dlg,
	short item,
	short hilite,
	short value);
        
extern short get_selection_control_value(
        DialogPtr dialog,
        short which_control);
        
extern bool get_boolean_control_value(
        DialogPtr dialog,
        short which_control);
#endif//!mac



// ZZZ: (very) approximate SDL emulation of Mac OS Toolbox routines - see note in implementation
#ifndef mac
void HideDialogItem(DialogPtr dialog, short item_index);
void ShowDialogItem(DialogPtr dialog, short item_index);
#endif // !mac

#if defined(TARGET_API_MAC_CARBON)
#ifndef SDL
// JTP: Get the list manager handle for a listbox control
pascal OSStatus GetListBoxListHandle( ControlHandle control, ListHandle* list );
#endif
#endif

#endif//_CSERIES_DIALOGS_
