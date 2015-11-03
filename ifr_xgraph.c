#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
//#include <X11//cursorfont.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include "ifr.h"
#include "ifr_pixmap.h"
#include "ifr_graph_common.h"

#ifdef USE_XDGA
#include <X11/extensions/xf86dga.h>
#endif

  
PLATFORM pltf = PLTF_X11;
CMAP_TYPE cm_type;

extern int	PrivateColours;
extern  int LastSaveLayer;
extern int	ResumeMode;
/* extern char * ifr_icon_xpm[]; */

static int 	xf86release = 0;

#ifdef USE_XDGA
extern  DIR_GRAPHIC	DirGraphic;
int	dgaEventBase, dgaErrorBase;
static int	dgaVerMajor, dgaVerMinor;
static int 	dgaModeNeeded, dgaModeOriginal;
int    dgaModeCurrent;
#endif

Display *disp = NULL;
Drawable	wnd;
int		scrno;
GC	gc;

static int suspended = 0;

typedef unsigned long COLOUR;

static  Visual	*visual;
static  int	vis_class;

static  Pixmap WindowIcon = None;
//static  Cursor defCursor;

static	Window	root_wnd;
static  Colormap cm;
static	COLOUR  white_pix, black_pix;

static	int   alloc_colour_count = 0;
static  int   StartX, StartY;

COLOUR	*cm_pixels=NULL;

#define StippleCount 3
#define PaneCount 3
Pixmap all_stipples[StippleCount];
Drawable  all_panes[PaneCount];

static const char *WindowIdAtomName = "XIFRAC_WINDOW_ID";
static Atom	 WindowIdAtom = None;

void visualize_pane(PANE_TYPE which_pane, int x, int y, int wid, int hei);
static Status find_bossed_window(Window *pwnd, pid_t *ppid);
/* static Pixmap create_icon(char *pixmap[]); */


static unsigned short extend_colour_component(unsigned int c)
{
    unsigned short result;
    
    result = c << 8;
    if ((c & 0xf) == 0xf) result |= 0xff;
    
    return result; 
}  

static int alloc_colour_space(void)
{
    size_t space_used;
	if (ScrDepth <= 4) space_used = 17;
	else    space_used = alloc_colour_count;

    space_used *= sizeof(COLOUR);
	cm_pixels = (COLOUR *) malloc(space_used);
    return (cm_pixels != NULL);
	
}

static int alloc_colour_cells(void)
{
	return XAllocColorCells(disp, cm, False,
	       NULL, 0, cm_pixels, alloc_colour_count);
}    

static void set_bw_colours(void)
{
	XColor xc;
	
	memset(&xc, '\0', sizeof(XColor));
	xc.pixel = cm_pixels[0];
//	xc.red = xc.green = xc.blue = 0;
	xc.flags = DoRed | DoGreen | DoBlue;
	XStoreColor(disp, cm, &xc);
	
	xc.pixel = cm_pixels[1];
	xc.red = xc.green = xc.blue = 0xffff;
	xc.flags = DoRed | DoGreen | DoBlue;
	XStoreColor(disp, cm, &xc);
		
}    


int set_static_colours(int start_colour, int colour_count,
         const unsigned char rgb[])
{	
    XColor  xc;
		const unsigned char *rgbptr;
   
    int i;    
    
    memset (&xc, '\0', sizeof(xc));
    xc.flags = DoRed | DoGreen |  DoBlue;
		rgbptr = rgb;
    
    for (i=0; i<colour_count; i++)
    {
      xc.red = extend_colour_component(*rgbptr++);
      xc.green = extend_colour_component(*rgbptr++);
      xc.blue = extend_colour_component(*rgbptr++);

    
      if (stored_colours)
	  {  xc.pixel = cm_pixels[i+start_colour];
/*	   	if (!XStoreColor(disp, cm, &xc)) break; */
		 XStoreColor(disp, cm, &xc);
	  }
	  else
	  {       
  		if (!XAllocColor(disp, cm, &xc)) break; 
	  	cm_pixels[i+start_colour] = xc.pixel;	
	  }	       
    }

    return 1;
}

#ifdef USE_XDGA
static Status check_dga_version(void)
{

  if (!XDGAQueryExtension(disp, &dgaEventBase, &dgaErrorBase))
  {
	fprintf (stderr, "Sorry, this X configuration does appear to support XF86DGA ! \n");
	if (xf86release < 4000)
  	  fprintf (stderr, "Consider upgrading to XFree86-4.0+\n");
	else
	  fprintf (stderr, "Possible solution: disable \"omit xfree86-dga\" in XF86Config.\n");

    fprintf (stderr, "See 'Using XF86DGA' in ifrac INSTALL for more hints.\n");
	return False;   
  }
	  

  if (!XDGAQueryVersion(disp, &dgaVerMajor, &dgaVerMinor) ||
     dgaVerMajor < 2)
  {
	fprintf (stderr, "Sorry, this required XF86DGA 2.0, found XF86DGA %d.%d\n",
	                  dgaVerMajor, dgaVerMinor);
  	fprintf (stderr, "Consider upgrading to XFree86-4.0+\n");
	return False;   
  }

  return True;  		
}	

