/*  =================================================== */
/*  Intelligent FRAC.	(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "ifr_pixmap.h"
#include "ifr.h"
#include "ifr_common.h"

static COLENTRY  PauseNormCol, PauseHiCol;
static int	 PauseScaleX, PauseScaleY;

extern PLATFORM pltf;
extern const char *PauseText1[], *PauseText2[], *PauseText3[];
extern const char 	*MiscText[];
extern const int SideTextLines, BottomTextLines;
extern const char *SummaryText[];
extern const unsigned char base_colours[][3]; 
extern unsigned char ifrac_hlp_bw_bits[];
extern int  ScrWidth, ScrHeight, WndWidth, WndHeight, ScrDepth;
extern int  FixedFontWidth, FontHeight[2];
extern int  OffsetX, OffsetY;
extern  MPMP_INFO hypoth_mpmp_info, mandel_mpmp_info;
extern  int	HelpMode;
extern  int LastSaveLayer;

extern char	*FontName[2];
extern int   JoyVersion, JoyEventDriven;
extern char JoyTypeName[65];
extern const char *JoyDevName;
extern int	ConsFont;


#ifdef USE_BACKGROUND_MUSIC
extern  const char *PlayListFileName;
extern const char *DefaultPlayListFileName;
#endif

extern GAME game;
/* extern double	 Efficiency; */

static const int StatistX = 110, StatistY = 130; // 69;
static int  StatistScaleY;

static void show_pause_line(const char *text, int x, int y)
{
	char *text1, *textptr;
	COLENTRY col;

	text1 = strdup(text);
	if (text1 == NULL) return;

	textptr = strtokm(text1, "\\");
	col = PauseNormCol;

	while(textptr)
	{ if (*textptr != '\0')
		{  show_shadow_text(PT_FOREGROUND, FT_FIXED,
		                    textptr, x, y, PauseScaleX, PauseScaleY, 0, 0,
		                    col, 0, ST_NO_STIPPLE);
		   x += text_width(FT_FIXED, textptr, -1);
		}
		textptr = strtokm(NULL, "\\");
	  col = (PauseHiCol + PauseNormCol) - col;
	}

  free(text1);
			 		
}

