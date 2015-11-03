#include "ifr_autoconf.h"

#define FULL_NAME_SIZE  25
#define JOY_MAX_BUTTON_COUNT 8
#define MOUSE_MAX_CUSTOM_BUTTONS 3


#define MaxPlayers	4	/* # players allowed */
#define Levels	        9	/* # levels          */
#define BoardDepth	6	/* # cubies along X  */
#define BoardWidth      6	/* # cubies along Y  */
#define BoardHeight    12	/* # cubies along Z  */
#define Tiles		9	/* # tiles           */
#define Dimension	3	/* # coordinates     */
#define TopScorers 10   /* Number of kept top scorers */

#define BoardLayer(Board, h)  ((Board)+(h)*BoardDepth*BoardWidth) 
#define BoardCubie(Board, w, d, h) *((Board)+((h)*BoardDepth+(d))*BoardWidth+(w)) 

#define bkgr_colour     0   /* Background colour */
#define inverse_colour  1   /* Inverse colour */
#define line_colour 	2
#define shadow_colour   3
#define TileEntry	 	4   /* First tile colour */	

#define StaticColours  (TileEntry + Tiles)
//#define line_colour   StaticColours   /* Colour for background line */
#define score_colour  shadow_colour
#define border_colour inverse_colour    /* Colour for tile separator lines */

#define IntroBkgrColour     0
#define IntroBorderColour   1
#define IntroScrollerColour 2
#define IntroVscrollerColour 3
#define IntroStaticColours  9

#define Intro1ColourEntry Levels
#define Intro2ColourEntry (Levels+1)
#define ScoresColourEntry (Levels+2)
#define StatisticsColourEntry (Levels+3)

#define IntroLevel     Levels
#define ScoresLevel    (Levels+1)
#define TopLevel       (Levels+2)
#define SummaryLevel   (Levels+3)
#define TotLevels			 (Levels+4)


#if SIZEOF_LONG_LONG > SIZEOF_INT
typedef long long JUMBO;
#else
typedef double JUMBO;
#endif


#if defined(USE_BSD_RAND) 
void srandbsd (unsigned int x);
long int randbsd (void);
#else
# if HAVE_SRANDOM
#  define srandbsd srandom
# else
#  define srandbsd srand
# endif

# if HAVE_RANDOM
#  define randbsd random
# else
#  define randbsd rand
# endif
#endif


/* Scales a random number to range 0 - maxval */
#define norm_int(val, maxval) ((int)((JUMBO)maxval*val/((JUMBO)RAND_MAX+1)))

/* Returns a random value in range 0 <= r < maxval */
#define rand_int(maxval) ((int)((JUMBO)maxval*randbsd()/((JUMBO)RAND_MAX+1)))


typedef enum
{	PLTF_SVGA,
	PLTF_X11,
} PLATFORM;

#ifdef USE_XDGA
typedef enum
{	DGA_NOTUSED,
	DGA_NOZOOM,
	DGA_ZOOM
} DIR_GRAPHIC;
#endif

typedef enum 
{
	FT_FIXED,
	FT_VAR
} FONT_TYPE;

typedef enum 
{	ST_NO_STIPPLE,
	ST_DARK_STIPPLE,
	ST_LIGHT_STIPPLE
} STIPPLE_TYPE;

typedef enum 
{	PT_FOREGROUND,
	PT_BACKGROUND,
	PT_TEMP
} PANE_TYPE;


typedef enum 
{  ACT_NONE,
   ACT_FRONTLEFT, ACT_FRONT,    ACT_FRONTRIGHT,	
   ACT_LEFT,      ACT_TURNFWD,  ACT_RIGHT,	
   ACT_BACKLEFT,  ACT_BACK,     ACT_BACKRIGHT,
   ACT_DOWN,      ACT_DROP,     ACT_TURNBACK,
   ACT_ROTATEFWD, ACT_ROTATEBACK, ACT_REDRAW,
   ACT_LEVEL,     ACT_SHOWNEXT,
   ACT_JOYSTICK,  ACT_KEYBOARD, ACT_MUSIC,
   ACT_PAUSE,     ACT_BOSS,     ACT_STATUS,
#ifdef USE_VOLUME_CONTROL
   ACT_VOLUPLEFT,   ACT_VOLUP,   ACT_VOLUPRIGHT,
   ACT_VOLLEFT,                  ACT_VOLRIGHT,
   ACT_VOLDOWNLEFT, ACT_VOLDOWN, ACT_VOLDOWNRIGHT,
#endif
   ACT_TAB,       ACT_BACKTAB,  ACT_QUIT
} ACTION;             


typedef enum
{
    SL_LEVEL,
    SL_LAYERS,
    SL_SCORE,
    SL_BONUS
} SCORE_LINE;

