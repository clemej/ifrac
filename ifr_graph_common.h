#define DRAW_LEVEL_DELAY1  30		/* For fade/morph image */
#define DRAW_LEVEL_DELAY2  50	  /* For palette update */


#define MORPH_STEP 7


extern VIDEO_RES 	VideoRes;
extern int				VideoBpp;
extern unsigned char quarter_bmp_bits[];
extern unsigned char half_bmp_bits[];
extern char *mandel_xpm[];

extern const unsigned char base_colours[Levels][3];
extern  unsigned  short	current_base_colour[3];

extern const int  CubScrWidth, CubScrDepth, CubScrHeight;   
extern const int  CubStepX, CubStepY;
extern const int  ShadowGap, ShadowHeight;

extern int  CubScrDepthX, CubScrDepthY;   
extern int  stored_colours;

extern int  BoardCornerX, BoardCornerY; 
extern int  ShowNextX,  ShowNextY;

extern int  WndWidth, WndHeight;
extern int  ScrWidth, ScrHeight, ScrDepth;    	
extern int	run_background;


extern int  OffsetX, OffsetY;
extern MPMP_INFO mandel_mpmp_info, hypoth_mpmp_info, intlig_mpmp_info, julia_mpmp_info;


extern int  BoardAreaX, BoardAreaY, BoardAreaWidth, BoardAreaHeight;

