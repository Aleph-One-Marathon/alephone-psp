#include <pspctrl.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <SDL.h>

#include "psp_mouse_sdl.h"

const PspCtrlButtons PSPSDLMouse::PSP_BUTTONS_MAP[] =  {
	PSP_CTRL_SELECT,	PSP_CTRL_START,		PSP_CTRL_UP,
	PSP_CTRL_RIGHT,		PSP_CTRL_DOWN,		PSP_CTRL_LEFT,
	PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	PSP_CTRL_TRIANGLE,
	PSP_CTRL_CIRCLE,	PSP_CTRL_CROSS,		PSP_CTRL_SQUARE,
	PSP_CTRL_HOME,		PSP_CTRL_HOLD,		PSP_CTRL_NOTE,
	PSP_CTRL_SCREEN,	PSP_CTRL_VOLUP,		PSP_CTRL_VOLDOWN,
	PSP_CTRL_WLAN_UP,	PSP_CTRL_REMOTE,	PSP_CTRL_DISC,
	PSP_CTRL_MS
};

PSPSDLMouse::PSPSDLMouse(PSPSDLMouseMode mode, float acceleration, 
							float vmax, int analog_threshold)
{
	set_mouse_mode(mode);
	set_analog_threshold(analog_threshold);
	
	simulate_mouse_position(0, 0);

	set_mouse_vx(0);	
	set_mouse_vy(0);
	set_mouse_vmax(vmax);
	
	set_mouse_acceleration(acceleration);

	memset(mouse_bindings, 0, MAX_MOUSE_BUTTONS * sizeof(PspCtrlButtons));

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	
	psp_old_buttons = 0;
}

void PSPSDLMouse::update()
{
	sceCtrlReadBufferPositive(&psp_pad, 1); 
	
	/* Update only if needed */
	if (psp_pad.Buttons != psp_old_buttons) {

		for (int i=0; i < MAX_PSP_BUTTONS; i++) {
			bool new_state = (bool)(psp_pad.Buttons & PSP_BUTTONS_MAP[i]);
		
			if (new_state != buttons_state[i]) {
				process_button_event(PSP_BUTTONS_MAP[i], new_state);
				buttons_state[i] = new_state;			
			}
				
		}

		psp_old_buttons = psp_pad.Buttons;
	}

	update_mouse();
}

void PSPSDLMouse::update_mouse(void)
{
	if (get_mouse_mode() == MOUSE_DIGITAL)
		update_mouse_digital();
	else
		update_mouse_analog();
}

void PSPSDLMouse::update_mouse_digital(void) 
{

	if (buttons_state[PSPB_UP])
		set_mouse_vy(get_mouse_vy() - get_mouse_acceleration());

	if (buttons_state[PSPB_DOWN])
		set_mouse_vy(get_mouse_vy() + get_mouse_acceleration());

	if (buttons_state[PSPB_LEFT])
		set_mouse_vx(get_mouse_vx() - get_mouse_acceleration());

	if (buttons_state[PSPB_RIGHT])
		set_mouse_vx(get_mouse_vx() + get_mouse_acceleration());

	//Decelerate on the corresponding axis if no button was pressed

	if (!buttons_state[PSPB_UP] && !buttons_state[PSPB_DOWN]) {
		if (get_mouse_vy() > 0.0f)
			set_mouse_vy(get_mouse_vy() - get_mouse_acceleration());
		else if (get_mouse_vy() < 0.0f)
			set_mouse_vy(get_mouse_vy() + get_mouse_acceleration());
	}

	if (!buttons_state[PSPB_LEFT] && !buttons_state[PSPB_RIGHT]) {
		if (get_mouse_vx() > 0.0f)
			set_mouse_vx(get_mouse_vx() - get_mouse_acceleration());
		else if (get_mouse_vx() < 0.0f)
			set_mouse_vx(get_mouse_vx() + get_mouse_acceleration());
	}

	//We have to manually stick the speed to zero to avoid
	//the cursor from sliding when released (because of the
	//small amount of velocity that wasn't subtracted while
	// decelerating)

	if (fabs(get_mouse_vx()) > 0 && fabs(get_mouse_vx()) < get_mouse_acceleration())
		set_mouse_vx(0);
	
	if (fabs(get_mouse_vy()) > 0 && fabs(get_mouse_vy()) < get_mouse_acceleration())
		set_mouse_vy(0);

	//We also need to limit the speed to avoid the 
	// cursor from breaking the speed of light

	if (fabs(get_mouse_vx()) > get_mouse_vmax())
		set_mouse_vx(get_mouse_vmax() * (get_mouse_vx() > 0 ? 1 : -1));

	if (fabs(get_mouse_vy()) > get_mouse_vmax())
		set_mouse_vy(get_mouse_vmax() * (get_mouse_vy() > 0 ? 1 : -1));

	// Finally update the mouse based on the computed speed

	simulate_mouse_move(get_mouse_vx(), get_mouse_vy());
}

