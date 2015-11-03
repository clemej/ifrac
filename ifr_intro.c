/*  ===========================`======================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ifr.h"
#include "ifr_pixmap.h"
#include "time.h"


#define ms2ticks(ticks) ((ticks) * CLOCKS_PER_SEC / 1000)


extern int  OffsetX, OffsetY;
extern int  ScrWidth, ScrHeight;
extern int  interface_mask;
extern int  WndWidth, WndHeight, ScrDepth, FixedFontWidth, FontHeight[2];
extern unsigned const char half_bmp_bits[];
extern const unsigned char base_colours[][3]; 
extern const unsigned char frac_rgb[]; 
extern char *hypoth_xpm[], *intlig_xpm[], *julia_xpm[];
extern PLAYER  *players[];

static int ScrollTimeout =  24;
static int VscrollTimeout =  100;
static int InterfaceTimeout = 60;

#define FracTextHeight 9
#define FracTextWidth 22
#define VscrollLineWidth 27		
#define VscrollMaxLines  200		

static  int FracTextX=60, FracTextY=100;

extern  MPMP_INFO hypoth_mpmp_info, intlig_mpmp_info, julia_mpmp_info;

char FracText[FracTextHeight][FracTextWidth]=
{
  {1,1,1,1,0,0,2,2,2,2,0,0,0,3,3,3,0,0,4,4,4,4},
	{1,0,0,0,0,0,2,0,0,2,0,0,3,0,0,3,0,0,4,0,0,0},
	{1,0,0,0,0,0,2,0,0,2,0,0,3,0,0,3,0,0,4,0,0,0},
	{1,0,0,0,0,0,2,0,2,2,0,0,3,0,0,3,0,0,4,0,0,0},
	{1,1,1,0,0,0,2,2,2,0,0,0,3,0,0,3,0,0,4,0,0,0},
	{1,0,0,0,0,0,2,0,2,0,0,0,3,3,3,3,0,0,4,0,0,0},
	{1,0,0,0,0,0,2,0,0,2,0,0,3,0,0,3,0,0,4,0,0,0},
	{1,0,0,0,0,0,2,0,0,2,0,0,3,0,0,3,0,0,4,0,0,0},
	{1,0,0,0,0,0,2,0,0,2,0,0,3,0,0,3,0,0,4,4,4,4}
}; 


static char ScrollerText[] = 
	"Original FRAC was designed and implemented "
	"by Max Shapiro & Per Bergland in 1990. "
  "I was really addicted to this elegant toy, "
	"until it happened to be rather fast for a new Pentium. "
	"Let this return of FRAC remind you the good old days "
	"of slow CPU-s and skilled game developers, when robustness, "
	"high efficiency and small size were the major criteria for the quality. "
	"Enjoy ...";
	

extern GAME game;
extern const int  CubScrWidth, CubScrHeight;   
extern const int  CubScrDepthX, CubScrDepthY;
extern int  WndWidth, WndHeight;
extern unsigned long *cm_pixels;


static int ScrollY = 228;
static int ScrollPixWid=5, ScrollPixHei=5;
static int ScrollShadHei=3, ScrollShadWid=3;
static unsigned char *ScrollImage=NULL;
static int ScrollImageWidth, ScrollImageHeight;
static struct scroll_chunk
{	int chunk_pos;
	int screen_x;
	int screen_width;
} ScrollChunks[] =
{ { 0,  0, 181},
  {232, 216, 212},
  {484, 462, 177}
};
static const int ScrollChunkCount = sizeof(ScrollChunks)/sizeof(struct scroll_chunk);

/* ----------------------------------------- */

static unsigned char *VscrollImage=NULL;
static int	VscrollImageWidth, VscrollImageHeight;
static int  VscrollY = 90;

static char *VscrollText =
        "Tips                       "
        "====                       "
        "Use arrow keys  or joystick"
        "lever  to  control speed of"
		"scrollers. To exit and come"
        "to the real stuff hit SPACE"
		"                           "
        "Press H for hot keys       "
        "                           "
