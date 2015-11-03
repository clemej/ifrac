#include "ifr.h"

#ifdef USE_MOUSE
typedef struct 
{  int buttonNumber; ACTION act; } XBUTTONS;

XBUTTONS  IntergameButtons[] =			/* Used with human player only */
{
	{ 1,  ACT_TURNFWD},
	{ 3,  ACT_DROP}, 
	{ 2,  ACT_QUIT},
	{ 4,  ACT_BACK}, 
	{ 5,  ACT_FRONT}, 
	{ 6,  ACT_LEFT}, 
	{ 7,  ACT_RIGHT} 
};

const int IntergameButtonsCount = sizeof(IntergameButtons) / sizeof(XBUTTONS);

XBUTTONS  DemoButtons[] =			/* Used in demo mode */
{
	{ 1,  ACT_TURNFWD},
	{ 3,  ACT_QUIT}, 
	{ 2,  ACT_LEVEL} 
};

const int DemoButtonsCount = sizeof(DemoButtons) / sizeof(XBUTTONS);

XBUTTONS  PlayerButtons[] =			/* Used with human player only */
{
	{ 1,  ACT_TURNFWD},
	{ 3,  ACT_DROP},
	{ 2,  ACT_DOWN}, 
	{ 4,  ACT_BACK}, 
	{ 5,  ACT_FRONT}, 
	{ 6,  ACT_LEFT}, 
	{ 7,  ACT_RIGHT} 
};

const int PlayerButtonsCount = sizeof(PlayerButtons) / sizeof(XBUTTONS);

#ifdef USE_VOLUME_CONTROL
XBUTTONS  VolumeButtons[] =		
{
	{ 4,  ACT_VOLUP},
	{ 5,  ACT_VOLDOWN},
	{ 6,  ACT_VOLLEFT},
	{ 7,  ACT_VOLRIGHT}
};	

const int VolumeButtonsCount = sizeof(VolumeButtons) / sizeof(XBUTTONS);
#endif


XBUTTONS  ShiftButtons[] =	
{
	{ 1,  ACT_TURNBACK},
	{ 3,  ACT_DROP},
#ifdef USE_VOLUME_CONTROL
	{ 2,  ACT_DOWN}
#else
	{ 2,  ACT_DOWN},
	{ 4,  ACT_BACK}, 
	{ 5,  ACT_FRONT}, 
	{ 6,  ACT_LEFT}, 
	{ 7,  ACT_RIGHT} 
#endif

};

const int ShiftButtonsCount = sizeof(ShiftButtons) / sizeof(XBUTTONS);

XBUTTONS  EditButtons[] =			/* Used for entering top scorer */
{
	{ 1,  ACT_TURNBACK}, 			/* Turn back, finish editing */
	{ 3,  ACT_DROP}, 			/* Delete letter */ 		
	{ 2,  ACT_BOSS},
	{ 4,  ACT_LEFT}, 
	{ 5,  ACT_RIGHT}, 
	{ 6,  ACT_LEFT},
	{ 7,  ACT_RIGHT},
};
const int EditButtonsCount = sizeof(EditButtons) / sizeof(XBUTTONS);

#endif  // USE_MOUSE
