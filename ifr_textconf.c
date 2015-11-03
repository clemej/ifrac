#include <stdio.h>		     /* NULL */
#include "ifr_autoconf.h"  /* const */

/*
   ====================================================
	 This file declares text and fonts used by the game.
   ====================================================
*/


/* The following two parameters are needed
   for svgalib version only. You can happily
   comment them out, if only X version will
   be compiled */
const char *ConsoleFontDir = CFONTDIR;
const char *FontFileName = CFONTFILE;

/*====================================================*/
/* On the contrary, the next list of fonts is used
   exclusively with the X version only. Any fixed font
   of a resonable size should provide an adequate
   performance, though size 7x13 is preferred.
   First and last list elements must be NULL-s
*/ 
    	
char *XFixedFontList[] = 
   { NULL,
	  "-misc-fixed-bold-r-*-*-14-*-75-75-*-80-*-*",
	  "-*-fixed-*-r-*-*-14-*-75-75-*-80-*-*",
	  "-*-fixed-*-r-*-*-*-*-75-75-*-80-*-*",
      "fixed", NULL 
	};

char *XVarFontList[] = 
   { NULL,
	  "-adobe-helvetica-bold-r-*-*-11-*-75-75-*-*-*-*",
	  "-*-helvetica-*-r-*-*-11-*-75-75-*-*-*-*",
	  "-*-helvetica-*-r-*-*-11-*-*-*-*-*-*-*",
      "variable", NULL 
	};

/* Replace the following text with the equivalents
   in your language. Parameter MaxScoreTermLength
   should not be less than the length of the longest
   text, you can increase it to adjust the size of
   score board.
*/ 

const int MaxScoreTermLength = 7;
const char *ScoreLineText[] =
       { "Speed:", "Layers:", "Score:", "Bonus:" }; 

const char *FrontPageText[] =
       { "Level", "Layers", "Play", "Demo", "Quit",  "Top Scorers"};

/* For screen height 480 */
const char *FrontPageFooterLong[] =
       { "Use: arrow keys or joystick lever for changing options,",
				 "     ENTER or Button1 - start game, TAB or Button2 - select option",
				 "     J / K - joystick on/off, H - help, O or F1 - boss, Esc - quit",
         NULL
			 };	

const char *TopScorerPageFooterLong[] =
       { "Use Left and Right keys to move pointer, Backspace/DEL to clear",
				 "current/previous character, ESC - clear name, ENTER - accept name",
				 NULL
			 };

/* For screen height < 480 */
const char *FrontPageFooterShort[] =
       { "SPACE select,  ENTER start,  H help, F1 boss, J/K joy/kbd,  Esc quit",
				 NULL
			 };

const char *TopScorerPageFooterShort[] =
       { "Arrows - left/right, Back, DEL - delete, ESC - clear,  ENTER - accept",
				 NULL
			 };

const char *TopScorerLeft[] =
			{ "--------------------",
				"T O P  S C O R E R !",
			  "--------------------",
				"Edit name if needed,",
			  "or clear the name to",
				"discard the result. ",
				"",
				"Press ENTER         ",
				"        to continue.",
			  "--------------------",
				NULL
		  };

const int TotScorerLeftWidth = 20;

const char *MiscText[] =
       { "HELP",  "Press SPACE to resume", "DEMO", "STATUS"};

const char *SummaryText[] =
       { "SUMMARY", "Blocks:",  "Full layers:", "Score:"};

const char *MonthNames[] =
	{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
													  
const char *PauseText1[] =
		 { "Use keyboard or keypad to control blocks:",
				NULL
		 };

const char *PauseText2[] =
		{  "\\Special keys:",
			 "\\P or H   - pause / help",
			 "\\O or F1  - OS / boss",
			 "\\T or F2  - status",
			 "\\N  - show next block",
			 "\\L  - next speed level",
			 "\\Esc- quit game",
			 NULL
	 	};

		const int SideTextLines = sizeof(PauseText2)/sizeof(char *) - 1;

		const char *PauseText3[] =
		{  "Other:  Space - drop block   Enter - rotate block back",
       "        0 or Ins - down      \\Plus  - rotate board clockwise",
       "Press Shift with a rotation key to invert the direction of rotation.",
       "",
		   "All keys are expected to operate with no regard to CapsLock or NumLock",
	     "position. Edit  ifr_skeys or ifr_xkeys  for a non-QWERTY keyboard use.",
  		 "\\Special keys\\ operate also in Demo mode.",
		   "",
		   "Joystick use:  lever drives a block,  button1 rotates,  button2 drops.",
		   "        Keys:  J - enable or calibrate joysick, K - use keyboard only.",  
		   "Background music and volume control:  M - turn music on/off, ",
		   "  [ or ] - reduce or increase volume, { or } - balance left or right",
		   "",
		   "See manual for more information.",   NULL
		 };	

		const int BottomTextLines = sizeof(PauseText3)/sizeof(char *) - 1;