void PSPSDLMouse::update_mouse_analog(void)
{
	
	int ax = psp_pad.Lx - 127;
	int ay = psp_pad.Ly - 127;

	/* Update the mouse speed only if the stick's distance
		from its center exceeds a treshold to calibrate
		the stick
	*/

	if (fabs(ax) < get_analog_threshold())
		set_mouse_vx(0);
	else
		set_mouse_vx((ax*get_mouse_vmax())/127.0f);
	
	if (fabs(ay) < get_analog_threshold())
		set_mouse_vy(0);
	else
		set_mouse_vy((ay*get_mouse_vmax())/127.0f);

	simulate_mouse_move(get_mouse_vx(), get_mouse_vy());

}

unsigned int PSPSDLMouse::get_button_id(PspCtrlButtons button)
{
	for (int i=0; i< MAX_PSP_BUTTONS; i++)
		if (PSP_BUTTONS_MAP[i] == button)
			return i;

	return -1;
}

void PSPSDLMouse::process_button_event(PspCtrlButtons button, bool state)
{
	for (int i=0; i < MAX_MOUSE_BUTTONS; i++) {
		if (button == get_button_binding(i))
			simulate_mouse_button(i, state);
	}
}

void PSPSDLMouse::simulate_mouse_button(Uint8 button, bool state)
{

	simulated_event.type = (state ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP);
	simulated_event.button.type = simulated_event.type;
	simulated_event.button.which = 0;
	simulated_event.button.button = button;
	simulated_event.button.state = (state ? SDL_PRESSED : SDL_RELEASED);
	simulated_event.button.x = get_mouse_x();
	simulated_event.button.y = get_mouse_y();

	SDL_PushEvent(&simulated_event);
}

void PSPSDLMouse::simulate_mouse_move(float dx, float dy)
{
	simulate_mouse_position(get_mouse_x() + dx, get_mouse_y() + dy);
}

void PSPSDLMouse::simulate_mouse_position(float x, float y)
{
	mouse_x = x;
	mouse_y = y;

	SDL_Surface* screen = SDL_GetVideoSurface();
	if (screen == NULL) {
		printf("PSPSDLMouse: Can't get video surface (%s)\n", SDL_GetError());
		return;
	}

	//We need to constrain the cursor inside the video surface
	//and we need to zero the speed too if it went outside to avoid
	//the "sticks to the border" effect :)

	if (mouse_x < 0) { mouse_x = 0; mouse_vx = 0; }
	if (mouse_y < 0) { mouse_y = 0; mouse_vy = 0; }
	if (mouse_x >= screen->w) { mouse_x = screen->w-1; mouse_vx = 0; }
	if (mouse_y >= screen->h) { mouse_y = screen->h-1; mouse_vy = 0; }

	SDL_WarpMouse((Uint16)mouse_x, (Uint16)mouse_y);
}

void PSPSDLMouse::bind_button(PspCtrlButtons psp_button, Uint8 mouse_button)
{
	if (mouse_button > MAX_MOUSE_BUTTONS)
		return;

	mouse_bindings[mouse_button] = psp_button;
}

PspCtrlButtons PSPSDLMouse::get_button_binding(Uint8 mouse_button)
{
	if (mouse_button > MAX_MOUSE_BUTTONS)
		return (PspCtrlButtons)0;

	return mouse_bindings[mouse_button];
}