Status select_dga_mode(int width, int height)
{
  XDGAMode *modes, *modes1 = NULL;
  int i, modecount;
  int dist, mindist = 0; /* To avoid gcc complaint */
  int cwid, chei;
  int bwid=0, bhei=0;
  

    
  modes = modes1 = XDGAQueryModes(disp, scrno, &modecount);
  if (modes == NULL) modecount = 0; 	/* Forces printing error later */
  else dgaModeOriginal = modes->num;
  
  for(i=0; i<modecount; i++)
  { 
	cwid = modes->viewportWidth;
	chei = modes->viewportHeight;

	/* A dumb algorithm, but should prevent from overflow 
	   in case of huge screens */	
	if (cwid >= width && chei >= height && 
		modes->depth == ScrDepth  && 
	       ((modes->flags) & XDGAPixmap) == XDGAPixmap ) 
	{  dist = (cwid-width) + (chei-height);
	   if (dgaModeNeeded < 0 || dist < mindist)
	   { dgaModeNeeded = modes->num;
	     bwid = cwid; bhei = chei;
	     mindist = dist;
		 if (dist == 0) break;
	   }		 		
        }
        modes++;		
  }
  
  if (modes1) XFree(modes1);
  
  if (dgaModeNeeded < 0) {
  	fprintf (stderr, "Could not find an appropriate video mode\n");
	return False;
  }
  
  ScrWidth = bwid; ScrHeight = bhei;
  	
  if (bwid != width || bhei != height)
  {
  	fprintf (stderr, "\nWarning: Could not get exact screen size, %dx%d is used\n",
	                  bwid, bhei);
    fprintf (stderr, "Hint: video mode %dx%d should be listed in \"Modes\" line\n", width, height);
    fprintf (stderr, "for the current depth: use xf86config, or add mode to\n");
	fprintf (stderr, "XF86Config: \"Screen\"/\"Display\"/\"Modes\"\n");
    fprintf (stderr, "See 'Using XF86DGA' in ifrac INSTALL for more hints.\n");
  } 
      
  return True;

}

static void start_dga(void)
{  	XDGADevice *dev;

	wnd = None;
	dev = XDGASetMode (disp, scrno, dgaModeNeeded);
    if (dev == NULL) return;

	dgaModeCurrent = dgaModeNeeded;
  	wnd = dev->pixmap;
	XFree(dev);

	XDGASetViewport(disp, scrno, 0, 0, XDGAFlipRetrace);

    if (PrivateColours) 
    { 
	XDGAInstallColormap(disp, scrno, cm);
    }

   if (WndWidth < ScrWidth || WndHeight < ScrHeight)
	  XDGAFillRectangle(disp, scrno, 0, 0, ScrWidth, ScrHeight, black_pix);

}

static void terminate_dga(void)
{
	if (dgaModeCurrent != dgaModeOriginal) {
	   XDGASetMode(disp, scrno, dgaModeOriginal); /* Revert to original resolution */
	   XSync(disp, True);
	}	   
		       
	 XDGASetMode(disp, scrno, 0);     /* Disable DGA */
	 wnd = None;

     dgaModeCurrent = -1;
}
#endif  

int select_video_mode(void)
{
  switch(VideoRes)
  {	
	case VRES_NORMAL:
	  WndWidth = 640;		    
	  WndHeight = 480;		    
	  break;

  	case VRES_LOW:
	  WndWidth = 600;		    
	  WndHeight = 400;		    
	  break;

	case VRES_DEFAULT:
#ifdef USE_XDGA
	  if (DirGraphic == DGA_ZOOM)  
		{ WndWidth = 640;
	  	WndHeight = (ScrHeight < 480) ? 400 : 480;
		} 
    else
#endif
    {
      WndWidth = (ScrWidth <= 640) ? 600 : 640;
	    WndHeight = (ScrHeight <= 480) ? 400 : 480;
		}
	  break;
  }						

  
#ifdef USE_XDGA
  if (DirGraphic == DGA_ZOOM) 
	{ WndWidth = 640;
	  return select_dga_mode(WndWidth, WndHeight);
	}  	 

  if (DirGraphic == DGA_NOZOOM && 
	  WndWidth <= ScrWidth && WndHeight <= ScrHeight) 
	return select_dga_mode(ScrWidth, ScrHeight);
#endif  

  if  (WndWidth > ScrWidth || WndHeight > ScrHeight) 
  {	  fprintf (stderr, "Insufficient screen resolution: "
                 "%dx%d is required\n", WndWidth, WndHeight);
	  return 0;
  }
  return 1;

}

