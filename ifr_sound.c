/*  =================================================== */
/*  Intelligent FRAC.	(C) 2000  Michael Glickman        */
/* ---------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "ifr.h"
#include "ifr_common.h"

#define MinVolume     0
#define MaxVolume     99
#define SameVolume   (-1) 
#define MinBalance  (-50)
#define MaxBalance    50

#ifdef USE_BACKGROUND_MUSIC 
#include <sys/wait.h>
#include <limits.h>
#include <ctype.h>
#include <signal.h>

/*
#if HAVE_VALUES_H
#   include <values.h>
#elif HAVE_LIMITS_H
#   include <limits.h>
#endif

#if defined(PATH_MAX) && (PATH_MAX < 1025)
#define BUF_LENGTH (PATH_MAX + 1)
#else
#define BUF_LENGTH 1025
#endif
*/


#define BUF_LENGTH 1025
#define MAX_APPS    50

#define DefaultEntry   0
#define AppEntry			 100
#define ExtEntry			 101
#define WrongEntry		 199

#ifdef USE_VOLUME_CONTROL
static int set_volume(int new_volume, int new_balance);
#endif

volatile MUSIC_STATUS mstatus;
static volatile  pid_t music_pid;
static  int music_opt;

extern GAME game;
static int   cur_entry;

#define LoopMask     1
#define KillMask     2
#define StopMask	   4

#define DefOption   LoopMask 
#define DefVolume   SameVolume
#define DefPriority 4
#define DefPause    0

char short_music_file_name[61];
const char *PlayListFileName = NULL;


typedef struct
{ int	primary;
	char *ext;
	char *app;
	long pause;
	int	 priority;
	int  options;
	int  optmask;
} APP_ENTRY;

static APP_ENTRY AppEntries[MAX_APPS];
static int AppEntriesCount = 0;
static int AppEntriesTop = 0;

typedef struct 
{	int options;
	int optmask;
	int	volume;
	long pause;
	int	 priority;
	char *n_stdin;
	char *n_stdout;
	char *n_stderr;
	char *n_file;
	char *n_app;
} BGL_ENTRY;

static BGL_ENTRY plist[TotLevels+1];

typedef struct
{	const char code[4];
	int entry;
} ENTRY_CODES;


static const char *ZipExt[] = 
  {"gz", "bz", "bz2", "z", "zip"};
static int ZipExtCount = sizeof(ZipExt) / sizeof(const char *);
  

static const ENTRY_CODES EntryCodes[] =
{ {"def", DefaultEntry}, {"int", IntroLevel+1},
  {"sco", ScoresLevel+1},  {"top", TopLevel+1},
  {"sum", SummaryLevel+1}, {"app", AppEntry},
  {"ext", ExtEntry}
};

static int EntryCodesCount = sizeof(EntryCodes) / sizeof(ENTRY_CODES);

static const ENTRY_CODES OptCodes[] =
{ {"loo", LoopMask}, {"kil", KillMask}, 
  {"sto", StopMask} };

static int OptCodesCount = sizeof(OptCodes) / sizeof(ENTRY_CODES);


/* A useful function, arrived here from ... Visual Basic */
static char *trim(char *line)
{	char *pos;

	const char *sep = " \t";
	/* Left trim */
	line += strspn(line, sep);
	
	/* And right trim */
	pos = line + strlen(line);
	while (strchr(sep, *--pos) && (pos >= line));
	*++pos = '\0';

	return line;
}

/* ============================================= */
static void init_list_structures(void)
{	int i;

	/* Default Entries */	
	plist [DefaultEntry].options = DefOption;
	plist [DefaultEntry].volume = DefVolume;
	plist [DefaultEntry].pause = DefPause;
	plist [DefaultEntry].priority = DefPriority;
	plist [DefaultEntry].n_stdin = strdup("%null%");
	plist [DefaultEntry].n_stdout = strdup("%std%");
	plist [DefaultEntry].n_stderr = strdup("%std%");
	plist [DefaultEntry].n_file = NULL;
	plist [DefaultEntry].n_app = NULL;

	/* Other Entries */	
	for (i=1; i<=TotLevels; i++)
	{	plist [i].options = DefOption;
		plist [i].volume =  DefVolume;
		plist [i].pause =   -1;
		plist [i].priority = -1;
		plist [i].n_stdin = NULL;
		plist [i].n_stdout = NULL;
		plist [i].n_stderr = NULL;
		plist [i].n_file = NULL;
		plist [i].n_app = NULL;
	}

	AppEntriesCount = 0;
}

