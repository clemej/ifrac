/*  =================================================== */
/*  Intelligent FRAC.	(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <vga.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

#include "ifr.h"
#include "ifr_pixmap.h"

#include "ifr_graph_common.h"

extern  PLAYER *players[];
extern  GAME	game;
extern  int LastSaveLayer;
extern	int DirVideo;
PLATFORM pltf = PLTF_SVGA;

int BytesPerPixel;
int kbd_raw;

typedef unsigned char *BITMAP;
static int vga_mode;
static int	run_background;

/* The following fields are used for linear addressing */
static int	lbuf_len = -1;
static int	scan_line_width, screen_size, screen_page;
static unsigned char *lbuf_addr = NULL;

typedef struct 
{	int width;
	int height;
	char *pixels;
	int  start_colour;
	int  pageno;  // > 0 - using linear fb memory
	char end_colour;
} PIXMAP_INFO;

#define StippleCount 3
#define PaneCount 3
BITMAP all_stipples[StippleCount];
PIXMAP_INFO  *all_panes[PaneCount];


unsigned char *cm_pixels = NULL;

PIXMAP_INFO *create_pixmap(int width, int height, int pageno)
{	PIXMAP_INFO *pi;

	pi = (PIXMAP_INFO *)malloc(sizeof(PIXMAP_INFO));
	if (!pi) return NULL;

/*	if (lbuf_addr != NULL && screen_size*(pageno+1) <= lbuf_len)
	{ pi->pixels = lbuf_addr + screen_size * pageno;
	  pi->pageno = pageno;
	}
	else */
	{  pi->pixels = malloc(width * height * BytesPerPixel);
	   if (pi->pixels == NULL) { free(pi); return NULL; }
	   pi->pageno = 0;
	}

	pi->width = width;
	pi->height = height;
	return pi;
}

void free_pixmap(PIXMAP_INFO *pi)
{
	if (pi->pageno == 0)
		free(pi->pixels);
	free(pi);
}

/* ================================================================= */

static int set_static_stored_colours(int start_colour, int colour_count,
        const unsigned char rgb[])
{	int i;    
  int *pal_buffer, *pal_buf_ptr;
	const unsigned char *rgbptr;

	pal_buf_ptr = pal_buffer = (int *) malloc(StaticColours * 3 * sizeof(int));
	if (pal_buffer == NULL) return 0;
	rgbptr = rgb;
    
  for (i=0; i<colour_count; i++)
	{
	  *pal_buf_ptr++ = *rgbptr++ >> 2; 
    *pal_buf_ptr++ = *rgbptr++ >> 2; 
    *pal_buf_ptr++ = *rgbptr++ >> 2; 

		cm_pixels[start_colour+i] = start_colour + i;	
  }

	i = vga_setpalvec(start_colour, colour_count, pal_buffer);
	free(pal_buffer);
  return  (i == colour_count);

}

static int set_static_high_colours(int start_colour, int colour_count,
        const unsigned char rgb[])
{	int i, s;    
  unsigned char *pal_buf_ptr;
	const unsigned char *rgbptr;
	union { unsigned int s; unsigned char c[2]; } pix_union;
	unsigned short r, g, b;

	pal_buf_ptr = cm_pixels + start_colour*2;
	s = ScrDepth - 15; rgbptr = rgb;
    
  for (i=0; i<colour_count; i++)
	{	 r = *rgbptr++ >> 3;
		 g = *rgbptr++ >> (3-s);
		 b = *rgbptr++ >> 3;
		 pix_union.s = (r << (10+s)) | (g << 5) | b;
		 *pal_buf_ptr++ = pix_union.c[0];
		 *pal_buf_ptr++ = pix_union.c[1];
	}
  return  1;

}

static int set_static_true_colours(int start_colour, int colour_count,
        const unsigned char rgb[])
{	int i, s;    
  unsigned char *pal_buf_ptr;
	const unsigned char *rgbptr;

	pal_buf_ptr = cm_pixels + start_colour*BytesPerPixel;
	rgbptr = rgb; s = BytesPerPixel-3;

  for (i=0; i<colour_count; i++)
	{		*pal_buf_ptr++ = rgbptr[2];
			*pal_buf_ptr++ = rgbptr[1];
			*pal_buf_ptr++ = rgbptr[0];
			rgbptr += 3;
			if (s>0)
			{	memset(pal_buf_ptr, '\0', s); pal_buf_ptr += s; }
	}

  return  1;
}