void pause_game(void)
{
	  int x, y, i;
		long next_clock;
		ACTION act;

		const char *text;
		const int PauseX = 24, PauseY = 4;
		int PauseTopLineY;
		int fheight = FontHeight[FT_FIXED];
		int side_height;

		HelpMode = 1;
		copy_pane(PT_FOREGROUND, PT_TEMP, 0, 0, WndWidth, WndHeight, 0, 0);

/*
		fade_image(bkgr_colour);
		unsigned short pause_base_colour[3];
//		const MPMP_INFO *mpmpi;
	  for (x=0; x<3; x++)
			pause_base_colour[x] = base_colours[PauseColourEntry][x];

		mpmpi = (ScrDepth > 4) ? &hypoth_mpmp_info : &mandel_mpmp_info;
		{ if (!fill_background_pixmap(PT_FOREGROUND, mpmpi,
                       pause_base_colour, StaticColours))
			    return;
		}
		else
		copy_pane(PT_BACKGROUND, PT_FOREGROUND, 0, 0, WndWidth, WndHeight, 0, 0);
*/

	fill_parallelogramm(PT_FOREGROUND, 0, 0,
		      WndWidth, 0, 0, WndHeight,
			  bkgr_colour, NO_COLOUR, ST_NO_STIPPLE);

	y = game.level;
	if (y < Levels || y == SummaryLevel)	 
	{	PauseNormCol = TileEntry+6;
		PauseHiCol = shadow_colour;
	}
	else
	{	PauseNormCol = 6;
		PauseHiCol = 7;
	}

	PauseScaleX = 1;
	PauseScaleY = (WndHeight <= 400 || fheight >=10) ? 1 : 2;  
  y = PauseY; i = 0;	

  side_height = fheight * PauseScaleY * (BottomTextLines + SideTextLines + 3);

	if (side_height + fheight*7 < WndHeight)
	{
		text = MiscText[0];
	  	x = (WndWidth - text_width(FT_FIXED, text, -1) * 4) / 2;
        show_shadow_text(PT_FOREGROUND, FT_FIXED, text, x, y, 4, 5,
							 2, 2, inverse_colour, 6, ST_DARK_STIPPLE);
		y = PauseY + fheight * 5 ;											   
	}											 

	x = PauseX - OffsetX; PauseTopLineY = y;
	while ((text = PauseText1[i++]) != NULL)
 	{ show_pause_line(text, x, y);
  	  y +=	fheight * PauseScaleY;
	}

  side_height = fheight * PauseScaleY * SideTextLines;
	i = y-PauseTopLineY;
	if ((side_height - i) < 80) side_height = 80+i;

	y  += (side_height-i-78)/2;		
  draw_bitmap(PT_FOREGROUND, ifrac_hlp_bw_bits, 376, 78,
  		 x-4, y,  1, 1, 0, 0,
         PauseNormCol, NO_COLOUR, ST_NO_STIPPLE);
		
  y = PauseTopLineY; i = 0;	
	while ((text = PauseText2[i++]) != NULL)
 	{ show_pause_line(text, x+390, y);
    y += fheight * PauseScaleY;
	}

	y = PauseTopLineY + side_height + fheight*PauseScaleY/4;
	i = 0;
	while ((text = PauseText3[i++]) != NULL)
 	{  show_pause_line(text, x, y);
		y += fheight * PauseScaleY + 4;
	}


/*

	if ( WndHeight > 400 && PauseScaleY == 1)
	{  PauseScaleX = 6; PauseScaleY = 6; }
	else
	if (fheight <= 10)
	{  PauseScaleX = 6; PauseScaleY = 4;
       y += 6;
    }
	else
	{  PauseScaleX = 3; PauseScaleY = 3;
       y += 2;
    }
	
		x = (WndWidth - text_width(FT_FIXED, text1, -1) * PauseScaleX) / 2;
		y += (WndHeight - fheight * (PauseScaleY + 3) - y) / 2;

        show_shadow_text(PT_FOREGROUND, FT_FIXED, text1, x, y, PauseScaleX, PauseScaleY,
														PauseScaleX/2, PauseScaleY,
													 6, 12, ST_DARK_STIPPLE);

		y += (fheight + 1) * PauseScaleY; 
*/
		PauseScaleX = 2;
		y = WndHeight - fheight * (PauseScaleY + 1);
		text = MiscText[1];
		x = (WndWidth - text_width(FT_FIXED, text, -1) * PauseScaleX) / 2;
	    show_shadow_text(PT_FOREGROUND, FT_FIXED, text, x, y, PauseScaleX, PauseScaleY, 0, 0,
								 inverse_colour, bkgr_colour, ST_NO_STIPPLE);

		flush_all_pane(PT_FOREGROUND);

		do 
		{  next_clock = start_clock(500);
			act = wait_clock(next_clock, NULL);
		}	while (act == ACT_NONE);

/*
 		fade_image(bkgr_colour);
	  set_bkgr_colours(&mandel_mpmp_info, current_base_colour, StaticColours);
*/

		copy_pane(PT_TEMP, PT_FOREGROUND, 0, 0, WndWidth, WndHeight, 0, 0);		
		flush_all_pane(PT_FOREGROUND);
	 
    LastSaveLayer = -1;

		reset_advance();
		HelpMode = 0;

}

/* -------------------------------------------------------- */
/*                       S T A T U S                        */
/* -------------------------------------------------------- */
static int StatScaleX, StatScaleY;
static int StatX, StatY, StatLineHeight;
static COLENTRY  StatNormCol, StatHiCol;
static int FreeMemoryY;
int		 stat_first_run;

static void show_status_line(const char *text, int type)
{	
	COLENTRY col = type ?  StatHiCol : StatNormCol;
	int offset = type ? 0 : 24;
	
  show_shadow_text(PT_FOREGROUND, FT_FIXED, text,
					StatX+offset, StatY, StatScaleX, StatScaleY, 0, 0,
      						   col, 0, ST_NO_STIPPLE);

	StatY += StatLineHeight;
}

void fill_configuration(void)
{	char value[81];
	char *appname = NULL;		

	show_status_line("Configuration", 1);

	switch(pltf)
	{
		 case PLTF_SVGA:
			 appname = "IFRAC";
			 break;
	
		 case PLTF_X11:
			 appname = "XIFRAC";
			 break;
	}	
	
	sprintf (value, "%s version %d.%d.%d,   compiled %s %s", 
           appname,  IFRAC_VERSION >> 16,
           (IFRAC_VERSION >> 8) & 0xff, IFRAC_VERSION & 0xff,
						__DATE__ ,  __TIME__);

	show_status_line(value, 0);

	get_graphics_line(value);
	show_status_line(value, 0);


	if (ConsFont)
	{	if (FontName[FT_FIXED])
		{ snprintf (value, 81, "Font: %s",  FontName[FT_FIXED]);
			show_status_line(value, 0);
		}
	}		  
	else	
	{	if (FontName[FT_FIXED])
		{ snprintf (value, 81, "Fixed Font: %s",  FontName[FT_FIXED]);
			show_status_line(value, 0);
		}

		if (FontName[FT_VAR])
		{ snprintf (value, 81, "  Var Font: %s",  FontName[FT_VAR]);
			show_status_line(value, 0);
		}
	}
	
	if (WndHeight >400 ) show_status_line("", 0);

}