static void condfree(char **area)
{	if (*area)
	{  free(*area); *area = NULL; }
}

static void deinit_list_structures(void)
{	int i;

	/* Default Entries */	
	
	for (i=0; i<=TotLevels; i++)
	{	condfree(&(plist[i].n_stdin));
		condfree(&(plist[i].n_stdout));
		condfree(&(plist[i].n_stderr));
		condfree(&(plist[i].n_file));
		condfree(&(plist[i].n_app));
	}

	for (i=0; i<AppEntriesCount; i++)
	{	
		condfree(&(AppEntries[i].ext));
		if (AppEntries[i].primary)
			condfree(&(AppEntries[i].app));
	}

	AppEntriesCount = 0;
}


static void process_opt_line_common(char *line, int *popt, int *pmask)
{	int  i, neg;
	int opt, mask;

	line = strchr(line, '=');
	if (line == NULL) return;
	line = trim(line+1);

	line = strtokm(line, ",");
	opt = mask = 0;

	while (line)
	{	line = trim(line);	neg = 0;
		if (strncasecmp(line, "no", 2) == 0)
		{ neg = 1; line+=2; }
		
		for (i=0; i<OptCodesCount; i++)
		{	if (strncasecmp(line, OptCodes[i].code, 3) == 0)
				break;	
		}
		if (i < OptCodesCount)
		{	 i = OptCodes[i].entry;
			 mask |= i; if (neg==0) opt |= i;	
		}
		line = strtokm(NULL, ",");
	}

	if (popt) *popt = opt;
	if (pmask) *pmask = mask;

}


/* opt from [ext] section */
static void process_ext_opt_line(char *line)
{		int i, options, optmask;
		
	process_opt_line_common(line, &options, &optmask);

	for (i=AppEntriesTop; i<AppEntriesCount; i++)
	{	AppEntries[i].options = options;
		AppEntries[i].optmask = optmask;
	}
}

static int process_pause_line_common(char *line, long *ppauseval)
{	
	long pauseval = -1;
	char *next = NULL;

	line = strchr(line, '=');
	if (line == NULL) return 0;
	line = trim(line+1);

	if (*line!='\0')
	 pauseval = strtol(line, &next,	0);

	if (next==NULL || *next != '\0' || pauseval < 0 )
	{	fprintf (stderr, "Invalid pause value '%s' ignored.\n", line);
		fprintf (stderr, "Pause should be a non-negative number of milliseconds\n");
		return 0;
	}
	
	*ppauseval = pauseval;
	return 1;
}

static void process_ext_pause_line(char *line)
{
	long pauseval;
		
	if (process_pause_line_common(line, &pauseval))
	{	int i;
		for (i=AppEntriesTop; i<AppEntriesCount; i++)
			AppEntries[i].pause = pauseval;
	}

}

static int process_priority_line_common(char *line, int *pval)
{	
	int prtyval = -1;
	char *next = NULL;

	line = strchr(line, '=');
	if (line == NULL) return 0;
	line = trim(line+1);

	if (*line!='\0')
	 prtyval = (int) strtol(line, &next,	0);

	if (next==NULL || *next != '\0' || prtyval < 0 || prtyval>=20 )
	{	fprintf (stderr, "Invalid priority value '%s' ignored.\n", line);
		fprintf (stderr, "Pause should be a whole number from 0 to 19\n");
		return 0;
	}
	
	*pval = prtyval;
	return 1;
}

static void process_ext_priority_line(char *line)
{
	int prtyval;
		
	if (process_priority_line_common(line, &prtyval))
	{	int i;
		for (i=AppEntriesTop; i<AppEntriesCount; i++)
			AppEntries[i].priority = prtyval;
	}

}

static void process_volume_line(char *line)
{ int vol = SameVolume;
	char *next = NULL;

	line = strchr(line, '=');
	if (line == NULL) return;
	line = trim(line+1);

	if (line=='\0' || strncasecmp(line, "%n", 2) == 0 ||
                    strncasecmp(line, "%s", 2) == 0)
		vol = SameVolume;
	else
	{  vol = strtol(line, &next,	0);

	   if (next==NULL || *next != '\0' ||
			vol < MinVolume || vol > MaxVolume)
	   {  fprintf (stderr, "Invalid volume '%s' ignored.\n", line);
		  fprintf (stderr, "Volume should be '%%no' or a number between %d and %d\n",
										  MinVolume, MaxVolume);	
		  return;										  
	   }
	}	   
	
	plist[cur_entry].volume = vol;
}