int set_static_colours(int start_colour, int colour_count,
        const unsigned char rgb[])
{	switch (BytesPerPixel)
	{  case 1:
		return set_static_stored_colours(start_colour,
	             colour_count, rgb);
 
	   case 2:	 	
		return set_static_high_colours(start_colour,
	             colour_count, rgb);

	   default:	 	
		return set_static_true_colours(start_colour,
	             colour_count, rgb);
	}

}

/* ---------------------------------------------------------------- */
int set_bkgr_stored_colours(const unsigned short *base_colour,
                            const unsigned char *grey_levels, 
                          	int colour_start, int colours_used)
{ 
    unsigned short r, g, b, grey;
    int *pal_buffer,  *pal_buf_ptr;
    int x;

    pal_buf_ptr = pal_buffer = (int *) malloc(colours_used * 3 * sizeof(int));
		if (pal_buffer == NULL) return 0;

		r = base_colour[0];
		g = base_colour[1];
		b = base_colour[2];
    for (x = 0; x<colours_used; x++)
    {   grey = grey_levels[x];
				*pal_buf_ptr++ = (grey * r) >> 10;	
				*pal_buf_ptr++ = (grey * g) >> 10;	
				*pal_buf_ptr++ = (grey * b) >> 10;	
	   	  cm_pixels[colour_start+x] = colour_start+x;
    }

    x = vga_setpalvec(colour_start, colours_used, pal_buffer);

    free(pal_buffer);
  	return  (x == colours_used);
}

int set_bkgr_high_colours(const unsigned short *base_colour,
                            const unsigned char *grey_levels, 
                          	int colour_start, int colours_used)
{ 
    unsigned short r, g, b, r1, g1, b1, grey;
    unsigned char *pal_buf_ptr;
		union { unsigned int s; unsigned char c[2]; } pix_union;
    int x, s;

    pal_buf_ptr = cm_pixels + colour_start*2;

		r = base_colour[0];
		g = base_colour[1];
		b = base_colour[2];
		s = ScrDepth-15;
    for (x = 0; x<colours_used; x++)
    {   grey = grey_levels[x];
				r1 = (grey * r) >> 11;	
				g1 = (grey * g) >> (11-s);	
				b1 = (grey * b) >> 11;	
				pix_union.s = (r1 << (10+s)) | (g1 << 5) | b1;
				*pal_buf_ptr++ = pix_union.c[0];				
				*pal_buf_ptr++ = pix_union.c[1];				
    }

  	return 1;
}

int set_bkgr_true_colours(const unsigned short *base_colour,
                            const unsigned char *grey_levels, 
                          	int colour_start, int colours_used)
{ 
    unsigned short r, g, b, grey;
    unsigned char *pal_buf_ptr;
    int x, s;

    pal_buf_ptr = cm_pixels + colour_start*BytesPerPixel;

		r = base_colour[0];
		g = base_colour[1];
		b = base_colour[2];
		s = BytesPerPixel-3;

    for (x = 0; x<colours_used; x++)
    {   grey = grey_levels[x];
				*pal_buf_ptr++ = (unsigned char)((grey * b) >> 8);	
				*pal_buf_ptr++ = (unsigned char)((grey * g) >> 8);	
				*pal_buf_ptr++ = (unsigned char)((grey * r) >> 8);	
				if (s>0) { memset(pal_buf_ptr, '\0', s); pal_buf_ptr += s; }
		}

  	return 1;
}

int set_bkgr_colours(const MPMP_INFO *mpmpi, 
										 const unsigned short *base_colour,	int colour_start)
{ int colours_used;
  unsigned char *grey_levels;    

  colours_used = mpmpi->colours_used;
  grey_levels = mpmpi->grey_levels;    

	switch (BytesPerPixel)
	{  case 1:
			return set_bkgr_stored_colours(base_colour,grey_levels, 
                          	       colour_start, colours_used);
 
	   case 2:	 	
			return set_bkgr_high_colours(base_colour,grey_levels, 
                          	       colour_start, colours_used);

	   default:	 	
			return set_bkgr_true_colours(base_colour,grey_levels, 
                          	       colour_start, colours_used);
	}
}
/* ================================================================= */

