/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "ifr.h"
#include "ifr_common.h"
#include "ifr_cfont.h"

#define ScoreLines   4
#define ScoreVertGap 4
#define ScoreHorzGap 4

typedef struct 
{	int width;
	int height;
	char *pixels;
} PIXMAP_INFO;

typedef unsigned char COLOUR;

extern int BytesPerPixel;
extern COLOUR *cm_pixels;

#define PaneCount 2

extern int OffsetX, OffsetY;
extern PIXMAP_INFO  *all_panes[PaneCount];
extern const int MaxScoreTermLength;
extern const char *ScoreLineText[];
extern int	ConsFont;


char *FontBits = NULL;


static int ScoreX = 55, ScoreY = 20;
static int ScoreWidth = 170, ScoreHeight = 60;

static int ScoreLineY[ScoreLines];
static int ScoreLineX, ScoreValueX;

int  FontHeight[2], FixedFontWidth;
char	*FontName[2] = {NULL, NULL};

/*
int init_text(void)
{

   ConsFont = 1;

   return init_cfont();
}
*/

static int show_char(PANE_TYPE which_pane, unsigned int chr,
                     int x, int y, COLENTRY fgcol, COLENTRY bgcol)
{
    const char *bits;
    int  bit_current, height;
	const PIXMAP_INFO *pi;
    unsigned char *pixaddr, *colptr;
    unsigned char *bgcolptr, *fgcolptr;
		
    int  mask, leap;


    if (FontBits == NULL) return 0;

 	 pi = all_panes[which_pane];        

    leap = pi->width;
    pixaddr = pi->pixels + (leap * y + x) * BytesPerPixel;
    leap -= FixedFontWidth;
    height = FontHeight[FT_FIXED];	
	 bgcolptr = cm_pixels + bgcol*BytesPerPixel;
	 fgcolptr = cm_pixels + fgcol*BytesPerPixel;

    bits = FontBits + (chr * height);
    leap *= BytesPerPixel;
    
    while (--height >= 0)
    {  bit_current = *bits++;
    	/*    mask = 1 << FixedFontWidth; */
	  mask = 0x100;
      while ( (mask >>= 1) != 0) 
	  { colptr = (bit_current & mask) ? fgcolptr : bgcolptr;
	      memcpy(pixaddr, colptr, BytesPerPixel);
		  pixaddr += BytesPerPixel;
	  }
	  pixaddr += leap;	        
    }		
    
    return 1;
}		     


static int show_text(PANE_TYPE which_pane, const unsigned char *text,
                 		    int x, int y, COLENTRY fgcol, COLENTRY bgcol)
{
    unsigned int c;    

    while((c = *text++) != '\0')
    {	if (!show_char(which_pane, c, x, y, fgcol, bgcol))
		return 0;
		   
		x += FixedFontWidth;
    }		

    return 1;
}



/*--------------------------  SCORE BOARD ------------------------*/
void draw_score_line(PANE_TYPE which_pane, int line, int value)
{
    char text[7];
    int y, y1;

		if (value < 0)
	    sprintf(text, "%6s", " ");
		else	
	    sprintf(text, "%6d", value);
    
    y = y1 = ScoreLineY[line];
    
    show_text(which_pane, text, ScoreValueX, y, 
		          border_colour, bkgr_colour);

   flush_pane(which_pane, ScoreValueX, y1 , FixedFontWidth*strlen(text), FontHeight[FT_FIXED]);
     
}
/*
void draw_frame(PANE_TYPE which_pane,
		 int x, int y, int wid, int hei,
		 int fg, int bg)
{
    draw_pane(which_pane, x, y, wid, 0, 
			  0, hei, 1, 1,
			  fg,  bg, -1, ST_NO_STIPPLE);
}
*/

void draw_score_board(PANE_TYPE which_pane)
{
    const unsigned char *string;
    int i, y;
    int fg, bg;
        
    fg = score_colour;
    bg = bkgr_colour;

    draw_pane(which_pane, ScoreX-3, ScoreY-3, ScoreWidth+6, 0, 0, ScoreHeight+6,
				  1, 1,   fg,  bg, -1, ST_NO_STIPPLE);

    draw_pane(which_pane, ScoreX-1, ScoreY-1, ScoreWidth+2, 0, 0, ScoreHeight+2,
				  1, 1,   fg,  bg, -1, ST_NO_STIPPLE);
//    draw_frame(which_pane, ScoreX-3, ScoreY-3, ScoreWidth+6,  ScoreHeight+6,
//							fg, bg);

//    draw_frame(which_pane, ScoreX-1, ScoreY-1, ScoreWidth+2, ScoreHeight+2,
//							fg, -1);
   
    for (i=0; i< ScoreLines; i++)
    {
			string = ScoreLineText[i];
			y= ScoreLineY[i];
			if (!show_text(which_pane, string, ScoreLineX, y, fg, bg))
			break;
    }

}


int  init_score_board(void)
{
    int i;
    
    ConsFont = 1;
    if (!init_cfont()) return 0;
    
    ScoreHeight = FontHeight[FT_FIXED] * ScoreLines +
                   ScoreVertGap * (ScoreLines + 1);

    ScoreWidth = (MaxScoreTermLength+7)*FixedFontWidth + 2*ScoreHorzGap;
    if (ScoreWidth < 170) ScoreX += (170 - ScoreWidth)/2;

    ScoreX -= OffsetX;
        
    ScoreLineX = ScoreX + ScoreHorzGap;
//    ScoreValueX = ScoreX + ScoreWidth - 6*FixedFontWidth - ScoreHorzGap;
    ScoreValueX = ScoreLineX + (MaxScoreTermLength+1)*FixedFontWidth;


    for (i=0; i<ScoreLines; i++)
        ScoreLineY[i] = ScoreY + (i+1)*ScoreVertGap + i*FontHeight[FT_FIXED]; 

    
    return 1;

}

void  deinit_score_board(void)
{

    deinit_cfont();
}

/*---------------------------------------------------*/
long	text_width(FONT_TYPE which_font, const char *text, int len)
{
	return text_width_cfont(which_font, text, len);
}

unsigned char *text_to_bitmap(FONT_TYPE which_font, const char *text,
											        int *width_return, int *height_return)
{
	return text_to_bitmap_cfont(which_font, text, width_return, height_return);
}

unsigned char *text_to_bitmap_multiline(char *text, 
				int linecharwid, int *width_return, int *height_return)
{
	return text_to_bitmap_multiline_cfont(text, 
				    linecharwid, width_return, height_return);
}					 