void fill_memory_usage(void)
{
	FILE *fi;
	char line[81];
	char outline[81];
	int done, mask, offset, percent;
	const char *header;
	long totalmem, usedmem, freemem;

	
	fi = fopen("/proc/meminfo", "r");
	if (fi == NULL) return;
	done = 0;


	outline[0] = '\0';
	while (!feof(fi) && done != 3)
	{  
	  fgets(line, 81, fi);
	  if (memcmp(line, "Mem:", 4) == 0)
	  {	header = "RAM"; mask = 1; offset = 5;}
      else				
	  if (memcmp(line, "Swap:", 5) == 0)
	  {	header = "Swap"; mask = 2; offset = 6; }
	  else
		continue;
	  
	  done |= mask;			 		 

	  sscanf (line+offset, "%ld %ld %ld", &totalmem, &usedmem, &freemem);
	  percent = (int)(((double)freemem * (double)100)/totalmem);
		if (percent < 0) percent = 0; else
	  if (percent > 100) percent = 100;

	  sprintf (outline + strlen(outline), "%-4s: %ldK %3d%%    ",
               header, freemem/1024, percent);
	}
	fclose(fi);
	
	if (done)
	{  if (stat_first_run)
		 { 	show_status_line("FreeMemory",  1);
				FreeMemoryY = StatY;
		 }	
		 else 
	   { StatY = FreeMemoryY;
			 fill_parallelogramm(PT_FOREGROUND, StatX, StatY,
		     			 WndWidth, 0, 0, StatLineHeight,
							 bkgr_colour, NO_COLOUR, ST_NO_STIPPLE);
		 }	 	

		 show_status_line(outline,  0);

		 if (stat_first_run)			
			  if (WndHeight >400) show_status_line("", 0);
	}
}


#ifdef USE_JOYSTICK
static int JoystickY;

void fill_joystick_usage(void)
{

  if (JoyVersion == 0)
  { if (stat_first_run)
	{ show_status_line("Joystick is unavailable", 1);
	  show_status_line("", 0);
	}
  }
  else
  {	char joy_version_text[15], line[81];
	long jx, jy;
	int jxp, jyp; 
	char *interface;

	if (stat_first_run)
	{	
	  if (JoyVersion < 0x10000)
		strcpy(joy_version_text, "classic");
	    else
		{	sprintf(joy_version_text, "%d.%d.%d", (JoyVersion >> 16) & 255,
		                   (JoyVersion >> 8) & 255, JoyVersion & 255);
		}

		interface = JoyEventDriven ? "modern" : "classic";

		snprintf (line, 81, "Joystick: %s   version: %s   interface: %s", 
							JoyDevName, joy_version_text, interface);
		show_status_line(line, 1);
		
		if (JoyVersion >= 0x10000 && strlen(JoyTypeName) > 0)
		{	snprintf (line, 81, "Type: %s",  JoyTypeName);
			show_status_line(line, 0);
		}
	}

	if (stat_first_run) 
		JoystickY = StatY;
	else
	{ 	StatY = JoystickY;
		fill_parallelogramm(PT_FOREGROUND, StatX, StatY,
		  	  WndWidth, 0, 0, StatLineHeight,
			  bkgr_colour, NO_COLOUR, ST_NO_STIPPLE);
    }
	
	if (get_joystick_axes(&jx, &jy, &jxp, &jyp))
	{
		snprintf (line, 81, "Axes:  X = %ld [%d%%]  Y = %ld [%d%%]",
				 jx, jxp, jy, jyp);
		show_status_line(line, 0);
			
	}	
	else															
		show_status_line("Disabled", 0);


	if (stat_first_run && WndHeight >400)
	  	show_status_line("", 0);
	
  }						 

}
#endif		

/*
void fill_environment(void)
{
	PLAYER *player = players[0];
	char  uid_txt[11];

  append_vscroller_line("", "");
	append_vscroller_line("Environment", "");

	sprintf(uid_txt, "%ld", player->uid);
	append_vscroller_line("UserId", uid_txt);

	append_long_name("FullName", player->fullname);

}

void fill_efficiency(void)
{		
		if (Efficiency > 0)
		{ char line[81];

			snprintf (line, 81, "Efficiency: %.2f",  Efficiency);
			show_status_line("", 0);
			show_status_line(line, 0);
		}
}
*/

#ifdef USE_BACKGROUND_MUSIC
extern char short_music_file_name[];
extern MUSIC_STATUS mstatus;
static int MstatusY;