void graph_end(void)
{
	int i;
	PIXMAP_INFO *xpm;

  if(vga_getcurrentmode() != TEXT)
			vga_setmode(TEXT);


	if (cm_pixels) free(cm_pixels);

	for (i=0; i<PaneCount; i++)
	{ xpm = all_panes[i];
     if (xpm != NULL)  free_pixmap(xpm);    
	}


}

int set_video_mode(void)
{	int row, col, initcol, flag = 1;
	const static int vmodes[2][6] =
		{ {G640x350x16, G640x350x16,  G640x350x16,  G640x350x16,
                            G640x350x16,  G640x350x16},
	 	  {G640x480x16, G640x480x256, G640x480x32K, G640x480x64K,
                            G640x480x16M, G640x480x16M32}
		};

	row = (VideoRes == VRES_LOW) ? 0 : 1;
	if (VideoBpp >= 32) initcol = 5;
	else
	if (VideoBpp >= 24) initcol = 4;
	else
	if (VideoBpp >= 16) initcol = 3;
	else
	if (VideoBpp >= 15) initcol = 2;
	else
	if (VideoBpp >= 8) initcol = 1;
	else  initcol=0;

	while (row >= 0)
	 {	col = (row==0) ? 0 : initcol;
			
		while (col >= 0)
		{ if (flag && (row==0 || col == 0))
		  {	vga_setchipset(VGA);
			flag = 0;
		  }
		  vga_mode = vmodes[row][col];
		  if(vga_setmode(vga_mode) >= 0)
			 return 1;
		  col--;
		}
	
		if (VideoRes != VRES_DEFAULT) break;
		row--;
	}

	return 0;

}


#if defined(VGA_GOTOBACK) && defined(VGA_COMEFROMBACK) 

#define SVGALIB_BACKGROUND_SUPPORT
static int palvec[256*3];

static void focus_out_routine(void)
{   
		vga_getpalvec(0, 256, palvec);
/*	  if (game.level < Levels && (players[0])->type == 0) */
		  game.status |= GMST_SUSPENDED;	 
}

static void focus_in_routine(void)
{
    game.status &= ~GMST_SUSPENDED;	 
		vga_setpalvec(0, 256, palvec);
		reset_advance();
}

static void set_background_routines(void)
{
/* 	  run_background = vga_runinbackground_version(); */
 	  run_background = 1;

    if (run_background)			
    {    vga_runinbackground(VGA_GOTOBACK, focus_out_routine);	
         vga_runinbackground(VGA_COMEFROMBACK, focus_in_routine);
    }
}

#endif  /* defined(VGA_GOTOBACK) ...  */

int graph_init_video(void)
{
  int 	rc = 0;
	int   screen_colours;
	vga_modeinfo *vminfo;
	
	if (tcgetpgrp(STDIN_FILENO) != getpid())
	{	fprintf(stderr, "Sorry, this application cannot start in background\n");
		return 0;
	}

	pltf = PLTF_SVGA;

  /*-----------------------------------------------------------*/     
	/* Checking libvga version, 1.4.0 and prior will return (-1) */
	/* Currently you need IOPERM for 1.9.x to run as a non-priv  */
	/* user, however set IOPERM upsets upsets previous versions  */
  /*-----------------------------------------------------------*/     
  if (vga_setmode(-1) >= 0x1900)	setenv("IOPERM", "1", 0);  

	rc = vga_init();
	if (rc != 0)
	{ fprintf(stderr, "Can't initialise SVGA library. \n"
     		    "Possible solution: log as root, and enter:\n"
		    "\tchown 0 ifrac\n"
		    "\tchmod 4755 ifrac\n");
	  return 0;
	}	

#ifdef SVGALIB_BACKGROUND_SUPPORT
  set_background_routines();
#else
	run_background = 0;
#endif

	if (!set_video_mode())
	{  fprintf(stderr, "Sorry, unable to set video mode. \n");
      	   return 0;		
	}

	vminfo = vga_getmodeinfo(vga_getcurrentmode());
	
	WndWidth = ScrWidth = vminfo->width;
	WndHeight = ScrHeight = vminfo->height;

	screen_colours = vminfo->colors;
	if (screen_colours < 16)
	{	fprintf (stderr,
		   "This application requires at least 16-colour mode\n");
		return 0;
	}	 

	ScrDepth = 0;
	while((screen_colours>>=1) != 0) ScrDepth++;

	if (ScrDepth <= 8)
	{  BytesPerPixel = 1;
	   stored_colours = 1;
	}
	else
	{  BytesPerPixel = vminfo->bytesperpixel;
	   stored_colours = 0;
	}
	
	scan_line_width = vminfo->linewidth;
	screen_size = scan_line_width * ScrHeight;

	if (ScrDepth < 8) 
		DirVideo = 0;
	else {
		vga_setpage(screen_page=0);

		if (DirVideo == 2 && (vminfo->flags & CAPABLE_LINEAR)) 
			lbuf_len = vga_setlinearaddressing();
	
		if (DirVideo > 0) 
			lbuf_addr = vga_getgraphmem();
	}
	
	return  1;
}

