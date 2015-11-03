/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ifr.h"
#include "ifr_pixmap.h"

#define DRAW_BOARD_DELAY 40 //  90

#define BoardSaveLayer 4
int LastSaveLayer = -1;

extern PLATFORM pltf;
extern PLAYER  *players[];
extern GAME game;
extern const unsigned char static_rgb[]; 
extern const unsigned char shadow_rgb[]; 
extern const unsigned char base_colours[Levels][3];
extern int		FontHeight[2];
extern const char 	*MiscText[];

int  WndWidth, WndHeight;
int  ScrWidth, ScrHeight, ScrDepth;    	

const int  CubScrWidth=25, CubScrDepth=11, CubScrHeight=20;   
const int  CubStepX=1, CubStepY=1;
const int  ShadowGap = 9, ShadowHeight = 10;

int  BoardCornerX = 388, BoardCornerY = 330; 
int  ShowNextX = 168,  ShowNextY = 222;
int  OffsetX, OffsetY;

int  BoardAreaX, BoardAreaY, BoardAreaWidth, BoardAreaHeight;
int  CubScrDepthX, CubScrDepthY;   

int	stored_colours;
unsigned  short	current_base_colour[3];

extern char *mandel_xpm[], *hypoth_xpm[], *intlig_xpm[], *julia_xpm[];
MPMP_INFO mandel_mpmp_info, hypoth_mpmp_info, intlig_mpmp_info, julia_mpmp_info;

static short new_game;

static const char *BottomText[] =
        {"Press 'H' for help", "http://www.geocities.com/xifrac/", "xifrac@yahoo.com"};
const int BottomTextCount = sizeof(BottomText) / sizeof(char *);

/* which_pane:  0 - draw in background, 1 - draw in foreground
   cx, cy - screen corner coordinates
   hx, hy - step along horizontal axis
   vx, vy - step along vertical axis
   nx, xy - number of cells
   linecol - line colour entry,
   fillcol - fill colour entry( NO_COLOUR - don't fill)
   bgcol   - background  colour entry( use with stipple)
   which_stipple - 0 - solid fill, 1 - dark, 2 - light;
*/   
void draw_pane(PANE_TYPE which_pane,
		      int cx, int cy, int dx1, int dy1,
		      int dx2, int dy2, int n1, int n2,
		      COLENTRY linecol, COLENTRY fillcol,
    		  COLENTRY bgcol, STIPPLE_TYPE which_stipple)
{
    int i, x1, y1;    

    // Fill the backgound first //

    if (fillcol >= 0)
 	   fill_parallelogramm(which_pane, cx, cy, n1*dx1, n1*dy1,
     		                 n2*dx2, n2*dy2,
				           fillcol, bgcol, which_stipple);

    // Draw horizontal lines (<= is not a bug!) //
    x1 = cx; y1 = cy; 
    for (i=0; i<=n2; i++)
    {  draw_line(which_pane, x1, y1, n1*dx1, n1*dy1, linecol, NO_COLOUR, ST_NO_STIPPLE);
	   x1 += dx2;  y1 += dy2;
    }

    // Draw vertical lines (<= is not a bug!) //
    x1 = cx; y1 = cy; 
    for (i=0; i<=n1; i++)
    {  draw_line(which_pane, x1, y1, n2*dx2, n2*dy2, linecol, NO_COLOUR, ST_NO_STIPPLE);	
	   x1 += dx1;  y1 += dy1;
    }

}		      

static void clear_shadow(PANE_TYPE which_pane, COLENTRY fill_colour)
{
    int sy;

    // ---------Shadow back----------- //
    sy = BoardCornerY-CubScrHeight*BoardHeight-ShadowGap;
    draw_pane(which_pane, BoardCornerX, sy,
	             CubScrWidth, 0,
	             0, -ShadowHeight, 
	             BoardWidth, 1,
	             shadow_colour, fill_colour, NO_COLOUR, ST_NO_STIPPLE);

    // ---------Shadow side----------- //
    draw_pane(which_pane, BoardCornerX, sy,
	             -CubScrDepthX, CubScrDepthY,
	             0, -ShadowHeight, 
	             BoardDepth, 1,
	             shadow_colour, fill_colour, NO_COLOUR, ST_NO_STIPPLE);
}

