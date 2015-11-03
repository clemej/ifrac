/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <stdlib.h>
#include <time.h>
#include "ifr.h"

#ifdef USE_BACKGROUND_MUSIC 
extern MUSIC_STATUS mstatus;
#endif

extern GAME  game;
extern PLAYER  *players[MaxPlayers];
extern  int LastSaveLayer;


/* Timeout in millisecons */
static long time_outs[Levels+1] = {1200, 1000,  900, 700,  600,
                                    500,  350,  275,  200, 100};
//static long time_outs[Levels+1] = {800, 725,  650,  575,  500, 
//                                      425,  350,  275,  200, 100};

static long tile_counts[Levels-1] = {20, 40, 50, 60, 70, 
                                                80, 90, 100};

static long tile_sizes[Tiles][2] =
{  {1, 1}, {1,2}, {1,3}, {1,4},
   {2, 1}, {2,2}, {2,3}, {3,1}, {3,3} 
};


static int tile_frequencies[Tiles] =
  { 12, 11, 11, 4, 11, 8, 5, 5, 1};     /* ifrac  */
/*
  static int tile_frequencies[Tiles] =
  { 30, 60, 60, 6, 60, 40, 6, 15, 1};   // frac original
*/  
    
static int tile_total_frequency;
static int LevelTileCount;


static void select_next_tile(void)
{
    /* Tile code are counted from 1 */
    int choice;
    int code, freq;
    
    choice = rand_int(tile_total_frequency);

    code = 0; freq = tile_frequencies[code];

    while (choice >= freq)
    { choice -= freq;
			code ++;
			freq = tile_frequencies[code];
    }    

    game.next_tile_code = code+1;
    game.next_tile_turn = rand_int(Dimension);

}

    
void get_tile_size(int tile_code, int tile_rotation, short  tile_size_return[])
{
    int i;

    tile_code--;
    for (i=0; i<Dimension; i++) tile_size_return[i] = tile_sizes[tile_code][0];
    tile_size_return[tile_rotation] = tile_sizes[tile_code][1];

}

void reset_advance(void)
{
    game.next_adv = clock() + game.timeout;
}	

static void realize_next_tile(void)
{
    short  tile_pos[Dimension];	
    short  tile_size[Dimension];
    short  tile_code, tile_turn;
	PLAYER *player;
    
    int    i;

    tile_code = game.next_tile_code;
    tile_turn = game.next_tile_turn;

    get_tile_size(tile_code, tile_turn, tile_size);
	(game.stat[tile_code-1])++;
  
    /* Locate tile in the middle of well */
//    tile_pos[0] = (BoardWidth - tile_size[0]) >> 1;    
//    tile_pos[1] = (BoardDepth - tile_size[1]) >> 1;    
    tile_pos[0] = 0;
    tile_pos[1] = 0;
    tile_pos[2] = BoardHeight - tile_size[2];    

    select_next_tile();
    
    for (i=0; i<game.nplayers; i++)
		{	player = players[i];
	    if (player->type) ai_process_new_tile(player, tile_size);
			board_accept_new_tile(player, tile_code, tile_pos, tile_size);
		}

		reset_advance();    	
}

/* This function is activated when all 
   boards are in dropped state */
static void finalize_tile(void)
{
    PLAYER *player;
    int  need_release = 0;
    int  i;


    for (i=0; i<game.nplayers; i++)
    {
		player = players[i];
		player_process_dropped_tile(player);	/* Puts mask */
		if (player->released_details != 0) need_release = 1;
    }	    


    if (!need_release) return;
    
    for (i=0; i<game.nplayers; i++)
    {
		player = players[i];
		player_release_layers(player);  /* Replaces mask with detailes */
    }	    

	LastSaveLayer = -1;
	flush_input();    
} 

    
static void start_level(int level)
{

    int i;
    PLAYER *player;
    
    game.level   = level;
#ifdef USE_BACKGROUND_MUSIC 
		process_music();
#endif

    /* Convert milliseconds into ticks */
    game.timeout = (long)time_outs[level] * CLOCKS_PER_SEC / 1000;
    LevelTileCount = level < (Levels-1) ? tile_counts[level] : 0;

		    
    for (i=0; i<game.nplayers; i++)
    { 	player = players[i];
        draw_start_level(player, level);
				player->status |= (PLST_NEWLEVEL);		
    }
    
}



int game_start(void)
{  
    int i;
		PLAYER *player;

    game.status = 0;
    
    tile_total_frequency = 0;
    for (i=0; i<Tiles; i++)
		tile_total_frequency += tile_frequencies[i];
        
   
    for (i=0; i<game.nplayers; i++)
	{   player = players[i];
        if ((player->type == 0) || (player->status & PLST_QUITTED))
		{
		   if (!show_hiscores()) return 0;
		}
	}

    srandbsd(time(NULL));

    for (i=0; i<game.nplayers; i++)
	{   player = players[i];
		player_start_game(player);
	}

    for (i=0; i<Tiles; i++) game.stat[i] = 0;

    if (!draw_start_game()) return 0;
    start_level(game.start_values[0]);    

    select_next_tile();
    realize_next_tile();
        
    return 1;
}



void game_end(void)
{
		int i;
		PLAYER *player;

		game.plevel = game.level;
	
    for (i=0; i<game.nplayers; i++) 
		{	player = players[i];
		  display_statistics(player);
			if (player->type == 0)
			   process_new_score(player);
		}
}


    

/* Returns
    1 - need to finish game
    0 - continue 
*/    

int game_analyze(void)
{
    int i;
    int finish_count, drop_count;
    int status, bonus, nplayers; 
    PLAYER *player;
    clock_t cur_clock;


    nplayers = game.nplayers;
	
    
    finish_count = drop_count = 0;    
    for (i=0; i<nplayers; i++)
    {  
	player = players[i];
	status = player->status;

	if (status & PLST_CHANGED)
	{   draw_board(PT_FOREGROUND, player, 0);
	    player->status &= ~PLST_CHANGED;
	}
	
	if (status & PLST_DROPPED) drop_count++;
	if (status & PLST_FINISHED) finish_count++;

    }

    if (finish_count ==  nplayers) return 1;

    if (drop_count == nplayers)
    { 
		finalize_tile();    
		if (game.level< Levels-1 && --LevelTileCount <= 0)
	 	     start_level(game.level+1);
   	 	realize_next_tile();
    }
    
    cur_clock = clock();	
	
    
    while ((game.status & GMST_SUSPENDED) == 0 && cur_clock > game.next_adv)
    {
	  for (i=0; i<nplayers; i++)
	  {  	    	    
		player = players[i];

		if ((player->status & PLST_DROPPED) == 0)
	    { board_shift_h(player);
	  	  bonus = player->bonus;
	  	  if (--bonus >= 0) player->bonus = bonus;
	  	  draw_board(PT_FOREGROUND, player, 0);
			  update_bonus(PT_FOREGROUND, player->bonus);
	  	}	      

	  }
	    	
        game.next_adv += game.timeout;
    }
    

    player = players[0];
    if (player->status & PLST_SKIPLEVEL && game.level < Levels-1)
    {	start_level(game.level+1);
        player->status &= ~PLST_SKIPLEVEL;
    }	
    
	
    return 0;
}