/* [app] section is an obsolete feature */
static void process_app_section_line(char *line)
{	char *exts;
	char *ext, *app;
	int primary;

	AppEntriesTop = AppEntriesCount;

	exts = strtokm(line, "=");
	app = strtokm(NULL, "\r\n");
	if (exts == NULL || app == NULL) return;

	app = get_full_fname(app);	
	if (app == NULL) return;

	primary =  1;

	for (ext=strtokm(exts, ","); ext != NULL; ext = strtokm(NULL, ","))
	{	ext = trim(ext);
		if (*ext == '\0') continue;
			
		ext = strdup(ext);
		if (ext == NULL) continue;

		if (AppEntriesCount >= MAX_APPS)
		{ fprintf (stderr,
		     "Warninng: More then %d extensions have been specified:\n"
		     "          Extensions starting with %s are ignored\n", 
		     MAX_APPS, ext);
			break;
		}

		AppEntries[AppEntriesCount].ext =  ext;
		AppEntries[AppEntriesCount].app =  app; 
		AppEntries[AppEntriesCount].primary = primary;
		AppEntries[AppEntriesCount].optmask = 0;
		AppEntries[AppEntriesCount].options = 0;

		AppEntriesCount++;	primary = 0;
	}
		
}

static void process_ext_header(char *header)
{
	char *exts, *ext;
	int primary;

	AppEntriesTop = AppEntriesCount;
	exts = strchr(header, ':');
	if (exts == NULL) return;

	primary =  1;
	exts++;

	for (ext=strtokm(exts, ","); ext != NULL; ext = strtokm(NULL, ","))
	{	ext = trim(ext);
		if (*ext == '\0') continue;
			
		ext = strdup(ext);
		if (ext == NULL) continue;

		if (AppEntriesCount >= MAX_APPS)
		{ fprintf (stderr,
		     "Warninng: More then %d extensions have been specified:\n"
		     "          Extensions starting with %s are ignored\n", 
		     MAX_APPS, ext);
			break;
		}

		AppEntries[AppEntriesCount].primary = primary;
		AppEntries[AppEntriesCount].ext = ext;
		AppEntries[AppEntriesCount].app = NULL;
		AppEntries[AppEntriesCount].optmask = 0;
		AppEntries[AppEntriesCount].options = 0;

		AppEntriesCount++;	primary = 0;
	}
}


static char *extract_file_name(char *line)
{	char *fname;

	if (cur_entry > TotLevels) return NULL;
	fname = strchr(line, '=');
	if (fname == NULL) return NULL;
	return trim(fname+1);
}

static void set_file_name(char **where, const char *which)
{ 	char *oname = *where;
		if (oname) free(oname);
		*where = get_full_fname(which);
}

static void process_file_line(char *line)
{
	char *fname;

	if (cur_entry == DefaultEntry) return;

	fname = extract_file_name(line);
	if (fname == NULL) return;
	
	set_file_name(&(plist[cur_entry].n_file), fname);
}

/* app from global or level section */
static void process_list_app_line(char *line)
{
	char *fname;

	fname = extract_file_name(line);
	if (fname == NULL) return;
	
	set_file_name(&(plist[cur_entry].n_app), fname);

}

/* app from ext or section */
static void process_ext_app_line(char *line)
{
	int i;
	char *fname, *fname1;

	fname = strchr(line, '=');
	if (fname == NULL) return;
	fname1 = trim(fname+1);
	if (fname == NULL) return;
	fname = get_full_fname(fname1);

	for (i=AppEntriesTop; i<AppEntriesCount; i++)
	{
		fname1 = AppEntries[i].app;
		if (fname1) free(fname1);	
		AppEntries[i].app = fname;
	}
}

static void process_in_line(char *line)
{
	char *fname;
	fname = extract_file_name(line);
	if (fname == NULL) return;
	
	set_file_name(&(plist[cur_entry].n_stdin), fname);
}

static void process_out_line(char *line)
{
	char *fname;
	fname = extract_file_name(line);
	if (fname == NULL) return;
	
	set_file_name(&(plist[cur_entry].n_stdout), fname);
}

