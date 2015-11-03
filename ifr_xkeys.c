#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "ifr.h"

/*
   This table associates any of key1, key2
   and key3 with action act. Set key2 or key3
   to zero, if you need less keys. Case-
   insensitivity, XKB and XK_KP issues are
   maintained by the code.

   You might wish to change the key layout,
   e.g. when using a non-QWERTY keyboard.
	
   This layout applies to GUI (X11) version.
   For console (svgalib) version see ifr_srkeys.c (raw mode)
	 and ifr_skeys.c ("cooked" mode).
*/ 


typedef struct 
{  KeySym key1; KeySym key2; KeySym key3; ACTION act; } XKEYS;

XKEYS  PlayerKeys[] =			/* Used with human player only */
{	{ XK_1, XK_End,   XK_Z,  ACT_FRONTLEFT},
	{ XK_2, XK_Down,  XK_X,  ACT_FRONT},
	{ XK_3, XK_Next,  XK_C,  ACT_FRONTRIGHT}, 

	{ XK_4, XK_Left,  XK_A,  ACT_LEFT},
    { XK_5, XK_Begin, XK_S,  ACT_TURNFWD}, 
	{ XK_6, XK_Right, XK_D,  ACT_RIGHT},

	{ XK_7, XK_Home,  XK_Q,  ACT_BACKLEFT},
	{ XK_8, XK_Up,    XK_W,  ACT_BACK},
	{ XK_9, XK_Prior, XK_E,  ACT_BACKRIGHT},
	
	{ XK_0, XK_Insert, XK_KP_Insert,  ACT_DOWN},
    { XK_space, XK_KP_Space, 0, ACT_DROP},
	{ XK_Return, XK_KP_Enter,  0,  ACT_TURNBACK}

};

const int PlayerKeysCount = sizeof(PlayerKeys) / sizeof(XKEYS);

#ifdef USE_VOLUME_CONTROL
XKEYS  VolumeKeys[] =		
{	{ XK_1, XK_End,   XK_Z,  ACT_VOLDOWNLEFT},
	{ XK_2, XK_Down,  XK_X,  ACT_VOLDOWN},
	{ XK_3, XK_Next,  XK_C,  ACT_VOLDOWNRIGHT}, 

	{ XK_4, XK_Left,  XK_A,  ACT_VOLLEFT},
	{ XK_6, XK_Right, XK_D,  ACT_VOLRIGHT},

	{ XK_7, XK_Home,  XK_Q,  ACT_VOLUPLEFT},
	{ XK_8, XK_Up,    XK_W,  ACT_VOLUP},
	{ XK_9, XK_Prior, XK_E,  ACT_VOLUPRIGHT}
};	

const int VolumeKeysCount = sizeof(VolumeKeys) / sizeof(XKEYS);
#endif

XKEYS  ShiftKeys[] =			/* Used in Help/Pause/Status mode,
								   or with shift_key_down */
{
#ifndef USE_VOLUME_CONTROL
	{ XK_1, XK_End,   XK_Z,  ACT_FRONTLEFT},
	{ XK_2, XK_Down,  XK_X,  ACT_FRONT},
	{ XK_3, XK_Next,  XK_C,  ACT_FRONTRIGHT}, 

	{ XK_4, XK_Left,  XK_A,  ACT_LEFT},
	{ XK_6, XK_Right, XK_D,  ACT_RIGHT},

	{ XK_7, XK_Home,  XK_Q,  ACT_BACKLEFT},
	{ XK_8, XK_Up,    XK_W,  ACT_BACK},
	{ XK_9, XK_Prior, XK_E,  ACT_BACKRIGHT},

#endif

    { XK_5, XK_Begin, XK_S,  ACT_TURNBACK}, 
	{ XK_0, XK_Insert, XK_KP_Insert,  ACT_DOWN},
    { XK_space, XK_KP_Space, 0, ACT_DROP},
	{ XK_Return, XK_KP_Enter,  0,  ACT_TURNFWD}
};

const int ShiftKeysCount = sizeof(ShiftKeys) / sizeof(XKEYS);

XKEYS  CommonKeys[] =			/* Used in all cases */
{
	{ XK_plus,   XK_equal, XK_KP_Add,  ACT_ROTATEFWD},
	{ XK_V,    0,      0,    ACT_REDRAW},
	{ XK_J,    0,      0,    ACT_JOYSTICK},
	{ XK_K,    0,      0,    ACT_KEYBOARD}, 

	 {XK_L,       0,      0,   ACT_LEVEL},
	 {XK_M,       0,      0,   ACT_MUSIC},
	 {XK_N,       0,      0,   ACT_SHOWNEXT},
     {XK_P,     XK_H,     0,   ACT_PAUSE},
	 {XK_BackSpace, XK_Delete, XK_Escape,  ACT_QUIT},
     {XK_F1,   XK_grave, XK_O, ACT_BOSS},
	 {XK_F2,    XK_T,     0,   ACT_STATUS},
     {XK_Tab,     0,      0,   ACT_TAB}
};


const int CommonKeysCount = sizeof(CommonKeys) / sizeof(XKEYS);

XKEYS  EditKeys[] =			/* Used for entering top scorer */
{	{ XK_Left,     XK_KP_Left,   0,  ACT_LEFT},
	{ XK_Right,    XK_KP_Right,  0,  ACT_RIGHT},
	{ XK_Home,     XK_KP_Home,   0,  ACT_BACKLEFT},	  /* Start of text */
	{ XK_End,      XK_KP_End,    0,  ACT_FRONTLEFT},  /* End of text */
	{ XK_BackSpace,   0,         0,  ACT_BACK},		  /* Back */
	{ XK_Delete,   XK_KP_Delete, 0,  ACT_DROP},		  /* Delete letter */
	{ XK_Escape,      0,         0,  ACT_QUIT},		  /* Delete prev letter */
	{ XK_Return,   XK_KP_Enter,  0,  ACT_TURNBACK}, /* Turn back, finish editing */
    { XK_F1,       XK_KP_F1,     0,  ACT_BOSS}
};

const int EditKeysCount = sizeof(EditKeys) / sizeof(XKEYS);
