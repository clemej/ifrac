#include "kbcodes.h"
#include "ifr.h"

/*
   This table associates any of key1, key2
   and key3 with action act. Set key2 or key3
   to zero, if you need less keys. Case-
   insensitivity is provided by the code.

   You might wish to change the key layout,
   e.g. when using a non-QWERTY keyboard.
	
   This layout applies to console version - "cooked mode".
	 For raw mode see ifr_srkeys.c
   For GUI versions see ifr_xkeys.c
*/ 


typedef struct 
{  int key1; int key2; int key3; ACTION act; } SKEYS;


SKEYS  PlayerKeys[] =			/* Used with human player only */
{
  {KY_1, KY_END,   'Z', ACT_FRONTLEFT},			
  {KY_2, KY_DOWN,  'X', ACT_FRONT},
  {KY_3, KY_NEXT,  'C', ACT_FRONTRIGHT},
  {KY_4, KY_LEFT,  'A', ACT_LEFT},
  {KY_5, KY_BEGIN, 'S', ACT_TURNFWD},
  {KY_6, KY_RIGHT, 'D', ACT_RIGHT},

  {KY_7, KY_HOME,  'Q', ACT_BACKLEFT},
  {KY_8, KY_UP,    'W', ACT_BACK},
  {KY_9, KY_PREV,  'E', ACT_BACKRIGHT},

  {KY_0, KY_INSERT, 0,  ACT_DOWN},
  {KY_SPACE, 0,     0,  ACT_DROP},
  {KY_ENTER, 0,     0,  ACT_TURNBACK}
};

const int PlayerKeysCount = sizeof(PlayerKeys) / sizeof(SKEYS);

#ifdef USE_VOLUME_CONTROL 
SKEYS  VolumeKeys[] =		
{
  {KY_1, KY_END,   'Z', ACT_VOLDOWNLEFT},			
  {KY_2, KY_DOWN,  'X', ACT_VOLDOWN},
  {KY_3, KY_NEXT,  'C', ACT_VOLDOWNRIGHT},

  {KY_4, KY_LEFT,  'A', ACT_VOLLEFT},
  {KY_6, KY_RIGHT, 'D', ACT_VOLRIGHT},

  {KY_7, KY_HOME,  'Q', ACT_VOLUPLEFT},
  {KY_8, KY_UP,    'W', ACT_VOLUP},
  {KY_9, KY_PREV,  'E', ACT_VOLUPRIGHT}
}; 
const int VolumeKeysCount = sizeof(VolumeKeys) / sizeof(SKEYS);
#endif

SKEYS  ShiftKeys[] =			/* Used in Help/Pause/Status mode 
								   or with Shift or Alt down */
{

#ifndef USE_VOLUME_CONTROL 
  {KY_1, KY_END,   'Z', ACT_FRONTLEFT},			
  {KY_2, KY_DOWN,  'X', ACT_FRONT},
  {KY_3, KY_NEXT,  'C', ACT_FRONTRIGHT},

  {KY_4, KY_LEFT,  'A', ACT_LEFT},
  {KY_6, KY_RIGHT, 'D', ACT_RIGHT},

  {KY_7, KY_HOME,  'Q', ACT_BACKLEFT},
  {KY_8, KY_UP,    'W', ACT_BACK},
  {KY_9, KY_PREV,  'E', ACT_BACKRIGHT},
  
#endif		
  {KY_5, KY_BEGIN, 'S', ACT_TURNBACK},
  {KY_0, KY_INSERT, 0,  ACT_DOWN},
  {KY_SPACE, 0,     0,  ACT_DROP},
  {KY_ENTER, 0,     0,  ACT_TURNFWD}
};

const int ShiftKeysCount = sizeof(ShiftKeys) / sizeof(SKEYS);

SKEYS  CommonKeys[] =			/* Used in all cases */
{
  {KY_PLUS,  KY_EQUAL,  0,  ACT_ROTATEFWD},
  {'V',         0,      0,  ACT_REDRAW},
  {'J',         0,      0,  ACT_JOYSTICK},
  {'K',         0,      0,  ACT_KEYBOARD},
  {'L',         0,      0,  ACT_LEVEL},
  {'M',         0,      0,  ACT_MUSIC},
  {'N',         0,      0,  ACT_SHOWNEXT},
  {'P',        'H',     0,  ACT_PAUSE},
  {KY_BACK,   KY_ESC,   0,  ACT_QUIT},
  {KY_DELETE, KY_ERASE, 0,  ACT_QUIT},
  {KY_F1,      '`',    'O', ACT_BOSS},
  {KY_F2,      'T',     0,  ACT_STATUS},
  {KY_TAB,      0,      0,  ACT_TAB}
};

const int CommonKeysCount = sizeof(CommonKeys) / sizeof(SKEYS);

SKEYS  EditKeys[] =			/* Used for entering top scorer */
{
  { KY_LEFT,        0,   0,  ACT_LEFT},
  { KY_RIGHT,       0,   0,  ACT_RIGHT},
  { KY_HOME,        0,   0,  ACT_BACKLEFT},		/* Start of text */
  { KY_END,         0,   0,  ACT_FRONTLEFT},  /* End of text */
  { KY_BACK,   KY_ERASE, 0,  ACT_BACK},				/* Delete prev letter */
  { KY_DELETE,      0,   0,  ACT_DROP},			  /* Delete letter */
  { KY_ESC,         0,   0,  ACT_QUIT},				/* Clean */
  { KY_ENTER,       0,   0,  ACT_TURNBACK},   /* Turn back, finish editing */
  { KY_F1,          0,   0,  ACT_BOSS}
};

const int EditKeysCount = sizeof(EditKeys) / sizeof(SKEYS);