static void process_err_line(char *line)
{ 
	char *fname;
	fname = extract_file_name(line);
	if (fname == NULL) return;
	
	set_file_name(&(plist[cur_entry].n_stderr), fname);
}

/* opt from common or level section */
static void process_list_opt_line(char *line)
{
	process_opt_line_common(line,
		 &(plist[cur_entry].options), &(plist[cur_entry].optmask));
}

static void process_list_pause_line(char *line)
{	long pauseval;

	if (process_pause_line_common(line, &pauseval))
			plist[cur_entry].pause = pauseval;
}

static void process_list_priority_line(char *line)
{	int prtyval;

	if (process_priority_line_common(line, &prtyval))
			plist[cur_entry].priority = prtyval;
}

static int process_header(char *header)
{
	int result = WrongEntry;

	if (*header == '[') header++;
	strtokm(header, "]");

	header = trim(header);

	if (strncasecmp(header, "lev", 3) == 0)
	{  char c;

		 header += 3;
		 while ((c = *header)!='\0' && !isdigit(c))
						 header++;
	}
	
	if (isdigit(*header))
	{  int lev = atoi(header);
		 if (lev >= 1 && lev <= Levels)
				result = lev;
	}		 
	else		 
	{ int i;
		const char *pattern;

		for (i=0; i<EntryCodesCount; i++)
		{	pattern = EntryCodes[i].code;
			if (strncasecmp(header, pattern, 3) == 0)
			{ result = EntryCodes[i].entry;											
				break;
			}
		} 		
	}

	if (result == ExtEntry) 
			process_ext_header(header);

	return result;
}


typedef void (*BGL_ROUTINE)(char *line);
typedef struct
{	const char *keyword;
	BGL_ROUTINE proc;
} BGL_STRUCT;


static BGL_STRUCT exts[] = 
{
  {"opt",  process_ext_opt_line},
  {"app",  process_ext_app_line},			
  {"pau",  process_ext_pause_line},
  {"pri",  process_ext_priority_line}
};
static int exts_count = sizeof(exts)/sizeof(BGL_STRUCT);			


static BGL_STRUCT bgls[] = 
{
  {"opt",  process_list_opt_line},
  {"file", process_file_line},
  {"app",  process_list_app_line},			
  {"out",  process_out_line},			
  {"in",   process_in_line},			
  {"err",  process_err_line},
  {"vol",  process_volume_line},
  {"pau",  process_list_pause_line},
  {"pri",  process_list_priority_line}
};
static int bgls_count = sizeof(bgls)/sizeof(BGL_STRUCT);			

void process_bgl_line(BGL_STRUCT *bglp, int count, char *line)
{
	const char *kwd;
	int  i, len;

	for (i=0; i<count; i++, bglp++)
	{	kwd = bglp->keyword;
		len = strlen(kwd);
		if (strncasecmp(line, kwd, len) == 0)
		{	 (*(bglp->proc))(line);
				 break;
		}
	}
}

void load_sound_list(void)
{	char *fname;
	FILE *f = NULL;
	char buffer[121];
	char *pos;
	int  gzip = 0;
	int	 len;

	mstatus = MUSIC_OFF;
	music_pid = 0;		

	init_list_structures();

	if (PlayListFileName == NULL) return;

	fname = get_full_fname(PlayListFileName);

	if (fname)
	{ char *fname1; 

		len = strlen(fname);
		gzip = (len > 3) && (strcasecmp(fname+len-3, ".gz")==0);
		f = fopen(fname, "r");
		if (f == NULL && gzip == 0)
		{ fname1 = malloc(len+4);
			if (fname1)
			{ memcpy(fname1, fname, len);
			  strcpy(fname1+len, ".gz");
				f = fopen(fname1, "r");
				if (f == NULL) free(fname1);
				else { free(fname); fname = fname1; gzip=1; }
			}
		}

		if (gzip && f!=NULL)
		{ fclose(f); f = NULL;
			fname1 = malloc(len + 15);
			if (fname1)
			{ sprintf(fname1, "gunzip -c %s", fname);
				f = popen(fname1, "r");		/* Should always work! */
				free(fname1);
			}
		}		


	}

	if (f == NULL)
	{  fprintf (stderr, "Error opening play list.\n");
		 if (fname)
		 {	fprintf(stderr, "  File name: %s\n", fname);
				if (errno != ENOENT)
      	  fprintf(stderr, "      Error: %s\n", strerror(errno));
				free(fname);
		 }
		 else
		 {	fprintf(stderr, "  File name: %s\n", PlayListFileName);
	 	    fprintf(stderr, "      Error: Cannot get file path");
	   }
		 fprintf (stderr, "Background music will not be played.\n\n");
		 return;
	}		

	
	free(fname);

	cur_entry = WrongEntry;

	while (!feof(f))
	{
		fgets(buffer, 121, f);
	 	strtokm(buffer, "#\r\n");
		pos = trim(buffer);
		if (*pos == '[')
			 cur_entry = process_header(pos);
		else
		if (cur_entry == AppEntry)
		  process_app_section_line(pos);
		else
		if (cur_entry == ExtEntry)
		  process_bgl_line(exts, exts_count, pos);
		else
		  process_bgl_line(bgls, bgls_count, pos);
	}

	if (f)
	{ if (gzip) pclose(f); else fclose(f); }
	
}

