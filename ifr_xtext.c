/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ifr.h"
#include "ifr_common.h"
#include "ifr_cfont.h"

#define ScoreLines   4
#define ScoreVertGap 4
#define ScoreHorzGap 4


extern int BytesPerPixel;

extern int OffsetX, OffsetY;


typedef unsigned long COLOUR; 
typedef Drawable      PANE;

extern PANE    all_panes[];
extern COLOUR	*cm_pixels;

extern Display  *disp;
extern Window	wnd;
extern GC	gc;

extern int OffsetX, OffsetY;
extern const char *ScoreLineText[];
extern  char *XFixedFontList[], *XVarFontList[];
extern const int MaxScoreTermLength;
extern int  WndWidth, WndHeight;
extern int	ConsFont;

char   *FontName[2] = {NULL, NULL};
Font   FontId[2];
int    FixedFontAscent, FixedFontDescent, FixedFontWidth;

int		FontHeight[2];
char *FontBits = NULL;


//static int ScoreX = 55, ScoreY = 70;
//static int ScoreWidth = 170, ScoreHeight = 60;
static int ScoreX = 55, ScoreY = 20;
static int ScoreWidth = 170, ScoreHeight = 60;

static int ScoreLineY[ScoreLines];
static int ScoreLineX, ScoreValueX;


char *get_font_name(XFontStruct *xfs)
{
	XFontProp *props;
    int prop_count;
	
	
	props = xfs->properties;
	prop_count = xfs->n_properties;
	
	while (--prop_count >= 0)
	{
	  if (props->name == XA_FONT)
	  return XGetAtomName(disp, (Atom)(props->card32));
	  props++;		
	}
	
	return NULL;
}


XFontStruct *find_font(char *font_list[], char *def_font_name)
{
    XFontStruct  *xfs;
    char *fntNameCur;
    char **fntNamePtr;

    fntNamePtr = font_list;
    if (def_font_name)  font_list[0] = def_font_name; 
    else  fntNamePtr++;

    while ((fntNameCur = *fntNamePtr++) != NULL)
    {   xfs = XLoadQueryFont( disp,  fntNameCur);
        if (xfs) return xfs;
    }

	return NULL;  
}

static int init_xfont(void)
{
    XFontStruct  *xfs;

	xfs = find_font(XFixedFontList, NULL);
	if (xfs == NULL) {
  	  fprintf(stderr, "Could not load fixed font");
      return 0;
	}	  

	if (FontName[FT_FIXED]) XFree(FontName[FT_FIXED]);
    FontName[FT_FIXED] = get_font_name(xfs);        
	
    FontId[FT_FIXED] = xfs->fid;
    FixedFontAscent = xfs->ascent;
    FixedFontDescent = xfs->descent;
    FixedFontWidth = (xfs->min_bounds).width;
    FontHeight[FT_FIXED] = FixedFontAscent + FixedFontDescent;
	XFreeFontInfo(NULL, xfs, 1);


	xfs = find_font(XVarFontList, NULL);
	if (FontName[FT_VAR]) XFree(FontName[FT_VAR]);
	if (xfs == NULL)
	{
	  fprintf(stderr, "Unable to load variable font - fixed font will be used");
	  FontName[FT_VAR] = strdup(FontName[FT_FIXED]);
	  FontId[FT_VAR] = FontId[FT_FIXED];	  
	}	  
	else
	{
	  FontName[FT_VAR] = get_font_name(xfs);
	  FontHeight[FT_VAR] = xfs->ascent + xfs->descent;
	  FontId[FT_VAR] = xfs->fid;
	  XFreeFontInfo(NULL, xfs, 1);
	}

    return 1;

}


static void deinit_xfont(void)
{
	int i;

	for (i=0; i<2; i++)
	{
    if (FontId[i] != None)
    {   XUnloadFont(disp, FontId[i]);
				FontId[i] = None;
    }	
    if (FontName[i])
		{ XFree(FontName[i]);
	    FontName[i] = NULL;
    }
  }	
			
}

int  init_score_board(void)
{
    int i;
    

	if (ConsFont)
	{  if (!init_cfont()) return 0;
	   FixedFontDescent = FontHeight[FT_FIXED];
	   FixedFontAscent = 0;
	}
	else
	{  
	  if (!init_xfont()) return 0;
	}	  


    ScoreHeight = (FixedFontAscent + FixedFontDescent) * ScoreLines +
                   ScoreVertGap * (ScoreLines + 1);
    ScoreWidth = (MaxScoreTermLength+7)*FixedFontWidth + 2*ScoreHorzGap;
    if (ScoreWidth < 170) ScoreX += (170 - ScoreWidth)/2;

    ScoreX -= OffsetX;
        
    ScoreLineX = ScoreX + ScoreHorzGap;
//    ScoreValueX = ScoreX + ScoreWidth - 6*FontWidth - ScoreHorzGap;
    ScoreValueX = ScoreLineX + (MaxScoreTermLength+1)*FixedFontWidth;


    for (i=0; i<ScoreLines; i++)
        ScoreLineY[i] = ScoreY + (i+1)*ScoreVertGap + i*FixedFontDescent 
 		               + (i+1)*FixedFontAscent; 

    
    return 1;

}