#ifdef USE_XDGA
        "Full screen mode in X.     "
        "======================     "
        "With XFree86 4.0+ just type"
        "    xifrac -dga            "
		"to run in full screen mode."
        "See 'Using XFree86-DGA'  in"
        "INSTALL for more hints.    "
        "                           "
#endif
	    "World-wide score table.    "
	    "=======================    "
	    "Starting  from v 1.3.0 you "
	    "can submit  your score to  "
	    "World Wide Score Table.    "
#ifdef ENABLE_WWSCORE 
        "If  your score is 15000 or "
        "higher, don't miss a chance"
        "to enter  IFRAC  World-Wide"
        "Score  List.  You  will  be"
        "given a code to submit your"
        "score.                     "
#else    
	    "Unfortunately,  this  copy "
	    "does not appear to  support"
	    "score  submission,  but you"
	    "might get  a  better choice"
	    "after visiting IFRAC site  "
#endif  
        "                           "
        "Credits                    "
        "=======                    "
        " Max Tegmark, Per Bergland "
        "                           "
        "Greetings                  "
        "========= (alphabetically) "
        "  Fabio Giucci             "
        "   Vojtech Pavlik          "
        "     Matt Rhoten           "
        "      Marcus Sundberg      "
        "        Michael Weller     "
	"         William Zachary   "
        "           Matan Ziv-Av    "
        "                           "
	"This page appears silly and"
        "rather slow. Hopefully this"
        "is not the scrollers  you  "
        "downloaded the package for."
        "In future you can skip this"
        "part with  -noi  in command"
        "line.                      "  
	"        Go ahead!          ";


/* ================================================= */
/*
static char *VscrollText = NULL;
static char *VscrollLinePtr;
void append_vscroller_line(const char *left, const char *right)
{
	 int lleft, lright, lmid;
	 
	 lleft = strlen(left);
	 lright = strlen(right);
	 lmid = VscrollLineWidth -  (lleft + lright);

	 memcpy(VscrollLinePtr, left, lleft);
	 VscrollLinePtr += lleft;	
	 
	 if (lmid < 0) 
      lmid += VscrollLineWidth;

	 memset(VscrollLinePtr, ' ', lmid);
 	 VscrollLinePtr += lmid;	
	 memcpy(VscrollLinePtr, right, lright);
	 VscrollLinePtr += lright;
}

void append_vscroller_text(const char *text, int pos)
{  int lright, lmid;
 
	 lmid = strlen(text);
	 lright = VscrollLineWidth - pos - lmid;
	 if (lright < 0) 
	 { lmid += lright; lright = 0; }
	 if (lmid < 0)
	 { pos += lmid; lmid = 0;}
	 if (pos < 0) return;
	
	 if (pos > 0)
	 { memset(VscrollLinePtr, ' ', pos);
 	 	 VscrollLinePtr += pos;	
	 }	
	 if (lmid > 0)
	 { memcpy(VscrollLinePtr, text, lmid);
 	 	 VscrollLinePtr += lmid;	
	 }	
	 if (lright > 0)
	 { memset(VscrollLinePtr, ' ', lright);
 	 	 VscrollLinePtr += lright;	
	 }	
		
}

void fill_vscroller_text(const char **text)
{
	const char *text_line, **text_line_ptr;

	text_line_ptr = text;
	while ((text_line = *text_line_ptr++) != NULL)
	append_vscroller_line(text_line, "");
}


void append_long_name(const char *title, const char *name)
{	int len, offset = 0;
	char portion[VscrollLineWidth+1];
	static int portion_size;

	portion_size = VscrollLineWidth - strlen(title)-1;

	len = strlen(name);	
	portion[portion_size] = '\0';

	while(len > 0)
	{	if (len < portion_size)
			portion_size = len;

		memcpy(portion, name, portion_size);
		portion[portion_size] = '\0';
		len -= portion_size;
		name += portion_size;

		if (offset == 0)
		{	append_vscroller_line(title, portion);
			offset = strlen(title) + 1;
	    portion_size = VscrollLineWidth - offset;
		}
		else
		  append_vscroller_text(portion, offset);
		
	}		

}

void fill_vscroller_lines(void)
{

	VscrollText = (char *) malloc(VscrollLineWidth * VscrollMaxLines + 1);
	if (VscrollText == NULL) return;

	VscrollLinePtr = VscrollText;

	fill_vscroller_text(VscrollPreface);
	fill_ifrac_info();
	fill_configuration();
	fill_environment();
	fill_vscroller_text(VscrollAppendix);
	

	*VscrollLinePtr++ = '\0';
	VscrollText = realloc(VscrollText, VscrollLinePtr - VscrollText);

}
*/