void graph_end(void)
{
    
    if (disp != NULL)
    {
		int i;
		Drawable pane;
		Pixmap  stipple;

		if (WindowIcon != None)  XFreePixmap(disp, WindowIcon);
		
		if (WindowIdAtom != None) 
		{  pid_t pid_bossed;
		   if (find_bossed_window(NULL, &pid_bossed) && pid_bossed == getpid())
		       XDeleteProperty(disp, root_wnd, WindowIdAtom);
		}		   
	
		if (cm_pixels)	
		{   if (stored_colours && alloc_colour_count && PrivateColours  )
		      XFreeColors(disp, cm, cm_pixels, alloc_colour_count, 0);
		    free(cm_pixels);
		}	    

		if (PrivateColours  && cm != None) XFreeColormap(disp, cm);
		if (gc != None) XFreeGC(disp, gc);
		for (i=0; i<StippleCount; i++)
		{   stipple = all_stipples[i];
			if (stipple != None) XFreePixmap(disp, stipple);
		}
		for (i=0; i<PaneCount; i++)
		{   pane = all_panes[i];
		    if (pane != None) XFreePixmap(disp, (Pixmap)pane);
		}


#ifdef USE_XDGA
		if (dgaModeCurrent >= 0)
		  terminate_dga();
		else  
#endif
//	   if (defCursor != None)
//			XFreeCursor(disp, defCursor);

		if (wnd) XDestroyWindow(disp, wnd);
		XCloseDisplay(disp);
    }

}

int graph_init_video(void)
{
  Screen	*scr;

  pltf = PLTF_X11;

  disp = XOpenDisplay(NULL);
  if (disp == NULL)
  {  fprintf (stderr, "Can't open display: this is an X-Window application! \n");
     return 0;   
  }

  xf86release = VendorRelease(disp);

#ifdef USE_XDGA
  dgaModeCurrent = dgaModeNeeded =  dgaModeOriginal = -1;

  if (DirGraphic != DGA_NOTUSED)
  {
    if (!check_dga_version()) return 0;
	PrivateColours = 1;
  }	
#endif

  WindowIdAtom = XInternAtom(disp, WindowIdAtomName,  False); 
 
  scr = DefaultScreenOfDisplay(disp);
  root_wnd = scr->root;
  ScrWidth = scr->width;
  ScrHeight = scr->height;
  cm = scr->cmap;
  visual = scr->root_visual;
  scrno = DefaultScreen(disp);

  white_pix = scr->white_pixel;
  black_pix = scr->black_pixel;
    
  ScrDepth = scr->root_depth;
  if (ScrDepth < 4)
  {
	fprintf(stderr, "Sorry, at least 16 colours are needed to run the application");
	return 0;	    
  }

  
  vis_class = visual->class;

  all_panes[PT_BACKGROUND] = None;
  all_panes[PT_FOREGROUND] = None;

  if (PrivateColours &&
		vis_class != PseudoColor  && vis_class != TrueColor  )
  {  
     XVisualInfo vinfo;
       

     if (XMatchVisualInfo(disp, scrno, ScrDepth, PseudoColor,
				   &vinfo))
	{  visual = vinfo.visual;
	   vis_class = visual->class;
	}	     
  }

  switch(vis_class)
  {
	case TrueColor:
	  cm_type = CM_TRUE;
	  break;
	    
	case PseudoColor:
	  cm_type = CM_PALETTE;
	  break;

	default:
	  cm_type = CM_STATIC;
  }	    


  stored_colours = (cm_type == CM_PALETTE);
			
  if (!stored_colours) PrivateColours = 0;
  return   select_video_mode();

}