typedef enum 
{
  CM_STATIC,  CM_PALETTE,  CM_TRUE	
} CMAP_TYPE;


typedef enum 
{ VRES_LOW, VRES_NORMAL, VRES_DEFAULT} VIDEO_RES;

#ifdef USE_BACKGROUND_MUSIC 
typedef enum
{  MUSIC_OFF = 0,	      /* Stopped, no need to run */
	 MUSIC_RERUN = 1,		  /* Stopped, need to run    */
	 MUSIC_ON = 2,			  /* Running, stop on termination */
	 MUSIC_ON_RERUN = 3,  /* Running - rerun on termination */
	 MUSIC_SUSPENDED = 4  /* Suspended */
} MUSIC_STATUS;
#endif


typedef struct
{
    short  number;		/* Player number */
    short  type;		  /* 0-human, 1-computer */
    long   uid;				/* User ID */
    char   fullname[FULL_NAME_SIZE];	/* Full name */
    long   score;			/* Score (points) */	
    int	   bonus;			/* Bonus */	
    int	   layers;		/* Total number of full layers */	
    short  tile_pos[Dimension];	/* Tile position */	   
    short  tile_size[Dimension]; /* Tile position */	   
    short  tile_code;		/* Tile code (1-based) */
    int    status;		/* PLST_ flags */
    int    options;		/* PLOP_ flags */
		short  board_turn;		/* 0 - 2 */
    int    released_details;	/* Mask or count of layers released */
    int    best_pos[Dimension];	/* Used by computer player */
    int    best_pu[Dimension];	/* Used by computer player */
    long   move_next_clock;	/* Used by computer player */
    long   show_next_count;	/* Used by computer player */
    char   board[BoardDepth*BoardWidth*BoardHeight]; /* Player's board */
} PLAYER;

typedef struct
{
    short nplayers;			/* Number of players */    
    short start_values[2]; 		/* Starting level / layers */
    short level;			/* Game level */    
    short plevel;			/* Last play level */	
    short next_tile_code;		/* Next tile code (1-based) */
    short next_tile_turn;		/* Next tile orientation */
    unsigned long timeout;		/* Time-out */    
    unsigned long next_adv;		/* Time of the next advance */    
    int  status;			/* GMST_ flags */
    int  stat[Tiles];			/* Statistics */
} GAME;    

typedef int COLENTRY;
#define NO_COLOUR (-1)


#define PLST_DROPPED     0x0001	 /* Dropped (player)  */
#define PLST_SKIPLEVEL   0x0002  /* New level wanted  */
#define PLST_CHANGED     0x0004	 /* Need redraw	      */

#define PLST_NEWTILE     0x0100  /* New tile          */
#define PLST_NEWLEVEL    0x0200  /* New level (admin) */
#define PLST_FINISHED    0x0400  /* Game is finished  */
#define PLST_DELAY       0x0800	 /* Need delay	      */
#define PLST_QUITTED     0x1000	 /* Selected quit     */

#define PLOP_SHOWNEXT	 0x0001	 /* Need show next    */

#define GMST_SUSPENDED   0x0001  /* Suspended */


int draw_start(void);
int graph_init_video(void);
int graph_start(void);
void graph_end(void);
void draw_end(void);
unsigned long start_clock_from(unsigned long from, unsigned long interval);
unsigned long start_clock(unsigned long interval);
int time_up(unsigned long next_clock);
ACTION wait_clock(unsigned long next_clock, PLAYER *player);

int kbd_interface_start(void);
void kbd_interface_end(void);
ACTION kbd_interface_routine(PLAYER *player, int edit_mode);

ACTION ai_interface_routine(PLAYER *player);
ACTION edit_interface_routine(PLAYER *player);

int interface_start(void);
void interface_end(void);
void flush_input(void);
void flush_kbd(void);

ACTION interface_routine(PLAYER *player);

int joystick_start(void);
void joystick_stop(void);
int joystick_status(void);
void flush_joystick(void);
ACTION joystick_interface_routine(PLAYER *player);
int get_joystick_axes(long *x, long *y, int *xp, int *yp);

int game_start(void);
void game_end(void);
int game_analyze(void);
void reset_advance(void);
void get_tile_size(int tile_code, int tile_rotation, short  tile_size_return[]);
void  draw_next(PLAYER *player, int next_code, short next_size[]);
int  draw_start_game(void);
int draw_start_level(PLAYER *player, int level);

void board_action(int player_no, ACTION action);
void board_draw(int player_no);
int board_shift_h(PLAYER *player);
void draw_board(PANE_TYPE which_pane, PLAYER *player, int invert_mask);
int height_dropped(const char *board, const short *tile_pos,
													const short *tile_size);