/* ================================================= */

void show_vert_scroller(int starty)
{
		int x, y, s, sx, sy;
		int midx, midsx;
		unsigned char *pixbyteptr, pixbyte=0, msk;
		double scalex, scalexx[10];
	  int scaley;
		int bytewid;

		bytewid = VscrollImageWidth >> 3;
		pixbyteptr = VscrollImage;
		if (starty > 0) pixbyteptr += starty * bytewid;
		midsx = WndWidth>>1;  midx = VscrollImageWidth>>1;
		sy = VscrollY;

		for ( y = starty; y < VscrollImageHeight; y++, sy += scaley)
		{	
			 if( sy >= WndHeight) break;
			 x = sy-VscrollY;
			 scaley = 1. + 0.008 * x;	
			 if (sy + scaley > WndHeight) scaley = WndHeight-sy;	
			
			 if (y < 0) continue; 	
			 scalex =  WndHeight > 400 ? 0.00002 :
	                 (WndHeight > 350 ? 0.00003 : 0.000044);
			 for (s=0; s<scaley; s++, x++) 
			  	scalexx[s] = scalex * x * x;
			
	
		   msk = 0;

			 for (x=0; x<VscrollImageWidth; x++)
			 {	if (msk == 0)
				  {		pixbyte = *pixbyteptr++;
							msk = 1;
					}									
					if (pixbyte & msk)
						for(s=0; s<scaley; s++)
						{		scalex = scalexx[s];
								sx = midsx + (x- midx) * scalex;	
  			        	    	draw_line(PT_FOREGROUND, sx, sy+s,
									scalex, 1, IntroVscrollerColour,
                    			   NO_COLOUR, ST_NO_STIPPLE);
						}
				  msk <<= 1;
			 }

		}

}



