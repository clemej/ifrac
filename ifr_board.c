/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

// 11-May-2000
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "ifr.h"

extern PLAYER *players[MaxPlayers];
extern int LastSaveLayer;
extern GAME game;

static int board_shift_wd(PLAYER *player, int dirw1, int dird1)
{
	int	 wleft, wright, wcomes, wgoes;
	int  dback, dfront, dcomes, dgoes;
	int  hlow,  hhigh, dirw, dird;
	int  i, j, h;
	int  turn;
  char *board;
  short code;

	turn = player->type ? 0 : player->board_turn;

	switch(turn)
	{ case 0:
			dirw = dirw1; dird = dird1;
			break;

	 case 1:
			dirw = dird1; dird = -dirw1; 
			break;

	 case 2:
			dird = -dird1; dirw = -dirw1;
			break;

	 default:
			dirw = -dird1; dird = dirw1; 
			break;
	}

	wleft = player->tile_pos[0];
  wright = wleft + player->tile_size[0];   
	dback = player->tile_pos[1];
  dfront = dback + player->tile_size[1];   
	hlow = player->tile_pos[2];
	hhigh = hlow + player->tile_size[2];

	/* Check boundaries */
  if (dirw > 0)
	{  wgoes = wleft;  wcomes = wright;
     if (wcomes >= BoardWidth) goto Stopper;
	}
	else
  if (dirw < 0)
	{  wgoes = wright-1; wcomes = wleft-1;
	   if (wcomes < 0) goto Stopper;	
  }	
	else
	   wcomes = wgoes = -1;
	
    if (dird > 0)
	{  dgoes = dback;  dcomes = dfront;
       if (dcomes >= BoardDepth) goto Stopper;
	}
	else
    if (dird < 0)
	{  dgoes = dfront-1; dcomes = dback-1;
	   if (dcomes < 0) goto Stopper;	
  }	
	else
	   dcomes = dgoes = -1;

	
	board = player->board;

  /* Justify shift in width direction */
  if (wcomes != wgoes)
  {	j = dfront + dird;
   	for (i = dback+dird; i<j; i++)
	  for (h=hlow; h<hhigh; h++)
		if (BoardCubie(board, wcomes, i, h) != 0) return 0;
	}

  /* Justify shift in depth direction */
  if (dcomes != dgoes)
  {	j = wright + dirw;
   	for (i = wleft+dirw; i<j; i++)
	  for (h=hlow; h<hhigh; h++)
  	if (BoardCubie(board, i, dcomes, h) != 0) return 0;
	}

	/* Now shift! */
  code = player->tile_code;

  if (wcomes != wgoes)
  {	for (i=dback; i<dfront; i++)
		for (h=hlow; h<hhigh; h++)
	  {  BoardCubie(board, wcomes, i+dird, h) = code;
	     BoardCubie(board, wgoes, i, h) = 0;
	  }
  }

  if (dcomes != dgoes)
  {	for (i=wleft; i<wright; i++)
		for (h=hlow; h<hhigh; h++)
	  {   BoardCubie(board, i+dirw, dcomes, h) = code;
	      BoardCubie(board, i, dgoes, h) = 0;
	  }
	}

  player->tile_pos[0] += dirw; 
  player->tile_pos[1] += dird; 
  player->status |= PLST_CHANGED;
  player->status &= ~PLST_DROPPED;
  return 1;

Stopper:
	flush_input();
	return 0;

}



int board_shift_h(PLAYER *player)
{
    int  hgoes, hcomes;
    int  d, w, d1, d2, w1, w2;
    char *board;
    short code;

    if (player->status & PLST_DROPPED) return 0;        

    hcomes = player->tile_pos[2]-1;    
    hgoes = hcomes+ player->tile_size[2];    
    if (hcomes < 0) goto Dropped;
    
    /* Check-out coming slice */
    w1 = player->tile_pos[0];   
    w2 = w1 + player->tile_size[0];   
    d1 = player->tile_pos[1];   
    d2 = d1 + player->tile_size[1];   

    board = player->board;

    for (d=d1; d<d2; d++)    
      for (w=w1; w<w2; w++)
        if (BoardCubie(board, w, d, hcomes))
			goto Dropped; 		    
    
    /* Now shift ! */
    code = player->tile_code;
    for (d=d1; d<d2; d++)
      for (w=w1; w<w2; w++)    
      {  BoardCubie(board, w, d, hgoes) = 0; 		    
		 BoardCubie(board, w, d, hcomes) = code;
      }

    player->tile_pos[2] = hcomes;
    player->status |= PLST_CHANGED;

    return 1;

Dropped:
    player->status |= (PLST_CHANGED | PLST_DROPPED);
    return 0;     		
}