int board_accept_new_tile(PLAYER *player, short tile_code,
                          const short tile_pos[], const short tile_size[]);
void show_next(PANE_TYPE which_pane, const PLAYER *player);
void ai_process_new_tile(PLAYER *player, const short *tile_size);
void draw_board_turn(PLAYER *player);

int player_initialize(int player_no);
void player_deinitialize(int player_no);
void player_start_game(PLAYER *player);
void player_process_dropped_tile(PLAYER *player);
void player_release_layers(PLAYER *player);

int  init_score_board(void);
void update_level(PANE_TYPE which_pane, int level);
void update_score(PANE_TYPE which_pane, int score, int layers);
void update_bonus(PANE_TYPE which_pane, int bonus);
void deinit_score_board(void);


void  draw_score_board(PANE_TYPE which_pane);
void draw_score_line(PANE_TYPE which_pane, int line, int value);
void prepare_pane_background(void);
int set_line_colour( unsigned short base_colour[3]);

int show_shadow_text(PANE_TYPE which_pane,
                FONT_TYPE which_font, const unsigned char *text,
                int xleft, int ytop, int scalex, int scaley, 
								int shad_width, int shad_height,
                COLENTRY fgcol, COLENTRY bgcol, STIPPLE_TYPE which_stipple);

ACTION morph_to_colour(unsigned short new_base_colour[3]);
ACTION morph_to_image(PANE_TYPE source_pane);


unsigned char *text_to_bitmap(FONT_TYPE which_font, const char *text,
											        int *width_return, int *height_return);
unsigned char *text_to_bitmap_multiline(char *text, 
				int linecharwid, int *width_return, int *height_return);

void get_graphics_line(char *value);
void flush_pane(PANE_TYPE which_pane, int x, int y, int wid, int hei);
void flush_all_pane(PANE_TYPE which_pane);
int set_static_colours(int start_colour, int colour_count,
           const unsigned char rgb[]);

void  draw_tile(PANE_TYPE which_pane, int sx, int sy,
			    short tile_size[3], COLENTRY face_colour, int opaque);

void draw_pane(PANE_TYPE which_pane,
		      int cx, int cy, int dx1, int dy1,
		      int dx2, int dy2, int n1, int n2,
		      COLENTRY linecol, COLENTRY fillcol,
		      COLENTRY bgcol, STIPPLE_TYPE which_stipple);
void  draw_line(PANE_TYPE which_pane, int x, int y,
			  int wx, int wy, COLENTRY fillcol,
              COLENTRY bgcol, STIPPLE_TYPE which_stipple);
void  fill_parallelogramm(PANE_TYPE which_pane, int x, int y,
		      int wx, int wy, int hx, int hy,
			  COLENTRY fillcol, COLENTRY bgcol, STIPPLE_TYPE which_stipple);
void  fill_hor_trapezium(PANE_TYPE which_pane, int x, int y,
	          int w, int h, int dw1, int dw2,
			  COLENTRY fillcol, COLENTRY bgcol, STIPPLE_TYPE which_stipple);
void copy_pane(PANE_TYPE pt_source, PANE_TYPE pt_destination, 	
          int srcx, int srcy, int wid, int hei, int destx, int desty);
int draw_bitmap(PANE_TYPE which_pane, 
							  unsigned char *par_bmp, int width, int height,
                int xleft, int ytop, int scalex, int scaley, 
								int shad_width, int shad_height,
                COLENTRY fgcol, COLENTRY bgcol, STIPPLE_TYPE which_stipple);

ACTION fade_image(COLENTRY colentry);

void show_intro();
void pause_game(void);
void boss_routine(void);
void show_status(void);
int unboss_routine(int mode);
void termination_routine(int sigcode);

int show_hiscores(void);
int  display_statistics(PLAYER *player);
int  process_new_score(PLAYER *player);
long	text_width(FONT_TYPE which_font, const char *text, int len);

int process_arguments(int argc, const char *argv[]);
char *get_full_fname(const char *filename);

void suspend_app(int hide);
int  resume_app();
int is_full_screen(void);
void show_cursor(int show);

#ifdef USE_BACKGROUND_MUSIC 
void load_sound_list(void);
void free_sound_list(void);
void process_music(void);
int start_background_music(void);
int stop_background_music(int terminate);
void toggle_background_music(void);
#endif

#ifdef USE_VOLUME_CONTROL
int init_volume_control(void);
void deinit_volume_control(void);
int get_volume_device(char *devname, int devnamesize);
int get_volume(int *volume_ptr, int *balance_ptr);
int change_volume(int delta_volume, int delta_balance);
#endif

#if !HAVE_SRANDOM
#define srandom()  srand()
#endif

#if !HAVE_RANDOM
#define random()  rand()
#endif