void show_horz_scroller(int chunk, int start_pos)
{
  int y, w;
  int lx, lo;
  int px, mx;
  int sx, sy, sx1;
  int gy, by;
  int bl, br;
  int linewidth;
  unsigned char bytes[20], byte;
  const unsigned char *bitptr;
  int  ScrollX, ScrollWid;

  ScrollX =  ScrollChunks[chunk].screen_x;
  ScrollWid =  ScrollChunks[chunk].screen_width;

	start_pos += ScrollChunks[chunk].chunk_pos;
// if (start_pos >= ScrollX + ScrollWid) return;

  if (start_pos < 0)
  { sx = -start_pos;
		start_pos = 0;
		lx = lo = 0;
  }	 
  else
  {
/*    div_t	quotrem; 
      quotrem = div(start_pos, ScrollPixWid);
      lx = quotrem.quot;
	  	lo = quotrem.rem;
 */
	  lx = start_pos/ScrollPixWid;
	  lo = start_pos % ScrollPixWid;
	  sx = 0;
  }	

  linewidth = ScrollImageWidth >> 3 ; 

  px = lx >> 3;
  mx = 1 << (lx & 7);


  bitptr = ScrollImage + px;
  for (y=0; y<ScrollImageHeight; y++)
	   { bytes[y] = *bitptr; bitptr += linewidth; }  

  br = ScrollShadWid;

  while(sx< ScrollWid)
  {	
		if (++lx >= ScrollImageWidth) break;

		if (mx >= 0x100)
		{	px ++; 	
		  bitptr = ScrollImage + px;
	  	for (y=0; y<ScrollImageHeight; y++)
	   	{ bytes[y] = *bitptr; bitptr += linewidth; }  
		   mx = 1;
	  }

	  bl = br;
	  br = ScrollShadWid*(1. - 2.*(sx+1)/ScrollWid);
	  gy = 12. * (ScrollWid/2. - abs(sx-ScrollWid/2))/ScrollWid;

		sx1 = sx+ScrollX;
		w = ScrollPixWid-lo;
		if (sx1+w > WndWidth) w = WndWidth-sx1;
	
	  if (px >= 0 && w>0)  
	  { 
	    by = -1;
		
	    for (y=ScrollImageHeight-1; y>=0; y--)
	  	{
		    byte = bytes[y];
		    if (byte & mx)
		    {
		      if (by < 0) by = y;
		    }
		    else
		    { if (by >= 0)
		      {
					  sy = (y+1) * ScrollPixHei + gy + ScrollY;
			      draw_pane(PT_FOREGROUND, sx1, sy,
	 		   	    w, 0, 0, ScrollPixHei, 1, by-y,
		     	    IntroBorderColour, IntroScrollerColour,
			  	    IntroBkgrColour, ST_NO_STIPPLE);

			      fill_hor_trapezium(PT_FOREGROUND, sx1, sy,
	       	 	    w, -ScrollShadHei, bl, br, IntroScrollerColour,
						   IntroBkgrColour, ST_LIGHT_STIPPLE);
	
					  by = -1;					
			    }					
		    }	
  		}   
		
			if (by >= 0)
			{ 	sx1 = sx+ScrollX;
	        	sy = ScrollY + gy;	
				w = ScrollPixWid-lo;
			  	if (sx1+w > WndWidth) w = WndWidth-sx1;
				  draw_pane(PT_FOREGROUND, sx1, sy,
	 		       w, 0, 0, ScrollPixHei, 1, by+1,
		   	      IntroBorderColour, IntroScrollerColour,
		  		  IntroBkgrColour, ST_NO_STIPPLE);

				fill_hor_trapezium(PT_FOREGROUND, sx1, sy,
	     	  	  w, -ScrollShadHei, bl, br,
		  	  	  IntroScrollerColour, IntroBkgrColour, ST_DARK_STIPPLE);
			}					
		
		
	  }		
    
	  sx += w;	
	  lo = 0;
	  mx <<= 1;
   }	  
		

}  
  			