void free_sound_list(void)
{
	if (stop_background_music(1))
		wait(NULL);

	mstatus = MUSIC_OFF;
	deinit_list_structures();	

	if (PlayListFileName) free((char *)PlayListFileName);
}

static int combine_options(int options, int opt_contrib, int mask_contrib)
{	int curmask;

	curmask = 1;
	while (mask_contrib)
	{	if (mask_contrib & 1)
		{ 
			if(opt_contrib & curmask)
			   options |= curmask;
			else     			
			   options &= ~curmask;
		}
		mask_contrib >>= 1;
		curmask <<= 1;
	}	

	return options;	
}

static const APP_ENTRY *get_file_app(const char *file, const char *appext)
{
	int i;
	const char *ext = NULL;
	char *fnamecpy = NULL;
	const APP_ENTRY *result = NULL;

	if (appext != NULL) 
	  ext = appext;
	else {	
	  ext = strrchr(file, '.');
	  if (ext == NULL) goto Oops;
	}	  
    ext++;

	for (i=0; i< ZipExtCount &&
            strcasecmp(ext, ZipExt[i]); i++);

	if (i<ZipExtCount)
	{  int len = ext-file;
	   fnamecpy = malloc(len);
	   if (fnamecpy == NULL) goto Oops;
	   memcpy(fnamecpy, file, --len);
	   fnamecpy[len] = '\0';
	   ext = strrchr(fnamecpy, '.');
 	   if (ext == NULL)
	   {  free(fnamecpy); goto Oops; }
	   ext++;
	}	   	

	for (i=0; i< AppEntriesCount && 
         strcasecmp(ext, AppEntries[i].ext); i++);


	if (i < AppEntriesCount) result = AppEntries+i;

Oops:
	if (fnamecpy != NULL) free(fnamecpy);  
	return result;
}


/*========================================================*/
#define MAX_ARGS 20

void endmusic_handler(int signo)
{	int status;

	if (music_pid <= 0) return;

	while (waitpid(music_pid, &status, WNOHANG | WUNTRACED) > 0)
	{ if (WIFEXITED(status) || WIFSIGNALED(status))
		{	music_pid = 0;
			mstatus &= ~MUSIC_ON;
		}
	}

}

static void std_redirect(const char *fname, FILE *file, const char *mode)
{
	if (fname == NULL || (strncasecmp(fname, "%s", 2) == 0))
			return;

	if (strncasecmp(fname, "%n", 2) == 0)
			fname = "/dev/null";

   freopen(fname, mode, file);
}

/* This function is alwys called with no MUSIC_SUSENDED
   and with not music or in killing process  */
