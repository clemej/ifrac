/*====================================================*/
/*  IFRAC configuration file                          */
/*====================================================*/

#include <stdlib.h>		/* This will declare NULL */
#include "ifr.h"


/*====================================================*/
/* Path to top scores file                            */
/*====================================================*/
/* const char *TopScoresFileName = "/var/games/ifrac.scores"; */
const char *TopScoresFileName = SCOREDIR "/" SCOREFILE;
const char *DefaultPlayListFileName = "~/.ifrac.bgl";

/*====================================================*/
/* Textmode clear screen sequence. 										*/		
/* Standard ANSI should normally be OK for a UNIX,		*/
/* so that you probably should leave it unchanged.    */
/*====================================================*/
const char *ClearScreenSequence = "\033[2J";

/*====================================================*/
/* Background base colours                            */
/*====================================================*/
const unsigned char base_colours[][3] = 
{
	{ 255,   0,  0},    /* Level 1 */
	{ 180, 180,  0},	/* Level 2 */
	{   0,   0, 255},	/* Level 3 */
	{ 255, 192, 203},	/* Level 4 */
	{   0, 180, 180},	/* Level 5 */
	{ 240, 100, 170},	/* Level 6 */ 
	{ 238, 130, 238},	/* Level 7 */
	{ 160,  32, 240},	/* Level 8 */
	{ 255, 165,   0},	/* Level 9 */
	{ 210, 104, 238},	/* Intro 1 */
	{  0,  255, 255},	/* Intro 2 */
	{  15, 191, 110}, /* Scores */
	{  64, 64,  196}, /* Statistics  */
};

/* Colours for "FRAC" in intro */
const unsigned char frac_rgb[(IntroStaticColours-2)*3] = 
{
    96,  96,    196,     /* H Scroller */
   250,  215,    0,      /* V Scroller */
   255,   0,    127,     /* 'F' */
    63,  255,    0,      /* 'R' */
    0,   255,   255,     /* 'A' */
   255,  255,    0,      /* 'C' */
     0,  176,    0 
};

/* Colours for lines and blocks */
/*
const unsigned char static_rgb[(StaticColours-3)*3] = 
{
   0xff, 0xff,   0,         // shadow  colour 

   //  BLOCKS 
     0,    0,  0x80,        // 1x1 - navy blue 
     0,    0,  0xff,        // 1x2 - light blue 
     0,  0xB0,   0,         // 1x3 - green 
   0xff,   0,  0xff,        // 1x4 - pink 
   0xC0,   0,    0,         // 2x1 - red 

   0xA0, 0xA0,   0,         // 2x2 - brown/ 
     0,  0xff, 0xff,        // 2x3 - cyan
     0,  0xff,   0,         // 3x1 - light green 
   0xff, 0xff,   0,         // 3x3 - yellow 
};
*/

unsigned char static_rgb[(StaticColours-3)*3] = 
{
   0xff, 0xff,   0,         /* shadow  colour */

   /*  BLOCKS */
     0,     0,  0x80,       /* 1x1 - magenta */
     0,     0,  0xFF,       /* 1x2 - blue */
     0,   0x80,    0,       /* 1x3 - green */
     0,   0xFF, 0xFF,       /* 1x4 - light cyan */
     0xFF,   0,    0,       /* 2x1 - light red */

    0xff, 0xff,    0,       /* 2x2 - yellow */ 
    0xff,    0, 0xff,       /* 2x3 - pink */
      0,  0xff,    0,       /* 3x1 - light green */
    0xA0, 0xA0, 0xA0,       /* 3x3 - light grey */
};


const unsigned char shadow_rgb[4*3] = 
{
   0xff, 0xff,   0, 
   0xff,  0,   0xff,
    0,   0xff,   0,
    0,    0,   0xff
};

#ifdef USE_JOYSTICK 

/* Joystick device name. NULL - don't use joystick */
const char *JoyDevName = "/dev/js0";
/* const char *JoyDevName = NULL; */

ACTION  JoyButtonDefActions[] ={ACT_TURNFWD, ACT_DROP };

const int JoyButtonDefActionsCount = sizeof(JoyButtonDefActions)/sizeof(ACTION);

#endif