static void show_intro_background(void)
{
    int x, y; 
    int i, j;
    unsigned short base_colour[3];
    COLENTRY col_entry;
    unsigned long end_period = 0;
    int  delay = 1, scaley;
    int IntligX, IntligY;
    int  VarFontHeight, FixedFontHeight;

//	show_cursor(0);	

    for (i=0; i<3; i++)
		base_colour[i] = base_colours[Intro1ColourEntry][i];

    fill_background_pixmap(PT_BACKGROUND, &hypoth_mpmp_info, base_colour, IntroStaticColours);

    set_static_colours(2, IntroStaticColours-2, frac_rgb);


    copy_pane(PT_BACKGROUND, PT_FOREGROUND,
                0, 0, WndWidth, WndHeight, 0, 0);
    flush_all_pane(PT_FOREGROUND);
	

    
    y = FracTextY + (FracTextHeight-1) *CubScrHeight;
		
	for (j=FracTextHeight-1; j>=0; j--)
	{
	   if(delay) end_period = start_clock(100); 
	   x = FracTextX;
	   if (x + CubScrWidth >= WndWidth) break;
	   for (i=0; i<FracTextWidth/2; i++)
	   {  col_entry = FracText[j][i];

		  if (col_entry > 0)
		  {	 col_entry += 3;

		     if (delay || j==0 || FracText[j-1][i] == 0)
			    draw_pane(PT_FOREGROUND, x, y,
	             -CubScrWidth, 0,  CubScrDepthX, -CubScrDepthY,
	              1, 1, IntroBorderColour, col_entry, -1, ST_LIGHT_STIPPLE);

			   if (delay || i==21 || FracText[j][i+1] == 0) 
	             draw_pane(PT_FOREGROUND, x, y,
	  		       0, CubScrHeight,CubScrDepthX, -CubScrDepthY,
		           1, 1, IntroBorderColour, col_entry, NO_COLOUR, ST_LIGHT_STIPPLE);
				   
		        draw_pane(PT_FOREGROUND,  x, y,
		          -CubScrWidth, 0,   0, CubScrHeight,
	    	      1, 1, IntroBorderColour, col_entry, IntroBkgrColour, ST_NO_STIPPLE);
			
		  }

 		  x += CubScrWidth;
	  }	

	  x = FracTextX + CubScrWidth*FracTextWidth;
	  for (i=FracTextWidth-1; i>=FracTextWidth/2; i--)
	  { col_entry = FracText[j][i];
 		  x -= CubScrWidth;

		  if (col_entry > 0)
		  {	 col_entry += 3;

			 if (delay || j==0 || FracText[j-1][i] == 0)
			     draw_pane(PT_FOREGROUND, x+CubScrWidth, y,
	             -CubScrWidth, 0,  -CubScrDepthX, -CubScrDepthY,
	              1, 1, IntroBorderColour, col_entry, -1, ST_LIGHT_STIPPLE);

			 if (delay || i==0 || FracText[j][i-1] == 0) 
	             draw_pane(PT_FOREGROUND, x, y,
	  		       0, CubScrHeight, -CubScrDepthX, -CubScrDepthY,
		           1, 1, IntroBorderColour, col_entry, NO_COLOUR, ST_LIGHT_STIPPLE);
				   
		      draw_pane(PT_FOREGROUND,  x+CubScrWidth, y,
		          -CubScrWidth, 0,   0, CubScrHeight,
	    	      1, 1, IntroBorderColour, col_entry, IntroBkgrColour, ST_NO_STIPPLE);

			}

	  }	

	  if (delay) 
	  {		
	 	  flush_pane(PT_FOREGROUND, FracTextX-CubScrWidth, y-CubScrDepthY,
      			  CubScrWidth*(FracTextWidth+1)+CubScrDepthX, 
				      CubScrHeight+CubScrDepthY);
		  if (wait_clock(end_period, NULL) != ACT_NONE) delay = 0;
	  }				
	  y -= CubScrHeight;
	}

	
	if(delay == 0)
	{		delay = 1;
	    end_period = start_clock(300); 
 	    flush_pane(PT_FOREGROUND, FracTextX-CubScrWidth, FracTextY-CubScrDepthY,
                              CubScrWidth*(FracTextWidth+1)+CubScrDepthX,
							  CubScrHeight*FracTextHeight+CubScrDepthY);
		  if (wait_clock(end_period, NULL) != ACT_NONE) delay = 0;
	}

  if (delay) end_period = start_clock(300); 
  for (i=0; i<3; i++)
	   base_colour[i] = base_colours[Intro2ColourEntry][i];

  FixedFontHeight = FontHeight[FT_FIXED];
  VarFontHeight = FontHeight[FT_VAR];

	scaley = (FixedFontHeight<10 && WndHeight>400) ? 9 : 6;
	
	IntligX = FracTextX-30;
  IntligY = FracTextY-FixedFontHeight*scaley + 17 + scaley/2; 
/*
	if (ScrDepth > 4)
	{
	  insert_to_pixmap(PT_FOREGROUND, &intlig_mpmp_info,
		      base_colour, IntligX, IntligY, 
 					IntroStaticColours + hypoth_mpmp_info.colours_used);
	}
	else
*/
	  x = IntligX; y = IntligY - FontHeight[FT_FIXED];
	  show_shadow_text(PT_FOREGROUND, FT_FIXED, "Intelli", x, y, 
			3, scaley, 2, 5, 3,4, /* IntroBkgrColour,*/ ST_LIGHT_STIPPLE);
	  x += 7*FixedFontWidth*3;


	  show_shadow_text(PT_FOREGROUND, FT_FIXED, "g", x-1, y+2, 
			3, scaley, 3, 7, 3,4, /* IntroBkgrColour,*/ ST_LIGHT_STIPPLE);
	  
	  x += FixedFontWidth*3;
	  show_shadow_text(PT_FOREGROUND, FT_FIXED, "ent", x, y, 
			3, scaley, 2, 5, 3,4, /* IntroBkgrColour,*/ ST_LIGHT_STIPPLE);
		
	 i = WndWidth - FracTextX + 35;	 // 30
	 { char text[15];
		 j = IFRAC_VERSION;
		 y = 69/VarFontHeight;
		 sprintf (text, "%d.%d.%d", j>>16, (j>>8) & 0xff, j & 0xff);
		 show_shadow_text(PT_FOREGROUND, FT_FIXED, text, i, IntligY-2, 
			-3, scaley-1, 2, 5, 4, IntroBorderColour, ST_LIGHT_STIPPLE);
/*		 show_shadow_text(PT_FOREGROUND, FT_VAR, text, i, IntligY - VarFontHeight*4+119,
			-3, -7, 2, 4, 4, IntroBorderColour, ST_LIGHT_STIPPLE);
*/
	 }

	 x = FracTextX - CubScrWidth;
     y = FracTextY + FracTextHeight *CubScrHeight + 20;

	 show_shadow_text(PT_FOREGROUND, FT_VAR,"Simsalabim", x, y, 2, 3, 0, 0,
								 5, IntroBorderColour, ST_LIGHT_STIPPLE);
	 show_shadow_text(PT_FOREGROUND, FT_VAR,"A.C.E.", i, y+VarFontHeight, -2, 4, 0, 0,
								 6, IntroBorderColour, ST_LIGHT_STIPPLE);

	 y +=  (VarFontHeight*3-3); 
	 show_shadow_text(PT_FOREGROUND, FT_VAR,"Software", x, y, 2, 3, 0, 0,
								 5, IntroBorderColour, ST_LIGHT_STIPPLE);

	 y +=  (VarFontHeight*3-3); 
	 show_shadow_text(PT_FOREGROUND, FT_VAR,"1990", x, y, 2, 3, 0, 0,
								 5, IntroBorderColour, ST_LIGHT_STIPPLE);

	 show_shadow_text(PT_FOREGROUND, FT_VAR,"2000", i, y-VarFontHeight+4, -2, 3, 0, 0,
								 6, IntroBorderColour, ST_LIGHT_STIPPLE);

   flush_all_pane(PT_FOREGROUND);
   if(delay) wait_clock(end_period, NULL);
}



