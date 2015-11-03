/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include "ifr.h"

#define ANIMATE_RELASE_DELAY1 150
#define ANIMATE_RELASE_DELAY2 200

extern GAME  game;
PLAYER  *players[MaxPlayers];
extern int LastSaveLayer;
extern const int	draw_events[], play_events[];

static void random_layer_fill(char *board, int count)
{
	int x, y, z;
	int rv1, rv[BoardDepth];
	int mask, orbits, andbits;


	mask = (1 << BoardWidth) - 1;

	for (z=0; z<count; z++)
	{
	  /* We don't allow an empty layer or a full layer */
		do 
		{  orbits = 0; andbits = mask;
			 for(y=0; y<BoardDepth; y++)
			 { rv1 = rv[y] = randbsd() & mask;
				 orbits |= rv1; andbits &= rv1;
			 }
		} while (orbits == 0 || andbits	== mask);

		for (y =0; y<BoardDepth; y++)
		{ rv1 = rv[y];
			for (x=0; x<BoardWidth; x++)
			{  *board++ = (rv1 & 1) ? rand_int(Tiles)+1 : 0; 
				 rv1 >>= 1;
			}
		} /* for y ... */
	}	  /* for z ... */

}

void player_start_game(PLAYER *player)
{
	int count;

    player->score = 0;
    player->layers = 0;
    player->status = 0;
	player->board_turn = 0;
	player->show_next_count = 0;
        
    memset (player->board, '\0', BoardDepth*BoardWidth*BoardHeight);
	count = game.start_values[1];
	if (count > 0)	random_layer_fill(player->board, count);
}

void  get_login_info(PLAYER *player)
{
		char *fullname = NULL;
		uid_t  uid=-1;
		struct passwd *pwdinfo;
		
		if (fullname == NULL)
		 	fullname = getenv("LOGNAME");
		
		if (fullname == NULL)
	  	fullname = getlogin();						

		uid = getuid();

		if (fullname)
		{	pwdinfo = getpwnam(fullname);
			if (pwdinfo) 
			{	fullname = pwdinfo->pw_gecos;
				uid = pwdinfo->pw_uid;
			}
		}
	
		player->uid = uid;

		if (fullname != NULL)
		{  char *nameend;	 
		   strncpy(player->fullname, fullname, FULL_NAME_SIZE);
		   player->fullname[FULL_NAME_SIZE-1] = '\0';
		   nameend = strchr(player->fullname, ',');
		   if (nameend) *nameend = '\0';
		   // No need to free fullname
		}

}

int player_initialize(int player_no)
{
    PLAYER   *player;
  
    player = malloc(sizeof(PLAYER));
    if (player == NULL) return 0;

		get_login_info(player);
    
    player->options = 0;
		player->score = -1;
		player->type = 0;

    players[player_no] = player;

    return 1;        
}

void player_deinitialize(int player_no)
{
    PLAYER   *player = players[player_no];
    
    if (player)
    {
//		  if (player->logname) free(player->logname);
//      if (player->fullname) free(player->fullname);

			free(player);
			players[player_no] = NULL;
    }	

}


void player_release_layers(PLAYER *player)
{
    char *source, *target;
    int  mask;
    int  capacity = BoardDepth * BoardWidth;
    int  source_count, released_count;
    const static int release_bonus[4] =
        { 100, 300, 600, 1000};        


    source_count = BoardHeight;
    released_count = 0;        

    source = target = player->board;
    mask = player->released_details;

    while(--source_count >= 0)
    {
	if (mask & 1)
	    released_count++;
	else		
	{ if (target != source)
	      memmove(target, source, capacity);
	  	       
    
           target += capacity;     
	}
	
        mask >>= 1, source += capacity;		    
    }

    
    source_count = released_count;

    while(--source_count >= 0)
    {   memset(target, '\0', capacity);
        target += capacity;     
    }       	    


    player->tile_code = 0;
    memset(player->tile_pos, '\0', sizeof(short)*Dimension);
    memset(player->tile_size, '\0', sizeof(short)*Dimension);

    player->layers += released_count;
    player->released_details = released_count;

    if (released_count)
    {	if(player->type == 0) player->status |= PLST_DELAY;
       draw_board(PT_FOREGROUND, player, 0);
			player->status &= ~PLST_DELAY;
    }	

/*    player->score += released_count * 100; */
    player->score += release_bonus[released_count-1];    
    update_score(PT_FOREGROUND, player->score, player->layers);

}


static void player_animate_release(PLAYER *player)
{
    int i;
    int mask = player->released_details;
    int need_break;
    unsigned long  next_clock;

    
    for (i=0; i<3; i++)
    { next_clock = start_clock(ANIMATE_RELASE_DELAY1);
	  need_break = (wait_clock(next_clock, player) != ACT_NONE);
	  if (! need_break) 
	  {
		LastSaveLayer = -1;
    	draw_board(PT_FOREGROUND, player, mask);
        next_clock = start_clock(ANIMATE_RELASE_DELAY2);
    	need_break = (wait_clock(next_clock, player) != ACT_NONE);
		LastSaveLayer = -1;
	  }	    
  	  draw_board(PT_FOREGROUND, player, 0);
	  if (need_break) break;
    }	

    

}


/* Return release mask */
void player_process_dropped_tile(PLAYER *player)
{	
    int  released_mask; 
    int  w, d, h, layer_count;
    int  layer_capacity;
    int  layer_mask;
    char *layer;
    
    released_mask = 0;
    layer_mask = 1;
    layer_capacity = BoardWidth * BoardDepth;
    layer = player->board;
    
    player->score += player->bonus;
    update_score(PT_FOREGROUND, player->score, player->layers);

    player->bonus = -1;
    update_bonus(PT_FOREGROUND, -1);

    for (h=0; h<BoardHeight; h++)
    {	layer_count = 0;
    
	for (d=0; d<BoardDepth; d++)
	  for (w=0; w<BoardWidth; w++)
	     if (*layer++) layer_count++;

	if (layer_count == layer_capacity)
	    released_mask |= layer_mask;
	else
	if (layer_count == 0)
	    break;
	    
	layer_mask <<= 1;	    
    }    

    player->released_details = released_mask;
    if (released_mask)
        player_animate_release(player);

}

        