int start_background_music_internal(const BGL_ENTRY *entry)
{
	const char *app;
	const char *file = NULL;
	char *app1;
	char app2[BUF_LENGTH];
	char *args[MAX_ARGS+1];
	int file_name_used;
	int arg_count;
	int prtyval, prtytemp;
	long pauseval, pvtemp;

	mstatus = MUSIC_OFF;		
  app = NULL; 
	strcpy(short_music_file_name, "");

	music_opt = plist[DefaultEntry].options;
	pauseval = plist[DefaultEntry].pause;
	prtyval = plist[DefaultEntry].priority;

	if (entry)
	{  file = entry->n_file;
 	   if ( file == NULL || *file != '%')
		 {  app = entry->n_app;
		    if ((app == NULL || *app=='%') && file != NULL) 
				{ const APP_ENTRY *appe = get_file_app(file, app);
					if (appe) 
					{ app = appe->app;			
					  music_opt = combine_options(music_opt, appe->options, appe->optmask);
						pvtemp = appe->pause;
						if (pvtemp >= 0) pauseval = pvtemp;
						prtytemp = appe->priority;
						if (prtytemp >= 0) prtyval = prtytemp;
					}						
				}
		 }
	}

	if (app == NULL) 
	{  /* Stop music  */
	   music_opt = 0;
	   mstatus = (music_pid > 0) ? MUSIC_ON : MUSIC_OFF; 
	   return 0;
	}

	if (file != NULL)
	{ const char *fn1 = strrchr(file, '/');
	  if (fn1 == NULL) fn1 = (char *)file; else fn1++;
		strncpy(short_music_file_name, fn1, sizeof(short_music_file_name));
		short_music_file_name[sizeof(short_music_file_name)-1] = '\0';
	}

	music_opt = combine_options(music_opt, entry->options, entry->optmask); 
	signal(SIGCHLD, endmusic_handler);	

	music_pid = fork();
	
	if (music_pid == 0)
	{ 
	 	signal(SIGCHLD, SIG_DFL);	

		/* Parsing application name */		
		strncpy(app2, app, sizeof(app2));

		file_name_used = 0; arg_count = 0;		
		for (app1 = strtok(app2, " \t");
			 app1 != NULL  && arg_count < MAX_ARGS; app1 = strtok(NULL, "\t "))
		{ 
			app1 = trim(app1);
                        if(strncasecmp(app1, "%f", 2) == 0) 
			{ if (file==NULL) app1 = "";
			             else app1 = (char *) file;
                          file_name_used = 1;
                        }
			if (strlen(app1)) args[arg_count++] = app1;
		}

		if (file != NULL && *file!='\0' && file_name_used==0
                   && arg_count < (MAX_ARGS-1) && file != NULL)
				args[arg_count++] = (char *)file;

		args[arg_count] = NULL;

		app1 = strrchr(app2, '/');
		if (app1) args[0] = app1+1; 

		file = entry->n_stdout;
		if (file == NULL) file = plist[0].n_stdout;
		std_redirect (file, stdout, "w");
	
		file = entry->n_stdin;
		if (file == NULL) file = plist[0].n_stdin;
		std_redirect (file, stdin, "r");
		 	
		file = entry->n_stderr;
		if (file == NULL) file = plist[0].n_stderr;
		std_redirect (file, stderr, "w");

#ifdef USE_VOLUME_CONTROL
		{	int vol, vol1, bal1;
			vol = entry->volume;	
			if (vol != SameVolume)
			{  if (get_volume(&vol1, &bal1) && vol1 != vol)
						set_volume(vol, bal1);
			}	
		}			
#endif

		prtytemp = entry->priority;
		if (prtytemp >= 0) prtyval = prtytemp;
		if (prtyval>0) nice(prtyval);
		execvp (app2, args);
		exit(16);	
	}	

	if (music_pid > 0)
	{ 
		pvtemp = entry->pause;	
		if (pvtemp >= 0) pauseval=pvtemp;
		if (pauseval>0) usleep(pauseval * 1000);
		if (kill(music_pid, 0)) 
			 music_pid = 0;
		else 
		 { if (music_opt & LoopMask) mstatus = MUSIC_ON_RERUN;
											   else	 mstatus = MUSIC_ON;
		   return 1;
		 }	
	}	

	return 0;
}

/*
   Returns: -1 - leave unchanged,
             0 - pick up entry in *par_entry,
             1 - quit
*/
int get_entry(int level, const BGL_ENTRY **par_entry)
{
	BGL_ENTRY *entry;
	char *fname;

	*par_entry = NULL;

	/* Processing 'Leave' options */
	for (;;) 
 	{
	  entry = plist + (level+1);
	  fname =  entry->n_file;

	  if (fname == NULL || *fname != '%')
	  { *par_entry = entry;
		return 0;
	  }

	  if (strchr("CL", toupper(*(fname+1))) == NULL)
			break;	          		/* quit */		

	  /* Leave tune unchanged */
	  if ((mstatus & MUSIC_ON) != 0)
		 return -1;        /* leave unchanged */

	  /* Down one level */
	   if (level == 0) level = ScoresLevel;
	   else 
	   if (level <= ScoresLevel && level != IntroLevel)
	   level--;
	   else
	   break; 
	}	 

	return 1;
}