void prepare_pane_background(void)
{
	PLAYER *player;
	
    // ---------Back pane----------- //
    draw_pane(PT_BACKGROUND, BoardCornerX, BoardCornerY,
	             CubScrWidth, 0,
	             0, -CubScrHeight,
	             BoardWidth, BoardHeight,
	             line_colour, NO_COLOUR, NO_COLOUR, ST_NO_STIPPLE);
    
    // ---------Side pane----------- //
    draw_pane(PT_BACKGROUND, BoardCornerX, BoardCornerY,
	            -CubScrDepthX, CubScrDepthY,
	             0, -CubScrHeight,
	             BoardDepth, BoardHeight,
	             line_colour, NO_COLOUR, NO_COLOUR, ST_NO_STIPPLE);

    // ---------Bottom pane----------- //
    draw_pane(PT_BACKGROUND, BoardCornerX, BoardCornerY,
	             CubScrWidth, 0,
	             -CubScrDepthX, CubScrDepthY,
 	             BoardWidth, BoardDepth,
	             line_colour, NO_COLOUR, NO_COLOUR, ST_NO_STIPPLE);

	clear_shadow(PT_BACKGROUND, NO_COLOUR);
	LastSaveLayer = -1;

	player = players[0];
	/* Show DEMO */
	if (player->type)
	{  	const char *text = MiscText[2];
		int  scalex = 1, scaley = 2;
		int width, height, x, y;

		width = text_width(FT_VAR, text, -1) * scalex;
		height = FontHeight[FT_VAR] * scaley;
		if (WndHeight >= 400)
		{	x = BoardCornerX + 63 - width/2,
			y = BoardCornerY - 298 - height/2 + FontHeight[FT_VAR];
			if (y<2) y = 2;
		}
		else
		{	x = ShowNextX - width - 116;
			y = ShowNextY + 20 - height/2;
		}
		show_shadow_text(PT_BACKGROUND, FT_VAR, text,
									 x, y, scalex, scaley, 0, 0,
        shadow_colour, NO_COLOUR, ST_NO_STIPPLE);
	}

	{
		int x, y, w, i;
		FONT_TYPE ft;	

		
		ft = FT_VAR;
		
		x = 80 - OffsetX;
		y = WndHeight - BottomTextCount * (FontHeight[ft]+2);
		if (WndWidth > 400) y -= 16;
/*  		show_shadow_text(PT_BACKGROUND, ft, BottomText[0],
			 x, y+FontHeight[FT_VAR]/2, 1, 1, 0, 0, 
		         shadow_colour, NO_COLOUR, ST_NO_STIPPLE);
*/
		if (WndHeight < 480) {
		    y -= 2;
	   	    x = 60-OffsetX;
		}			
		else
		{	
			w = 0;
			for (i=0; i<BottomTextCount; i++)
			{  x = text_width(ft, BottomText[i], -1);
			   if (x > w) w = x;
			}	

			x = 600 - OffsetX - w;
		}
		
		for (i=0; i<BottomTextCount; i++)
		{  
  			show_shadow_text(PT_BACKGROUND, ft, BottomText[i],
				x, y, 1, 1, 0, 0,
		        i == 0 ? inverse_colour : shadow_colour, 
				NO_COLOUR, ST_NO_STIPPLE);
			y += FontHeight[FT_VAR] + 2;	 
			if (i==0) 
				y+= FontHeight[FT_VAR]/2;
		}
				
	}	
}

int prepare_starting_level(int level)
{
    int i;
    
    for (i=0; i<3; i++)
	  current_base_colour[i] = base_colours[level][i];
    
    if (!fill_background_pixmap(PT_BACKGROUND, &mandel_mpmp_info,
		  current_base_colour, StaticColours)) return 0;
		if (!set_line_colour(current_base_colour)) return 0;

	  prepare_pane_background();

    return 1;

}

