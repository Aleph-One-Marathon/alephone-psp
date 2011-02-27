#ifndef __PSP_MOUSE_SDL__
#define __PSP_MOUSE_SDL__

#include <pspctrl.h>
#include "SDL.h"

class PSPSDLMouse {

public:

	static const int 			MAX_MOUSE_BUTTONS = 5;
	static const int			MAX_PSP_BUTTONS = 22;
	static const float			DEFAULT_ACC = 0.2;
	static const float			DEFAULT_VMAX = 5.0f;
	static const int			DEFAULT_ATHD = 40; 

	static const PspCtrlButtons	PSP_BUTTONS_MAP[];

	enum PSPButtons {
		PSPB_SELECT = 0,
		PSPB_START,
		PSPB_UP,
		PSPB_RIGHT,
		PSPB_DOWN,
		PSPB_LEFT,
		PSPB_LTRIGGER,
		PSPB_RTRIGGER,
		PSPB_TRIANGLE,
		PSPB_CIRCLE,
		PSPB_CROSS,
		PSPB_SQUARE,
		PSPB_HOME,
		PSPB_HOLD,
		PSPB_NOTE,
		PSPB_SCREEN,
		PSPB_VOLUP,
		PSPB_VOLDOWN,
		PSPB_WLAN_UP,
		PSPB_REMOTE,
		PSPB_DISC,
		PSPB_MS
	};

	enum PSPSDLMouseMode {
		MOUSE_ANALOG,
		MOUSE_DIGITAL
	};

private:

	PSPSDLMouseMode 	mode;

	bool				buttons_state[MAX_PSP_BUTTONS];
	PspCtrlButtons		mouse_bindings[MAX_MOUSE_BUTTONS];

	float				mouse_x;
	float				mouse_y;

	float				mouse_vx;
	float				mouse_vy;

	float				mouse_acc;
	float				mouse_vmax;

	int					analog_threshold;

	SDL_Event			simulated_event;

	SceCtrlData 		psp_pad;
	unsigned int		psp_old_buttons;

public:

	PSPSDLMouse(PSPSDLMouseMode mode = MOUSE_DIGITAL,
				float acceleration = DEFAULT_ACC, 
				float vmax = DEFAULT_VMAX,
				int analog_threshold = DEFAULT_ATHD);

	void				update(void);
	
	void				bind_button(PspCtrlButtons psp_button, Uint8 mouse_button);
	PspCtrlButtons		get_button_binding(Uint8 mouse_button);

	PSPSDLMouseMode 	get_mouse_mode(void) { return mode; }

	float				get_mouse_x(void) { return mouse_x; }
	float				get_mouse_y(void) { return mouse_y; }
	
	float				get_mouse_vx(void) { return mouse_vx; }
	float				get_mouse_vy(void) { return mouse_vy; }
	float				get_mouse_vmax(void) { return mouse_vmax; }

	float				get_mouse_acceleration(void) { return mouse_acc; }
	
	int					get_analog_threshold(void) { return analog_threshold; }
		
	void				set_mouse_mode(PSPSDLMouseMode m) { mode = m; }
	
	void				set_mouse_acceleration(float acceleration) { mouse_acc = acceleration; }

	void				set_mouse_vx(float vx) { mouse_vx = vx; }
	void				set_mouse_vy(float vy) { mouse_vy = vy; }
	void				set_mouse_vmax(float vmax) { mouse_vmax = vmax; }

	void				set_analog_threshold(int thd) { analog_threshold = thd; }

	void				simulate_mouse_button(Uint8 button, bool state);
	void				simulate_mouse_move(float dx, float dy);
	void				simulate_mouse_position(float x, float y);

private:

	void				update_mouse();
	void				update_mouse_digital(void);
	void				update_mouse_analog(void);
	void				process_button_event(PspCtrlButtons button, bool state);
	unsigned int		get_button_id(PspCtrlButtons button);

};

#endif /* __PSP_MOUSE_SDL__ */