int graph_start(void)
{
        int i;
  PIXMAP_INFO  *xpm;
        unsigned char   bw_colours[6];

/*      xmode = (vminfo->flags & IS_MODEX) ? 1: 0; */

/*  cm_pixels = (unsigned char *) malloc(StaticColours * BytesPerPixel); */
        if (BytesPerPixel > 1) i = 256; else i = 1 << ScrDepth;
  cm_pixels = (unsigned char *) malloc(i * BytesPerPixel);
  if (cm_pixels == NULL) return 0;

        for (i=0; i<PaneCount; i++)
                all_panes[i] = NULL;

        for(i=0; i<3; i++)
        {  bw_colours[i] = 0;
           bw_colours[i+3] = 255;
        }

        if (!set_static_colours(0, 2, bw_colours))
                                return 0;

        all_stipples[ST_NO_STIPPLE] = NULL;
        all_stipples[ST_DARK_STIPPLE] = quarter_bmp_bits;
        all_stipples[ST_LIGHT_STIPPLE] = half_bmp_bits;

        for (i=0; i<PaneCount; i++)
        {  xpm = create_pixmap(WndWidth, WndHeight, i+1);
           if(xpm == NULL) break;
           all_panes[i] = xpm;
        }

        return (i>=PaneCount);
}

void get_graphics_line(char *value)
{	char *video_access;
	int  bpp;

	if (lbuf_addr == NULL)
		video_access = "";
	else
	if (lbuf_len < screen_size)
		video_access = " banked";
	else
		video_access = " linear";

	bpp = (ScrDepth < 8) ? ScrDepth : BytesPerPixel*8;		
	snprintf (value, 81, "Mode %d: %dx%d %dbpp%s, kbd: %s, bkgr: %s",
  	  vga_getcurrentmode(), ScrWidth, ScrHeight, bpp,
	   video_access, kbd_raw ? "raw" : "cooked",
          run_background ? "yes" : "no");
}


void flush_line(int offset, char *buffer, int width)
{	int	tmp;

	
	if (lbuf_len <= screen_size)
	{  // Banked memory
		tmp = offset >> 16;
		if (tmp != screen_page) 
			vga_setpage(screen_page = tmp);
		offset &= 0xffff;
		tmp = 0x10000 - offset;
		if (tmp > width) tmp = width;
	}
	else
		tmp = width;

	memcpy(lbuf_addr + offset, buffer, tmp);
	if (tmp < width) 
	{	vga_setpage(++screen_page);
		memcpy(lbuf_addr, buffer+tmp, width-tmp);
	}		

}


void flush_all_pane(PANE_TYPE which_pane)
{
	flush_pane(which_pane, 0, 0, WndWidth, WndHeight);

}

void flush_pane(PANE_TYPE which_pane, int x, int y, int wid, int hei)
{
	char *image;
	int  width;
	PIXMAP_INFO *pixmap;

/*	if (x+wid > ScrWidth) wid = ScrWidth-x; */
	pixmap = all_panes[which_pane];
	width = pixmap->width * BytesPerPixel;
	image = pixmap->pixels+ y*width+ x * BytesPerPixel;
	wid   *= BytesPerPixel;

	if (lbuf_addr == NULL)
	{					
		while (--hei>=0)
		{	
			vga_drawscansegment(image, x, y, wid);
			y++; image += width;
		}
	}
	else
	{ 
		int offset = y*scan_line_width + x*BytesPerPixel;
		while (--hei>=0)
		{	
			flush_line(offset, image, wid);
			offset += scan_line_width;
			image += width;
		}
	} 				
	
}