void fill_bkgr_music(void)
{	char line[81];
	char *s1;
	
  if (mstatus & MUSIC_SUSPENDED)	s1 = "suspended";
	else  s1 = (mstatus & MUSIC_ON) ? "ON" : "off";
	
  if (stat_first_run) 
		  MstatusY = StatY;
	else
	{   StatY = MstatusY;
			fill_parallelogramm(PT_FOREGROUND, StatX, StatY,
		     WndWidth, 0, 0, StatLineHeight,
				 bkgr_colour, NO_COLOUR, ST_NO_STIPPLE);
	}

	sprintf (line, "Background music: %s", s1);
 	show_status_line(line, 1);

	if  (short_music_file_name && *short_music_file_name	!= '\0')
	{	sprintf (line, "Music file: %s", short_music_file_name);
	 	show_status_line(line, 0);
	}

	if (stat_first_run && WndHeight >400)
				show_status_line("", 0);

}
#endif

#ifdef USE_VOLUME_CONTROL
static int VolumeControlY;

void fill_volume(void)
{	char voldevname[21];
	int vol, bal;

	if (get_volume_device(voldevname, sizeof(voldevname)) &&
	           get_volume(&vol, &bal))
	{		char line[81];

			if (stat_first_run) 
			{
			snprintf (line, 81, "Volume control [%s]:", voldevname);
		    	show_status_line(line, 1);
			VolumeControlY = StatY;
			}
      else
			{   StatY = VolumeControlY;
					fill_parallelogramm(PT_FOREGROUND, StatX, StatY,
		     			 WndWidth, 0, 0, StatLineHeight,
							 bkgr_colour, NO_COLOUR, ST_NO_STIPPLE);
			}

			snprintf (line, 81, "Volume %d, balance %d", vol, bal);
			show_status_line(line, 0);

	}

}
#endif

void show_status(void)
{
		long next_clock;
		ACTION act;

		const char *text;
		int fheight = FontHeight[FT_FIXED];
		const int StatLeft = 30, StatTop = 4;
		int level;


		HelpMode = 1;
		copy_pane(PT_FOREGROUND, PT_TEMP, 0, 0, WndWidth, WndHeight, 0, 0);

/*
		const MPMP_INFO *mpmpi;
		fade_image(bkgr_colour);
		unsigned short pause_base_colour[3];
	  for (x=0; x<3; x++)
			pause_base_colour[x] = base_colours[PauseColourEntry][x];

		mpmpi = (ScrDepth > 4) ? &hypoth_mpmp_info : &mandel_mpmp_info;
		{ if (!fill_background_pixmap(PT_FOREGROUND, mpmpi,
                       pause_base_colour, StaticColours))
			    return;
		}
		else
		copy_pane(PT_BACKGROUND, PT_FOREGROUND, 0, 0, WndWidth, WndHeight, 0, 0);

		char welcome[81];
    PauseScaleY = fheight >= 10 ? 1 : 2;

	sprintf (welcome, 
           "%s %d.%d.%d. (C) 2000, Michael Glickman <xifrac@yahoo.com.au>",
			(pltf == PLTF_SVGA) ? "ifrac" : "xifrac",
			IFRAC_VERSION >> 16, (IFRAC_VERSION >> 8) & 0xff,
            IFRAC_VERSION & 0xff);
    show_shadow_text(PT_FOREGROUND, FT_FIXED, welcome, x, y, PauseScaleX, PauseScaleY, 0, 0,
								PauseNormCol, NO_COLOUR, ST_NO_STIPPLE);
   	y += 24;
	if (WndHeight <= 400) PauseScaleY = 1;
*/	

		fill_parallelogramm(PT_FOREGROUND, 0, 0,
		      WndWidth, 0, 0, WndHeight,
			  bkgr_colour, NO_COLOUR, ST_NO_STIPPLE);

		StatY = StatTop;
		StatScaleX = 4; 	text = MiscText[3];
		StatScaleY = (WndHeight > 400) ? 6 : 4;
		StatX = (WndWidth - text_width(FT_VAR, text, -1) * StatScaleX) / 2;

		level = game.level;
		if (level < Levels || level == SummaryLevel)	 
		{ 	  StatNormCol =  TileEntry + 6;
			  StatHiCol =  shadow_colour; /* TileEntry + 3; */
		}
		else
		{	  StatNormCol = 6;
			  StatHiCol = 4;
		}
	
		StatLineHeight = fheight * StatScaleY + 6;			 

    show_shadow_text(PT_FOREGROUND, FT_VAR, text,
							   StatX, StatY,
								 StatScaleX, StatScaleY, StatScaleX/2, StatScaleY/2,
								 inverse_colour, 6, ST_DARK_STIPPLE);
		StatY += StatLineHeight;
		
	  StatX = StatLeft - OffsetX;
		StatScaleX = 1;	
	  StatScaleY = (fheight >=10) ? 1 : 2;  
		StatLineHeight = fheight * StatScaleY + 4;			 
		stat_first_run = 1;
				
		fill_configuration();
		fill_memory_usage();
#ifdef USE_JOYSTICK
		fill_joystick_usage();
#endif
/*		fill_efficiency(); */
#ifdef USE_BACKGROUND_MUSIC
	  fill_bkgr_music();
#endif
#ifdef USE_VOLUME_CONTROL
		fill_volume();
#endif
		StatScaleX = 2;
		StatY = WndHeight - fheight * (StatScaleY + 1);
		text = MiscText[1];
		StatX = (WndWidth - text_width(FT_FIXED, text, -1) * StatScaleX) / 2;
    show_shadow_text(PT_FOREGROUND, FT_FIXED, text, StatX, StatY, 
									   StatScaleX, StatScaleY, 0, 0,
								     inverse_colour, bkgr_colour, ST_NO_STIPPLE);

		flush_all_pane(PT_FOREGROUND);
		stat_first_run = 0;
		StatScaleX = 1;	
	  StatX = StatLeft - OffsetX;
		do 
		{ next_clock = start_clock(500);
			act = wait_clock(next_clock, NULL);
			fill_memory_usage();
#ifdef USE_JOYSTICK
			fill_joystick_usage();
#endif
/*		fill_efficiency(); */
#ifdef USE_BACKGROUND_MUSIC
		  fill_bkgr_music();
#endif
#ifdef USE_VOLUME_CONTROL
			fill_volume();
#endif
 			flush_all_pane(PT_FOREGROUND);
		}	while (act != ACT_DROP && act != ACT_TURNFWD && act != ACT_TURNBACK &&
						 act != ACT_TAB && act != ACT_BACKTAB && act != ACT_QUIT);

/*
		fade_image(bkgr_colour);
 	  set_bkgr_colours(&mandel_mpmp_info, current_base_colour, StaticColours);
*/

		copy_pane(PT_TEMP, PT_FOREGROUND, 0, 0, WndWidth, WndHeight, 0, 0);		
		flush_all_pane(PT_FOREGROUND);
		LastSaveLayer = -1;

		reset_advance();
		HelpMode = 0;

}	 


