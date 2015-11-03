/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "ifr.h"
#include "ifr_cfont.h"
#include "ifr_common.h"

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifndef P_tmpdir
#define P_tmpdir "/tmp"
#endif

extern const char *ConsoleFontDir;
extern const char *FontFileName;
extern char *FontBits;
const char *FontFileDir;

extern int  FontHeight[2],  FixedFontWidth;
extern  char	*FontName[2];


int init_cfont(void)
{
    int  fd = -1; 
    int   unzipped;
    char  *FontFilePath = NULL;
    char  *GzipCmdLine = NULL;
    char  *TempFontFilePath = NULL;
    char  *FontFileNamePtr = NULL;
    char  strt[4];
    int	  fheight;
    const static char  *exts[] = {"", ".psf", ".gz", ".psf.gz", NULL};
    const char  *ext;
    static const char *tmp_file_pref="ifr";

    int len, i;
    int rc = 0;

    FontFileDir = ConsoleFontDir; 	
#if HAVE_DIRENT_H
    if (memcmp(FontFileDir, "/usr", 4) == 0)
    {
        DIR *DirTest = opendir(FontFileDir);

        if (DirTest == NULL)
        {
                FontFileDir +=4;
                DirTest = opendir(FontFileDir);
        	if (DirTest == NULL) return rc;
        }

        closedir(DirTest);
    }
#endif

    len = strlen(FontFileDir);

    FontFilePath = malloc(len + strlen(FontFileName) + 15);
    if (FontFilePath == NULL) goto BadLuck;

    memcpy(FontFilePath, FontFileDir, len);    
    if (FontFilePath[len-1] != '/') FontFilePath[len++] = '/';
    strcpy(FontFilePath+len, FontFileName);
    len += strlen(FontFileName);

    for (i=0; (ext = exts[i]) != NULL; i++)
    {  	strcpy(FontFilePath+len, ext);
      	fd = open(FontFilePath, O_RDONLY);
	if (fd >= 0) break;
    }

    if (ext == NULL)
    { FontFilePath[len] = '\0';
      fprintf (stderr, "Failed to open font file %s\n", FontFilePath);
      goto BadLuck;
    }      

    len += strlen(ext);    
    unzipped = strcasecmp(FontFilePath+len-3, ".gz");

    /* Try to open uncpompressed file */    
    if (fd < 0 && unzipped)
    { /* File does not exist - try with .gz */
	strcpy(FontFilePath+len, ".gz");
	len += 3;
	unzipped = 0;	
        fd = open(FontFilePath, O_RDONLY);
    }

    if (fd >= 0 && !unzipped)
    {
	const char *TempFontDir;

	close(fd); fd = -1;
	TempFontDir = getenv("TMPDIR");
        if( TempFontDir == NULL) TempFontDir = P_tmpdir;

 	GzipCmdLine = malloc(strlen(FontFilePath) +
	              strlen(TempFontDir) + strlen(tmp_file_pref) + 51);
 	if (GzipCmdLine == NULL) goto BadLuck;

 	sprintf(GzipCmdLine, "zcat -f %s > ", FontFilePath);
 	TempFontFilePath = GzipCmdLine + strlen(GzipCmdLine);
      	sprintf (TempFontFilePath, "%s/%s%08x.cfont", TempFontDir,
	              tmp_file_pref, getpid()); 
	
        system(GzipCmdLine);
        fd = open(TempFontFilePath, O_RDONLY);
    }       

    FontFileNamePtr = strrchr(FontFilePath, '/');
    if (FontFileNamePtr) FontFileNamePtr++;
	else FontFileNamePtr = FontFilePath;

    FontName[0] = FontName[1] = strdup(FontFileNamePtr);

    FixedFontWidth = 8;	/* This is always the case */
    	
    /* Try to find the height of characters */
    read(fd, strt, 4);
    
    if (strt[0] == 0x36 && strt[1] == 4)
    {  /* psf file */
	fheight = strt[3];
	len = fheight << 8;
    }
    else
    {   /* Size of the font file */
	int hdrlen;
	
	len  = (size_t) lseek(fd, 0, SEEK_END);
	fheight = len>>8;     // Assume 256 characters, FontHeight bytes each
    	hdrlen  = len & 255;
        
	if (hdrlen != 0 && hdrlen != 4)
	{  fprintf (stderr, "'%s' - invalid font format\n", FontFilePath);
	   goto BadLuck;
	}
    	len -= hdrlen;
		
      lseek(fd, hdrlen, SEEK_SET);  // Bypassing header
    }
	
    FontHeight[0] = FontHeight[1] = fheight;

    FontBits = (char *) malloc(len + sizeof(int));
    if (!FontBits) goto BadLuck;

    if (read(fd, FontBits, len) != len) goto BadLuck;
    rc = 1; 
 
BadLuck:  

   if (rc == 0)
	fprintf (stderr, "Consult 'Font' section in 'INSTALL'\n");

   if (fd >= 0) close(fd);
       
   if (TempFontFilePath)
      remove(TempFontFilePath);
   
   if (GzipCmdLine)
       free(GzipCmdLine);

   if (FontFilePath)
       free(FontFilePath);
   

   return rc;
}