static void copy_pmp(PIXMAP_INFO *source, PIXMAP_INFO *destination, 	
          int srcx, int srcy, int wid, int hei, int destx, int desty)
{
	char *src, *dest;
	int srcwid, destwid;

/*	if (destx+wid > ScrWidth) wid = ScrWidth-destx; */
/*	if (srcx+wid > ScrWidth) wid = ScrWidth-srcx;   */
	srcwid = source->width * BytesPerPixel;
	destwid = destination->width * BytesPerPixel;
	src  = source->pixels + srcy*srcwid + srcx*BytesPerPixel;
	dest = destination->pixels + desty*destwid + destx*BytesPerPixel;
	wid *= BytesPerPixel;
	
	while (--hei >= 0)
	{ memcpy(dest, src, wid);
	  src += srcwid;
	  dest += destwid;
	}
}

void copy_pane(PANE_TYPE pt_source, PANE_TYPE pt_destination, 	
          int srcx, int srcy, int wid, int hei, int destx, int desty)
{
	copy_pmp(all_panes[pt_source], all_panes[pt_destination],
				srcx, srcy, wid, hei, destx, desty);
}


void  draw_line(PANE_TYPE which_pane, int x, int y,
			     int wx, int wy, COLENTRY fillcol,
                 COLENTRY bgcol, STIPPLE_TYPE which_stipple)
{
	PIXMAP_INFO *pane;
	BITMAP  stipple;

	int swx, swy, count;
	int dxnum, dxden, dynum, dyden;
	int  width, x1, y1;
	unsigned char *image, *colptr, *imptr;
	COLENTRY  col;


	/* Get sign and abs values */
	swx = (wx >= 0) ? 1 : -1;	
	swy = (wy >= 0) ? 1 : -1;	
	wx *= swx;  wy *= swy;

	if (wx >= wy)
	{ /* Going along x */
		if (wx == 0) return;
		dxnum=swx; dxden = 1;
		dynum=wy*swy; count = dyden=wx;
	}
	else
	{ /* Going along y */
		dynum=swy; dyden = 1;
		dxnum=wx*swx; count = dxden=wy;
	}


	pane = all_panes[which_pane];
	stipple = all_stipples[which_stipple];

	width = pane->width;
	image = pane->pixels;
	count++;
	
	col = fillcol;
 
	x1 = x * dxden;
	y1 = y * dyden;
	while(--count>=0)
	{  x = x1/dxden; y = y1/dyden;		
	  if (stipple) 
	  	     col = stipple[y&1] & (1<<(x&1)) ? fillcol : bgcol;
     if(col >= 0 && x < width)
		 {  imptr = image + (width*y + x) * BytesPerPixel;
				colptr = cm_pixels + col * BytesPerPixel;
				memcpy(imptr, colptr, BytesPerPixel);
			  imptr += BytesPerPixel;
		 }
	   x1 += dxnum;  y1 += dynum;
		 	
	}

}

/* which_pane:  0 - draw in background, 1 - draw in foreground */
void  fill_hor_trapezium(PANE_TYPE which_pane, int x, int y,
	          int w, int h, int dw1, int dw2,
			  COLENTRY fillcol, COLENTRY bgcol, STIPPLE_TYPE which_stipple)
{

	int i;

	if (h==0) return;
	
	dw2 -= dw1;
	if (h<0) 
	{  x += dw1; y += h; w += dw2;
	   h = -h;  dw1 = -dw1; dw2 = -dw2;
	}	   
	   
	w *= h; x *= h;
		
	for (i=0; i<h; i++)
	{  draw_line(which_pane, x/h, y, w/h, 0, 
	       fillcol, bgcol, which_stipple);
	   x += dw1;  w += dw2;  y++;				  
	}
	

}			  

/* which_pane:  0 - draw in background, 1 - draw in foreground */