static void show_tile_statistics(int code, int x, int y, long *total)
{ 
  short tile_size[Dimension];
	char text[11];
	int  count;

	if (code < Tiles)
	{  	get_tile_size(code+1, 0, tile_size);
		  draw_tile(PT_FOREGROUND, x, y, tile_size, TileEntry+code, 0);
			count = game.stat[code];  *total += count;
			sprintf(text, "%d", count);
			show_shadow_text(PT_FOREGROUND, FT_FIXED, text,
						  x+40, y+2 , 1, StatistScaleY, 
							0, 0, inverse_colour, bkgr_colour, ST_NO_STIPPLE );
	}
}


int display_statistics(PLAYER *player)
{ unsigned short base_colour[3];
  int  x, y, i, j;
	static MPMP_INFO *bmp;
  long  next_clock, blocks_total;
  char  text[81];
	const char *textHeader = SummaryText[0];
	const char *textTiles = SummaryText[1];
	const char *textLayers = SummaryText[2];
	const char *textScore = SummaryText[3];
	ACTION act;


	game.level = SummaryLevel;
#ifdef USE_BACKGROUND_MUSIC 
	process_music();
#endif

  fade_image(bkgr_colour);
	player->board_turn = 0;
	draw_board_turn(player);

  for (i=0; i<3; i++)
		base_colour[i] = base_colours[StatisticsColourEntry][i];

	bmp = (ScrDepth < 8) ? &mandel_mpmp_info : &hypoth_mpmp_info;
		
  fill_background_pixmap(PT_BACKGROUND, bmp, base_colour, StaticColours);
	copy_pane(PT_BACKGROUND, PT_FOREGROUND, 0, 0, WndWidth, WndHeight, 0, 0);


  i = (WndHeight <= 400) ? 6 : 7;
  x = (WndWidth - 4 * text_width(FT_VAR, textHeader, -1)) / 2;
  y = (StatistY - OffsetY - FontHeight[FT_VAR]*i)/2;
  show_shadow_text(PT_FOREGROUND, FT_VAR, textHeader, x, y, 4, i, 
							2, i/2, shadow_colour, bkgr_colour, ST_LIGHT_STIPPLE );

  x = StatistX - OffsetX;  y = StatistY - OffsetY;
  StatistScaleY = (FontHeight[FT_FIXED] >= 10) ? 2 : 3;

  j = Tiles/2 + 2; blocks_total = 0;
  for (i=0; i< Tiles;)
  {
		 show_tile_statistics(i++, x, y, &blocks_total);
		 show_tile_statistics(i++, x+200, y,  &blocks_total);
		 show_tile_statistics(i++, x+400, y,  &blocks_total);
	   y += (i < 2) ? 40 : (i < 4) ? 60 : 90;
  }

	y += (WndHeight - y - FontHeight[FT_FIXED]*StatistScaleY)/2;
	x = StatistX-90;
	sprintf (text, "%s %ld", textTiles, blocks_total);
	show_shadow_text(PT_FOREGROUND, FT_FIXED, text, x, y, 1, StatistScaleY,
							0, 0, shadow_colour, bkgr_colour, ST_NO_STIPPLE );


	sprintf (text, "%s %d", textLayers, player->layers);
	x = StatistX + (400 - text_width(FT_FIXED, text, -1))/2;
    show_shadow_text(PT_FOREGROUND, FT_FIXED, text, x, y, 1, StatistScaleY, 
							0, 0, shadow_colour, bkgr_colour, ST_NO_STIPPLE );


	sprintf (text, "%s %ld", textScore, player->score);
	x = StatistX + 472 - text_width(FT_FIXED, text, -1);
  show_shadow_text(PT_FOREGROUND, FT_FIXED, text, x, y, 1, StatistScaleY, 
							0, 0, shadow_colour, bkgr_colour, ST_NO_STIPPLE );

  flush_all_pane(PT_FOREGROUND);
  next_clock = start_clock(player->type ? 10000 : 120000);
  act = wait_clock(next_clock, NULL);
	if (act == ACT_QUIT) player->status |= PLST_QUITTED;
	

  fade_image(bkgr_colour);
	return 1;
}

