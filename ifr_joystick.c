/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include "ifr.h"
#if defined(USE_JOYSTICK) && HAVE_LINUX_JOYSTICK_H 
#define JOYSTICK_ON
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/joystick.h>


#define joy_normalize_axis(vcur, vmin, vmax, vnormal) \
	  if (vcur < vmin) vmin = vcur;	 \
		if (vcur > vmax) vmax = vcur;  \
		vnormal = (vcur-vmin) * 3 / (vmax-vmin+1)

extern const char *JoyDevName;

static int joy_fd = -1;

#define JoyEventsMax 32767l
#define JoyEventsMin -32767l

#define BUTTONS_TIMEOUT1 100
#define BUTTONS_TIMEOUT2 400
#define AXES_TIMEOUT1 100
#define AXES_TIMEOUT2 150

long JoyXMin, JoyXMax;
long JoyYMin, JoyYMax;

static long joy_x, joy_y;
static int  joy_buttons;
static unsigned long btn_next_clock=0, axes_next_clock=0;
#endif

int JoyVersion = 0, JoyEventDriven = 0;
int	JoyInUse, JoyClassic;
int JoyButtonCount = 2;
char JoyTypeName[65] = "";
ACTION JoyButtonActions[JOY_MAX_BUTTON_COUNT];

#ifdef JOYSTICK_ON
static int joystick_read(void)
{
#if defined(USE_JOYSTICK_EVENTS) && defined(JS_EVENT_INIT)
	if (JoyEventDriven)
	{	struct js_event joyEvent;
    unsigned short eType, eNumber;	/* Actually 8 bits would be enough */
		signed short eValue;
		int mask;

		while(read(joy_fd, &joyEvent, sizeof(struct js_event)) > 0)
    {	eType = joyEvent.type; 
			eNumber = joyEvent.number;
			eValue = joyEvent.value;

		  switch(eType & ~JS_EVENT_INIT)
			{	case JS_EVENT_BUTTON:
					mask  = 1 << eNumber;
          if (eValue) joy_buttons |= mask;
   					   else   joy_buttons &= ~mask;
					break;
				
				case JS_EVENT_AXIS:
					if (eNumber == 0) joy_x = eValue;
		      else
					if (eNumber == 1) joy_y = eValue;
					break;
      }
    
		}
		return 1;
	}
	else
#endif
#ifdef JS_RETURN
	{ struct JS_DATA_TYPE js;

   	if (read (joy_fd, &js, JS_RETURN) != JS_RETURN)
	 	{	 close(joy_fd); joy_fd = -1;
			 return 0;
		}

		joy_x = js.x;
		joy_y = js.y;	
 		joy_buttons = js.buttons;	
	}
  return 1;
#else
  return 0;
#endif
}

/* No calibration for event-driven joystick */
static int joystick_calibrate(void)
{
	if (JoyEventDriven)
	{
		JoyXMin = JoyYMin = JoyEventsMin;
		JoyXMax = JoyYMax = JoyEventsMax;
	}
	else
	{
		JoyXMin = JoyYMin = 0;
    if (!joystick_read()) return 0;

		JoyXMax = joy_x * 2;
		JoyYMax = joy_y * 2;		
	}

	btn_next_clock = start_clock(BUTTONS_TIMEOUT1);
	axes_next_clock = start_clock(AXES_TIMEOUT1);

	return 1;
}




static ACTION joystick_process_buttons(void)
{		ACTION act = ACT_NONE;
		int  i;
		unsigned long timeout = BUTTONS_TIMEOUT1;

		if (!time_up(btn_next_clock)) return ACT_NONE;

		for (i=0; i<JoyButtonCount; i++)
		{ 	if (joy_buttons & (1<<i))
				{	act = JoyButtonActions[i];
					timeout = BUTTONS_TIMEOUT2;
					break;
				}
		}
	
	  btn_next_clock = start_clock_from(btn_next_clock, timeout);
		return act;
}
				
static ACTION joystick_process_axes(void)
{		int joy_normal_x, joy_normal_y;
		ACTION act;

		static const ACTION joy_actions[] =
			 { ACT_BACKLEFT, ACT_BACK,  ACT_BACKRIGHT,
				  ACT_LEFT,    ACT_NONE,  ACT_RIGHT,
			 	 ACT_FRONTLEFT, ACT_FRONT, ACT_FRONTRIGHT
		   };

		if (!time_up(axes_next_clock)) return ACT_NONE;

		joy_normalize_axis(joy_x, JoyXMin, JoyXMax, joy_normal_x);
		joy_normalize_axis(joy_y, JoyYMin, JoyYMax, joy_normal_y);

		act = joy_actions[joy_normal_y * 3 + joy_normal_x];
		axes_next_clock = start_clock_from(axes_next_clock, 
												  act == ACT_NONE ? AXES_TIMEOUT1 : AXES_TIMEOUT2);
		return act;
}
#endif