/* Try to insert tile into specified
   postiton. Return 1 on success 
*/

static int check_tile_insert(const char *board, const short tile_pos[], const short tile_size[])
{
    short pos1[Dimension];
    int  w, d, h;
    
    for (w=0; w<Dimension; w++)
			pos1[w] = tile_pos[w]+ tile_size[w];

    /* I am rather lazy to implenent stack algorithm here */    
    for (h=tile_pos[2]; h<pos1[2]; h++)
      for (d=tile_pos[1]; d<pos1[1]; d++)      	     
				for (w=tile_pos[0]; w<pos1[0]; w++)
	  		 if (BoardCubie(board, w, d, h)) return 0;
    
    return 1;
}

/*
   This will insert/uninsert tile  
*/   

static void tile_insert(char *board, const short tile_pos[], const short tile_size[], int code)
{
    int  w, d, h;

    /* I am rather lazy to implenent stack algorithm here */    
    for (h=0; h<tile_size[2]; h++)
      for (d=0; d<tile_size[1]; d++)      	     
				for (w=0; w<tile_size[0]; w++)
	   BoardCubie(board, w+tile_pos[0], d+tile_pos[1], h+tile_pos[2]) = code;
    
}

int board_turn(PLAYER *player, int direction)
{	
    short old_size[Dimension], new_size[Dimension];
    short old_pos[Dimension], new_pos[Dimension];
    int max_size[Dimension], tmp;
    char *board;
    int i, size_changed;
    
    for (i=0; i<Dimension; i++)
    {   old_size[i] = player->tile_size[i];
				old_pos[i] = player->tile_pos[i];
    }	
	
    size_changed = 0;

    for (i=0; i<Dimension; i++)
    {
			new_size[i] = old_size[(i+Dimension+direction) % Dimension];
			if (new_size[i] == old_size[i])
    	    new_pos[i] = old_pos[i];
			else
			{	size_changed = 1;
				if (i<Dimension-1 || new_size[i] < old_size[i])
					new_pos[i] = old_pos[i] + (old_size[i] - new_size[i])/2;		
				else	    
					new_pos[i] = old_pos[i] + old_size[i] - new_size[i];
			}
		}    

    if (size_changed == 0) return 1;

    /* Adjust coordinates */

    max_size[0] = BoardWidth;
    max_size[1] = BoardDepth;
    max_size[2] = BoardHeight;
    
    for (i=0; i<Dimension; i++)
    {   if (new_pos[i] < 0) new_pos[i] = 0;
			else
			if (new_pos[i] > (tmp = (max_size[i]-new_size[i])))
	    new_pos[i] = tmp;
    }

    board = player->board;

    /* Remove old tile */    
    tile_insert(board, old_pos, old_size, 0);

    /* Check if we can turn, if not - revert */
    if (!check_tile_insert(board, new_pos, new_size))
    {	tile_insert(board, old_pos, old_size, player->tile_code);
			return 0;
    }

    /* Insert new tile */
    tile_insert(board, new_pos, new_size, player->tile_code);
    for (i=0; i<Dimension; i++)
    {	player->tile_pos[i] = new_pos[i];
    	player->tile_size[i] = new_size[i];
    }    
    
    player->status |= PLST_CHANGED;
    return 1;
    

}

int height_dropped(const char *board, const short *tile_pos, const short *tile_size)
{
    int w, d, h;
    short w1, d1, w2, d2;

    w1 = tile_pos[0];   
    w2 = w1 + tile_size[0];   
    d1 = tile_pos[1];   
    d2 = d1 + tile_size[1];   

    
    /* Find height to drop */
    for (h=tile_pos[2]-1; h>=0; h--)
      for (d=d1; d<d2; d++)	
				for (w=w1; w<w2; w++)
	    		if (BoardCubie(board, w, d, h)) goto OutOfLoop;

OutOfLoop:
		return ++h;
}

static void board_drop(PLAYER *player)
{
    char *board;
		int  h;
		const short *tile_size;
		short *tile_pos;

    if (player->status & PLST_DROPPED) return;        

		board = player->board;
		tile_pos = player->tile_pos;
		tile_size = player->tile_size;

    tile_insert(board, tile_pos, tile_size, 0);
		h = height_dropped(board, tile_pos, tile_size);

    tile_pos[2] = h;
    tile_insert(board, tile_pos, tile_size, player->tile_code);
    player->status |= (PLST_CHANGED | PLST_DROPPED);
        
}