int graph_start(void)
{
    XGCValues xgcv; 
	Pixmap    stipple, xpm;
    int 	rc = 0;
    int  	x, y;

    /* ------------- Colour allocation ------------ */
	if (ScrDepth <= 4)
	  alloc_colour_count = 16;
	else
	{ 	alloc_colour_count = StaticColours + mandel_mpmp_info.colours_used;
		  x = StaticColours + hypoth_mpmp_info.colours_used;

//		x = IntroStaticColours + hypoth_mpmp_info.colours_used;
//					           + intlig_mpmp_info.colours_used;
//		x = IntroStaticColours + julia_mpmp_info.colours_used
//					           + intlig_mpmp_info.colours_used;
		if (x > alloc_colour_count) alloc_colour_count = x;
		if (ScrDepth < 16) 
		{	x = 1 << ScrDepth;
			if (x < alloc_colour_count) alloc_colour_count = x;
		}
	}		

    if (!alloc_colour_space()) goto BadLuck;
    if (cm_type == CM_PALETTE && PrivateColours == 0)
    {
       if (!alloc_colour_cells())
       { fprintf(stderr, "xifrac: couldn't allocate all colours; private colour map will be used\n");
          PrivateColours = 1;
       }	
		
    }

	cm_pixels[bkgr_colour] = black_pix;
	cm_pixels[inverse_colour] = white_pix;
    	
    /* Creating a colour map */
    if (PrivateColours) 
    {
	    cm = XCreateColormap(disp, root_wnd, visual, AllocNone);
        if (cm == None) goto BadLuck;
        if (cm_type == CM_PALETTE && 
	      !alloc_colour_cells()) goto BadLuck;
		set_bw_colours();
    }

  x = (ScrWidth - WndWidth) /2;
  y = (ScrHeight - WndHeight)/2;


#ifdef USE_XDGA
  if (DirGraphic != DGA_NOTUSED)
  {
	if (dgaModeNeeded < 0) goto BadLuck;
	StartX = x; StartY = y;
//	defCursor = None;


	start_dga();  
	if (wnd == None) goto BadLuck;
  }	  
  else
#endif      
  {
      XSizeHints	xsh;
	  XClassHint	xch;
      XSetWindowAttributes xswa;

	  xswa.colormap = cm;
  //    xswa.backing_store = Always; // WhenMapped;

//	  xswa.cursor = defCursor = XCreateFontCursor(disp, XC_arrow);


      wnd = XCreateWindow(disp, root_wnd, x, y, WndWidth, WndHeight,
	       1, ScrDepth, InputOutput, visual,
	                    /*  CWBackingStore  | CWCursor | */ CWColormap, &xswa);
	  if (wnd == None) goto BadLuck;



    /* Create background pixmap */
//    XChangeWindowAttributes(disp, wnd,
//            CWBackingStore | CWColormap /* | CWBackPixmap */,  &xswa);
    
  	  /* Size hints */
      xsh.max_width = xsh.min_width = WndWidth;
	  xsh.max_height = xsh.min_height = WndHeight;
    
  	  xsh.flags = PMaxSize | PMinSize;
      XSetWMNormalHints(disp, wnd, &xsh);
    
	  /* Class hints */
  	  xch.res_name = "xifrac";
      xch.res_class = "XIfrac";
	  XSetClassHint(disp, wnd, &xch);
    
  	  XStoreName(disp, wnd, "Intelligent FRAC");    

	  
  }

    /* ------------- GC ------------ */

	all_stipples[ST_NO_STIPPLE] = None;
    all_stipples[ST_DARK_STIPPLE] =
    stipple = XCreateBitmapFromData(disp, wnd, quarter_bmp_bits, 8, 2);
    if (stipple == None) goto BadLuck;

    all_stipples[ST_LIGHT_STIPPLE] =
    stipple =  XCreateBitmapFromData(disp, wnd, half_bmp_bits, 8, 2);
    if (stipple == None) goto BadLuck;

    xgcv.function = GXcopy;
    xgcv.line_style = LineSolid;
    xgcv.background = cm_pixels[bkgr_colour];
	xgcv.graphics_exposures = False;
    gc = XCreateGC(disp, wnd,
	      GCFunction|GCLineStyle|GCBackground|GCGraphicsExposures, &xgcv);
    if (gc == None) goto BadLuck;    

    /* Create pane pixmaps */
	for (x=0; x<PaneCount; x++) 
	{	xpm = XCreatePixmap(disp, wnd, WndWidth, WndHeight, ScrDepth);
		if (xpm == None) goto BadLuck;
  		all_panes[x] = xpm;
	}		

	/* Window Icon */ 
/*
#ifdef USE_XDGA
	if (dgaModeCurrent < 0)
#endif
	{  WindowIcon = create_icon(ifr_icon_xpm);
	  if (WindowIcon) {
		XWMHints xwmh;
		xwmh.icon_pixmap = WindowIcon;
		xwmh.flags = IconPixmapHint;
		XSetWMHints(disp, wnd, &xwmh);
	  }			 
	}	  
*/
	rc = 1;

BadLuck:    	
    return rc;
            	
}

void  draw_line(PANE_TYPE which_pane, int x, int y,
			     int wx, int wy, int fillcol,
      		     int bgcol, STIPPLE_TYPE which_stipple)
{
	
	if (which_stipple == ST_NO_STIPPLE)
	   XSetFillStyle(disp, gc, FillSolid);
	else
	{
	   XSetFillStyle(disp, gc, FillOpaqueStippled);
	   XSetBackground(disp, gc, cm_pixels[bgcol]);
	   XSetStipple(disp, gc, all_stipples[which_stipple]);    	   
	}	   

	XSetForeground(disp, gc, cm_pixels[fillcol]);
	XDrawLine(disp, all_panes[which_pane], gc, x, y, x+wx, y+wy);	       

}

void  fill_hor_trapezium(PANE_TYPE which_pane, int x, int y,
	          int w, int h, int dw1, int dw2,
			  int fillcol, int bgcol, STIPPLE_TYPE which_stipple)
{
	XPoint xpnt[4];
	
	xpnt[0].x = x;
	xpnt[1].x = x+w;
	xpnt[1].y = xpnt[0].y = y;

	xpnt[2].x = x+w+dw2;
	xpnt[3].x = x+dw1;
	xpnt[3].y = xpnt[2].y = y + h;

	if (which_stipple == ST_NO_STIPPLE)
	   XSetFillStyle(disp, gc, FillSolid);
	else
	{
	  if (bgcol >= 0)  
	  {  XSetFillStyle(disp, gc, FillOpaqueStippled);
	     XSetBackground(disp, gc, cm_pixels[bgcol]);
	  }
	  else
	     XSetFillStyle(disp, gc, FillStippled);
	  		 
	   XSetStipple(disp, gc, all_stipples[which_stipple]);    	   
	}	   
	
	XSetForeground(disp, gc, cm_pixels[fillcol]);
    XFillPolygon(disp, all_panes[which_pane], gc, xpnt, 4, Convex, CoordModeOrigin);

}			  