long	text_width_cfont(FONT_TYPE which_font, const char *text, int len)
{
	if (len < 0) len = strlen(text);
	return (long)len * FixedFontWidth;
}

unsigned char *text_to_bitmap_cfont(FONT_TYPE which_font, const char *text,
											        int *width_return, int *height_return)
{
	int x, y, i, j, linewid;
	char chr, chr1;
	unsigned char *bmp, *bits;	
	int height;
	

/*	int len, wid;
	len = strlen(text);
	wid = len * FixedFontWidth;
	linewid = wid / 8; 
	if (wid & 7) linewid++;
*/
	linewid = strlen(text);
	height = FontHeight[which_font];	
	bmp = malloc(height * linewid);
	if (bmp == NULL) return bmp;

	for (x=0; x<linewid; x++)
	{  chr = *text++;  i = x;
	   bits = (unsigned char *)(FontBits + (chr * height));
       for (y=0; y<height; y++, i+=linewid)
	   {  chr = bits[y];
	      chr1 = 0;
		  for (j=0; j<8; j++)
		  {  chr1 = (chr1<<1) | (chr&1);
		     chr >>= 1;
		  }			 
	      bmp[i] = chr1;
	   }		  
	}				

	*width_return = linewid << 3;
	*height_return = height;
	return bmp;
}


unsigned char *text_to_bitmap_multiline_cfont(char *text, 
				int linecharwid, int *width_return, int *height_return)
{
	int x, y, i, j, pos;
	char chr, chr1;
	unsigned char *bmp, *bits;	
	int textcharwid, linecount, linebytewid;		
	int lineheight;

	textcharwid = strlen(text);
	linecount = (textcharwid+linecharwid-1) / linecharwid;
/*	linebytewid = (linecharwid*FixedFontWidth+7)/8; */
	linebytewid = linecharwid;
	lineheight = FontHeight[FT_FIXED];	
	
	bmp = malloc(lineheight * linebytewid * linecount);
	if (bmp == NULL) return bmp;

	for (y=0; y<linecount; y++)
	{	for (x=0; x<linecharwid; x++)
	  {	chr = *text++;  pos = x+linebytewid*lineheight*y;
			bits = (unsigned char *)(FontBits + (chr * lineheight));
			for (i=0; i<lineheight; i++, pos+=linebytewid)
			{	chr = bits[i];
				chr1 = 0;
		 	  for (j=0; j<8; j++)
			  {  chr1 = (chr1<<1) | (chr&1);
		  	   chr >>= 1;
		  	}			 
	      bmp[pos] = chr1;
			}
	   }		  
	}				

	*width_return = linebytewid * FixedFontWidth;
	*height_return = lineheight * linecount;
	return bmp;
}


void deinit_cfont(void)
{

    if (FontBits)
    {	free(FontBits);
			FontBits = NULL;
    }	
		
	if (FontName[0]) free(FontName[0]);

}
