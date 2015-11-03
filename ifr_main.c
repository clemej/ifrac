/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "ifr.h"

PLATFORM pltf;
GAME	game;

extern PLAYER *players[];
VIDEO_RES 	VideoRes = VRES_DEFAULT;
int		PrivateColours = 0;
int		ResumeMode = 0;  // 0 - auto, 1 - kill,
                         // 2 - resume and quit if none, 3 - never resume
int		HelpMode = 0;
int		NoIntro  = 0;
int		DirVideo  = 2;
int		ConsFont = 0;
int		VideoBpp = 8;
#ifdef USE_XDGA
DIR_GRAPHIC	DirGraphic = DGA_NOTUSED;
#endif

void termination_routine(int sigcode)
{
    int i;

    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);

    interface_end();
    draw_end();

    for (i=0; i<game.nplayers; i++)
    	player_deinitialize(i);

#ifdef USE_BACKGROUND_MUSIC 
    free_sound_list();
#endif
#ifdef USE_VOLUME_CONTROL
    deinit_volume_control();
#endif

   if(sigcode > 0) exit(4); 
}

int main(int argc, const char *argv[])
{
    ACTION act;
    int rc;
    int i;

    rc = process_arguments(argc, argv);
    if (rc > 0) return rc;
	
    if (ResumeMode < 3)
	{ 
      i = unboss_routine(ResumeMode ? ResumeMode-1 : 1);
      if (i == 0) return i;
	  if (i > 4 || ResumeMode) return 20;
    }		


	signal(SIGTERM, termination_routine);
	signal(SIGINT, termination_routine);
	
    game.nplayers = 1;

    for (i=0; i<game.nplayers; i++)
    if (!player_initialize(i)) goto TheEnd;

    if (!draw_start())  goto TheEnd;

#ifdef USE_BACKGROUND_MUSIC 
    load_sound_list();
#endif
#ifdef USE_VOLUME_CONTROL
		init_volume_control();
#endif
	
    if (!interface_start())  goto TheEnd;
	
    if (NoIntro == 0)	show_intro();


    while (1)
    { 
    	if (!game_start()) break;
	
    	while(1) 
      {
				act = interface_routine(players[0]);
				board_action(0, act);

				if (game_analyze()) break;
			}

			game_end();
    }

    rc = 0;

TheEnd:
    termination_routine(0);

    return rc;
}

