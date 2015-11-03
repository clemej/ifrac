/*  =================================================== */
/*  Intelligent FRAC.		(C) 2000  Michael Glickman      */
/*  --------------------------------------------------- */
/*  See LICENSE regarding distribution policy and       */
/*  conditions of use.                                  */ 
/*  =================================================== */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ifr_autoconf.h"
#include "ifr_common.h"
#include "ifr_pixmap.h"

#define HASH_SIZE 16

struct hash_entry
{
    char *code_ptr;
    int	   value;
    struct hash_entry *next;
};

static struct hash_entry *hash_table[HASH_SIZE];


static int hash_value(unsigned char *source, int length)
{
    int hv = 0;
    while (--length >= 0) hv += *source++;
    return hv % HASH_SIZE;      
}

static int store_value(unsigned char *code, int length, int value)
{
    int hv;
    struct hash_entry *this_entry, *next_entry;
    
    this_entry = malloc(sizeof(struct hash_entry));
    if (!this_entry) return 0;
    this_entry->code_ptr = code;
    this_entry->value = value;
    
    hv = hash_value(code, length);
    next_entry = hash_table[hv];
    this_entry->next = next_entry;
    hash_table[hv] = this_entry; 
    return 1;       
}

static int retrieve_value(unsigned char *code, int length)
{
    int hv;
    struct hash_entry *this_entry;
    
    hv = hash_value(code, length);
    this_entry = hash_table[hv];
    
    while(this_entry)
    {  if (memcmp(code, this_entry->code_ptr, length) == 0)
	    return this_entry->value;
       this_entry = this_entry->next;
    }
    
    return 0;       	    
}


static void free_hash_table(void)
{
    int i;
    struct hash_entry *this_entry, *next_entry;
    
    for (i=0; i<HASH_SIZE; i++)
    {
	this_entry = hash_table[i];
	while(this_entry)
	{   next_entry = this_entry->next;
	    free(this_entry);
	    this_entry = next_entry;
	}
    }		    	        

}

int process_monochrome_pixmap(char *pixmap[], MPMP_INFO *info)
{

    char *pmp_line, **pmp_line_ptr;
    int  width, height, colours_used;	
    int  cpp;		/* Bytes per pixel */
    int  i, j , k;
    unsigned char *grey_levels = NULL, *grey_levels_ptr;
    unsigned char *pixels = NULL, *pixels_ptr;
    
    memset(info, '\0', sizeof(MPMP_INFO));
    memset(hash_table, '\0', sizeof(hash_table));

    pmp_line_ptr = pixmap;

    /* Header line */
    pmp_line = *pmp_line_ptr++;
    if (sscanf(pmp_line, "%d %d %d %d", &width, &height, &colours_used, &cpp) != 4)
	    goto BadLuck;

    grey_levels = malloc(colours_used);
    pixels = malloc(width * height);    
    if (grey_levels == NULL || pixels == NULL) goto BadLuck;

    grey_levels_ptr = grey_levels;
    
    /* Colour entries */
    for(k=0; k<colours_used; k++)
    {
	pmp_line = *pmp_line_ptr++;

	store_value(pmp_line, cpp, k);

	pmp_line+= cpp;
	pmp_line += strspn(pmp_line, " \t");
	if (strlen(pmp_line) < 3) goto BadLuck;
	pmp_line += 2;
	
	if (*pmp_line++ != '#')
	    i = 0;
	else 
	/* This sould work for any type of pixmap colour scheme */
	    sscanf(pmp_line, "%2x", &i);
	    
	*grey_levels_ptr++ = i;	    
    }    
    

    /* Pixmap entries */
    i = height;
    pixels_ptr = pixels;
    
    while(--i>= 0)
    {
	pmp_line = *pmp_line_ptr++;
	j = width;
	while (--j >= 0)
	{
	    *pixels_ptr++ = retrieve_value(pmp_line, cpp);
	    pmp_line += cpp;
	}	    	    
	    
    }
    

    free_hash_table();
    info->width = width;
    info->height = height;
    info->colours_used = colours_used;
    info->grey_levels = grey_levels;    	
    info->pixels = pixels;    	
    return 1;    
    
BadLuck:
    free_hash_table();
    if (grey_levels) free(grey_levels);
    if (pixels) free(pixels);
    return 0;
}    