void  deinit_score_board(void)
{
	if (ConsFont) deinit_cfont();
    else  deinit_xfont();
}

static void copy_bmp_line(char *to, const char *from, int width, int bit_order)
{
	int i;
	char fc, tc;
	
	if (bit_order == LSBFirst)
	  memcpy(to, from, width);	
	else	  
	while (--width>=0)
	{  fc = *from++; 
	   tc = 0; i = 8;
	   while (--i >= 0)
	   {   tc = (tc << 1) | (fc & 1);
		   fc >>= 1;
	   }		   
	   *to++ = tc;	
	}
	
}


unsigned char *squeeze_ximage(XImage *ximg, int rwidth)
{	unsigned char *source;
	unsigned char *resptr, *srcptr;
	int  swidth, height, h;
	int  bit_order;
	
	source = ximg->data;
	swidth = ximg->bytes_per_line;
	bit_order = ximg->bitmap_bit_order;
		
	if (swidth == rwidth && bit_order == 0)
		return source;
			
	srcptr = source; resptr = source;
	h = height = ximg->height;

	while (--h >= 0)
	{ copy_bmp_line(resptr, srcptr, rwidth, bit_order);
	  resptr += rwidth; srcptr += swidth;	
	}

	return realloc(source, rwidth * height);
}


void draw_score_line(PANE_TYPE which_pane, int line, int value)
{
    char text[7];
    int y, y1;
    
	if (value < 0)
	    sprintf(text, "%6s", " ");
	else
	    sprintf(text, "%6d", value);
    
    y = ScoreLineY[line];
	y1 = y - FixedFontAscent;

	if (ConsFont == 0)
	{ 	PANE  pane = all_panes[which_pane];

		XSetFont(disp, gc, FontId[FT_FIXED]);
	    XSetForeground(disp, gc, cm_pixels[border_colour]);
	    XDrawImageString (disp, pane, gc, ScoreValueX, y, text, 6);	
	}
	else 
	{
	    fill_parallelogramm(which_pane, ScoreValueX, y1,
		      FixedFontWidth*6, 0, 0, FontHeight[FT_FIXED],
		      bkgr_colour, bkgr_colour, ST_NO_STIPPLE);
	
	    show_shadow_text(which_pane, FT_FIXED,
		      text, ScoreValueX, y1, 1, 1, 0, 0,
		      border_colour, bkgr_colour, ST_NO_STIPPLE);
	}        

    flush_pane(which_pane, ScoreValueX, y1,
	           FixedFontWidth*6, FixedFontAscent+FixedFontDescent);         
}


void draw_score_board(PANE_TYPE which_pane)
{
    int i, y;
    const char *string;

    PANE  pane = all_panes[which_pane];
	
	XSetForeground(disp, gc, cm_pixels[bkgr_colour]);
    XFillRectangle(disp, pane, gc, ScoreX-5, ScoreY-5, ScoreWidth+10, ScoreHeight+10);         
    XSetForeground(disp, gc, cm_pixels[score_colour]);
    XDrawRectangle(disp, pane, gc, ScoreX-5, ScoreY-5, ScoreWidth+10, ScoreHeight+10);         
    XDrawRectangle(disp, pane, gc, ScoreX-3, ScoreY-3, ScoreWidth+6, ScoreHeight+6);         
	if (ConsFont == 0) 
	{  XSetBackground(disp, gc, cm_pixels[bkgr_colour]);
	   XSetFont(disp, gc, FontId[FT_FIXED]);
	}	   

    for (i=0; i< ScoreLines; i++)
	{
	  string = ScoreLineText[i];
	  y = ScoreLineY[i];

	  if (ConsFont == 0)
		XDrawImageString (disp, pane, gc, ScoreLineX, y, 
			string, strlen(string));	
	  else
  		show_shadow_text(which_pane, FT_FIXED,
  		  string, ScoreLineX, y-FixedFontAscent, 1, 1, 0, 0,
	      score_colour, bkgr_colour, ST_NO_STIPPLE);
    }
	
	flush_pane(which_pane, ScoreX-5, ScoreY-5, ScoreWidth+10, ScoreHeight+10);         
}

static long text_width_xfont(FONT_TYPE which_font, const char *text, int len)
{
	XCharStruct xcs;
	int width;

	if (len < 0) len = strlen(text);

	XQueryTextExtents(disp, FontId[which_font],  text, len,
	  			  &width, &width, &width, &xcs);

	/* Make the width a multiple of 8 */
/*	width = (xcs.width + 7) >> 3;
//	return width << 3; */

	return xcs.width;
}

long text_width(FONT_TYPE which_font, const char *text, int len)
{
  if (ConsFont)
	  return text_width_cfont(which_font, text, len);

  return text_width_xfont(which_font, text, len);
}


