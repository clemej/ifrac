/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <ctype.h>

#include "ifr.h"

#ifdef USE_XDGA
#include <X11/extensions/xf86dga.h>
#endif


extern Display	*disp;
extern Drawable  wnd;
extern char  *board;
extern CMAP_TYPE cm_type;
extern GAME  game;

static Atom    WMProtAtom, WMDelAtom;


#ifdef USE_XDGA
extern 	int	dgaEventBase, dgaErrorBase;
extern	int	dgaModeCurrent;
extern	int	scrno;
extern	DIR_GRAPHIC	DirGraphic;
#endif

void visualize_pane(PANE_TYPE which_pane, int x, int y, int wid, int hei);

typedef struct 
{ KeySym key1; KeySym key2; KeySym key3; ACTION act; } XKEYS;


extern  GAME game;
extern  int	HelpMode;
extern XKEYS PlayerKeys[], ShiftKeys[], CommonKeys[], EditKeys[];
extern const int PlayerKeysCount, ShiftKeysCount,  CommonKeysCount, EditKeysCount;

#ifdef USE_MOUSE
extern int MouseInUse;
extern ACTION process_button_event(PLAYER *player, XButtonEvent *evnt);
extern ACTION process_button_event_edit(PLAYER *player, XButtonEvent *evnt);
extern void mouse_interface_start(void);
extern ACTION process_top_screen_coordinate(int x, int y);
#ifdef USE_XDGA
extern void  ConvertXDGAButtonEventToXButtonEvent(XDGAButtonEvent *dgaEvent, XButtonEvent *xEvent);
#endif
#endif


#ifdef USE_VOLUME_CONTROL
extern XKEYS VolumeKeys[];
extern const int VolumeKeysCount;
#endif

int kbd_interface_start(void)
{
   int mask;	

#ifdef USE_XDGA
  if (dgaModeCurrent >= 0)
  {
	mask= KeyPressMask;
#ifdef	USE_MOUSE
	if (MouseInUse) mask |= ButtonPressMask;
#endif	 
	XDGASelectInput(disp, scrno,  mask); 
	flush_all_pane(PT_FOREGROUND);
  }
  else 
#endif
  {  
	mask = ExposureMask | KeyPressMask | 
	       StructureNotifyMask | SubstructureNotifyMask |
	       VisibilityChangeMask;
#ifdef	USE_MOUSE
	if (MouseInUse) mask |= ButtonPressMask;
#endif	 
	XSelectInput(disp, wnd, mask);

	WMProtAtom = XInternAtom(disp, "WM_PROTOCOLS", True);
	WMDelAtom = XInternAtom(disp, "WM_DELETE_WINDOW", True);
    
	if (WMProtAtom != (Atom)None && WMDelAtom != (Atom)None)
		XSetWMProtocols(disp, wnd, &WMDelAtom, 1);
	XMapWindow(disp, wnd);

		flush_all_pane(PT_FOREGROUND);

/*   if (cm_type == CM_PALETTE) */
		{	  XWarpPointer(disp, None, wnd, 0, 0, 0, 0, 10, 10);
/*      XSetInputFocus(disp, wnd, RevertToParent, CurrentTime);   */
    }    
  }
#ifdef	USE_MOUSE
	if (MouseInUse)	mouse_interface_start();
#endif	 
  return 1;
}

void kbd_interface_end(void)
{
	if ( 
#ifdef USE_XDGA
      dgaModeCurrent < 0 &&
#endif  
	    wnd != None)
    XUnmapWindow(disp, wnd);
}

static int process_nonkey_event(PLAYER *player, XEvent *evnt)
{
	if (XFilterEvent(evnt, wnd)) return 0;

  switch (evnt->type)
  {
		case  Expose:
		{
	    XExposeEvent *xee;
	    game.status &= ~GMST_SUSPENDED;
	    xee = (XExposeEvent *) evnt;
//		  if (xee->count == 0) 
	      flush_pane(PT_FOREGROUND, 
		     xee->x, xee->y, xee->width, xee->height);
		}	    
		break;
    
		case MapNotify:
	    game.status &= ~GMST_SUSPENDED;
	    break;
		
		case UnmapNotify:
			if (game.level < Levels && player->type == 0)
					game.status |= GMST_SUSPENDED;
	    break;
	
		case ClientMessage:
		{
	   	XClientMessageEvent *xcme;
	   	xcme = (XClientMessageEvent *)evnt;    
	   	if (xcme->message_type == WMProtAtom &&	
    	   xcme->data.l[0] == WMDelAtom)	
	  	  return 1;
		}
		break;	       		       	

	}
  return 0;

}


static ACTION get_key_action(KeySym ks, XKEYS table[], int tabsize)
{
	int i;

	for (i=0; i<tabsize; i++)
		if (ks == table[i].key1 ||
    	  ks == table[i].key2 ||  
        ks == table[i].key3)
				  return table[i].act; 

	return ACT_NONE;
}


