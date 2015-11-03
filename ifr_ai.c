/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <stdlib.h>
#include "ifr.h"

#define AI_QUANTUM_LEAP	30

#if HAVE_VALUES_H
#	include <values.h>
#elif HAVE_LIMITS_H
#	include <limits.h>
#endif

#ifndef MINLONG
#	ifdef LONG_MIN
#		define MINLONG LONG_MIN
# else
#		define MINLONG (1 << (SIZEOF_LONG-1))
# endif
#endif

extern GAME game;

/* Returns:
		> 0 number of neighbours
		= 0 no neighbours, and no push-under 
		-1 - push-under 
*/
long ai_side_neighbours(const char *board, int x, int y, int z,
																					 const short *tile_size)
{	 int y1, z1;
	 int sizey, sizez;
	 long count;

	 sizey = tile_size[1];
	 sizez = tile_size[2];

	 count = sizey * sizez;
	 
	 if (x>=0 && x<BoardWidth)
	 {  sizey += y; sizez += z;
		if (sizez > BoardHeight) sizez = BoardHeight;
		for (z1 = z; z1 < sizez; z1++)
		  for (y1 = y; y1 < sizey; y1++)
			if (BoardCubie(board, x, y1, z1) == 0) count--;
	 }
	
	 if (count == 0)
	 {  for (z1 = sizez; z1<BoardHeight; z1++)
		  for (y1 = y;  y1< sizey; y1++)
			if (BoardCubie(board, x, y1, z1)) return -1;
	 }	

	 return count;	
}


int ai_front_neighbours(const char *board, int x, int y, int z, const short *tile_size)
{	 int x1, z1;
	 int count = 0;
	 int sizex, sizez;

	 sizex = tile_size[0];
	 sizez = tile_size[2];

  	 count = sizex * sizez;

	 if (y>=0 && y<BoardDepth)
	 {  sizex += x; sizez += z;
		if (sizez > BoardHeight) sizez = BoardHeight;
		for (z1 = z; z1 < sizez; z1++)
		  for (x1 = x; x1 < sizex; x1++)
			if (BoardCubie(board, x1, y, z1)==0) count--;
	 }

	 if (count == 0)
	 {  for (z1 = sizez; z1<BoardHeight; z1++)
		  for (x1 = x;  x1< sizex; x1++)
			if (BoardCubie(board, x1, y, z1)) return -1;
	 }	

	 return count;	
}


int ai_bottom_neighbours(const char *board, int x, int y, int z, const short *tile_size)
{	 int x1, y1;
	 int count = 0;
	 int sizex, sizey;

	 sizex = tile_size[0];
	 sizey = tile_size[1];

	 if (z<0)
		count = sizex * sizey;
 	 else
	 {  count = 0;
		sizex += x; sizey += y;
		for (y1 = y; y1 < sizey; y1++)
		  for (x1 = x; x1 < sizex; x1++)
			if (BoardCubie(board, x1, y1, z)) count++;
	 }

	 return count;	
}


