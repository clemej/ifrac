/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <stdio.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "ifr.h"
#include "ifr_pixmap.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
# include <sys/time.h>
# else
#  include <time.h>
# endif
#endif


/* #include <unistd.h> 
#ifndef _SC_LOGIN_NAME_MAX
#define _SC_LOGIN_NAME_MAX 33
#endif
*/

#ifdef ENABLE_WWSCORE
extern void show_wwscore(long score, const char *name);     
#endif

extern GAME game;
extern PLAYER  *players[];

extern int  WndWidth, WndHeight, ScrDepth;
extern int  FixedFontWidth, FontHeight[2];
extern int  OffsetX, OffsetY;
extern char *MonthNames[];
extern const char *FrontPageText[];
extern const char *FrontPageFooterLong[], *FrontPageFooterShort[];
extern const char *TopScorerPageFooterLong[], *TopScorerPageFooterShort[];
extern const char *TopScorerLeft[];
extern const int   TotScorerLeftWidth;
extern const unsigned char base_colours[][3]; 
extern const unsigned char frac_rgb[]; 
extern char *hypoth_xpm[], *intlig_xpm[], *julia_xpm[];
extern  MPMP_INFO hypoth_mpmp_info, mandel_mpmp_info;
extern int FixedFontWidth;

static const char HdrText[] = "IFRAC Scores File";


typedef struct 
{
	short	type;					// 0
	long	date;					// (YYYYMMDD)
	long	version;			// Version
	long	seed;					// Seed
	long  reserved[5];	// Reserved
	char  text[sizeof(HdrText)];		// IFRAC SCORES FILE
} HIGH_SCORE_HEADER;

typedef struct 
{
	short	type;					// 1 - Top scores 
	long	date;					// (YYYYMMDD)
	long	score;				// Score
	int		level;				// Level reached
	int		uid;					// User ID
	long  layers;				// Free layers
	int   slevel;				// Level start
	int   slayers;			// Full layers start
	char  dispname[FULL_NAME_SIZE];	// Display Name		
} HIGH_SCORE_ENTRY;


const HIGH_SCORE_ENTRY hse_default[] = 
{  {1, 19900520,  5000, 1, -1, 500, 0, 0,  "Max Shapiro"},
	 {1, 19901029,  4000, 1, -1, 400, 0, 0,  "Per Bergland"},
	 {1, 20000629,  3000, 1, -1, 300, 0, 3,  "Michael Glickman"} };
 

#define HISCORE_ENTRIES_MAX	15
#define MAX_CHUNK_SIZE 100

extern char *TopScoresFileName;
extern PLAYER *players[];

static HIGH_SCORE_ENTRY hse[HISCORE_ENTRIES_MAX];
static int hs_count;

static const int ScoresX = 50, ScoresY = 69;
static int  ScoresTopLineY, ScoresLineX;
static int  ScoresTop, HiScoresTotalHeight;
static int  ScoresScaleY, ScoresLineHeight;

static int select_level(PLAYER *player);

static void get_date_and_seed(long *date, unsigned int *seed)
{
	struct tm *local_tm;
	time_t time_temp;

#if HAVE_GETTIMEOFDAY
	struct timeval tv;

	gettimeofday(&tv, NULL);
	if (seed) *seed = (unsigned int) tv.tv_usec;
	time_temp = tv.tv_sec;	
#else
	time_temp = time(NULL);
	if (seed) *seed = (unsigned int) time_temp;
#endif
			
	local_tm = localtime(&time_temp);
	*date = (1900+local_tm->tm_year)*10000 +
		       	 local_tm->tm_mon * 100 + local_tm->tm_mday;

}