static ACTION process_key_event(PLAYER *player, XKeyEvent *evnt)
{
  KeySym ks;
  ACTION act = ACT_NONE;
  int player_type = player && player->type;	
  int shift_state;
    
  ks = XLookupKeysym(evnt, 0);
/*    printf  ("KeySym = %lx\n", ks); */
     
  if (ks >= XK_a && ks <= XK_z)
	ks -= (XK_a - XK_A);		
  else
  if (ks >= XK_KP_Home && ks <= XK_KP_Begin)
	 		ks -= (XK_KP_Home - XK_Home);		

  if (ks >= XK_KP_0 && ks <= XK_KP_9)
	  ks -= (XK_KP_0 - XK_0);		
  else
	  if (ks == 0) ks = XK_Begin;		


  if (HelpMode || game.level == SummaryLevel)
	shift_state = 1;
  else
	shift_state = evnt->state & (ShiftMask | Mod1Mask);	

#ifdef USE_VOLUME_CONTROL
  if (shift_state || (player_type > 0 && game.level < Levels))
	  act = get_key_action(ks, VolumeKeys, VolumeKeysCount);

  if (act == ACT_NONE && player_type == 0)
#else
  if (player_type == 0)
#endif
  { if (shift_state)
	  act = get_key_action(ks, ShiftKeys, ShiftKeysCount);
	else      
	  act = get_key_action(ks, PlayerKeys, PlayerKeysCount);
  }	  

  if (act == ACT_NONE)
  {  act = get_key_action(ks, CommonKeys, CommonKeysCount);
		if(shift_state && act == ACT_ROTATEFWD) act = ACT_ROTATEBACK; 
  }

  if  ((act == ACT_TAB || act == ACT_DROP) && shift_state)
							 act = ACT_BACKTAB;

  return act;
}

static ACTION process_key_event_edit(PLAYER *player, XKeyEvent *xkey)
{	KeySym ks;
	char buffer[2];
	int  c;  

	c = XLookupString(xkey, buffer, 2, &ks, 0);
	if (c == 1)
	{  c = buffer[0];
//		if (!isprint(c))
		 if (!iscntrl(c)) return (ACTION) (0x100 | c);
	   
	}
	 
	return  get_key_action(ks, EditKeys, EditKeysCount);
}

ACTION kbd_interface_routine(PLAYER *player, int edit_mode)
{	
  XEvent  evnt;
  ACTION  act = ACT_NONE;
  int    evnt_count;
  ACTION (*process_key_fun)(PLAYER *, XKeyEvent *);
#ifdef USE_MOUSE 
  ACTION (*process_button_fun)(PLAYER *, XButtonEvent *);
#endif
	

/*	evnt_count = XPending(disp);  */
  evnt_count = XEventsQueued(disp, QueuedAfterReading);
  process_key_fun = edit_mode ? process_key_event_edit : process_key_event;

#ifdef USE_MOUSE 
  process_button_fun = edit_mode ? process_button_event_edit : process_button_event;
#endif

  while (--evnt_count >= 0)
  {  XNextEvent(disp, &evnt);
	
#ifdef USE_XDGA
	if (dgaModeCurrent >= 0) {
		if (evnt.type == (dgaEventBase + KeyPress))
		{ if (act == ACT_NONE)
		  {  XKeyEvent xk;
  		     XDGAKeyEventToXKeyEvent((XDGAKeyEvent *) &evnt, &xk);
        	 act = (*process_key_fun)(player, &xk);
		  }
		}
# ifdef USE_MOUSE		
		else
		if (MouseInUse && evnt.type == (dgaEventBase + ButtonPress))
		{ 
		  if (act == ACT_NONE)
		  {  XButtonEvent xb;
  		     ConvertXDGAButtonEventToXButtonEvent((XDGAButtonEvent *) &evnt, &xb);
        	 act = (*process_button_fun)(player, &xb);
		  }
		}
# endif
	}
	else
#endif	  		  
	if (evnt.type == KeyPress)
	{  if (act == ACT_NONE && !XFilterEvent(&evnt, wnd))
         act = (*process_key_fun)(player, (XKeyEvent *)&evnt);
	}
# ifdef USE_MOUSE		
	else
	if (MouseInUse && evnt.type == ButtonPress)
	{  if (act == ACT_NONE && !XFilterEvent(&evnt, wnd)) {
			XButtonEvent  *bevnt = (XButtonEvent *)&evnt;
			if (bevnt->button == 1 && game.level == ScoresLevel) {
				act = process_top_screen_coordinate(bevnt->x, bevnt->y);
			}	
			else
	        	act = (*process_button_fun)(player, bevnt);
		} 
	}
#endif	
	else
	if (process_nonkey_event(player, &evnt))
	{  act = ACT_QUIT; 
	   break;
	}	   
	
  }
    
  return act;
}

void flush_kbd(void)
{
	XEvent evnt;
	
	while (XCheckTypedEvent(disp, KeyPress, &evnt) == True 
#ifdef USE_XDGA
		   && XCheckTypedEvent(disp, dgaEventBase+KeyPress, &evnt) == True
#endif	
	      );

}