/* which_pane:  0 - draw in background, 1 - draw in foreground */

void  fill_parallelogramm(PANE_TYPE which_pane, int x, int y,
		          int wx, int wy, int hx, int hy,
			  int fillcol, int bgcol, STIPPLE_TYPE which_stipple)
{

    
	if (which_stipple == ST_NO_STIPPLE)
	   XSetFillStyle(disp, gc, FillSolid);
	else
	{
	   if (bgcol >= 0)	
	     XSetFillStyle(disp, gc, FillOpaqueStippled);
	   else
	     XSetFillStyle(disp, gc, FillStippled);
	   		  
	   XSetStipple(disp, gc, all_stipples[which_stipple]);    	   
	}	   

    XSetBackground(disp, gc, cm_pixels[bgcol]);
	XSetForeground(disp, gc, cm_pixels[fillcol]);

	if (wy == 0 && hx == 0)
	{	if (wx < 0) { x += wx; wx = -wx; }	
		if (hy < 0) { y += hy; hy = -hy; }	
	    XFillRectangle(disp, all_panes[which_pane], gc,
		  x, y, wx, hy);
	}	
	else 
	{ XPoint xpnt[4];
	  xpnt[0].x = x;  xpnt[0].y = y;
	  xpnt[1].x = x+wx;  xpnt[1].y = y+wy;
	  xpnt[2].x = x+wx+hx;  xpnt[2].y = y+wy+hy;
	  xpnt[3].x = x+hx;	xpnt[3].y = y+hy;
      XFillPolygon(disp, all_panes[which_pane], gc, xpnt, 4, Convex, CoordModeOrigin);
	}

}


int set_line_colour(unsigned short base_colour[3])
{
    XColor  xc;

	
    /* Allocate line colour */   
    xc.red =  extend_colour_component(base_colour[0]);	
    xc.green = extend_colour_component(base_colour[1]);	
    xc.blue =  extend_colour_component(base_colour[2]);
    xc.flags = DoRed | DoGreen | DoBlue;

    if (stored_colours)
    {   xc.pixel = cm_pixels[line_colour];
		XStoreColor (disp, cm, &xc);
    }
    else
    {   if(XAllocColor (disp, cm, &xc))
				  cm_pixels[line_colour] = xc.pixel;
				else
				  return 0;  
    }	    

	return 1;
}

int set_bkgr_colours(const MPMP_INFO *mpmpi,
			    const unsigned short *base_colour, int start_colour)
{
    unsigned int colours_used;
    unsigned char *grey_levels;    
    unsigned short grey;
    int	x;
    unsigned short r, g, b;
    XColor  xc;


    colours_used = mpmpi->colours_used;
    grey_levels = mpmpi->grey_levels;    

    r = base_colour[0];
    g = base_colour[1];
    b = base_colour[2];
    memset(&xc, '\0', sizeof(xc));

	
    /* Allocate colours */   
    for (x = 0; x<colours_used; x++)
    { grey = grey_levels[x];
      xc.red = grey * r;	
      xc.green = grey * g;	
      xc.blue = grey * b;
      xc.flags = DoRed | DoGreen | DoBlue;

      if (stored_colours)
	  {  xc.pixel = cm_pixels[x+start_colour];
		 XStoreColor (disp, cm, &xc);
	  }
	  else
		cm_pixels[x+start_colour] = 	   
	  	 XAllocColor (disp, cm, &xc) ? xc.pixel : black_pix;
    }
	

    return 1;

}			    
/*
int insert_to_pixmap(int which_pane, const MPMP_INFO *mpmpi,
	      const unsigned short  *base_colour,
	      int sx, int sy, int colour_start)
{
    Pixmap  xpm;    
    XImage  *xi = NULL;
    int  width, height;
    unsigned char *pixels, p;    

	int  x, y;

//	colour_start = 0;
    if (!set_bkgr_colours(mpmpi, base_colour, colour_start)) return 0;
    width = mpmpi->width;
    height = mpmpi->height;

    xpm = all_panes[which_pane];
    xi = XGetImage(disp, xpm, sx, sy, width, height, ~0, ZPixmap);
    if (xi == NULL) return 0;
    pixels = mpmpi->pixels;
   
    for (y=0; y<height; y++)
    {
	  	for (x=0; x<width; x++)	
	   	{  p = *pixels++;
		   if (p!=0) 
		     XPutPixel(xi, x, y, cm_pixels[colour_start+p]);
		}
    }

    XPutImage(disp, xpm, gc, xi, 0, 0, sx, sy,  width, height);
    XDestroyImage(xi); 
    return 1;
}
*/