/* -------------------------------------------------------- */
/*                         H E L P                          */
/* -------------------------------------------------------- */
extern const char *ClearScreenSequence;


char *get_ifrac_version(void)
{	static char version[15];

	sprintf (version, "%d.%d.%d", 
             (IFRAC_VERSION >> 16),
					   (IFRAC_VERSION >> 8) & 0xff,
					  	IFRAC_VERSION & 0xff);

	return version;
}
							

void show_help(void)
{
/*	
	printf (ClearScreenSequence);
	printf ("\r");	// Cleans column counter
*/

	printf ("\n");	// Cleans column counter
	printf ("Intelligent FRAC (%s) version %s.\n"
					"(C) 2000 Michael Glickman <xifrac@yahoo.com.au>\n\n",
					(pltf == PLTF_SVGA) ? "console" : "X11",
					get_ifrac_version());

	printf ("Command line options:\n");

	if (pltf == PLTF_SVGA)
	printf ("\t-bpp=value  screen depth (4, 8, 15, 16, 24, 32)\n");
	else
	printf ("\t-cf         use console font\n");
	
	
	if (pltf == PLTF_X11) {
#ifdef USE_XDGA	
		printf ("\t-dga        direct graphic access (DGA)\n");
		printf ("\t-dnz        DGA, no zoom (keep resolution)\n");
#endif
		printf ("\t-pri        use private colour map\n");
	}

	if (pltf == PLTF_SVGA)
	printf ("\t-vr=value   resolution (low - 640x350,\n"
		"\t            norm - 640x480, best - use best)\n");
	else
	printf ("\t-vr=value   window/screen size (low - 600x400,\n"
		"\t            norm - 640x480, best - use best)\n");
	if (pltf == PLTF_SVGA)
		printf ("\t-dva=value  direct video access (no,banked,linear)\n");

	printf ("\t-noi        skip introduction\n");
	printf ("\t-h          help (this text)\n");

#ifdef USE_JOYSTICK
	printf ("\t-jb=value   joystick button layout\n");
	printf ("\t-jc         use classic inteface with joystick\n");
	printf ("\t-jn         disable joystick\n");
#endif	
#ifdef USE_MOUSE
	printf ("\t-mb=value   mouse button layout\n");
	printf ("\t-mn         disable mouse\n");
#endif

#ifdef USE_BACKGROUND_MUSIC
	printf ("\t-mus=fname  name of background music play list\n");
	printf ("\t-mus=*      don't play background music\n");
#endif
	if (pltf == PLTF_X11) {
		printf ("\t-res        resume 'bossed' game.\n");
		printf ("\t-kill       kill 'bossed' game\n");
	}

	printf ("\n");
	printf ("All keywords and arguments are case-insensitive\n");
	printf ("See manual for details.\n\n");

}
/* -------------------------------------------------------- */
/*                    A R G U M E N T S                     */
/* -------------------------------------------------------- */
extern VIDEO_RES 	VideoRes;
extern int   PrivateColours;
extern int   ResumeMode;
extern int		HelpMode;
extern int		VideoBpp;
extern int		NoIntro, DirVideo;
#ifdef USE_XDGA
extern  DIR_GRAPHIC DirGraphic;
#endif

