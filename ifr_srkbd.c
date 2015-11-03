/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <stdio.h>
#include <vgakeyboard.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ctype.h>
#include "ifr.h"


#if defined(HAVE_SYS_KD_H)
#include <sys/kd.h>
#elif defined(HAVE_LINUX_KD_H)
#include <linux/kd.h>
#endif

extern char  *board;
typedef struct 
{ int key1; int key2; ACTION act; } SRKEYS;

static  int  fd;
static  int  capslock;
static  int  shiftstate;
static  int  leds;

extern  GAME game;
extern  int	HelpMode;
extern  int	kbd_raw;
extern SRKEYS PlayerKeys[], ShiftKeys[], CommonKeys[], EditKeys[];
extern const int PlayerKeysCount, ShiftKeysCount, CommonKeysCount, EditKeysCount;
extern const unsigned char *AsciiTables[];
extern const int  AsciTableSizes[];
#ifdef USE_VOLUME_CONTROL
extern SRKEYS VolumeKeys[];
extern const int VolumeKeysCount;
#endif
#ifdef USE_MOUSE
extern int mouse_interface_start(void);
extern int mouse_interface_end(void);
extern ACTION mouse_interface_routine(PLAYER *player, int shift_state,
                                      int edit_mode);
#endif


static void kbd_get_leds(void)
{
	leds = 0;
#if defined(KDGETLED)
	if (fd >=0) ioctl(fd, KDGETLED, &leds);
#endif  	
	return;
}

static void kbd_toggle_capslock(void)
{
#if defined(KDSETLED)
	if (fd >=0)
	{ int stat = leds ^ LED_CAP;
		if (ioctl(fd, KDSETLED, stat) >=0) leds = stat;
	} 
#endif
	return;
}	

int kbd_interface_start(void)
{	
		kbd_raw = 1;
		capslock = 0;
		shiftstate = 0;

		fd = keyboard_init_return_fd();
		if (fd >= 0)
		{	kbd_get_leds();
			keyboard_translatekeys(TRANSLATE_CURSORKEYS | TRANSLATE_KEYPADENTER);
#ifdef USE_MOUSE			
			mouse_interface_start();
#endif			
 		  return 1;
		}
		return 0;
}

void kbd_interface_end(void)
{
#if defined(KDSETLED)
	if (fd >=0) ioctl(fd, KDSETLED, -1);
#endif  	
		keyboard_close();
#ifdef USE_MOUSE			
		mouse_interface_end();
#endif			
}


static ACTION get_kbd_action(char *buf, SRKEYS table[], int tabsize)
{		int i, key;

		
		for (i=0; i<tabsize; i++)
		{  if (buf[table[i].key1] ||
           ((key=table[i].key2) != 0 && buf[key])
			    )
					return  table[i].act; 
		}
		return ACT_NONE;
}

static ACTION process_kbd_buffer(PLAYER *player, char *buf)
{	int player_type = player && player->type;	
	ACTION act = ACT_NONE;
	int shift_state_local;

  if (buf[SCANCODE_LEFTALT] | buf[SCANCODE_RIGHTALT]) return act;	

  if (HelpMode || game.level == SummaryLevel)
		shift_state_local = 1;
	else
		shift_state_local = shiftstate;

#ifdef USE_VOLUME_CONTROL
  if (shift_state_local || (player_type > 0 && game.level < Levels))
	  act = get_kbd_action(buf, VolumeKeys, VolumeKeysCount);

  if (act == ACT_NONE && player_type == 0)
#else
  if (player_type == 0)
#endif
	{	
		if (shift_state_local)
	 		act = get_kbd_action(buf, ShiftKeys, ShiftKeysCount);
		else
		 	act = get_kbd_action(buf, PlayerKeys, PlayerKeysCount);
	}

	if (act == ACT_NONE) 
	{	act = get_kbd_action(buf, CommonKeys, CommonKeysCount);
		if(shift_state_local && act == ACT_ROTATEFWD) act = ACT_ROTATEBACK;
	}

	if  ((act == ACT_TAB || act == ACT_DROP) && shift_state_local)
			  act = ACT_BACKTAB;

	return act;
}

static int lookup_ascii(char *buf)
{	const unsigned char *tabptr;
	char  chr = 0;
	int i, tabptr_size, index = 0;

	if (buf[SCANCODE_LEFTSHIFT] | buf[SCANCODE_RIGHTSHIFT]) index++;
#ifdef LED_CAP
	if (leds & LED_CAP) index += 2;
#endif

	tabptr = AsciiTables[index];
	tabptr_size = AsciTableSizes[index];
	

	for(i=0; i< tabptr_size; i++)
	{	chr = *tabptr++;
		if (chr && buf[i]) break;
	}
	
	if (i >= tabptr_size) return 0;
	return 0x100 | chr;	
}


static ACTION process_kbd_buffer_edit(PLAYER *player, char *buf)
{		ACTION act;

    act = get_kbd_action(buf, EditKeys, EditKeysCount);

		if (act == ACT_NONE)
			act = (ACTION) lookup_ascii(buf);

		return act;

}



ACTION kbd_interface_routine(PLAYER *player, int edit_mode)
{	char *buf;
	ACTION act = ACT_NONE;

	if(keyboard_update() &&	
		(buf = keyboard_getstate()) != NULL) {

		int capslock_new = buf [SCANCODE_CAPSLOCK];
		if (capslock_new != capslock)
		{	 capslock = capslock_new;
			 if (capslock)  kbd_toggle_capslock();
		}
		shiftstate = buf[SCANCODE_LEFTSHIFT] | buf[SCANCODE_RIGHTSHIFT];
		//  |  buf[SCANCODE_LEFTALT] | buf[SCANCODE_RIGHTALT] ; 
		
		act = edit_mode ? process_kbd_buffer_edit(player, buf) :
                     process_kbd_buffer(player, buf);
	}

#ifdef USE_MOUSE
	if (act == ACT_NONE)
	   act = mouse_interface_routine(player, shiftstate, edit_mode);
#endif

	return act;
					
}

void flush_kbd(void)
{
	return;
}