static int switch_level_stored_colours(int level)
{
  int i;
  ACTION new_action = ACT_NONE;
  unsigned short new_base_colour[3];

  for (i=0; i<3; i++)
  	  new_base_colour[i] = base_colours[level][i];

  new_action = morph_to_colour(new_base_colour);

  for (i=0; i<3; i++)
	  current_base_colour[i] = new_base_colour[i];

  if (new_action == ACT_SHOWNEXT)
    	   board_action(0, new_action);

  update_level(PT_FOREGROUND, level);
  return 1;
}

	
static void redraw_alloc_colours(PLAYER *player, PANE_TYPE pt, int level)
{
  draw_board(pt, player, 0);
	draw_score_board(pt);
  update_level(pt, level-1);
  update_score(pt, player->score, player->layers);
  update_bonus(pt, player->bonus);
  show_next(pt, player);
}
	
static int switch_level_alloc_colours(PLAYER *player, int level)
{
  ACTION new_action = ACT_NONE;
  int  rc = 0;

  if (!prepare_starting_level(level)) goto TheEnd;
	copy_pane(PT_BACKGROUND, PT_TEMP, 0, 0, WndWidth, WndHeight, 0, 0);
	redraw_alloc_colours(player, PT_TEMP, level);

  new_action = morph_to_image(PT_TEMP);
  
  if(new_action == ACT_SHOWNEXT)
    board_action(0, new_action);
    
  update_level(PT_FOREGROUND, level);
  rc = 1;

TheEnd:
  return rc;
    
}

void draw_board_turn(PLAYER *player)
{	int turn, level;

	turn = player->board_turn;

/*	memcpy(static_rgb, shadow_rgb+3*turn, 3); */
/*	set_static_colours(3, 1, static_rgb); */
	set_static_colours(3, 1, shadow_rgb+3*turn); 

	if (!stored_colours	&& (level = game.level) < Levels)
	{ 	prepare_pane_background();
			copy_pane(PT_BACKGROUND, PT_FOREGROUND, 0, 0, WndWidth, WndHeight, 0, 0);
			redraw_alloc_colours(player, PT_FOREGROUND, level);
 			flush_all_pane(PT_FOREGROUND); 
	}
}
	
int draw_start_level(PLAYER *player, int level)
{
    int rc;

	if (new_game)
    {	rc = prepare_starting_level(level);
			draw_score_board(PT_BACKGROUND);
		update_level(PT_BACKGROUND, level);
        copy_pane(PT_BACKGROUND, PT_FOREGROUND, 	
            0, 0, WndWidth, WndHeight, 0, 0);
        flush_all_pane(PT_FOREGROUND); 
		if(rc)
	   	   update_score(PT_FOREGROUND, player->score, player->layers);  
    }
    else
	{
      if (stored_colours)        	
			rc = switch_level_stored_colours(level);
	  else
			rc = switch_level_alloc_colours(player, level);
	  if (rc) LastSaveLayer = -1;
	}	  
    return rc;      

}



/* Parameters:
    px, py, pz - board position of the tile
    dx, dy, dz - board dimension of the tile
*/


