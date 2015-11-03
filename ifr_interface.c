/*  =================================================== */
/*  Intelligent FRAC.	(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <time.h>
#include "ifr.h"

extern PLAYER *players[];
extern  int	HelpMode;

#ifdef USE_BACKGROUND_MUSIC 
extern  volatile MUSIC_STATUS mstatus;
#endif

/*
double	 Efficiency;

#define FRAME_COUNT_QUANTUM 10000
static  long FrameCount;
static  clock_t  FrameClockOrigin;
*/

int interface_start(void)
{
/*	FrameCount = 0;
	  Efficiency = .0; */
	joystick_start();	
	return kbd_interface_start();
}

void interface_end(void)
{
	joystick_stop();	
	kbd_interface_end();
}


int process_std_oper(ACTION act)
{	
	switch(act)
	{ case ACT_JOYSTICK:
		  joystick_start();
	    return 1;

		case ACT_KEYBOARD:
		  joystick_stop();
	    return 1;

		case ACT_PAUSE:
		  if (!HelpMode) pause_game();
	    return 1;

		case ACT_BOSS:
		  boss_routine();
	    return 1;

		case ACT_STATUS:
		  if (!HelpMode) show_status();
	    return 1;

#ifdef USE_BACKGROUND_MUSIC 
		case ACT_MUSIC:
				toggle_background_music();
			return 1;
#endif

#ifdef USE_VOLUME_CONTROL
		case ACT_VOLUPLEFT:
			change_volume(1, -1);
			return 1;

		case ACT_VOLUP:
			change_volume(1, 0);
			return 1;

		case ACT_VOLUPRIGHT:
			change_volume(1, 1);
			return 1;

		case ACT_VOLLEFT:
			change_volume(0, -1);
			return 1;

		case ACT_VOLRIGHT:
			change_volume(0, 1);
			return 1;

		case ACT_VOLDOWNLEFT:
			change_volume(-1, -1);
			return 1;

		case ACT_VOLDOWN:
			change_volume(-1, 0);
			return 1;

		case ACT_VOLDOWNRIGHT:
			change_volume(-1, 1);
			return 1;
#endif			

		default:
			;
	}	

	return 0;
}


ACTION interface_routine(PLAYER *player)
{	ACTION act;

/*
	if (++FrameCount >= FRAME_COUNT_QUANTUM)
	{	clock_t now = clock();
		if (now > FrameClockOrigin)
		{  Efficiency = (FrameCount * CLOCKS_PER_SEC) / (now - FrameClockOrigin); 		
			 FrameClockOrigin = now;
		}
		FrameCount = 0;
	}
*/
	
  act = joystick_interface_routine(player);

	if (act == ACT_NONE)	
		 act = kbd_interface_routine(player, 0);
 	if (act == ACT_NONE && player && player->type)
	    act = ai_interface_routine(player);

	if (process_std_oper(act))
			act = ACT_NONE;			

#ifdef USE_BACKGROUND_MUSIC 
	if (mstatus == MUSIC_RERUN)
			start_background_music();
#endif

	return act;
}

ACTION edit_interface_routine(PLAYER *player)
{	ACTION act = ACT_NONE;
	
	act = kbd_interface_routine(player, 1);

	if (act == ACT_BOSS)
	{	boss_routine();
		act = ACT_NONE;
	}	

	return act;
}

void flush_input(void)
{
	flush_joystick();
	flush_kbd();
}


unsigned long start_clock_from(unsigned long from, unsigned long interval)
{
    return from + (interval * CLOCKS_PER_SEC / 1000);
}

unsigned long start_clock(unsigned long interval)
{
    return start_clock_from(clock(), interval);
}

int time_up(unsigned long next_clock)
{
		return (clock() >= next_clock);
}


ACTION wait_clock(unsigned long next_clock, PLAYER *player)
{
    ACTION act;

	 do
    {
			act = interface_routine(player);
/*			if (act == ACT_QUIT || act == ACT_LEVEL || act == ACT_SHOWNEXT ||
			    act == ACT_TURNFWD || act == ACT_TURNBACK || act == ACT_DROP ) */
			if (act != ACT_NONE) return act;    
    
    }
    while (clock() < next_clock);

    return ACT_NONE;

}