typedef enum
{	ARG_PRIVATE, ARG_BPP, ARG_VRES, ARG_CONSFONT,
#ifdef USE_JOYSTICK
  ARG_JOYBTN, ARG_NOJOY, ARG_JOYCLASSIC,
#endif
#ifdef USE_MOUSE
  ARG_MOUSEBTN, ARG_NOMOUSE,
#endif
  ARG_RESUME, ARG_NORESUME, ARG_KILL,  ARG_HELP,
#ifdef USE_BACKGROUND_MUSIC
  ARG_BGLIST,
#endif
#ifdef USE_XDGA
  ARG_DIRGRAPH,  ARG_DIRGRAPHNZ,
#endif  
  ARG_NOINTRO, ARG_DIRVIDEO} ARG_ENUM;

typedef struct
{  ARG_ENUM	code;
	 const char *memo;
	 int need_value;
} ARG_STRUCT;

ARG_STRUCT arg_list[] =
{
  {ARG_PRIVATE,    "pri", 0}, 
  {ARG_BPP,        "bpp", 1},
  {ARG_CONSFONT,      "cf", 0},
  {ARG_VRES,       "vr",  1},
  {ARG_HELP,       "h",   0},
  {ARG_NORESUME,   "nor", 0},
  {ARG_RESUME, 	   "res", 0},
  {ARG_KILL,       "kil", 0},
#ifdef USE_MOUSE
  {ARG_MOUSEBTN,    "mb",  1},
  {ARG_NOMOUSE,     "mn",  0},
#endif
#ifdef USE_JOYSTICK
  {ARG_JOYBTN,     "jb",  1},
  {ARG_NOJOY,      "jn",  0},
  {ARG_JOYCLASSIC, "jc",  0},
#endif
#ifdef USE_BACKGROUND_MUSIC
   {ARG_BGLIST,     "mus", 1},
#endif
#ifdef USE_XDGA
   {ARG_DIRGRAPH,   "dga", 0},
   {ARG_DIRGRAPHNZ, "dnz", 0},
#endif  
  {ARG_DIRVIDEO,  "dva", 1},
  {ARG_NOINTRO,    "noi", 0},
};
 
static const int arg_count = sizeof(arg_list) / sizeof(ARG_STRUCT);

#ifdef USE_MOUSE
extern int  MouseInUse;
extern const ACTION  MouseButtonDefActions[];
extern ACTION  MouseButtonActions[];
#endif

#ifdef USE_JOYSTICK
extern ACTION JoyButtonDefActions[], JoyButtonActions[];
extern const int JoyButtonDefActionsCount;
extern int   JoyInUse, JoyClassic;

typedef struct button_setting
	{ACTION act; int meta; } BUTTON_SETTING;
#endif

#if defined(USE_MOUSE) || defined(USE_JOYSTICK)
const BUTTON_SETTING BtnSettings[] =
{
	 {ACT_TURNFWD, 'R'}, {ACT_TURNBACK, 'B'},
	 {ACT_DROP, 'D'},    {ACT_DOWN, 'W'},
	 {ACT_LEVEL, 'L'}, 
	 {ACT_SHOWNEXT, 'N'},
	 {ACT_MUSIC, 'M'},
	 {ACT_PAUSE, 'P'},
	 {ACT_PAUSE, 'H'},   {ACT_BOSS, 'O'},
	 {ACT_STATUS, 'T'},  {ACT_NONE, 'V'} 
};

const int BtnSettingsCount = sizeof(BtnSettings)/sizeof(BUTTON_SETTING);
#endif

static void accept_button_settings(int max_count,
                const char *value, ACTION *result)
{
	char c;
	int i, j;

	for (i=0; i<max_count && (c=*value++) != '\0'; i++)
	{  c = toupper(c);
	   for (j=0; j<BtnSettingsCount; j++)
	   { if (BtnSettings[j].meta == c)
	     { result[i] = BtnSettings[j].act;
	       break;
	     }
	   }
	}
}