int fill_background_pixmap(int which_pane, const MPMP_INFO *mpmpi,
                      const  unsigned short *base_colour, int start_colour)
{	
    XImage  *xi = NULL;
    int width, height, pad, x, y;
    unsigned char *pixels;    
		int rc;
	
    rc = 0;
		if (base_colour && 
        !set_bkgr_colours(mpmpi, base_colour, start_colour)) goto Uscita;
      
    width =  mpmpi->width;
    height =  mpmpi->height;
    /* Now we are ready to create XImage */
    if (ScrDepth <= 8) pad = 8; else
    if (ScrDepth <= 16) pad = 16; else pad = 32;
    
    xi = XCreateImage(disp, visual, ScrDepth, ZPixmap, 0, NULL, width, height, pad, 0);
    if (!xi) goto Uscita;
    
    if ((xi->data = malloc(xi->bytes_per_line * height)) == NULL) goto Uscita;

    pixels = mpmpi->pixels;
    for (y=0; y<height; y++)
       for (x=0; x<width; x++)	
          XPutPixel(xi, x, y,  cm_pixels[start_colour+*pixels++]);  


    XPutImage(disp, all_panes[which_pane], gc, xi, OffsetX, OffsetY, 0,0, WndWidth, WndHeight);
	rc = 1;

  Uscita:
    if (xi) XDestroyImage(xi);
	return rc;
			
}

void get_graphics_line(char *value)
{  int len;
   
	snprintf (value, 81, "Screen res: %d x %d, depth %d. ",
  				      ScrWidth, ScrHeight, ScrDepth);

   len = strlen(value);
   if (len < 80)
   {
#ifdef USE_XDGA
	 if (dgaModeCurrent >= 0)
     { if (WndWidth < ScrWidth || WndHeight < ScrHeight)
  	   { snprintf (value+len, 81-len, "XDGA %d.%d app. size: %d x %d",
	  			dgaVerMajor, dgaVerMinor, WndWidth, WndHeight);
	   }
	   else
  	     snprintf (value+len, 81-len, "XDGA %d.%d, full screen",
	  			dgaVerMajor, dgaVerMinor);
	 }
	 else
#endif	 	   				
     {
	    snprintf (value+len, 81-len, "Window size: %d x %d",
									WndWidth, WndHeight);
	 }
   }	 									
}

void visualize_pane(PANE_TYPE which_pane, int x, int y, int wid, int hei)
{
    if (wnd != None)
  	    XCopyArea(disp, all_panes[which_pane], wnd, gc,
			x, y, wid, hei, StartX + x, StartY +  y);
}

void flush_all_pane(PANE_TYPE which_pane)
{
	flush_pane(which_pane, 0, 0, WndWidth, WndHeight);
}	


void flush_pane(PANE_TYPE which_pane, int x, int y, int wid, int hei)
{
    if (wnd != None)
    {	
	  XCopyArea(disp, all_panes[which_pane], wnd, gc,
			x, y, wid, hei, StartX + x, StartY + y);
	  XSync(disp, scrno);
    }	
}

void copy_pane(PANE_TYPE pt_source, PANE_TYPE pt_destination, 	
          int srcx, int srcy, int wid, int hei, int destx, int desty)
{

    XCopyArea(disp, all_panes[pt_source],
                all_panes[pt_destination], gc,
			srcx, srcy, wid, hei, destx, desty);

}

int set_bkgr_line_colours(const MPMP_INFO *mpmpi,
						   unsigned short base_colour[3])
{
  return 
	set_bkgr_colours(mpmpi, base_colour, StaticColours) &&
	set_line_colour(base_colour);

}

ACTION morph_to_colour(unsigned short new_base_colour[3])
{ int i, k;
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
  XImage *xiFrom = NULL, *xiTo = NULL;
  ACTION new_action = ACT_NONE;
  COLOUR col;

  xiFrom = XGetImage(disp, all_panes[source_pane],
          0, 0, WndWidth, WndHeight, ~0, ZPixmap);
  xiTo = XGetImage(disp, wnd,
          StartX, StartY, WndWidth, WndHeight, ~0, ZPixmap);

  if (xiFrom == NULL || xiTo == NULL) goto TheEnd;
    
  i = MORPH_STEP; 
  while (--i >= 0)
  {
    next_clock = start_clock(DRAW_LEVEL_DELAY1);
    for (y=0; y<WndHeight; y++)   
    {	
			for (x= (y*3+i) % MORPH_STEP; x<WndWidth; x += MORPH_STEP)
			{ col = XGetPixel(xiFrom, x, y);
	  		XPutPixel(xiTo, x, y, col);
			}
    } 
        
    XPutImage(disp, wnd, gc, xiTo, 0, 0, StartX, StartY, WndWidth, WndHeight);
    new_action = wait_clock(next_clock, NULL);
    if (new_action != ACT_NONE) break;
	
  }

  copy_pane(source_pane, PT_FOREGROUND, 0, 0, WndWidth, WndHeight, 0, 0);
  if (i>=0) flush_all_pane(PT_FOREGROUND);
    
TheEnd:    

  if (xiFrom) XDestroyImage(xiFrom);
  if (xiTo) XDestroyImage(xiTo);
    
  return new_action;
}