void show_intro(void)
{
  int i, j, s;
  int ScrollScrY, ScrollHei;  
  int ScrollScrWidth;	

  game.level = IntroLevel;
#ifdef USE_BACKGROUND_MUSIC 
  process_music();
#endif

  FracTextX -= OffsetX; FracTextY -= OffsetY;
	if (WndHeight <= 400) FracTextY += 20;
  ScrollY -= OffsetY;

  show_intro_background();
  ScrollImage = text_to_bitmap(FT_VAR, ScrollerText, &ScrollImageWidth, &ScrollImageHeight);
  ScrollScrWidth = ScrollChunks[ScrollChunkCount-1].screen_x +
			 							 ScrollChunks[ScrollChunkCount-1].screen_width;
  ScrollPixHei = 62/16;///FontHeight[FT_VAR];
  ScrollHei = ScrollPixHei * ScrollImageHeight + ScrollPixHei/2+ 7;
  ScrollScrY = ScrollY-ScrollPixHei/2-2;

  copy_pane(PT_FOREGROUND, PT_BACKGROUND, 
                0, 0, WndWidth, WndHeight, 0, 0);
  copy_pane(PT_FOREGROUND, PT_TEMP, 
                0, 0, WndWidth, WndHeight, 0, 0);
	
/*	fill_vscroller_lines(); */
	if (VscrollText != NULL)
    VscrollImage = text_to_bitmap_multiline(VscrollText, 
				  VscrollLineWidth, &VscrollImageWidth, &VscrollImageHeight);

  if (ScrollImage || VscrollImage) 
	{ clock_t now, end_scroll, end_vscroll, end_interface;
		short new_vscroller, new_scroller;
		ACTION act;

		if (OffsetX != 0)
		{ for (i=0; i<ScrollChunkCount; i++)
	  	{ if (i == 0)
				  ScrollChunks[i].screen_width -= OffsetX;
				else
			  {	 ScrollChunks[i].chunk_pos -= OffsetX;
			 		 ScrollChunks[i].screen_x -= OffsetX;
			  }
		  }
		}
		i = ScrollChunkCount-1;
		ScrollChunks[i].screen_width = WndWidth - ScrollChunks[i].screen_x;					

		end_scroll = end_vscroll = end_interface = clock();
		s = WndHeight > 400 ? -230 : -190;
		i=-ScrollScrWidth;

		while(i<ScrollImageWidth*ScrollPixWid || s < VscrollImageHeight )
		{
			new_vscroller = new_scroller = 0;
			
			now = clock();
			
			if (now >= end_interface)
			{
              act = interface_routine(NULL);
			  switch(act)
			  {	case ACT_NONE:
				  break;
				  
				case ACT_LEFT:
				  if (ScrollTimeout>=20) ScrollTimeout -= 10;
				  break;
				  
			  	case ACT_RIGHT:
				  if (ScrollTimeout <=490) ScrollTimeout += 10;
				  break;

			  	case ACT_BACK:
				  if (VscrollTimeout>=20) VscrollTimeout -= 10;
				  break;
				  
			  	case ACT_FRONT:
				  if (VscrollTimeout <=790) VscrollTimeout += 10;
				  break;
				  
				default:
				  goto  Exodos; 
			  }				  

			/*  while(act != ACT_NONE) act = interface_routine(NULL); */

			  end_interface = now + ms2ticks(InterfaceTimeout);
			}				
			

			if (now >= end_vscroll && VscrollImage)
			{
			    copy_pane(PT_BACKGROUND, PT_FOREGROUND, 0, VscrollY, 
					 	   WndWidth, WndHeight-VscrollY, 0, VscrollY);
				show_vert_scroller(s);
				s += 1;
			    copy_pane(PT_FOREGROUND, PT_TEMP, 0, ScrollScrY, 
					 	   WndWidth, ScrollHei, 0, ScrollScrY);
			    end_vscroll = now + ms2ticks(VscrollTimeout);
				new_vscroller = 1; 						   
			}


			if ((new_vscroller || now >= end_scroll) && ScrollImage)
		    {
				if (new_vscroller == 0) 						   
	  			  copy_pane(PT_TEMP, PT_FOREGROUND, 0, ScrollScrY, 
			  			  WndWidth, ScrollHei, 0, ScrollScrY);

				for (j=0; j<ScrollChunkCount; j++)
			 	   	show_horz_scroller(j, i);

				if (now >= end_scroll) {
				  i += 3;
	 		      end_scroll = now + ms2ticks(ScrollTimeout);
				}				  
				new_scroller = 1; 						   
			}					


			if (new_vscroller)
			    flush_pane(PT_FOREGROUND, 0, VscrollY, 
	  				  WndWidth, WndHeight-VscrollY);
		    else
			if (new_scroller)
		        flush_pane(PT_FOREGROUND, 0, ScrollScrY, 
	  					  WndWidth, ScrollHei);
		
	  }

	}	

	
Exodos:
	fade_image(IntroBkgrColour);
/*	if (VscrollText) free(VscrollText); */
	if (VscrollImage)free(VscrollImage);
	if (ScrollImage) free(ScrollImage);
	
}