void  fill_parallelogramm(PANE_TYPE which_pane, int x, int y,
			        int wx, int wy, int hx, int hy,
				    COLENTRY fillcol, COLENTRY bgcol, STIPPLE_TYPE which_stipple)
{
	int swx, swy, count;
	int dxnum, dxden, dynum, dyden;

	/* Get sign and abs values */
	swx = (wx >= 0) ? 1 : -1;	
	swy = (wy >= 0) ? 1 : -1;	
	wx *= swx;  wy *= swy;

	if (wx >= wy)
	{ /* Going along x */
		dxnum=swx; dxden = 1;
		dynum=wy*swy; count = dyden=wx;
	}
	else
	{ /* Going along y */
		dynum=swy; dyden = 1;
		dxnum=wx*swx; count = dxden=wy;
	}

	x *= dxden;
	y *= dyden;
	while(--count>=0)
	{  draw_line(which_pane, x/dxden, y/dyden, hx, hy, fillcol, bgcol, which_stipple);
	   x += dxnum; y += dynum;
	}  	

}

int set_line_colour( unsigned short base_colour[3])
{
   unsigned char rgb[3];
	 int i;

	 for (i=0; i<3; i++)
			rgb[i] = (unsigned char) base_colour[i]; 
	 
	 return set_static_colours(line_colour, 1, rgb);
}


int set_bkgr_line_colours(const MPMP_INFO *mpmpi, unsigned short base_colour[3])
{
	return 
		set_bkgr_colours(mpmpi, base_colour, StaticColours) &&
		set_line_colour(base_colour);
}

/*
int insert_to_pixmap(int which_pane, const MPMP_INFO *mpmpi,
	      unsigned short  base_colour[3],
			int sx, int sy,	 int colour_start)
{
    PIXMAP_INFO  *xpm;    
	int  width, height;
    unsigned char *pixels,  *bkgr_xpm_ptr, p;    

	int  x, y;

    if (!set_bkgr_colours(mpmpi, base_colour, colour_start)) return 0;
    width = mpmpi->width;
    height = mpmpi->height;

	xpm = all_panes[which_pane];

    bkgr_xpm_ptr = xpm->pixels + sy*WndWidth + sx;
    pixels = mpmpi->pixels;
   
    for (y=0; y<height; y++)
    {
	   for (x=0; x<width; x++)	
	   {   p = *pixels++;
		   if (p!=0)  *bkgr_xpm_ptr = colour_start + p;  
		   bkgr_xpm_ptr++;
		}
	
		bkgr_xpm_ptr += (WndWidth-width);
    }
	return 1;
}
*/

int fill_background_pixmap(int which_pane, const MPMP_INFO *mpmpi,
      const unsigned short  *base_colour, int first_colour)
{
    PIXMAP_INFO  *xpm;    
    unsigned char *pixels,  *bkgr_xpm_ptr;    
		int	x, y, width, height;

	  if (base_colour &&
		 		!set_bkgr_colours(mpmpi, base_colour, first_colour)) return 0;

    width = mpmpi->width;
    height = mpmpi->height;

		xpm = all_panes[which_pane];

    bkgr_xpm_ptr = xpm->pixels;
    pixels = mpmpi->pixels + OffsetY*width + OffsetX;
   
    for (y=0; y<WndHeight; y++)
    {
		  for (x=0; x<WndWidth; x++)	
			{  memcpy(bkgr_xpm_ptr,
               cm_pixels+(first_colour+*pixels++)*BytesPerPixel,
               BytesPerPixel);  
			  bkgr_xpm_ptr += BytesPerPixel;
			}

			pixels += (width - WndWidth);
    }

	return 1;
}
	      

ACTION morph_to_colour(unsigned short new_base_colour[3])
{
	int i, k;
  unsigned short mid_colour[3];
  unsigned long next_clock;
  ACTION new_action = ACT_NONE;

  for (k=1; k<=16; k++)
  {
    next_clock = start_clock(DRAW_LEVEL_DELAY2);
    for (i=0; i<3; i++)
	  mid_colour[i] = (current_base_colour[i] * (16-k) +
					  new_base_colour[i] * k) >> 4;
  	
		if (!set_bkgr_line_colours(&mandel_mpmp_info, mid_colour))
				  return new_action;
    new_action = wait_clock(next_clock, NULL);
		if (new_action != ACT_NONE) break;
  }

  set_bkgr_line_colours(&mandel_mpmp_info, new_base_colour);
  return new_action;  
}