ACTION fade_image(COLENTRY colentry)
{
  int  x, y, i;
  unsigned long next_clock;
  XImage *xiFrom = NULL, *xiTo = NULL;
  ACTION new_action = ACT_NONE;
  COLOUR col;

  xiTo = XGetImage(disp, all_panes[PT_FOREGROUND],
         0, 0, WndWidth, WndHeight, ~0, ZPixmap);

  if (xiTo == NULL) goto TheEnd;
	col = cm_pixels[colentry];    
  i = MORPH_STEP;

  while (--i >= 0)
  {
    next_clock = start_clock(DRAW_LEVEL_DELAY1);

		for (y=0; y<WndHeight; y++)
	  	for (x= (y*3+i) % MORPH_STEP; x<WndWidth; x += MORPH_STEP)
	  		XPutPixel(xiTo, x, y, col);
        
		XPutImage(disp, wnd, gc, xiTo, 0, 0, StartX, StartY, WndWidth, WndHeight);
		new_action = wait_clock(next_clock, NULL);
    if (new_action != ACT_NONE) break;
	
  }
    
TheEnd:    
  if (xiFrom) XDestroyImage(xiFrom);
  if (xiTo) XDestroyImage(xiTo);
    
  return new_action;
}

/* ========================================================= */
/*
static Bool is_app_window(Window wnd)
{	Bool ret = False;
	Window *WndPtr, WndTemp;
	unsigned int WndCount;  

  
  	if (XQueryTree(disp, root_wnd, &WndTemp, &WndTemp, &WndPtr, &WndCount))
	{	while (WndCount > 0 && *WndPtr != wnd)
		{	WndPtr++; WndCount--; }
		
		ret = (WndCount > 0);	
	
	}

	return ret;
}
*/

static Status find_bossed_window(Window *pwnd, pid_t *ppid)
{ 
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned long *data;
	pid_t pid_bossed;

	if (WindowIdAtom == None) return False;
   
    XGetWindowProperty(disp, root_wnd, WindowIdAtom, 0,
			     2, True, XA_WINDOW,
			     &actual_type,  &actual_format,
			     &nitems, &bytes_after, (unsigned char **)&data); 
    
    if (actual_type == None || nitems < 2) return None;
    
    if(pwnd) *pwnd = *data;	    
    *ppid= pid_bossed = *(data+1);	    
    XFree(data);
	
/*	if (wnd_return != root_wnd &&
 	    !is_app_window(wnd_return)) return None; */


	if (kill(*ppid, 0) < 0 &&
               errno != EPERM) return False;	

    return True;
}

/* Mode = 0 - kill, 1 - resume */
void release_bossed_window (Window bossed_wnd, pid_t bossed_pid,  int mode)
{
    XEvent xevnt;
   

    xevnt.xclient.type = ClientMessage;
    xevnt.xclient.serial = 0;
    xevnt.xclient.send_event = True;
    xevnt.xclient.message_type = WindowIdAtom; 
    xevnt.xclient.format = 32;
    xevnt.xclient.data.l[0] = (long) bossed_pid;
    xevnt.xclient.data.l[1] = (long) mode;
    xevnt.xclient.display = disp;

    XSendEvent(disp, bossed_wnd, False, KeyPressMask, &xevnt);   
}

void boss_routine(void)
{   XEvent xevnt;
	Window wnd_bossed;
	pid_t pid_bossed, pid_current;
	unsigned long bossed_data[2];

#ifdef USE_XDGA
	int  full_screen;
#endif

#ifdef USE_BACKGROUND_MUSIC
	extern MUSIC_STATUS mstatus;
	MUSIC_STATUS old_status = mstatus;
#endif	


#ifdef USE_BACKGROUND_MUSIC
	if ((mstatus & MUSIC_SUSPENDED) == 0)
	  stop_background_music(0);
#endif

#ifdef USE_XDGA
	full_screen = (dgaModeCurrent >= 0);
#endif
	
	pid_current = getpid();
		    
	if (find_bossed_window(&wnd_bossed, &pid_bossed) && 
	     pid_bossed != pid_current)
		 release_bossed_window (wnd_bossed, pid_bossed,  0);

	
	if (WindowIdAtom == None)
	{  fprintf (stderr, "Cant perform BOSS command: no property available\n"
												"Will terminate instead\n");
		goto KillMe;				 
	}

	
	suspend_app(1);

#ifdef USE_XDGA
	if (full_screen) 
	{
	   wnd = root_wnd; 
	   XSelectInput(disp, wnd, KeyPressMask);
   	}
#endif


	bossed_data[0] = (unsigned long)wnd;
	bossed_data[1] = (unsigned long)pid_current;
    XChangeProperty(disp, root_wnd, WindowIdAtom, XA_WINDOW, sizeof(unsigned long)*8,
			    PropModeReplace, (unsigned char *) bossed_data, 2);

    for(;;)
	{ if (XCheckTypedEvent(disp, ClientMessage, &xevnt))
	  { if(xevnt.xclient.message_type == WindowIdAtom) break;
	    if (xevnt.xclient.data.l[0] != pid_current)
	  	  XPutBackEvent(disp, &xevnt);
	  }
	}	  		  
			

    if (xevnt.xclient.data.l[1] > 0) 
	{ 

#ifdef USE_XDGA
	    if (full_screen)
		  XSelectInput(disp, wnd, 0);
#endif		
	    if (!resume_app()) goto KillMe;
		  
#ifdef USE_BACKGROUND_MUSIC
		mstatus |= (old_status & MUSIC_SUSPENDED);
		LastSaveLayer = -1;
		if (old_status & MUSIC_ON)	
 		  start_background_music();
#endif
		return;
	}

KillMe:
	  fprintf (stderr, "xifrac [pid %d] killed\n", getpid() );
	  termination_routine(SIGTERM);
}			       