static unsigned char *text_to_bitmap_xfont(FONT_TYPE which_font, const char *text,
                              int *width_return, int *height_return)
{							  
	Pixmap xpm = None;
	GC	gc2 = None;
	XImage *ximg = NULL;
	XCharStruct xcs;
	XGCValues xgcv;
	
	int width, height;
	int bwidth;
	int ascent, bit_order;
	int	len;

	unsigned char *result  = NULL;
	
	len = strlen(text);
	
	XQueryTextExtents(disp, FontId[which_font],  text, len,
										&width, &width, &width, &xcs);
    ascent = xcs.ascent;
	height = xcs.descent + ascent;
	width = xcs.width;

	/* Make the width a multiple of 8 */
	bwidth = width >> 3;	/* Width in bytes */
	if (width & 7) bwidth++;
	width = bwidth << 3;

	xpm = XCreatePixmap(disp, wnd, width, height, 1);	
	if (xpm == None) goto HastaLaVista;
	
	xgcv.font = FontId[which_font];
	xgcv.background = 0;
	xgcv.foreground = 0;
	xgcv.fill_style = FillSolid;
	gc2 = XCreateGC(disp, xpm, 
		            GCBackground | GCForeground | GCFont | GCFillStyle, &xgcv);
	if (gc2 == None) goto HastaLaVista;
				  
	XFillRectangle(disp, xpm, gc2, 0, 0, width, height);
	
	XSetForeground(disp, gc2, 1);
    XDrawString(disp, xpm, gc2, 0, ascent, text, len);
	
	ximg = 	XGetImage(disp, xpm, 0, 0, width, height, 1, ZPixmap);
	if (ximg == NULL) goto HastaLaVista;  			   			

	bit_order = ximg->bitmap_bit_order;

	XFreeGC(disp, gc2); gc2 = None;
	XFreePixmap(disp, xpm); xpm = None;

	result = squeeze_ximage(ximg, bwidth);

	ximg->data = NULL;
	
	*width_return = width;
	*height_return = height;

	
HastaLaVista:
	if (xpm) XFreePixmap(disp, xpm);
	if (gc2) XFreeGC(disp, gc2);
	if (ximg) XDestroyImage(ximg); 

	return result;
}

static unsigned char *text_to_bitmap_multiline_xfont(char *text, 
				int linecharwid, int *width_return, int *height_return)
{	Pixmap  xpm = None;
	int textcharwid, linecount;		
	int width, height, linehei;  
	int bwidth;
	XGCValues xgcv;
	GC	gc2 = None;
	XImage *ximg = NULL;
	unsigned char *result  = NULL;
	int y, l;

	textcharwid = strlen(text);
	linecount = (textcharwid+linecharwid-1) / linecharwid;

	width = FixedFontWidth * linecharwid;
	linehei = FixedFontAscent + FixedFontDescent;
	height = linehei * linecount;

	/* Make the width a multiple of 8 */
	bwidth = (width + 7) >> 3;	/* Width in bytes */
	width = bwidth << 3;
	
	xpm = XCreatePixmap(disp, wnd, width, height, 1);	
	if (xpm == None) goto HastaLaVista;
	
	xgcv.font = FontId[FT_FIXED];
	xgcv.background = 0;
	xgcv.foreground = 0;
	xgcv.fill_style = FillSolid;
	gc2 = XCreateGC(disp, xpm, 
		            GCBackground | GCForeground | GCFont | GCFillStyle, &xgcv);
	if (gc2 == None) goto HastaLaVista;
				  
	XFillRectangle(disp, xpm, gc2, 0, 0, width, height);
	
	XSetForeground(disp, gc2, 1);
	
	for (l=0; l<linecount; l++)
    {	y = l*linehei + FixedFontAscent;
		XDrawString(disp, xpm, gc2, 0, y, text, linecharwid);
		text += linecharwid;
	}		

	ximg = 	XGetImage(disp, xpm, 0, 0, width, height, 1, ZPixmap);
	if (ximg == NULL) goto HastaLaVista;  			   			



	XFreeGC(disp, gc2); gc2 = None;
	XFreePixmap(disp, xpm); xpm = None;

	result = squeeze_ximage(ximg, bwidth);
	
	ximg->data = NULL;

	*width_return = width;
	*height_return = height;

HastaLaVista:
	if (xpm) XFreePixmap(disp, xpm);
	if (gc2) XFreeGC(disp, gc2);
	if (ximg) XDestroyImage(ximg); 		/* This will not free the data */
	
	return result;
}

unsigned char *text_to_bitmap(FONT_TYPE which_font, const char *text,
                              int *width_return, int *height_return)
{
  if (ConsFont)
	return text_to_bitmap_cfont(which_font, text, width_return, height_return);

  return text_to_bitmap_xfont(which_font, text, width_return, height_return);
}							  

unsigned char *text_to_bitmap_multiline(char *text, 
				int linecharwid, int *width_return, int *height_return)
{
  if (ConsFont)
	 return text_to_bitmap_multiline_cfont(text, 
				 linecharwid, width_return, height_return);

   return text_to_bitmap_multiline_xfont(text, 
				 linecharwid, width_return, height_return);

}