static int process_argument(ARG_ENUM type, const char *value)
{
	switch(type)
	{
		case ARG_PRIVATE:
			PrivateColours = 1;
			break;

		case ARG_CONSFONT:
			ConsFont = 1;
			break;

		case ARG_VRES:
			if (value)
				switch(toupper(*value))
				{ case 'L':
						VideoRes = VRES_LOW;
						break;

					case 'N':			
						VideoRes = VRES_NORMAL;
						break;

					case 'B':
					case 'D':
						VideoRes = VRES_DEFAULT;
						break;
				}
			break;

		case ARG_BPP:
			if (value)
			{	int bpp = atoi(value);
				if (bpp<=32 && bpp >= 4) VideoBpp = bpp;
			}
			break;

		case ARG_NORESUME:
		  ResumeMode = 3;
		  break;

		case ARG_RESUME:
		  ResumeMode = 2;
		  break;
		  
		case ARG_KILL:
		  ResumeMode = 1;
		  break;

		case ARG_NOINTRO:
		  NoIntro = 1;
		  break;

		case ARG_DIRVIDEO:
		  if (value) 
		    switch(toupper(*value)) {
		       case 'N':
		       case '0':
		           DirVideo = 0;
		           break;
		       case 'B':
		       case 'Y':
		       case '1':
			   DirVideo = 1;	
		           break;
		    }			
		  break;

		case ARG_HELP:
			show_help();
			return 4;
#ifdef USE_MOUSE
		case ARG_NOMOUSE:
			MouseInUse =0;
			break;	
               case ARG_MOUSEBTN:
                if (value != NULL)
                {
                        MouseInUse = 1;
                        accept_button_settings(MOUSE_MAX_CUSTOM_BUTTONS,
			       value,   MouseButtonActions);
                }
                break;
#endif

#ifdef USE_JOYSTICK
		case ARG_NOJOY:
			JoyInUse = 0;
			break;

		case ARG_JOYCLASSIC:
			JoyClassic = 1;
			break;

		case ARG_JOYBTN:
		if (value != NULL)
		{
			JoyInUse = 1;
			accept_button_settings(JOY_MAX_BUTTON_COUNT, value, 
						JoyButtonActions);
		}
		break;

#endif		  
#ifdef USE_XDGA
		case ARG_DIRGRAPH:
			 DirGraphic = DGA_ZOOM;
			 break;

		case ARG_DIRGRAPHNZ:
			 DirGraphic = DGA_NOZOOM;
			 break;
#endif  
#ifdef USE_BACKGROUND_MUSIC
		case ARG_BGLIST:
			 if (PlayListFileName) free((char *)PlayListFileName);	
			 value += strspn(value, " ");
			 PlayListFileName = (*value) == '*' ? NULL
	                        : get_full_fname(value);
			 break;
#endif
	}

	return 0;
}


int process_arguments(int argc, const char *argv[])
{	int i, j;
	int len, rc;
	const char *cur_arg, *template;
	ARG_ENUM code;

	game.start_values[0] = 0;		// Starting level
	game.start_values[1] = 0;		// Starting number of levels
	
	ResumeMode = 0;				// Normal start	
	NoIntro = 0;				// Show introduction
	DirVideo = 2;				// Svgalib video (linear)
	VideoRes = VRES_DEFAULT;		// Normal resolution
	VideoBpp = 8;
	
	ConsFont = PrivateColours = (pltf == PLTF_SVGA) ? 1 : 0;

#ifdef USE_XDGA
	DirGraphic = DGA_NOTUSED;
#endif

#ifdef USE_BACKGROUND_MUSIC
    PlayListFileName = get_full_fname(DefaultPlayListFileName);
#endif

#ifdef USE_MOUSE
	MouseInUse = 1;
        for (i=0; i<MOUSE_MAX_CUSTOM_BUTTONS; i++)
		MouseButtonActions[i] = MouseButtonDefActions[i];


#endif

#ifdef USE_JOYSTICK
	JoyInUse = 1; JoyClassic = 0;

	for (i=0; i<JoyButtonDefActionsCount; i++)
			JoyButtonActions[i] = JoyButtonDefActions[i];

	for ( ; i<JOY_MAX_BUTTON_COUNT; i++)
			JoyButtonActions[i] = ACT_NONE;
#endif

	for (i=1; i<argc; )
	{
		cur_arg = argv[i++];
		if (*cur_arg++ != '-') 
		{ show_help(); return 4; } 

		if (*cur_arg == '-') cur_arg++;
				

		for (j=0; j<arg_count; j++)
		{ template = arg_list[j].memo;
		  len = strlen(template);
		  if (strncasecmp(cur_arg, template, len) == 0) break;
		}	

		code = ARG_HELP;
		template = NULL;

		if (j<arg_count)
		{ 
		  code = arg_list[j].code;	

		  if (arg_list[j].need_value) {
			cur_arg = strchr(cur_arg, '=');
			if (cur_arg)
			  template = cur_arg + 1;	
			else	
			if (i < argc) {
			  cur_arg = argv[i];			
		  	if (*cur_arg != '-') { 		
			  template = cur_arg;  i++; 
			}
		  }
		  if (template == NULL) code = ARG_HELP;
		}  				
	  }	

	   rc = process_argument(code, template);
	   if (rc) return rc;
	}

	return 0;
}