static int write_hiscores(void)
{
	unsigned int seed;
	short	reclen;
	long  cur_date;
	char *fname;

	HIGH_SCORE_HEADER hdh;
	char buf[MAX_CHUNK_SIZE];
	int fd=-1, i, j, retval = 0;

	
	fname = get_full_fname(TopScoresFileName);
	if (fname)
	{ mode_t old_mask = umask(0);
		fd = open(TopScoresFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		umask(old_mask);
	}

	if (fd < 0) 
	{ fprintf (stderr, "Error openning top scores file '");
		if (fname)
		{ 	fprintf (stderr, fname); free(fname);
				perror("': "); 
		}
		else
		{		fprintf(stderr, TopScoresFileName);
	 	    fprintf(stderr, "' - can't get full path");
		}
	  goto TheEnd;
	}

	if (fname) free(fname);

	get_date_and_seed(&cur_date, &seed);

	memset(&hdh, '\0', sizeof(hdh));
	hdh.type = 0;
	hdh.date = cur_date;
	hdh.version = IFRAC_VERSION;
	hdh.seed = seed;		
	strcpy(hdh.text, HdrText);				
	
	srandbsd(seed);
		
	reclen = sizeof(hdh);		
	if (write(fd, &reclen, sizeof(reclen)) != sizeof(reclen) ||
        write(fd, &hdh, reclen) != reclen)
				goto TheEnd;

	for (i=0; i<hs_count; i++)
	{	hse[i].type = 1;
		reclen = sizeof(HIGH_SCORE_ENTRY);
		memcpy(buf, &hse[i], reclen);
		for (j=0; j< reclen; j++)
			buf[j] = (char) ((int) buf[j] + (randbsd() & 0xff));

		if (write(fd, &reclen, sizeof(reclen)) != sizeof(reclen) ||
        	  write(fd, buf, reclen) != reclen)
		{ perror("Error writing top scores file: ");
			goto TheEnd;
		}
	}

	retval = 1;

TheEnd:
	if (fd >= 0) close(fd);
	return retval;
}

static int read_hiscores(int fd)
{	short type;
	HIGH_SCORE_HEADER *hdh;
	char buf[MAX_CHUNK_SIZE];
	size_t	read_result;
	short	reclen;

	int j, retval = 0;

	if (read(fd, &reclen, sizeof(reclen)) != sizeof(reclen) ||
		reclen > sizeof(buf) ||
		read(fd, &buf, reclen) != reclen)
	{ perror("Error reading top scores file: ");
	  goto TheEnd;
	}

	hdh = (HIGH_SCORE_HEADER *)buf;
	if(hdh->type != 0 || strcmp(hdh->text, HdrText)) goto TheEnd;
		
	srandbsd(hdh->seed);

  hs_count = 0;

	while (1)
	{ 
		read_result = read(fd, &reclen, sizeof(reclen));
		if (read_result < 0)
		{ perror("Error reading top scores file: ");
		  goto TheEnd;
		}
		if (read_result != sizeof(reclen)) break;
		if (read(fd, &buf, reclen) != reclen) goto TheEnd;
		for (j=0; j<reclen; j++)
			buf[j] = (char) ((int) buf[j] - (randbsd() & 0xff));
		type = *(short *)buf;	if (type != 1) break;
		memcpy(&hse[hs_count], buf,  sizeof(HIGH_SCORE_ENTRY));
		hs_count ++;
	}

	retval = 1;

TheEnd:
	return retval;
}

static void remove_char(char *text, int pos)
{
	if (pos >= strlen(text)) return;

	/* This probably can bre replaced by a single strcpy,
		 but I wouldn't risk */
	
	text += pos;
	while ((*text++ = *(text+1)) != '\0');

}

static void shop_top_scorer_frame(void)
{	int x, y;
	const char **PageFooter,  *TopScorersText;

	x = (ScoresLineX - (TotScorerLeftWidth+4) * FixedFontWidth)/2;

	y = ScoresTopLineY;

	PageFooter = TopScorerLeft;

	while ( (TopScorersText = *PageFooter++) != NULL)
	{	
		show_shadow_text(PT_FOREGROUND, FT_FIXED, TopScorersText,
									   x, y , 1, ScoresScaleY, 
										0, 0, 5, IntroBkgrColour, ST_NO_STIPPLE );
		
		y += ScoresLineHeight;
	}


	y = ScoresTopLineY + HiScoresTotalHeight;
	if (WndHeight >= 400) y += 4;
	x = ScoresX - OffsetX;

	PageFooter = WndHeight >= 480 ? TopScorerPageFooterLong : TopScorerPageFooterShort;

	while ( (TopScorersText = *PageFooter++) != NULL)
	{	show_shadow_text(PT_FOREGROUND, FT_FIXED, TopScorersText,
									   x, y , 1, ScoresScaleY, 
										0, 0, 4, IntroBorderColour, ST_LIGHT_STIPPLE );
		y += ScoresLineHeight;
	}
}
static int insert_char(char *text, int pos, int newchar)
{	int i, l;

	l = strlen(text);
	if (l >= (FULL_NAME_SIZE-1) || pos > l) return 0;

	for (i=l; i>= pos; i--)	text[i+1] = text[i]; 
	text[pos] = (char) newchar;
	return 1;
}

static int edit_text(int lineno)
{ ACTION act;
	int x, y, col, col1, pos;
	char text[FULL_NAME_SIZE];
	int  cursh, cursy, cursw, cursx;
	int  tot_width, newchar;

	game.level = TopLevel;
#ifdef USE_BACKGROUND_MUSIC 
	process_music();
#endif

	x = ScoresLineX;
	y = ScoresTopLineY + lineno * ScoresLineHeight;
	col = 4; col1 = IntroBorderColour;

	strncpy(text, hse[lineno].dispname, FULL_NAME_SIZE);
	text[FULL_NAME_SIZE-1] = '\0';
	tot_width = FixedFontWidth * (FULL_NAME_SIZE-1);
	pos = strlen(text);
	if (pos > FULL_NAME_SIZE-2) pos = FULL_NAME_SIZE-2;
	cursh = 2; cursw = FixedFontWidth-2;
    cursy = y + ScoresLineHeight - cursh-2;

	do {
		copy_pane(PT_BACKGROUND, PT_FOREGROUND,
                  x, y, tot_width, ScoresLineHeight, x, y);
				  
		if (text[0] != '\0')
 		   show_shadow_text(PT_FOREGROUND, FT_FIXED, text, x, y, 1, ScoresScaleY, 
										0, 0, col, IntroBkgrColour, ST_NO_STIPPLE );
		cursx = x + pos * FixedFontWidth;

	  	fill_parallelogramm(PT_FOREGROUND, cursx, cursy,
		      cursw, 0, 0, cursh, col1, NO_COLOUR, ST_NO_STIPPLE);

		flush_pane(PT_FOREGROUND, x, y, tot_width, ScoresLineHeight);	

		do
		{  sleep(0);
	 	   act = edit_interface_routine(NULL);
		}  while (act == ACT_NONE);

		switch(act)
		{  case ACT_LEFT:
					if (pos > 0) pos--;
					break;

			 case ACT_RIGHT:
					if (pos < strlen(text) && pos < FULL_NAME_SIZE-2) pos++;
					break;

			 case ACT_BACK:
					if (pos > 0) remove_char(text, --pos);
					break;

			 case ACT_DROP:
					remove_char(text, pos);
					break;

			 case ACT_BACKLEFT:
					pos = 0;
					break;

			 case ACT_FRONTLEFT:
					pos = strlen(text);
					if (pos >= FULL_NAME_SIZE-2) pos = FULL_NAME_SIZE-2;
					break;

			 case ACT_QUIT:
					pos = 0;
					text[pos] = '\0';
					break;

			 default:
					newchar = (int) act;
					if (newchar & 0xff00)
					{  if (insert_char(text, pos, newchar & 0xff) 
				            && pos < FULL_NAME_SIZE-2) pos++;
					   act = ACT_QUIT;
					}

		}

	} while (act != ACT_TURNBACK);

	text[FULL_NAME_SIZE-1] = '\0';	/* A mere safeguard */
	strcpy(hse[lineno].dispname, text);
	return  (int)text[0];
}


static int display_hiscores(PLAYER *player, int newpos)
{ unsigned short base_colour[3];
	char text[81];
	const char *TopScorersText, **PageFooter;
	int  x, y, theight;
	int  w, w1;
	int  dd, yy, mm, i;
	long	uid = player->uid;
	char *mmm;
	HIGH_SCORE_ENTRY *phse;
	COLENTRY col;


	for (i=0; i<3; i++)
		base_colour[i] = base_colours[ScoresColourEntry][i];


    set_static_colours(2, IntroStaticColours-2, frac_rgb);
    fill_background_pixmap(PT_BACKGROUND, &hypoth_mpmp_info, base_colour, IntroStaticColours);

	copy_pane(PT_BACKGROUND, PT_FOREGROUND, 0, 0, WndWidth, WndHeight, 0, 0);


	TopScorersText = FrontPageText[5];
	ScoresTop = ScoresY - OffsetY;
	if (WndHeight > 400) ScoresTop -= 25; 

	y = ScoresTop;
	w = FixedFontWidth*49;
//	x = WndWidth - w - ScoresX;
	x = ScoresX - OffsetX + 152;
	w1 = text_width(FT_VAR, TopScorersText, -1) * 4;
		
	show_shadow_text(PT_FOREGROUND, FT_VAR, TopScorersText,
                	 x + (w-w1)/2 , y, 4, 7, 
										1, 1, 5, IntroBkgrColour, ST_LIGHT_STIPPLE);
	theight = FontHeight[FT_VAR];
	y += theight * 7;
	if (OffsetY == 0) y += 4; else y -= 4;

	ScoresLineX = x + FixedFontWidth * 4;
	ScoresTopLineY = y;

	ScoresScaleY = theight >= 10 ? 1: 2;
	ScoresLineHeight = theight * ScoresScaleY;
	if (ScoresLineHeight < 18) ScoresLineHeight = 18;
	HiScoresTotalHeight = ScoresLineHeight * HISCORE_ENTRIES_MAX;
		
	for (i=0; i<hs_count; i++)
	{ phse = &hse[i];
		mm = phse->date;
		dd = mm % 100;
		mm /= 100;
		yy = mm / 100;
		mm %= 100; if (mm>11) mm = 11;
		mmm = MonthNames[mm];
		sprintf (text, "%2d. %-24s %6ld %1d %2d-%-3s-%4d",
										i+1, phse->dispname, phse->score, phse->level, dd, mmm, yy);
		if (i==newpos) col = 4;
		else col = (phse->uid == uid) ? 7 : 6;
		show_shadow_text(PT_FOREGROUND, FT_FIXED, text, x, y , 1, ScoresScaleY, 
										0, 0, col, IntroBkgrColour, ST_NO_STIPPLE );
		y += ScoresLineHeight;
	}


	if (newpos >= 0)
	{  shop_top_scorer_frame();
	   flush_all_pane(PT_FOREGROUND);	
		 return edit_text(newpos);		
	}

	game.level = ScoresLevel;
#ifdef USE_BACKGROUND_MUSIC 
	process_music();
#endif

	y = ScoresTopLineY + HiScoresTotalHeight;
	if (WndHeight >= 400) y += 4;
	x = ScoresX - OffsetX;
	PageFooter = WndHeight >= 480 ? FrontPageFooterLong : FrontPageFooterShort;

	while ( (TopScorersText = *PageFooter++) != NULL)
	{	show_shadow_text(PT_FOREGROUND, FT_FIXED, TopScorersText,
									   x, y , 1, ScoresScaleY, 
										0, 0, 5, IntroBorderColour, ST_LIGHT_STIPPLE );
		y += ScoresLineHeight;
	}
 
	return 1;
}



int show_hiscores(void)
{	int fd =-1;
	int retval = 0;
	PLAYER *player;
	char *fname;

	fname = get_full_fname(TopScoresFileName);
	if (fname)
	{	fd = open(TopScoresFileName, O_RDONLY);
		free(fname);
	}

	if (fd >= 0)
	{ retval = read_hiscores(fd);
		close(fd);
	}

	if (retval == 0)
	{	memcpy(&hse, &hse_default, sizeof(hse_default));
		hs_count = sizeof(hse_default)/sizeof(HIGH_SCORE_ENTRY);
/*		retval = write_hiscores(); */
	  write_hiscores(); 
	}
		
	retval = 1;
	player = players[0];	
	if (retval)	retval = display_hiscores(player, -1);
	if (retval)	retval = select_level(player);

#ifdef USE_BACKGROUND_MUSIC 
	stop_background_music(1);
#endif	
	return retval;
}


int  process_new_score(PLAYER *player)
{	
	long score;
	int i, j;

	if (player->type) return 0;

//	 player->score = 18000;

	score = player->score;
	if (score == 0) return 0;

	for (i=0; i<hs_count && hse[i].score >= score; i++);
	if (i >= HISCORE_ENTRIES_MAX) return 0;

	if (hs_count < HISCORE_ENTRIES_MAX)	hs_count++;
	for (j=hs_count-1; j>i; j--)
		memcpy(hse+j, hse+j-1, sizeof(HIGH_SCORE_ENTRY));


	hse[i].type = 1;
	hse[i].score = score;
	hse[i].uid = player->uid;
	hse[i].level = game.plevel+1;
	hse[i].layers = player->layers;
	hse[i].slevel = game.start_values[0];
	hse[i].slayers = game.start_values[1];
	strncpy(hse[i].dispname, player->fullname, FULL_NAME_SIZE);
	hse[i].dispname[FULL_NAME_SIZE-1] = '\0';

	get_date_and_seed(&(hse[i].date), NULL);

	if (display_hiscores(player, i))
				write_hiscores();

#ifdef ENABLE_WWSCORE
        show_wwscore(score, hse[i].dispname);
#endif
	player->score = -1;
	return 1;

}


/* =============================================== */
/*  Selection boxes                                */
/* =============================================== */
static int boxwid, digboxhei;
static int digscalex, digscaley;
static int digcellwidth, digcellheight;
static int digcellstrtx, digcellstrty;

static int pdscalex, pdscaley;
static int pdcellwidth, pdcellheight;
static int pdcellstrtx, pdcellstrty;

static int fwidth, fheight;
static int boxx, boxy[3];
static int curbox;

static int gvalues[2], ptype;


void show_digit_cell(int type, int cellno)
{	int x, y;
	char  digit[2];
	COLENTRY fg_colour;
	STIPPLE_TYPE which_stipple;

	x = boxx + (cellno%3) * digcellwidth + digcellstrtx;
	y = boxy[type] + (cellno/3) * digcellheight + digcellstrty;
	sprintf (digit, "%d", cellno+1-type);


	if (cellno != gvalues[type])
	{ 	fg_colour = IntroBorderColour;
			which_stipple = (curbox == type) ? ST_LIGHT_STIPPLE : ST_DARK_STIPPLE;
	}
	else
	{		fg_colour = 4;
			which_stipple = (curbox == type) ? ST_NO_STIPPLE : ST_LIGHT_STIPPLE;
	}
	
  show_shadow_text(PT_FOREGROUND, FT_FIXED, digit,
									  x, y, digscalex, digscaley, 
										0, 0, fg_colour, IntroBkgrColour, which_stipple);
}

static void	show_digit_cells(int type)
{	int i;

	for (i=0; i<9; i++) 
		show_digit_cell(type, i);		
}

static void show_digit_box(int type, int *py)
{
	const char *hdrtext;
	int  hdrwidth;
	static const int hscalex = 2, hscaley = 2;
	int fg_colour;
	int  x, y, sy;


//	fg_colour = (type == curbox) ? 7 : 5;
	fg_colour =  5;
	hdrtext = FrontPageText[type];
	hdrwidth = text_width(FT_VAR, hdrtext, -1) * hscalex;
	y = sy = *py;
	x = boxx + (boxwid-hdrwidth) / 2 + 4;
	show_shadow_text(PT_FOREGROUND, FT_VAR, hdrtext, x, y, hscalex, hscaley, 
										0, 0, fg_colour, NO_COLOUR, ST_NO_STIPPLE );
	
	y +=  FontHeight[FT_VAR]*hscaley;

	x = boxx; boxy[type] = y;
	draw_pane(PT_FOREGROUND, x, y,
					 boxwid, 0, 0, digboxhei,
					 1, 1, fg_colour, NO_COLOUR, NO_COLOUR, ST_NO_STIPPLE);
	x += 2; y+=2;
	draw_pane(PT_FOREGROUND, x, y,
					 (fwidth+5)*digscalex, 0, 0, (fheight+3)*digscaley,
					 3, 3, fg_colour, NO_COLOUR, NO_COLOUR, ST_NO_STIPPLE);

	show_digit_cells(type);

	
	*py = y + 3 * (fheight+3)*digscaley + digscaley*3 + 9;
	if (digscaley < 3) (*py) += 6;
}

void show_playdemo_cells(void)
{	int i, sx, sy, x;
	const char *text;
	STIPPLE_TYPE which_stipple;
	COLENTRY col;

	sx = boxx + pdcellstrtx;
	sy = boxy[2] + pdcellstrty;

	for (i=0; i<2; i++)	
	{  text = FrontPageText[i+2];
		 
		 x = sx + i*(pdcellwidth+1) + (pdcellwidth-text_width(FT_FIXED, text, -1))/2;		 
		
		if (i != ptype)
 	  {	col = IntroBorderColour;
			which_stipple = (curbox == 2) ? ST_LIGHT_STIPPLE : ST_DARK_STIPPLE;
		}
		else
		{	col = 4;
			which_stipple = (curbox == 2) ? ST_NO_STIPPLE : ST_LIGHT_STIPPLE;
		}

	 	show_shadow_text(PT_FOREGROUND, FT_FIXED, text, x, sy, pdscalex, pdscaley, 
									0, 0, col, IntroBkgrColour, which_stipple);
	}	
}

void show_playdemo_box(int *py)
{
	int sy, sx;
	pdscalex = 1; pdscaley = 3;
	pdcellwidth = (boxwid-5)/2;
	pdcellheight = fheight*pdscaley+3;
	pdcellstrtx = 4; pdcellstrty= pdscaley+1;

	sy = *py;
/*	sy = ScoresTop + HiScoresTotalHeight - (pdcellheight + 8); */
	sx = boxx;

	draw_pane(PT_FOREGROUND, sx, sy,
					 boxwid, 0, 0, pdcellheight + 4,
					 1, 1, 5, NO_COLOUR, NO_COLOUR, ST_NO_STIPPLE);

	sx += 2; sy += 2;
	draw_pane(PT_FOREGROUND, sx, sy,
					pdcellwidth, 0, 0, pdcellheight,
					 1, 1, 5, NO_COLOUR, NO_COLOUR, ST_NO_STIPPLE);
	draw_pane(PT_FOREGROUND, sx+pdcellwidth+1, sy,
					pdcellwidth, 0, 0, pdcellheight,
					 1, 1, 5, NO_COLOUR, NO_COLOUR, ST_NO_STIPPLE);

	boxy[2] = sy;
	show_playdemo_cells();  
	*py = sy + pdcellheight +  3;
 
}

static void show_sel_box(int which_box)
{	
	if (which_box < 2)
			show_digit_cells(which_box);
	else
	if (which_box == 2)
			show_playdemo_cells();  

}

static int select_level(PLAYER *player)
{
	ACTION act = ACT_NONE;
	int  sy;
	int increm = 0, old_value, new_value;

	for (sy=0; sy<2; sy++)
		gvalues[sy] = game.start_values[sy];
	ptype = player->type;

	sy = ScoresY-OffsetY;
	if (WndHeight > 400) sy -= 25; 
	boxx = ScoresX-OffsetX; 

	fwidth = FixedFontWidth; fheight = FontHeight[FT_FIXED];
	digscalex = 3; digscaley = fheight >= 10 ? 2: 3;
	
	boxwid = (fwidth+5)*digscalex*3 + 4;
	digboxhei = (fheight+3)*digscaley*3 + 4;

	digcellwidth = (fwidth+5)*digscalex;
	digcellheight = (fheight+3)*digscaley;
	digcellstrtx = digscalex*4+1;
	digcellstrty = digscaley*3+1;
	curbox = 0;

	show_digit_box(0, &sy);
	show_digit_box(1, &sy);
	show_playdemo_box(&sy);


	flush_all_pane(PT_FOREGROUND);
//	show_cursor(1);	

	while (1) 
	{
		act = interface_routine(NULL);
		increm = 0;

		switch(act)
		{
		  case ACT_BACKLEFT:
				if (curbox< 2) increm = -4;	break;

		  case ACT_BACK:
				if (curbox< 2) increm = -3;	break;

		  case ACT_BACKRIGHT:
				if (curbox< 2) increm = -2;	break;

		  case ACT_LEFT:
				increm = -1;	break;

		  case ACT_RIGHT:
				increm = 1;	break;

		  case ACT_FRONTLEFT:
				if (curbox< 2) increm = 2;	break;

		  case ACT_FRONT:
				if (curbox< 2) increm = 3;	break;

		  case ACT_FRONTRIGHT:
				if (curbox< 2) increm = 4;	break;

/*
		  case ACT_DROP:
			  ptype ^= 1;
				show_playdemo_cells();  
				break;							
*/

		  case ACT_DROP:
		  case ACT_TAB:
				old_value = curbox;
				curbox = (curbox+1)%3;
				show_sel_box(old_value);
				show_sel_box(curbox);
				break;	

			case ACT_BACKTAB:
				old_value = curbox;
				curbox = (curbox+2)%3;
				show_sel_box(old_value);
				show_sel_box(curbox);
				break;	

		  case ACT_QUIT:
				return 0;

			default:
				if (act != ACT_NONE) goto OutOfLoop;
		}					

		if (increm)
		{ if(curbox < 2)
			{	old_value = gvalues[curbox];
				new_value = (old_value + increm + 9) % 9;
	    	gvalues[curbox] = new_value;
		   	show_digit_cell(curbox, old_value);
		   	show_digit_cell(curbox, new_value);
			}
			else
		  if(curbox == 2) 
			{	ptype = 1-ptype;
				show_playdemo_cells();  
			}
		}	

		if (act != ACT_NONE)
			flush_pane(PT_FOREGROUND, boxx, 0, boxwid, WndHeight);

		sleep(0);
	}



OutOfLoop:
//	show_cursor(0);	

	for (sy=0; sy<2; sy++)
		 game.start_values[sy] = gvalues[sy];

	player->type = ptype;

	fade_image(IntroBkgrColour);
	return 1;

}

ACTION process_top_screen_coordinate(int x, int y)
{
	int i, cellcol, cellrow=-1;
	int temp;
	
	
//cellcol = x-digcellstrtx-boxx;
	cellcol = x-boxx;
	if (cellcol < 0 || cellcol >= boxwid) return ACT_NONE;

	// try digital boxes
	for (i=0; i<2; i++) {
//		cellrow = y - digcellstrty - boxy[i];
		cellrow = y - boxy[i];
		if (cellrow >= 0 && cellrow < digboxhei) break;		
	}
	
	if (i<2) {
		// This is a digital box !
		cellcol /= digcellwidth;
		cellrow /= digcellheight;
		if (cellcol >= 3 || cellrow >= 3) return ACT_NONE;			

		temp = curbox;
		curbox = i;
    	gvalues[curbox]  = cellrow*3 + cellcol;
		show_sel_box(temp);
		show_sel_box(curbox);
		flush_all_pane(PT_FOREGROUND); 
		return ACT_NONE;
	}

	cellrow = y - boxy[2];
	if (cellrow < 0 || cellrow >= pdcellheight)	return ACT_NONE;
	i = cellcol / (pdcellwidth+1);
	if (i<0 || i>=2) return ACT_NONE;	

	if (ptype != i || curbox != 2) {
		temp = curbox;
		curbox = 2;
		ptype = i;
		show_sel_box(temp);
		show_sel_box(curbox);
		//show_playdemo_cells();  
		flush_all_pane(PT_FOREGROUND); 
	}	

	return ACT_TURNFWD;
	
}
