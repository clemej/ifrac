/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <vga.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "kbcodes.h"
#include "ifr.h"

extern char  *board;
typedef struct 
{ int key1; int key2; int key3; ACTION act; } SKEYS;

extern  GAME game;
extern  int	HelpMode;
extern  int	kbd_raw;
extern SKEYS PlayerKeys[], ShiftKeys[], CommonKeys[], EditKeys[];
extern const int PlayerKeysCount, ShiftKeysCount, CommonKeysCount, EditKeysCount;
#ifdef USE_VOLUME_CONTROL
extern SKEYS VolumeKeys[];
extern const int VolumeKeysCount;
#endif

#ifdef USE_MOUSE
extern int mouse_interface_start(void);
extern int mouse_interface_end(void);
extern ACTION mouse_interface_routine(PLAYER *player,
                                      int shift_state, int edit_mode);
#endif

static int esc_pending = 0;


int kbd_interface_start(void)
{
	kbd_raw = 0;
	esc_pending = 0;
#ifdef USE_MOUSE
	mouse_interface_start();
#endif
  return 1;
}

void kbd_interface_end(void)
{
#ifdef USE_MOUSE
	mouse_interface_end();
#endif
	;
}


static int get_kbd_shift(void)
{
#ifdef TIOCLINUX
        u_char argp = 6;
        if (ioctl(STDOUT_FILENO, TIOCLINUX, &argp) >= 0)
                   return (u_short) argp;
#endif
        return 0x8000;
}

static ACTION get_key_action(int key, SKEYS table[], int tabsize)
{		int i;
		
		for (i=0; i<tabsize; i++)
			  if (key == table[i].key1 ||
            key == table[i].key2 ||  
            key == table[i].key3)
					return  table[i].act; 

		return ACT_NONE;
}

static ACTION process_key_pressed(PLAYER *player, int key)
{	int player_type = player && player->type;	
	int shift_state;
	ACTION act = ACT_NONE;

	if (key >= 'a' && key <= 'z') key -= ('a' - 'A');
 
    if (HelpMode || game.level == SummaryLevel)
		shift_state = 1;
	else
		shift_state = get_kbd_shift() & 9;	

#ifdef USE_VOLUME_CONTROL
  if (shift_state || (player_type > 0 && game.level < Levels))
	  act = get_key_action(key, VolumeKeys, VolumeKeysCount);

  if (act == ACT_NONE && player_type == 0)
#else
  if (player_type == 0)
#endif
  { if (shift_state)
	  act = get_key_action(key, ShiftKeys, ShiftKeysCount);
	else      
	  act = get_key_action(key, PlayerKeys, PlayerKeysCount);
  }	  

  if (act == ACT_NONE) 
	{	act = get_key_action(key, CommonKeys, CommonKeysCount);
		if(shift_state && act == ACT_ROTATEFWD) act = ACT_ROTATEBACK; 
	}

  if  ((act == ACT_TAB || act == ACT_DROP) && shift_state)
			  act = ACT_BACKTAB;

	return act;
}

static ACTION process_key_pressed_edit(PLAYER *player, int key)
{
	 		
//		if (isprint(key)) return (ACTION) (0x100 | key);
		if (key >= ' ' && key <= 0x100) return (ACTION) (0x100 | key);

		return  get_key_action(key, EditKeys, EditKeysCount);

}

int  get_kbd_press(void)
{
   int key;
   int buffer[10];
   int *buf;

	 if (esc_pending)
   {   key = KY_ESC;
       esc_pending = 0;
	 } 
   else
	 {   key = vga_getkey();
		   if (key == KY_ERASE) key = KY_BACK;
		   if (key != KY_ESC) return key;
	 }	

   /* Key == Esc look for special key */
   buf = buffer;
   while((key=vga_getkey()) != 0)
	 { if (key == KY_ESC)
		{ esc_pending = 1; break;}
      	   else  *buf++ = key;
	 }	
	 *buf = '\0';

   buf = buffer;
   /* Meta key of simple Esc */
   if ((key=*buf++) != 0x5b) return key ? 0x200 | key : KY_ESC; 
   /* Arrow keys */
   if ((key=*buf++) != 0x5b) return 0x100 | key;  
   /* Functional keys (F...) */

   return 0x300 | *buf;     

}


ACTION kbd_interface_routine(PLAYER *player, int edit_mode)
{	
	int key;
	ACTION act=ACT_NONE;

	key = get_kbd_press();

	if (key != 0)
	   act = edit_mode ? process_key_pressed_edit(player, key) :
                     process_key_pressed(player, key);
#ifdef USE_MOUSE
	if (act == ACT_NONE) {
		int shift_state = get_kbd_shift() & 9;	
		act = mouse_interface_routine(player, shift_state, edit_mode);
	}
#endif
	return act;
}

void flush_kbd(void)
{
	while (vga_getkey());
}