/* Restart music after suspension */
int start_background_music(void)
{
  int rc = 0;
  const BGL_ENTRY *entry;

  if ((mstatus & MUSIC_ON) == 0)
  { 
	int lev;
	  
	lev = game.level;
	if (lev >= 0 && lev < TotLevels && get_entry(lev, &entry) >= 0)
	   rc = start_background_music_internal(entry);
  }	   	     
  else
  if (music_pid > 0 ) 
  {	
	if (mstatus & MUSIC_SUSPENDED)
	{ kill(music_pid, SIGCONT);		 
	  mstatus &= ~MUSIC_SUSPENDED;
	}
	rc = 1;
  }
	
  return rc;

}


int stop_background_music(int terminate)
{

	if (mstatus & MUSIC_ON)
	{	if (music_pid == 0)
			  mstatus &= ~MUSIC_ON_RERUN;
		else
	  { 
			if (terminate == 0 && (music_opt & StopMask) != 0)
			{
				if ((mstatus & MUSIC_SUSPENDED)== 0) 
						 kill (music_pid, SIGSTOP);
			} else
			{
				int sgn;
				sgn = ((music_opt & KillMask)) ? SIGKILL : SIGTERM;			
			 	mstatus &= ~MUSIC_RERUN;
				kill(music_pid, sgn);
			}

			if (terminate == 0)
			    mstatus |= MUSIC_SUSPENDED;
			return 1;
		}
	}
	return 0;
}

void toggle_background_music(void)
{
	if (!(mstatus & MUSIC_ON) || (mstatus & MUSIC_SUSPENDED))
		start_background_music();
	else
	  stop_background_music(0);
}

			
void process_music(void)
{
	int lev, sgn;
	const BGL_ENTRY *entry;
  
	lev = game.level;
	if (lev< 0 || lev >= TotLevels) return;

	if (get_entry(lev, &entry) >= 0)
	{ 
	  if (music_pid > 0)
	  {
		sgn = ((mstatus & MUSIC_SUSPENDED) ||
		       (music_opt & KillMask)) ? SIGKILL : SIGTERM;	
	    kill(music_pid, sgn);
	  }

	  // Switch tune music
	  if ((mstatus & MUSIC_SUSPENDED) == 0) 
	  { if (music_pid > 0) mstatus = MUSIC_ON_RERUN;
			else start_background_music_internal(entry);
	  }	
	}	

}


#endif

#ifdef USE_VOLUME_CONTROL
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#if HAVE_LINUX_SOUNDCARD_H
# include <linux/soundcard.h>
#elif HAVE_SYS_SOUNDCARD_H
# include <sys/soundcard.h>
#elif HAVE_MACHINE_SOUNDCARD_H
# include <machive/soundcard.h>
#endif


static int mixer_fd = -1;
static int mixer_device=-1;
static int mixer_stereo=0;
static int mixer_volume, mixer_balance;
static int mixer_volume_ok = 0;
static char mixer_devname[21];

static int check_volume(void)
{
	mixer_volume_ok = 0;

#ifdef MIXER_READ
	{	int mixvol;
				
		if (mixer_fd >= 0 && mixer_device >= 0 &&
		    ioctl(mixer_fd, MIXER_READ(mixer_device), &mixvol) >= 0 )
		{	int volr, voll;


 	      voll = mixvol & 0xff;
		  volr = mixer_stereo ? (mixvol >> 8) : voll;

 		  mixer_volume = (voll >= volr) ? voll : volr;
		  if (mixer_volume > MaxVolume) mixer_volume = MaxVolume;
		  else
		  if (mixer_volume < MinVolume) mixer_volume = MinVolume;

		  voll += volr;
		  if (voll == 0) mixer_balance = (MinBalance+MaxBalance)/2;
		  else 
		  mixer_balance = volr*(MaxBalance-MinBalance)/voll + MinBalance;
		  
		  mixer_volume_ok = 1;
		}
  }				
#endif

	return mixer_volume_ok;

}

int get_volume(int *volume_ptr, int *balance_ptr)
{
	if (mixer_volume_ok)
	{  *volume_ptr = mixer_volume;
	   *balance_ptr = mixer_balance;
	}

	return mixer_volume_ok;

}