int board_rotate(PLAYER *player, int direction)
{
	LastSaveLayer = -1;
	player->board_turn = (player->board_turn + 4 + direction) % 4;
	draw_board_turn(player);
	return 1;
}

void board_action(int player_no, ACTION action)
{
    PLAYER   *player = players[player_no];

    player->status &= ~PLST_NEWTILE;

    switch(action)
    {
		case ACT_BACKRIGHT:
		    if (!board_shift_wd(player, 1, -1) &&
		        !board_shift_wd(player, 1, 0))
		         board_shift_wd(player, 0, -1);
		    break;

		case ACT_BACK:
		    board_shift_wd (player, 0, -1);
	    	break;

		case ACT_BACKLEFT:
		    if (!board_shift_wd (player, -1, -1) &&
		        !board_shift_wd(player, -1, 0))
		         board_shift_wd(player, 0, -1);
				
		    break;

		case ACT_LEFT:
		    board_shift_wd (player, -1, 0);
	    	break;

		case ACT_RIGHT:
		    board_shift_wd(player, 1, 0);
		    break;
    
		case ACT_FRONTLEFT:
		    if (!board_shift_wd (player, -1, 1) &&
                !board_shift_wd(player, -1, 0))
		         board_shift_wd(player, 0, 1);
		    break;
    
		case ACT_FRONT:
		    board_shift_wd (player, 0, 1);
	    	break;
    
		case ACT_FRONTRIGHT:
	    	   if(!board_shift_wd (player, 1, 1) &&
		          !board_shift_wd(player, 1, 0))
		           board_shift_wd(player, 0, 1);
		   break;
        
		case ACT_DOWN:
		    board_shift_h (player);
		    break;
	    
		case ACT_TURNFWD:
		    board_turn (player, 1);
		    break;

		case ACT_TURNBACK:
		    board_turn (player, -1);
		    break;

		case ACT_ROTATEFWD:
		    board_rotate (player, 1);
		    break;

		case ACT_ROTATEBACK:
		    board_rotate (player, -1);
		    break;

		case ACT_DROP:
		case ACT_BACKTAB:
		    board_drop (player);
	    	break;
    
		case ACT_QUIT:
		    player->status |= PLST_FINISHED;
				player->status |= PLST_QUITTED;
		    break;

		case ACT_SHOWNEXT:
		    player->options ^= PLOP_SHOWNEXT;
		    show_next(PT_FOREGROUND, player);
		    break;

	  case ACT_REDRAW:
        player->status |= PLST_DELAY;
        LastSaveLayer = -1;
        draw_board(PT_FOREGROUND, player, 0);
        player->status &= ~PLST_DELAY;
        if (player->type == 0)
        { long score = player->score;    
          if (score < 50) score = 0; else score-= 50;
          draw_score_line(PT_FOREGROUND, SL_SCORE, score);
          player->score = score;
        }                    
        break;

		case ACT_LEVEL:
		    if (player_no==0)
 					player->status |= PLST_SKIPLEVEL;
	    	break;
/*
		case ACT_TAB:
				boss_routine();
				break;

		case ACT_PAUSE:
				pause_game();
				break;
*/
	    
		default:
			;	    				    	    
    
    }    
    
    
}

int board_accept_new_tile(PLAYER *player, short tile_code,
                          const short tile_pos[], const short tile_size[])
{
    char *board;

    
    if (player->status & PLST_FINISHED) return 0;
    
    player->status &= ~PLST_DROPPED;
    player->status |= PLST_NEWTILE;
    
    player->released_details = 0;
    player->bonus = (player->options & PLOP_SHOWNEXT) ? 15 : 20;

    board = player->board;

    if (!check_tile_insert(board, tile_pos, tile_size))
    {   player->status |= PLST_FINISHED;
        draw_board(PT_FOREGROUND, player, 0);
        return 0;
    }	    

    memcpy(player->tile_pos, tile_pos, sizeof(short)*Dimension);
    memcpy(player->tile_size, tile_size, sizeof(short)*Dimension);
    player->tile_code = tile_code;


    tile_insert(board, tile_pos, tile_size, tile_code);
    player->status |= PLST_CHANGED;

    if (player->options & PLOP_SHOWNEXT)
	        show_next(PT_FOREGROUND, player);

    update_bonus(PT_FOREGROUND, player->bonus);    
    draw_board(PT_FOREGROUND, player, 0);
    
    return 1;
}
