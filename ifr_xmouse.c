/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman  */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <ctype.h>

#include "ifr.h"

#ifdef USE_MOUSE

# ifdef USE_XDGA
# include <X11/extensions/xf86dga.h>
# endif

typedef struct 
{  int buttonNumber; ACTION act; } XBUTTONS;

extern XBUTTONS PlayerButtons[],  EditButtons[];
extern XBUTTONS IntergameButtons[], ShiftButtons[];
extern XBUTTONS DemoButtons[];
extern const int PlayerButtonsCount, EditButtonsCount;
extern const int IntergameButtonsCount, ShiftButtonsCount;
extern const int DemoButtonsCount;

extern  int	HelpMode;
extern  GAME game;
int MouseInUse;

const ACTION  MouseButtonDefActions[MOUSE_MAX_CUSTOM_BUTTONS] =
		{ACT_TURNFWD, ACT_DROP, ACT_DOWN};

ACTION  MouseButtonActions[MOUSE_MAX_CUSTOM_BUTTONS];

#ifdef USE_VOLUME_CONTROL
extern XBUTTONS VolumeButtons[];
extern const int VolumeButtonsCount;
# endif


void mouse_interface_start(void)
{
	int i;
	
	for (i=0; i<MOUSE_MAX_CUSTOM_BUTTONS; i++)
		PlayerButtons[i].act = MouseButtonActions[i];
}


	
static ACTION get_button_action(int buttonNumber, XBUTTONS table[], int tabsize)
{
	int i;
	
	for (i=0; i<tabsize; i++)
		if (buttonNumber == table[i].buttonNumber)
				  return table[i].act; 

	return ACT_NONE;
}		




ACTION process_button_event(PLAYER *player, XButtonEvent *evnt)
{
  ACTION act = ACT_NONE;
  int player_type = player && player->type;	
  int shift_state;
  
  int buttonNumber = evnt->button;
    
  if (HelpMode || game.level == SummaryLevel)
	shift_state = 1;
  else
	shift_state = evnt->state & (ShiftMask | Mod1Mask);	

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

ACTION process_button_event_edit(PLAYER *player, XButtonEvent *evnt)
{
	return  get_button_action(evnt->button, EditButtons, EditButtonsCount);
}



#  ifdef USE_XDGA
void  ConvertXDGAButtonEventToXButtonEvent(XDGAButtonEvent *dgaEvent, XButtonEvent *xEvent)
{
	xEvent->type = dgaEvent->type;
	xEvent->serial = dgaEvent->serial;
	xEvent->display = dgaEvent->display;
	xEvent->send_event = False;
	xEvent->root = xEvent->window = RootWindow(dgaEvent->display, dgaEvent->screen);
	xEvent->time = dgaEvent->time;
	xEvent->subwindow = None;
	xEvent->x = 0;
	xEvent->y = 0;
	xEvent->x_root = 0;
	xEvent->y_root = 0;
	xEvent->state = dgaEvent->state;
	xEvent->button = dgaEvent->button;
}
#  endif



#endif  // USE_MOUSE