ACTION morph_to_image(PANE_TYPE source_pane)
{
  int  x, y, i;
  unsigned long next_clock;
  ACTION new_action = ACT_NONE;
  PIXMAP_INFO *from, *to;
  unsigned char *ptrf, *ptrt;
  unsigned char *ptrf1, *ptrt1;
    
  from  = all_panes[source_pane];
  to  = all_panes[PT_FOREGROUND];
  i = MORPH_STEP;

  while (--i >= 0)
  {
    next_clock = start_clock(DRAW_LEVEL_DELAY1);

	for (y=0; y<WndHeight; y++)
	{	
	  ptrf = from->pixels + y*WndWidth*BytesPerPixel;
	  ptrt = to->pixels + y*WndWidth*BytesPerPixel;

	  for (x= (y*3+i) % MORPH_STEP; x<WndWidth; x += MORPH_STEP)
	  {	 ptrf1 = ptrf + x*BytesPerPixel;
		 ptrt1 = ptrt + x*BytesPerPixel;
		 memcpy(ptrt1, ptrf1, BytesPerPixel);
	  }
	}

	flush_all_pane(PT_FOREGROUND);
	new_action = wait_clock(next_clock, NULL);
    if (new_action != ACT_NONE) break;
  }			

  if (i>=0)
  {	copy_pane(source_pane, PT_FOREGROUND, 0, 0, WndWidth, WndHeight, 0, 0);
    	flush_all_pane(PT_FOREGROUND);
  }	 
  return new_action;
}

ACTION fade_image(COLENTRY colentry)
{
  int  x, y, i;
  unsigned long next_clock;
  ACTION new_action = ACT_NONE;
  PIXMAP_INFO  *to;
  unsigned char *ptrt, *ptrt1, *colptr;

	colptr = cm_pixels + colentry * BytesPerPixel;    
  to  = all_panes[PT_FOREGROUND];
  i = MORPH_STEP;

  while (--i >= 0)
  {
    next_clock = start_clock(DRAW_LEVEL_DELAY1);

		for (y=0; y<WndHeight; y++)
		{ 	ptrt = to->pixels + y*WndWidth*BytesPerPixel;

		  	for (x= (y*3+i) % MORPH_STEP; x<WndWidth; x += MORPH_STEP)
			  {	 ptrt1 = ptrt + x*BytesPerPixel;
					 memcpy(ptrt1, colptr, BytesPerPixel);
			  }
		}

    flush_all_pane(PT_FOREGROUND);
		new_action = wait_clock(next_clock, NULL);
    if (new_action != ACT_NONE) break;
  }			

  return new_action;
}

void boss_routine(void)
{
	PLAYER *player;
	struct passwd *pwds;
	char *shell_line, *shell;
#ifdef USE_BACKGROUND_MUSIC
	extern MUSIC_STATUS mstatus;
	MUSIC_STATUS old_status = mstatus;
#endif	


#ifdef USE_BACKGROUND_MUSIC
	 mstatus &= ~MUSIC_SUSPENDED;
   stop_background_music(0);
#endif


	player = players[0];
	pwds = getpwuid(player->uid);
	if (pwds == NULL)
		shell = "sh";
	else
		shell = pwds->pw_shell;

	shell_line = malloc(strlen(shell) + 5);
	sprintf(shell_line, "%s", shell);

	suspend_app(1);

	system("clear");
	system("ls");
	system(shell_line);

#ifdef USE_BACKGROUND_MUSIC
	 mstatus |= (old_status & MUSIC_SUSPENDED);
	 if (old_status & MUSIC_ON)	
 			  start_background_music();
#endif

	free(shell_line);		
	resume_app();

	reset_advance();
}


int  unboss_routine(int mode)
{
    return 4;
}	  

static int *suspend_colmap = NULL;

void suspend_app(int hide)
{
  if (stored_colours)
  {
	suspend_colmap = (int *)malloc(256*3*sizeof(int));
	if (suspend_colmap) vga_getpalvec(0, 256, suspend_colmap);
  }		
  else
    suspend_colmap = NULL;

  kbd_interface_end();
  vga_setmode(TEXT);
}

int resume_app(void)
{
  vga_setmode(vga_mode);

  if (suspend_colmap)
  {	
		vga_setpalvec(0, 256, suspend_colmap);
		free((void *) suspend_colmap);
		suspend_colmap = NULL;
  }	
	
	flush_all_pane(PT_FOREGROUND);	
  LastSaveLayer = -1;
  kbd_interface_start();
  return 1;
}

int is_full_screen(void)
{
	return 1; 
}

void show_cursor(int show)
{ }