void  draw_tile(PANE_TYPE which_pane, int sx, int sy,
			    short tile_size[3], COLENTRY face_colour, int opaque)
{
    int dw, dd, dh;

    int draw_corner_x, draw_corner_y;
    int draw_area_x, draw_area_y;

    
    draw_corner_x = sx-CubScrWidth*4;
    draw_corner_y = sy - CubScrDepthY*4;
    draw_area_x = (CubScrWidth + CubScrDepthX)*4;
    draw_area_y = (CubScrHeight + CubScrDepthY)*4;


	if (opaque)
	  copy_pane(PT_BACKGROUND, which_pane,
		 draw_corner_x, draw_corner_y, draw_area_x, draw_area_y,
	     draw_corner_x,  draw_corner_y);

    if (face_colour <= 0) goto PutImage;

    sx += (-CubScrWidth*(4-tile_size[0]) + CubScrDepthX*(4-tile_size[1]))/2;
    sy += ( CubScrHeight*(4-tile_size[2]) - CubScrDepthY*(4-tile_size[1]))/2;
    
    dw = (int) tile_size[0];
    dd = (int) tile_size[1];
    dh = (int) tile_size[2];

    // ---------Top pane----------- //
    draw_pane(which_pane, sx, sy,
	           -CubScrWidth, 0,
  	           CubScrDepthX, -CubScrDepthY,
	           dw, dd,
	           border_colour, face_colour,
		       bkgr_colour, ST_LIGHT_STIPPLE);
    
    // ---------Front pane----------- //
    draw_pane(which_pane,  sx, sy,
	            -CubScrWidth, 0,
	            0, CubScrHeight,
	            dw, dh,
	            border_colour, face_colour,
				bkgr_colour, ST_DARK_STIPPLE);

    // ---------Side pane----------- //
    draw_pane(which_pane, sx, sy,
	           0, CubScrHeight,
	           CubScrDepthX, -CubScrDepthY,
	           dh, dd,
	           border_colour, face_colour,
			   bkgr_colour, ST_DARK_STIPPLE);

PutImage:
	if (which_pane == PT_FOREGROUND)
    flush_pane(PT_FOREGROUND, 	
		  draw_corner_x, draw_corner_y, draw_area_x, draw_area_y);
}


static void draw_tile_by_code(PANE_TYPE which_pane, int tile_code, int tile_turn)
{
    short tile_size[Dimension];
    short *tile_addr;

    if (tile_code > 0)     		
    {	tile_addr = tile_size;    
      get_tile_size(tile_code, tile_turn, tile_size);
			tile_code += TileEntry-1;
    }
    else
		tile_addr = NULL;
  
    draw_tile(which_pane, ShowNextX, ShowNextY, tile_addr, tile_code, 1);
}

void show_next(PANE_TYPE which_pane, const PLAYER *player)
{
    int  next_code;

    if (player->options & PLOP_SHOWNEXT) 
			next_code = game.next_tile_code;
    else
			next_code = 0;

    draw_tile_by_code(which_pane, next_code, game.next_tile_turn);
}

/*
    num - layer number (from bottom to top)
    layer - pointer to a layer
    returns 1 = empty layer;
*/    

static void wdh_to_screen(int w, int d, int h, int *sx, int *sy)
{
    *sx = BoardCornerX + w * CubScrWidth  - d*CubScrDepthX;
    *sy = BoardCornerY - h * CubScrHeight  + d*CubScrDepth*CubStepY;
}

#define LayerSize (BoardWidth*BoardDepth)