/* Mode = 0 - resume, 1 - kill */
int unboss_routine(int mode)
{   Window wnd_bossed = None;
	pid_t pid_bossed = 0;
    Screen	*scr;
	Status  found;
	int rc;

	pltf = PLTF_X11;

    disp = XOpenDisplay(NULL);
    if (disp == NULL)
    {  fprintf (stderr, "Can't open display: this is an X-Window application! \n");
       return 8;   
    }

    WindowIdAtom = XInternAtom(disp, WindowIdAtomName,  True); 
    found = False;

	if (WindowIdAtom != (Atom)None) 
	{   scr = DefaultScreenOfDisplay(disp);
		ScrWidth = scr->width;
		ScrHeight = scr->height;
	    root_wnd = scr->root;
		found = find_bossed_window(&wnd_bossed, &pid_bossed);
	}		
  
	if (found)
	{ release_bossed_window (wnd_bossed, pid_bossed, mode);
	  rc = 0;
	}	  
	else
	{ if(ResumeMode)
	     fprintf (stderr, "No suspended windows\n");
	  rc = 4;
	}
    
	XCloseDisplay(disp);
	return rc;
} 

void suspend_app(int hide)
{
#ifdef USE_XDGA
	if (dgaModeCurrent >= 0) {
	   terminate_dga();
	    XSync(disp, scrno);
	} else
#endif
    if(hide)
      XUnmapWindow(disp, wnd);
	
	suspended = 1;
}


int resume_app(void)
{
	if (suspended == 0) return 0;


#ifdef USE_XDGA
  if (dgaModeNeeded >= 0)
	{ start_dga();
	  if (wnd == None) return 0;
	  XDGASelectInput(disp, scrno,  KeyPressMask); 
	  flush_all_pane(PT_FOREGROUND);
	  XSync(disp, scrno);
	}		  
  else		  
#endif		
     XMapWindow(disp, wnd);
	
	suspended = 0;
	return 1;
}


int is_full_screen(void)
{
#ifdef USE_XDGA
	return (dgaModeCurrent >= 0);
#else
   return 0;
#endif   	
}

/*
void show_cursor(int show)
{
#ifdef USE_XDGA
	if (dgaModeCurrent < 0) 
#endif	
		XDefineCursor(disp, wnd, show ? defCursor : None);
}
*/

/*
static Pixmap create_icon(char *pixmap[])
{
   MPMP_INFO  mpmpi;
   XImage  *xim;
   Pixmap pmp = None;
   int wid, hei, colour_count, i, j;
   unsigned long *col_entries = NULL;
   const char *pixs;
   XColor  xcol;
   
  if (!process_colour_pixmap(pixmap, &mpmpi)) return None;

  wid = mpmpi.width;  hei = mpmpi.height;	
  colour_count = mpmpi.colours_used;
	
  xim = XCreateImage(disp, visual, ScrDepth, ZPixmap, 0,
                     NULL, wid, hei, 32, 0);
  if (xim == NULL) goto OutOfHere;
  if ((xim->data = malloc(mpmpi.height*xim->bytes_per_line)) == NULL)
	  goto OutOfHere;

  col_entries = malloc(colour_count * sizeof(unsigned long));
  if (col_entries == NULL) goto OutOfHere;

  for (i=j=0; i<colour_count; i++)
  {  
	 xcol.pixel = 0;
     xcol.red = mpmpi.grey_levels[j++];
	 xcol.green = mpmpi.grey_levels[j++];     
	 xcol.blue = mpmpi.grey_levels[j++];   
	 xcol.flags = DoRed | DoGreen | DoBlue;
	 xcol.pad = '\0';
	 XQueryColor(disp, cm, &xcol);	 
	 col_entries[i] = xcol.pixel;
  }	   

  pixs = mpmpi.pixels;

  for (j=0; j<hei; j++)
    for (i=0; i<wid; i++)
	  XPutPixel(xim, i, j, col_entries[(int)*pixs++ & 0xff]);
  free(col_entries); col_entries = NULL;
	  
  pmp = XCreatePixmap(disp, root_wnd, wid, hei, ScrDepth);
  if (pmp == None) goto OutOfHere;

  XSetFillStyle(disp, gc, FillSolid);   
  XPutImage(disp, pmp, gc, xim, 0, 0, 0, 0, wid, hei); 	  
  
OutOfHere:
  if (col_entries) free(col_entries);  
  if (xim) XDestroyImage(xim); 
  return pmp;

}
*/
