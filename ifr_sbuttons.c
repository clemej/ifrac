/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman   */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */
#include "ifr.h"


#ifdef USE_MOUSE
typedef struct 
{  int buttonNumber; ACTION act; } SBUTTONS;

SBUTTONS  IntergameButtons[] =			/* Used with human player only */
{
	{ 1,  ACT_TURNFWD},
	{ 3,  ACT_DROP},
	{ 2,  ACT_QUIT}, 
	{ 4,  ACT_LEFT}, 
	{ 5,  ACT_RIGHT},
	{ 10,  ACT_FRONT}, 
	{ 11,  ACT_BACK}, 
	{ 12,  ACT_LEFT}, 
	{ 13,  ACT_RIGHT}
};

const int IntergameButtonsCount = sizeof(IntergameButtons) / sizeof(SBUTTONS);

SBUTTONS  DemoButtons[] =			/* Used in demo mode */
{
	{ 1,  ACT_TURNFWD},
	{ 3,  ACT_QUIT}, 
	{ 2,  ACT_LEVEL} 
};

const int DemoButtonsCount = sizeof(DemoButtons) / sizeof(SBUTTONS);


SBUTTONS  PlayerButtons[] =	/* Used with human player only */
{	{ 1,  ACT_TURNFWD},
	{ 3,  ACT_DROP},
	{ 2,  ACT_DOWN}, 
	{ 4,  ACT_LEFT}, 
	{ 5,  ACT_RIGHT},
	{ 10,  ACT_FRONT}, 
	{ 11,  ACT_BACK}, 
	{ 12,  ACT_LEFT}, 
	{ 13,  ACT_RIGHT}
};

const int PlayerButtonsCount = sizeof(PlayerButtons) / sizeof(SBUTTONS);

#ifdef USE_VOLUME_CONTROL
SBUTTONS  VolumeButtons[] =		
{
	{ 4,  ACT_VOLLEFT},
	{ 5,  ACT_VOLRIGHT},
	{10,  ACT_VOLUP},
	{11,  ACT_VOLDOWN},
	{12,  ACT_VOLLEFT},
	{13,  ACT_VOLRIGHT}
};	

const int VolumeButtonsCount = sizeof(VolumeButtons) / sizeof(SBUTTONS);
#endif


SBUTTONS  ShiftButtons[] =	
{
	{  1,  ACT_TURNBACK},
	{  3,  ACT_DROP},
#ifdef USE_VOLUME_CONTROL
	{  2,  ACT_DOWN}
#else
	{  2,  ACT_DOWN},
        {  4,  ACT_LEFT},
        {  5,  ACT_RIGHT},
        { 10,  ACT_FRONT},
        { 11,  ACT_BACK},
        { 12,  ACT_LEFT},
        { 13,  ACT_RIGHT}
#endif
};	

const int ShiftButtonsCount = sizeof(ShiftButtons) / sizeof(SBUTTONS);

SBUTTONS  EditButtons[] =	/* Used for entering top scorer */
{
	{ 1,  ACT_TURNBACK}, 	/* Turn back, finish editing */
	{ 3,  ACT_DROP}, 	/* Delete letter */ 			
	{ 2,  ACT_BOSS},
	{ 4,  ACT_LEFT}, 
	{ 5,  ACT_RIGHT}, 
	{ 6,  ACT_LEFT},
	{ 7,  ACT_RIGHT},
};
const int EditButtonsCount = sizeof(EditButtons) / sizeof(SBUTTONS);

#endif  // USE_MOUSE

