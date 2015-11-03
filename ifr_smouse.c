/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include "ifr.h"

#ifdef USE_MOUSE
#include <vga.h>
#include <vgamouse.h>
#include <stdio.h>	// NULL

#define WIDTH 640
#define HEIGHT 480

typedef struct 
{ int buttonNo; ACTION act; } SBUTTONS;

extern GAME game;
extern  int     HelpMode;

int MouseInUse;

const ACTION  MouseButtonDefActions[MOUSE_MAX_CUSTOM_BUTTONS] =
		{ACT_TURNFWD, ACT_DROP, ACT_DOWN};

ACTION  MouseButtonActions[MOUSE_MAX_CUSTOM_BUTTONS];


extern SBUTTONS PlayerButtons[], ShiftButtons[], DemoButtons[];
extern SBUTTONS IntergameButtons[], EditButtons[];
extern const int PlayerButtonsCount, ShiftButtonsCount, DemoButtonsCount;
extern const int IntergameButtonsCount, EditButtonsCount;
#ifdef USE_VOLUME_CONTROL
extern SBUTTONS VolumeButtons[];
extern const int VolumeButtonsCount;
#endif

int old_button_state;
const static char *MouseDevice="/dev/mouse";
static int has_wheel;
static int has_mouse;

int mouse_interface_start(void)
{

	struct MouseCaps caps;
	int type, i;

	has_mouse = 0;
	has_wheel = 0;

	if (!MouseInUse) return 0;
	
	type = vga_getmousetype();
	if (type == MOUSE_NONE ||
	    mouse_init((char *)MouseDevice, type,
	            MOUSE_DEFAULTSAMPLERATE) == -1)
		return 0;	

	has_mouse = 1;
	mouse_setxrange(0, WIDTH - 1);
        mouse_setyrange(0, HEIGHT - 1);
	mouse_setwrap(MOUSE_NOWRAP);

	if (mouse_getcaps(&caps) < 0)
		// Oops - old version
		has_wheel = (type == MOUSE_INTELLIMOUSE ||
			     type == MOUSE_IMPS2);	
	else	
		has_wheel = ((caps.info & MOUSE_INFO_WHEEL) != 0);	

	if(has_wheel)
	   mouse_setrange_6d(0,0, 0,0, 0, 0, -180,180, -180, 180, 0,0,
	         MOUSE_RXDIM | MOUSE_RYDIM);

	mouse_update();
	old_button_state = mouse_getbutton();

	for (i=0; i<MOUSE_MAX_CUSTOM_BUTTONS; i++)
		PlayerButtons[i].act = MouseButtonActions[i];

	return 1;
}

void mouse_interface_end(void)
{
	if (has_mouse) mouse_close();
}


static ACTION get_button_action(int buttonNo, SBUTTONS table[], int tabsize)
{
	int i;

	for (i=0; i<tabsize; i++)
	{
	 	if (buttonNo == table[i].buttonNo)
			return  table[i].act; 
	}	
		return ACT_NONE;
}


static ACTION process_button_pressed(PLAYER *player, int shift_state, int buttonNumber)
{
	int player_type = player && player->type;	
	ACTION act = ACT_NONE;

	if (HelpMode || game.level == SummaryLevel)
		shift_state = 1;

# ifdef USE_VOLUME_CONTROL
	if (shift_state || (player_type > 0 && game.level < Levels))
	  act = get_button_action(buttonNumber, VolumeButtons, VolumeButtonsCount);
	
	if (act == ACT_NONE)  
# endif
   {
  		if(game.level >= Levels || player_type == 0) {
	  		if (shift_state)
				act = get_button_action(buttonNumber, ShiftButtons, ShiftButtonsCount);
		    if (act == ACT_NONE) {
			  	if (game.level >= Levels)	
					act = get_button_action(buttonNumber, IntergameButtons, IntergameButtonsCount);
				else	
					act = get_button_action(buttonNumber, PlayerButtons, PlayerButtonsCount);
			}		
		} else	
		  act = get_button_action(buttonNumber, DemoButtons, DemoButtonsCount);
   }
	
	return act;
}

static ACTION process_button_pressed_edit(PLAYER *player, int button)
{
	 		
//		if (isprint(key)) return (ACTION) (0x100 | key);
	return  get_button_action(button, EditButtons, EditButtonsCount);

}

static int get_mouse_button(void)
{	
	int buttonNo;
	unsigned int state_new, state_old;

	mouse_update();
	state_new = mouse_getbutton();
	state_old = old_button_state;

	if (state_new != state_old) {

		old_button_state = state_new;
		buttonNo = 1;	
		while(state_new != 0) {
			if ((state_new & 1) != 0 && (state_old & 1) == 0) {
				if (buttonNo <= 3) buttonNo = 4-buttonNo;
				return buttonNo;
			}
			state_new >>= 1;		
			state_old >>= 1;		
			buttonNo++;
		}
	}

	if (has_wheel) 
	{
		int rx, ry;
		mouse_getposition_6d(NULL, NULL, NULL,
		                     &rx, &ry, NULL);
		mouse_setposition_6d(0, 0, 0, 0, 0, 0,
		                     MOUSE_RXDIM|MOUSE_RYDIM);

		if (rx > 0) return 10;
		else
		if (rx < 0) return 11;
		else
		if (ry > 0) return 12;
		else
		if (ry < 0) return 13;
	}
/*
	state_new = MOUSE_XDIM | MOUSE_YDIM;
	if (has_wheel) 
	{
		mouse_getposition_6d(&x, &y, NULL, &rx, &ry, NULL);
		state_new |= (MOUSE_RXDIM | MOUSE_RYDIM);
	}
	else
	{
		mouse_getposition_6d(&x, &y, NULL, NULL, NULL, NULL);
		rx = ry = 0;
	}

	mouse_setposition_6d(0, 0, 0, 0, 0, 0, state_new);

	if (y > 0 || rx > 0) return 10;
	else
	if (y < 0 || rx < 0) return 11;
	else
	if (x > 0 || ry > 0) return 12;
	else
	if (x < 0 || ry < 0) return 13;
*/
	return -1;
}	
	

ACTION mouse_interface_routine(PLAYER *player, int shift_state, int edit_mode)
{	
	int	btn;

	if (!has_mouse) return ACT_NONE;	

	btn = get_mouse_button();

	if (btn <= 0) return ACT_NONE;
	
	return edit_mode ?
		process_button_pressed_edit(player, btn) :
                process_button_pressed(player, shift_state, btn);
}

void flush_mouse(void) { ; }
#endif