static int  draw_board_layer(PANE_TYPE which_pane, PLAYER *player, int num, int invert, int *delay1)
{
 	  int iw, id, cx, cy, sx, sy;
    int empty, delay;
		COLENTRY face_colour, stip_colour;
    unsigned long next_clock = 0;
		ACTION new_action = ACT_NONE;
		const char *board = player->board;
		int  force_draw_top, force_draw_front;
		int stepx, stepy, bturn;
    const char *layer, *layer_start;

    delay = *delay1;
		force_draw_top = delay || (num >= BoardHeight-1);  
    empty = 1;
		stip_colour = invert ? inverse_colour : bkgr_colour ;
		bturn = player->board_turn;

    wdh_to_screen(1, 1, num+1, &cx, &cy);

    layer = BoardLayer(board, num);

		switch(bturn)
		{  case 0:
				stepx = 1; stepy = BoardWidth; 
				break;

			case 1:
				stepx = -BoardWidth; stepy = 1;
				layer += (LayerSize-BoardDepth); 
				break;

			case 2:
				layer += (LayerSize-1); 
				stepx = -1; stepy = -BoardWidth;
				break;

			default:
				stepx = BoardWidth; stepy = -1;
				layer += (BoardWidth-1); 
				break;
		}
		layer_start = layer;				


    for (id=0; id<BoardDepth; id++)
    {
			sx = cx; sy = cy;
			force_draw_front = delay || (id >= BoardDepth-1);  

			for (iw=0; iw<BoardWidth; iw++)
			{	
				if (delay)
    		  next_clock = start_clock(DRAW_BOARD_DELAY);

		    face_colour = *layer;
		    if (face_colour == 0) goto NextIteration;
	    
	    	empty = 0;
				face_colour += TileEntry-1;  
				
				// Top surface //
				if (force_draw_top || *(layer+LayerSize) == 0)
					draw_pane(which_pane, sx, sy,
	    	   	  -CubScrWidth, 0,
	        	  CubScrDepthX, -CubScrDepthY,
	          	  1, 1,
		   	      border_colour, face_colour,
				  stip_colour, ST_LIGHT_STIPPLE);
	    
				if (force_draw_front || *(layer+stepy) == 0)
	    	   	draw_pane(which_pane,  sx, sy,
	        	  -CubScrWidth, 0, 0, CubScrHeight,
	          	 1, 1,
						border_colour, face_colour,
						stip_colour, ST_DARK_STIPPLE);

		   	// Side surface //
				if (iw >= BoardWidth-1 || !*(layer+stepx) || delay)
	    	    draw_pane(which_pane, sx, sy,
	       		  0, CubScrHeight,
			  		CubScrDepthX, -CubScrDepthY,
	          	1, 1,
	    	  	border_colour, face_colour,
						stip_colour, ST_DARK_STIPPLE);

				if (delay)
				{ 
					flush_pane(which_pane, BoardAreaX, BoardAreaY,
           		    BoardAreaWidth, BoardAreaHeight+2);
					new_action = wait_clock(next_clock, player);
					
					if (new_action != ACT_NONE && player->type==0)
         {
						 delay = 0;
					   force_draw_top = (num >= BoardHeight-1);  
				  	 force_draw_front = (id >= BoardDepth-1);  
					}
				}

		    NextIteration:    
					layer+=stepx;
		  	  sx += CubScrWidth;
				
  		}  
			cx -= CubScrDepthX;
			cy += CubScrDepthY;	    			    
			layer_start += stepy;
			layer = layer_start;  
    }

    if (new_action == ACT_SHOWNEXT || new_action == ACT_LEVEL)
	   board_action(0, new_action);

     *delay1 = delay;

    return empty;
}

static void draw_shadow(PANE_TYPE which_pane, PLAYER *player)
{
  int sx, sy;
	int x, y, w, d;

    // ---------Shadow back----------- //
	if (player->tile_code == 0) return;


	switch( player->board_turn)
	{ case 0:
			x = player->tile_pos[0];
			y = player->tile_pos[1];
			w = player->tile_size[0];
			d = player->tile_size[1];
			break;

	 case 1:
			w = player->tile_size[1];
			d = player->tile_size[0];
			x = BoardWidth - w - player->tile_pos[1];
			y = player->tile_pos[0];
			break;

	 case 2:
			w = player->tile_size[0];
			d = player->tile_size[1];
			x = BoardWidth - w - player->tile_pos[0];
			y = BoardDepth - d - player->tile_pos[1];
			break;

	 default:
			w = player->tile_size[1];
			d = player->tile_size[0];
			x = player->tile_pos[1];
			y = BoardDepth - d - player->tile_pos[0];
			break;
	}


  sy = BoardCornerY - CubScrHeight*BoardHeight-ShadowGap;
  sx  = BoardCornerX + CubScrWidth*x;
  draw_pane(which_pane, sx, sy,
	            CubScrWidth*w, 0,
	            0, -ShadowHeight, 
	            1, 1,
	            shadow_colour, shadow_colour, NO_COLOUR, ST_NO_STIPPLE);
    
  sx = BoardCornerX - CubScrDepthX * y;
  sy +=  CubScrDepthY * y;

  draw_pane(which_pane, sx, sy,
	            0, -ShadowHeight, 
 		          -CubScrDepthX * d, CubScrDepthY * d,
	            1, 1,
	            shadow_colour, shadow_colour, NO_COLOUR, ST_NO_STIPPLE);

}