int joystick_start(void)
{
#ifdef JOYSTICK_ON
	if (JoyDevName == NULL || JoyInUse == 0) return 0;

	if (joy_fd >= 0) goto JoyCalibrate;
	joy_fd = 	open(JoyDevName, O_RDONLY);
	if (joy_fd < 0) return 0;

  JoyVersion = 0x000800;	
  JoyEventDriven = 0;
	JoyButtonCount = 2;

	strcpy(JoyTypeName, "UNKNOWN");

#ifdef JSIOCGVERSION
	if (ioctl(joy_fd, JSIOCGVERSION, &JoyVersion) >= 0)
  { 
		/* This check is inherited from itetris, and
       I sincerely don't remember what is special about 1.2.8 */

#ifdef USE_JOYSTICK_EVENTS
		JoyEventDriven = (JoyClassic == 0 && JoyVersion >= 0x010208);
		if (JoyEventDriven)
			fcntl(joy_fd, F_SETFL, O_NONBLOCK);
#endif

#ifdef JSIOCGNAME
    if (ioctl(joy_fd, JSIOCGNAME(sizeof(JoyTypeName)), &JoyTypeName) >= 0) ;
	 		  JoyTypeName[sizeof(JoyTypeName)-1] = '\0';
#endif

		if (JoyVersion <= 0x01020D && strstr(JoyTypeName, "nalog"))
			printf ("-----------------------------------------------------\n"
			        "Warning. This joystick driver might work improperly!\n"
			        "         Use driver 0.8.0, or upgrade to 1.2.14+\n"
			        "-----------------------------------------------------\n"
						 );			


#ifdef JSIOCGBUTTONS
    if (ioctl(joy_fd, JSIOCGBUTTONS, &JoyButtonCount) >= 0)
		{  if (JoyButtonCount < 2) JoyButtonCount = 2;
			 if (JoyButtonCount > JOY_MAX_BUTTON_COUNT)
							JoyButtonCount = JOY_MAX_BUTTON_COUNT;
		}
#endif

			
	}
#endif  /* def JSIOCGVERSION */

JoyCalibrate:
	if (joystick_calibrate()) return 1;
	
	close(joy_fd); joy_fd = -1;
#endif	// JOYSTICK_ON
	return 0;

}

void joystick_stop(void)
{
#ifdef JOYSTICK_ON
	if (joy_fd >= 0)
	{  close(joy_fd);
	   joy_fd = -1;
	}	
#endif
	return;
}

int joystick_status(void)
{
#ifdef JOYSTICK_ON
	return (joy_fd >= 0);
#else
	return -1;
#endif
}

ACTION joystick_interface_routine(PLAYER *player)
{	ACTION act = ACT_NONE;
	
#ifdef JOYSTICK_ON
	if ((player==NULL || player->type==0) && joy_fd >= 0) 
	{
	  joystick_read();			
		act = joystick_process_buttons();
		if (act == ACT_NONE)
			act = joystick_process_axes();
	}
#endif
	
	return act;

}

int get_joystick_axes(long *x, long *y, int *xp, int *yp)
{
#ifdef JOYSTICK_ON
	if (joy_fd >= 0)
	{	 *x = joy_x;		
		 *y = joy_y;
		 *xp = (int) (((joy_x - JoyXMin) * 100)  / (JoyXMax - JoyXMin));
		 *yp = (int) (((joy_y - JoyYMin) * 100)  / (JoyYMax - JoyYMin));
		 return 1;
	}
#endif
	return 0;
}

void flush_joystick(void)
{
#ifdef JOYSTICK_ON
/*
#if defined(USE_JOYSTICK_EVENTS) && defined(JS_EVENT_INIT)
	if (JoyEventDriven)
	{	struct js_event joyEvent;
		while(read(joy_fd, &joyEvent, sizeof(struct js_event)) > 0);
  }
#endif
*/	
	joy_buttons = 0;
	joy_x = (JoyXMin + JoyXMax) / 2;
	joy_y = (JoyYMin + JoyYMax) / 2;

#endif
	return;
}