int get_volume_device(char *devname, int devnamesize)
{	
	if (mixer_device >= 0)
	{	strncpy(devname, mixer_devname, devnamesize);			
		return 1;
	}
	return 0;
}

static int set_volume(int new_volume, int new_balance)
{   

#ifdef MIXER_WRITE
	if (mixer_fd >= 0 && mixer_device >= 0)
	{ int voll, volr;

    if (new_volume > MaxVolume) new_volume = MaxVolume;	
		else
    if (new_volume < MinVolume) new_volume = MinVolume;	

	  if (mixer_stereo == 0) new_balance = (MinBalance+MaxBalance)/2;
		else
    if (new_balance > MaxBalance) new_balance = MaxBalance;	
		else
    if (new_balance < MinBalance) new_balance = MinBalance;	

    if (new_balance <= (MinBalance+MaxBalance)/2)
    {   voll = new_volume;
		 	  volr = ((new_balance-MinBalance) * new_volume) / (MaxBalance-new_balance);
    }
    else
    {   volr = new_volume;
				voll = ((MaxBalance-new_balance) * new_volume) / (new_balance-MinBalance);
    }			


		voll |= (volr << 8);
/*		if (ioctl(mixer_fd, MIXER_WRITE(mixer_device), &voll) >= 0 &&
				check_volume()) return 1; */
		if (ioctl(mixer_fd, MIXER_WRITE(mixer_device), &voll) >= 0)
		{		mixer_volume = new_volume;
				mixer_balance = new_balance;
		}

	}
#endif
	return 0;
}

int change_volume(int delta_volume, int delta_balance)
{
	if (mixer_volume_ok)
		return set_volume(mixer_volume+delta_volume,
											  mixer_balance+delta_balance);
	else
		return 0;
			
}

int init_volume_control(void)
{
#ifdef SOUND_MIXER_READ_DEVMASK 
	unsigned int devmask;
	int i;

	static struct 
	{  int devno; const char *devname; }

  VolDevices[] =
	{
#ifdef SOUND_MIXER_VOLUME  
     {SOUND_MIXER_VOLUME,  "Master Volume"},
#endif
#ifdef SOUND_MIXER_PCM
		 {SOUND_MIXER_PCM,  "PCM - 1" },
#endif
#ifdef SOUND_MIXER_ALTPCM
		 {SOUND_MIXER_ALTPCM, "PCM - 2"},
#endif
#ifdef SOUND_MIXER_SYNTH
		 {SOUND_MIXER_SYNTH, "Synthesizer"},
#endif
		 { -1, "DUMMY"} 
	};

	mixer_fd = open("/dev/mixer", O_RDWR);
	if (mixer_fd < 0) 
	{ perror("Failed to open /dev/mixer");
		return 0;
	}

	/* 	Get mask of supported devices */
	if (ioctl(mixer_fd, SOUND_MIXER_READ_DEVMASK, &devmask) < 0)
	{ perror("Error reading devmask");
		return 0;
	}

	/* 	Looking for device to support */

	for (i=0; (mixer_device=VolDevices[i].devno) >= 0 && 
					  (devmask & (1<<mixer_device)) == 0; i++);

	if (mixer_device < 0)
	{ fprintf (stderr, "Couldn't find mixer volume device!\n");
		return 0;
	}

	strncpy(mixer_devname, VolDevices[i].devname, sizeof(mixer_devname));

# ifdef SOUND_MIXER_READ_STEREODEVS
	/* Device is found. Is is stereo ? */
	mixer_stereo = 
	   ioctl(mixer_fd, SOUND_MIXER_READ_STEREODEVS, &devmask) >=0 &&
 						  (devmask & (1<<mixer_device)) ;
# endif

	if (! check_volume()) return 0;
#ifdef USE_BACKGROUND_MUSIC 
	i = plist[0].volume; 
  if (i != SameVolume)
	{	int vol1, bal1;
		if (!get_volume(&vol1, &bal1)) return 0;
    if (vol1 != i)	set_volume(i, bal1);
	}			
#endif
	return 1;

#else
	return 0;
#endif
	
}

		
void deinit_volume_control(void)
{
	if (mixer_fd >= 0)
	{  close(mixer_fd);
		 mixer_fd = -1;
	}

	mixer_volume_ok = 0;
}

#endif