int  draw_start_game(void)
{	int rc = 0;


	if (!set_static_colours(3, StaticColours-3, static_rgb)) goto BadLuck;
  new_game = 1;
	rc = 1;


BadLuck:
	return rc;
}


static int process_degrade_pixmap(char *pixmap[], MPMP_INFO *info, int reduced_colours)
{
    if (!process_monochrome_pixmap(pixmap, info)) return 0;

    if (ScrDepth < 8)
	  degrade_monochrome_pixmap(info, reduced_colours); 

	return 1;
}

int draw_start(void)
{

    if (!graph_init_video()) return 0;	

    if (!process_degrade_pixmap(mandel_xpm, &mandel_mpmp_info, 2))
				  return 0;

    if (!process_degrade_pixmap(hypoth_xpm,  &hypoth_mpmp_info, 5))
			     return 0;
/*	
 	if (ScrDepth> 4 && !process_monochrome_pixmap(intlig_xpm,  &intlig_mpmp_info))
			     return 0;
*/
    OffsetX =  (mandel_mpmp_info.width - WndWidth)/2;
    OffsetY =  (mandel_mpmp_info.height - WndHeight)/2;
    BoardCornerX -= OffsetX;
    BoardCornerY -= OffsetY;
    ShowNextX -= OffsetX;
    ShowNextY -= OffsetY;

    CubScrDepthX = CubScrDepth*CubStepX;
    CubScrDepthY = CubScrDepth*CubStepY;   

    BoardAreaX = BoardCornerX - CubScrDepthX * BoardDepth;
    BoardAreaY = BoardCornerY - CubScrHeight * BoardHeight - ShadowGap - ShadowHeight;
    BoardAreaWidth = BoardCornerX + CubScrWidth * BoardWidth - BoardAreaX + 1;
    BoardAreaHeight = BoardCornerY + CubScrDepthY * BoardDepth - BoardAreaY +1;

  	if (!graph_start()) return 0;	

    return init_score_board();

}

void draw_end(void)
{
    deinit_score_board();
    free_monochrome_pixmap(&mandel_mpmp_info);
    free_monochrome_pixmap(&hypoth_mpmp_info);
/*    free_monochrome_pixmap(&intlig_mpmp_info); */
/*    free_monochrome_pixmap(&julia_mpmp_info); */
	graph_end();
}


void  draw_board(PANE_TYPE which_pane, PLAYER *player, int invert_mask)
{
    int i;
    int delay, tile_top, tile_bottom;
    int empty, status;
	int start_layer;

    
    status = player->status;
    
    if (status & PLST_NEWLEVEL)		
    {  	player->status &= ~PLST_NEWLEVEL;
				new_game = 0;
    }  

	tile_bottom = player->tile_pos[Dimension-1];
	tile_top = tile_bottom + player->tile_size[Dimension-1];

	if (tile_bottom > LastSaveLayer) 
	  start_layer = LastSaveLayer + 1;
	else {	  
	  start_layer = 0;
	}	  
	
	if (start_layer > 0) {
	  copy_pane(PT_TEMP, which_pane, BoardAreaX, BoardAreaY,
      		BoardAreaWidth, BoardAreaHeight, BoardAreaX, BoardAreaY);
	  clear_shadow(which_pane, bkgr_colour);
	} 
	else {		 
	  LastSaveLayer = -1;
	  copy_pane(PT_BACKGROUND, which_pane, BoardAreaX, BoardAreaY,
    		BoardAreaWidth, BoardAreaHeight, BoardAreaX, BoardAreaY);
	}			

    draw_shadow(which_pane, player);	

    delay = status & PLST_DELAY;

    empty  = 0;
    for (i=start_layer; i<BoardHeight; i++)
    {   if (draw_board_layer(which_pane, player, i, invert_mask & 1, &delay)) 
                     empty = 1;

		if (i == BoardSaveLayer) {
	      copy_pane(which_pane, PT_TEMP, BoardAreaX, BoardAreaY,
              BoardAreaWidth, BoardAreaHeight, BoardAreaX, BoardAreaY);
		  LastSaveLayer = i;
		}		   			  

		if (empty && i >= tile_top) break;		     
        invert_mask >>= 1;
    }		   


    
    if (!delay && which_pane == PT_FOREGROUND)
       flush_pane(PT_FOREGROUND, BoardAreaX, BoardAreaY,
              BoardAreaWidth, BoardAreaHeight + 2);
   
}