/*
// Here grey levels is just an array of rgb 
int process_colour_pixmap(char *pixmap[], MPMP_INFO *info)
{
    char *pmp_line, **pmp_line_ptr;
    int  width, height, colours_used;
    int  cpp;           // Bytes per pixel 
    int  i, j , k;
    int  r, g , b;
    unsigned char *grey_levels = NULL, *grey_levels_ptr;
    unsigned char *pixels = NULL, *pixels_ptr;
    
    memset(info, '\0', sizeof(MPMP_INFO));
    memset(hash_table, '\0', sizeof(hash_table));

    pmp_line_ptr = pixmap;

    // Header line 
    pmp_line = *pmp_line_ptr++;
    if (sscanf(pmp_line, "%d %d %d %d", &width, &height, &colours_used, &cpp) != 4)
            goto BadLuck;

    grey_levels = malloc(colours_used*3);
    pixels = malloc(width * height); 
    if (grey_levels == NULL || pixels == NULL) goto BadLuck;

    grey_levels_ptr = grey_levels;

    // Colour entries 
    for(k=0; k<colours_used; k++)
    {
        pmp_line = *pmp_line_ptr++;
        store_value(pmp_line, cpp, k);
        pmp_line+= cpp;
        pmp_line += strspn(pmp_line, " \t");
        if (strlen(pmp_line) < 3) goto BadLuck;
        pmp_line += 2;

        if (*pmp_line++ != '#')
            r = g = b = 0;
        else if (strlen(pmp_line) >= 10)
        {   sscanf(pmp_line, "%4x%4x%4x", &r, &g, &b);
            r >>= 8; g >>=8; b>>=8;
        }
        else
            sscanf(pmp_line, "%2x%2x%2x", &r, &g, &b);

        *grey_levels_ptr++ = r;
        *grey_levels_ptr++ = g;
        *grey_levels_ptr++ = b;
    }  
  

    // Pixmap entries 
    i = height;
    pixels_ptr = pixels;
    
    while(--i>= 0)
    {
        pmp_line = *pmp_line_ptr++;
        j = width;
        while (--j >= 0)
        {
            *pixels_ptr++ = retrieve_value(pmp_line, cpp);
            pmp_line += cpp;
        } 

    }
  
    free_hash_table();
    info->width = width;
    info->height = height;
    info->colours_used = colours_used;
    info->grey_levels = grey_levels;
    info->pixels = pixels; 
    return 1;

BadLuck:
    free_hash_table();
    if (grey_levels) free(grey_levels);
    if (pixels) free(pixels);
    return 0;
}
*/

/* This will reduce the range of colours to r */

int degrade_monochrome_pixmap(MPMP_INFO *info, int r)
{
	int width, height, x, y, old_range; /*, iter; */
	unsigned char *old_grey_levels,  *pixels, *new_grey_levels;
	unsigned short v, minval, maxval, bestdist, dist, bestind;
	

	new_grey_levels = malloc(r * sizeof(unsigned char));
	if (new_grey_levels == NULL) return 0;


	old_range = info->colours_used;
	old_grey_levels = info->grey_levels;	

	minval = r-1; maxval = 0;

	for (x=0;x < old_range; x++)
	{  v = old_grey_levels[x];
	   if (v < minval) minval = v; 				
	   if (v > maxval) maxval = v; 				
	}            	
       	  	 
	/* Now rescale it */

	for (x=0; x<r; x++)
	  new_grey_levels[x] = minval + x * (maxval-minval)/(r-1);
	
	/* And adjust the values */
/*	iter = 3;
	while (--iter >= 0) */
	{
		for (x=0;x < old_range; x++)
		{  v = old_grey_levels[x];
		   bestdist = 0xffff; bestind = r;	
		   for ( y=0; y<r; y++)
		   { dist = abs(v - new_grey_levels[y]);
			 if (dist < bestdist)
			 { bestdist = dist; bestind = y; }
		   }
	       if (bestind >= r)
		   {  fprintf(stderr, "Something is rotten in the state of Denmark\n");
			  goto BadLuck;
		   }		
/*		   new_grey_levels[bestind] += (v - new_grey_levels[bestind])/128; */
           /* Along the last iteration, 'old grey levels' become a map */	 	
/*		   if (iter == 0)  old_grey_levels[x] = bestind; */
		   old_grey_levels[x] = bestind; 
		}
	}            	
	
	/* Remap pixels */
	width  = info->width;
	height = info->height;
	pixels = info->pixels;
	for (y=0; y<height; y++)
	   for (x=0; x<width; x++)
		{  *pixels = old_grey_levels[*pixels];
		   pixels++;
		}
	 
	free(old_grey_levels);
	info->colours_used = r;
	info->grey_levels = new_grey_levels;
	return 1;

BadLuck:
	free(new_grey_levels);
	return 0;

}

void free_monochrome_pixmap(MPMP_INFO *info)
{
    char *ptr;
    
    if ((ptr=info->pixels)!=NULL) free(ptr);
    if ((ptr=info->grey_levels)!=NULL) free(ptr);

}
    
