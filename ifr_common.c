/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "ifr_autoconf.h"
#include "ifr_common.h"

const char *get_full_fname(const char *filename)
{
	struct passwd *pwent;
	char *fname;
	char *home = NULL;
	int  l;

	if (!filename) return NULL;

	l = 0;

	if (*filename == '~')
	{  int ul;

	   filename++;

	   fname = strchr(filename, '/');
	   if (!fname) goto Adelante;

	   ul = fname - filename;
  	   home = NULL;
	   
	   if (!ul) {
	     fname = getenv("LOGNAME");
		 if (!fname) fname = getlogin();
       }		 
	   else {		 
 	  	 fname = malloc(ul+1);
		 if (fname) {
		   memcpy(fname, filename, ul);
		   fname[ul] = '\0';
	     }		   

         filename += ul;
	   }		 

	   pwent = NULL;		   	  
	   if (fname)  pwent = getpwnam(fname);
	   if(ul) free(fname);

	   if (pwent)
	     home = pwent->pw_dir;
	   else
         home = getenv("HOME"); 
		 
	   if (home) l=strlen(home);
   }

Adelante:
	l += strlen(filename) + 1;

	fname = (char *) malloc(l);

	if (fname)
	{ if (home)
	  { strcpy(fname, home);
	    strcat(fname, filename);
	  }
	  else
	    strcpy(fname, filename);
	}

	return fname;
}

/* A fixed version of strtok, based on strpbrk */
char *strtokm(char *line, const char *delim)
{	 static char *nextpos = NULL;

	 if (line == NULL) line = nextpos;
	 if (line)	
	 { nextpos = strpbrk(line, delim);
		 if (nextpos) *nextpos++ = '\0';	
	 }	
	 return line;
}

int sign(int val)
{
  return (val > 0) ? 1 : (val < 0) ? -1 : 0;

}

#if !HAVE_STRPBRK
char *strpbrk(const char *s, const char *accept)
{	char c;
	while ((c=*s)!='\0' && strchr(accept, c)==NULL)	s++;

	return (c=='\0') ? NULL : (char *)s;
}
#endif

#if !HAVE_STRSPN
size_t strpspn(const char *s, const char *accept)
{	char c;
	const char *s1;

	s1 = s;
	while((c=*s1) != '\0' && strchr(accept, c) != NULL) s1++;

	return (s1-s);
}
#endif

#if !HAVE_STRDUP
char *strdup(const char *s)
{	char *s1;
	int len;

	len = strlen(s)+1;
	s1 = malloc(len);
	if (s1 != NULL) memcpy(s1, s, len);
	return s1;
}
#endif

#if !HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2)
{	int dif;
	char c;

	do dif = toupper(c=*s1++) - toupper(*s2++); 
	while ((dif == 0) && c != '\0');

	return dif;
}
#endif

#if !HAVE_STRNCASECMP
int strncasecmp(const char *s1, const char *s2, size_t n)
{	int dif = 0;
	char c;
	
	while (n > 0)
	{ dif = toupper(c=*s1++) - toupper(*s2++); 
		if((dif != 0) || c == '\0') break;
		n--;
	}
	return dif;
}
#endif


