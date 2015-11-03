#include <vgakeyboard.h>
#include "ifr.h"

/*
   This table associates raw any of key1, key2
   with action act. Set key2 to zero, if you
   need less keys. Case-insensitivity is provided.

   You might wish to change the key layout,
   e.g. when using a non-QWERTY keyboard.
	
   This layout applies to console version - raw mode.
	 For "cooked" mode see ifr_skeys.c
   For GUI versions see ifr_xkeys.c
*/ 


typedef struct 
{  int key1; int key2; ACTION act; } SRKEYS;


SRKEYS  PlayerKeys[] =			/* Used with human player only */
{
    {SCANCODE_KEYPAD1, SCANCODE_Z, ACT_FRONTLEFT},			
    {SCANCODE_KEYPAD2, SCANCODE_X, ACT_FRONT},
    {SCANCODE_KEYPAD3, SCANCODE_C, ACT_FRONTRIGHT},

    {SCANCODE_KEYPAD4, SCANCODE_A, ACT_LEFT},
    {SCANCODE_KEYPAD5, SCANCODE_S, ACT_TURNFWD},
    {SCANCODE_KEYPAD6, SCANCODE_D, ACT_RIGHT},

    {SCANCODE_KEYPAD7, SCANCODE_Q, ACT_BACKLEFT},
    {SCANCODE_KEYPAD8, SCANCODE_W, ACT_BACK},
    {SCANCODE_KEYPAD9, SCANCODE_E, ACT_BACKRIGHT},

    {SCANCODE_KEYPAD0,      0,     ACT_DOWN},
    {SCANCODE_SPACE,        0,     ACT_DROP},
    {SCANCODE_ENTER,        0,     ACT_TURNBACK}
};

const int PlayerKeysCount = sizeof(PlayerKeys) / sizeof(SRKEYS);
#ifdef USE_VOLUME_CONTROL 
SRKEYS  VolumeKeys[] =	 
{	{SCANCODE_KEYPAD1, SCANCODE_Z, ACT_VOLDOWNLEFT},			
	{SCANCODE_KEYPAD2, SCANCODE_X, ACT_VOLDOWN},
 	{SCANCODE_KEYPAD3, SCANCODE_C, ACT_VOLDOWNRIGHT},

	{SCANCODE_KEYPAD4, SCANCODE_A, ACT_VOLLEFT},
	{SCANCODE_KEYPAD6, SCANCODE_D, ACT_VOLRIGHT},

  {SCANCODE_KEYPAD7, SCANCODE_Q, ACT_VOLUPLEFT},
	{SCANCODE_KEYPAD8, SCANCODE_W, ACT_VOLUP},
 	{SCANCODE_KEYPAD9, SCANCODE_E, ACT_VOLUPRIGHT}
};
const int VolumeKeysCount = sizeof(VolumeKeys) / sizeof(SRKEYS);
#endif

SRKEYS  ShiftKeys[] =	    /* Used in Help/Pause/Status mode 
        									   or with Shift or Alt down */
{
#ifndef USE_VOLUME_CONTROL 
  {SCANCODE_KEYPAD1, SCANCODE_Z, ACT_FRONTLEFT},			
	{SCANCODE_KEYPAD2, SCANCODE_X, ACT_FRONT},
  {SCANCODE_KEYPAD3, SCANCODE_C, ACT_FRONTRIGHT},

	{SCANCODE_KEYPAD4, SCANCODE_A, ACT_LEFT},
	{SCANCODE_KEYPAD6, SCANCODE_D, ACT_RIGHT},
 
	{SCANCODE_KEYPAD7, SCANCODE_Q, ACT_BACKLEFT},
	{SCANCODE_KEYPAD8, SCANCODE_W, ACT_BACK},
	{SCANCODE_KEYPAD9, SCANCODE_E, ACT_BACKRIGHT},
#endif		

	{SCANCODE_KEYPAD5, SCANCODE_S, ACT_TURNBACK},
	{SCANCODE_ENTER,     0,  ACT_TURNFWD},
	{SCANCODE_KEYPAD0,   0,  ACT_DOWN},
	{SCANCODE_SPACE,     0,  ACT_DROP},
};

const int ShiftKeysCount = sizeof(ShiftKeys) / sizeof(SRKEYS);

SRKEYS  CommonKeys[] =			/* Used in all cases */
{
	  {SCANCODE_KEYPADPLUS, SCANCODE_EQUAL, ACT_ROTATEFWD},
 	  {SCANCODE_V,              0,          ACT_REDRAW},
	  {SCANCODE_J,              0,          ACT_JOYSTICK},
		{SCANCODE_K,              0,          ACT_KEYBOARD},
    {SCANCODE_L,              0,          ACT_LEVEL},
    {SCANCODE_M,              0,          ACT_MUSIC},
		{SCANCODE_N,              0,          ACT_SHOWNEXT},
    {SCANCODE_P,         SCANCODE_H,      ACT_PAUSE},
    {SCANCODE_BACKSPACE, SCANCODE_ESCAPE, ACT_QUIT},
    {SCANCODE_REMOVE,         0,          ACT_QUIT},
    {SCANCODE_F1,        SCANCODE_GRAVE,  ACT_BOSS},
    {SCANCODE_O,              0,          ACT_BOSS},
    {SCANCODE_F2,        SCANCODE_T,      ACT_STATUS},
    {SCANCODE_TAB,            0,          ACT_TAB}
};

const int CommonKeysCount = sizeof(CommonKeys) / sizeof(SRKEYS);

SRKEYS  EditKeys[] =			/* Used for entering top scorer */
{	{ SCANCODE_CURSORLEFT,  0,  ACT_LEFT},
	{ SCANCODE_CURSORRIGHT, 0,  ACT_RIGHT},
	{ SCANCODE_HOME,        0,  ACT_BACKLEFT},		/* Start of text */
	{ SCANCODE_END,         0,  ACT_FRONTLEFT},  /* End of text */
	{ SCANCODE_BACKSPACE,   0,  ACT_BACK},				/* Delete prev letter */
	{ SCANCODE_REMOVE,      0,  ACT_DROP},			  /* Delete letter */
	{ SCANCODE_ESCAPE,      0,  ACT_QUIT},				/* Clean */
	{ SCANCODE_ENTER,       0,  ACT_TURNBACK},   /* Turn back, finish editing */
  { SCANCODE_F1,          0,  ACT_BOSS}
};

const int EditKeysCount = sizeof(EditKeys) / sizeof(SRKEYS);

/* Converts scan codes into ASCII */
const unsigned char AsciiTable[] = 
	{  0,   0, 
	  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t', 
	  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',  0,   0,
	  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,  '\\',
	  'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' '};

const unsigned char AsciiTableShift[] = 
	{  0,   0, 
	  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  0, 0, 
	  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',  0, 0,
	  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|',
	  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' '};

const unsigned char AsciiTableCaps[] = 
	{  0,   0, 
	  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t', 
	  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',  0, 0,
	  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\'', '`', 0, '\\',
	  'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, 0, 0, ' '};

const unsigned char AsciiTableCapsShift[] = 
	{  0,   0, 
	  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  0, 0, 
	  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}',  0,   0,
	  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\"', '~', 0,  '|',
	  'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', 0, 0, 0, ' '};

const unsigned char *AsciiTables[] =   {AsciiTable, AsciiTableShift,
                         AsciiTableCaps, AsciiTableCapsShift};
const int  AsciTableSizes[] = {sizeof(AsciiTable), sizeof(AsciiTableShift),
                         sizeof(AsciiTableCaps), sizeof(AsciiTableCapsShift)};