/*-----------------------  SCORE BOARD --------------------------*/

void update_level(PANE_TYPE which_pane, int level)
{
    draw_score_line(which_pane, SL_LEVEL, level+1);
}

void update_score(PANE_TYPE which_pane, int score, int layers)
{
    draw_score_line(which_pane, SL_SCORE, score);
    draw_score_line(which_pane, SL_LAYERS, layers);
	
}

void update_bonus(PANE_TYPE which_pane, int bonus)
{
    draw_score_line(which_pane, SL_BONUS, bonus);
}

int draw_bitmap(PANE_TYPE which_pane, 
			  unsigned char *par_bmp, int width, int height,
              int xleft, int ytop, int scalex, int scaley, 
		      int shad_width, int shad_height,
                COLENTRY fgcol, COLENTRY bgcol, STIPPLE_TYPE which_stipple)
{
	int bwidth;
	unsigned char *bmp_cur, *bmp_up;
	unsigned char bmp_byte = '\0', bmp_ubyte = '\0';    
  	int  x, y, sx, sy, mask;

/*
		int  ytop1, width1, height1;
*/	  
	bwidth = (width + 7) >> 3;
	y = height;
		
	if (xleft < 0)
	  xleft = (WndWidth - scalex*width) >> 1;
	else
	if (scalex < 0) {
	  xleft += scalex * width;
	  scalex = -scalex;
	}

	if (scaley < 0) {
	  ytop += scaley * height;
	  scaley = -scaley;
	}		  
		     

	while (--y >= 0)
	{
	  bmp_cur = par_bmp + bwidth * y;
	  bmp_up = (y<=0) ? NULL : bmp_cur - bwidth;
	  sy = ytop + scaley * y;

	  x = width; mask = 0x80;	 	
	  sx = xleft;
	  while (--x >= 0)			
	  {	mask <<= 1;
		if (mask >= 0x100)
		{ bmp_byte = *bmp_cur++;
		  bmp_ubyte = bmp_up ? *bmp_up++ : '\0';
		  mask = 1;
		}
					
		if (bmp_byte & mask) 	
		{ fill_parallelogramm(which_pane, sx, sy,
			     		        scalex, 0, 0, scaley,
								 fgcol, bgcol, which_stipple);
		  if (shad_width> 0)
		  { if (!(bmp_ubyte & mask))
			  fill_parallelogramm(which_pane, sx, sy,
				scalex, 0, shad_width, -shad_height,
				fgcol, bgcol, ST_NO_STIPPLE); 
			  fill_parallelogramm(which_pane, sx+scalex, sy,
			    0, scaley, shad_width, -shad_height, 
			   fgcol, bgcol, ST_NO_STIPPLE);
		  }

		}					

		sx += scalex;
	  }

	}

	return 1;
}


int show_shadow_text(PANE_TYPE which_pane, FONT_TYPE which_font,
							  const unsigned char *text,
                int xleft, int ytop, int scalex, int scaley, 
								int shad_width, int shad_height,
                COLENTRY fgcol, COLENTRY bgcol, STIPPLE_TYPE which_stipple)
{
	int width, height, rc = 0;
	unsigned char *text_bmp;
	
	if (*text == '\0') return 1;
	
	text_bmp = (unsigned char *)text_to_bitmap(which_font, text, &width, &height);
	if (text_bmp)
	{  rc = draw_bitmap(which_pane, text_bmp, width, height, xleft, ytop,
	                 scalex, scaley,  shad_width, shad_height,
                     fgcol,  bgcol, which_stipple);

	  free(text_bmp); 
	}	  
    return rc;
}