void ai_process_new_tile(PLAYER *player, const short *tile_size1)
{	short tile_pos[Dimension];	
	short tile_size[Dimension];
	int  bestx=0, besty=0, bestr=0, bestz=0;
	long best_rate, rate, neigh;
	long best_pux=0, best_puy=0;
	const char *board;

	int rot, x, y, z, t, new_winner;
	int sizex, sizey, sizez;
	int rotcount, pux, puy, pu1; 

	best_rate = MINLONG;
	board = player->board;

	player->move_next_clock = start_clock(AI_QUANTUM_LEAP*(Levels+1-game.level));

	rotcount = 1;
    x = tile_size[0] = tile_size1[0];
	for (y=1; y<Dimension; y++)
	{
		if ((tile_size[y] = tile_size1[y]) != x)
			rotcount = Dimension;
	}

	tile_pos[2] = BoardHeight;

	for (rot=0;;)
	{
		sizex = tile_size[0]; sizey = tile_size[1];
		sizez = tile_size[2];

		for (y=0; y<=BoardDepth-sizey; y++)
		{	tile_pos[1] = y;
			for (x=0; x<=BoardWidth-sizex; x++)
			{ tile_pos[0] = x;
			  new_winner = 0;
				
				z = height_dropped(board, tile_pos, tile_size);
				rate = 0;
//				rate = 4*ai_bottom_neighbours(board, x, y, z-1, tile_size);
				t = x-1; pux = 0;
				for (;;)
				{  neigh = ai_side_neighbours(board, t, y, z, tile_size);
					 if (neigh >= 0)
					 { rate += neigh; break; }
					 pux ++; t--;
				}
				t = x+sizex; pu1 = 0;
				for (;;)
				{  neigh = ai_side_neighbours(board, t, y, z, tile_size);
					 if (neigh >= 0)
					  { rate += neigh; break; }
					 pu1 ++;  t++;
				}
				if (pu1 > pux) 
				{ pux = pu1; rate += pux * 1000; }
				else
 				{ rate += pux * 1000;  pux = -pux; }

				t = y-1; puy = 0;
				for (;;)
				{  neigh = ai_front_neighbours(board, x, t, z, tile_size);
				   if (neigh >= 0)
					 { rate += neigh; break; }
					 puy ++; t--;
				}
				t = y+sizey; pu1 = 0;
				for (;;)
				{  neigh = ai_front_neighbours(board, x, t, z, tile_size);
					 if (neigh >= 0)
				     { rate += neigh; break; }
					 pu1 ++;  t++;
				}

				if (pu1 > puy) 
				{ puy = pu1;  rate += puy * 1000; }
				 else
				{ rate += puy * 1000;  puy = -puy; }
	  
				rate -= (3*z+sizez) *  200;
				rate -=  (sizex*sizey - ai_bottom_neighbours(board, x, y, z-1, tile_size))
									* 200;

				new_winner = (rate > best_rate); //  ||
//			               (rate == best_rate && rand_int(tile_size[2]) == 0);

				if (new_winner)
				{ bestx = x; besty = y; bestz = z;
					best_rate = rate;  bestr = rot;
					best_pux = pux; best_puy = puy;
			  }
			}
		}

		if (++rot >= rotcount) break;

		x = tile_size[0]; z = Dimension-1;
		for (y=0; y<z; y++)
		  tile_size[y] = tile_size[y+1];
		tile_size[z] = x;
	}

	player->best_pos[0] = bestx;
	player->best_pos[1] = besty;
	player->best_pos[2] = bestz;
	
	
	player->best_pu[0] = best_pux;
	player->best_pu[1] = best_puy;
	player->best_pu[2] = bestr;


}

ACTION ai_interface_routine(PLAYER *player)
{
	int i , factor, height_dif;
	int delta[2], dr;
	int rval;
	ACTION act = ACT_NONE;
	static int directions[] = 
		{ACT_BACKLEFT, ACT_BACK, ACT_BACKRIGHT,
		 ACT_LEFT,   ACT_NONE,   ACT_RIGHT,
		 ACT_FRONTLEFT, ACT_FRONT, ACT_FRONTRIGHT};					
		

	if  (!time_up(player->move_next_clock)) return act;
/*
	if ((player->status & PLST_DROPPED) == 0 &&
			 ++(player->show_next_count) > 100)
	{   rval = (player->options & PLOP_SHOWNEXT) ? 6 : 7;
		  player->show_next_count = 0;
			if(rand_int(rval) == 0) return ACT_SHOWNEXT;
	}
*/
	height_dif = player->tile_pos[2] - player->best_pos[2];


	if (height_dif <= 0)
	{  /* Push_under */
	   for (i=0; i<2; i++)
	   {  dr = player->best_pu[i];
		  dr = (dr > 0) ? 1 : (dr < 0) ? -1 : 0;
		  (player->best_pu[i]) -= dr;	
		  delta[i] = dr+1;
	   }
	   dr = delta[1]*3+delta[0];
	   return  directions[dr];			
	}
  	
 	factor = (Levels - 1 - game.level);
	i = height_dif - 3; if (i < 0) i = 0;  
	factor = (factor * i)/10;
	rval = (factor == 0) ? 0 : randbsd();

	if (player->best_pu[2] > 0)
	{
		if (norm_int(rval, factor) == 0)
		{  act = ACT_TURNFWD;
			 (player->best_pu[2])--;
		}
	}
	else
	{
		if (norm_int(rval, factor) == 0) 
		{	for (i=0; i<2; i++)
			{  dr = player->best_pos[i] - player->tile_pos[i];
				 delta[i] = (dr > 0) ? 2 : (dr < 0) ? 0 : 1;
			}
			dr = delta[1]*3+delta[0];
			act = directions[dr];			
			if (act == ACT_NONE && (rval & 1) ==0 )
				 act = (player->best_pu[0] || player->best_pu[1]) ? ACT_DOWN : ACT_DROP;
		}
	}
	
  player->move_next_clock = start_clock_from
      (player->move_next_clock, AI_QUANTUM_LEAP*(Levels+1-game.level));
	return act;

}

