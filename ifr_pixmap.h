typedef struct
{
    int	width, height;
    int colours_used;
    unsigned char *grey_levels;     
    unsigned char *pixels;
} MPMP_INFO;

int process_monochrome_pixmap(char *pixmap[], MPMP_INFO *info);
void free_monochrome_pixmap(MPMP_INFO *info);
int degrade_monochrome_pixmap(MPMP_INFO *info, int r);
int process_colour_pixmap(char *pixmap[], MPMP_INFO *info);

int fill_background_pixmap(int which_pane, const MPMP_INFO *mpmpi,
       const unsigned short *base_colour, int start_colour);

int set_bkgr_colours(const MPMP_INFO *mpmpi,
			 const unsigned short *base_colour, int start_colour);

extern int insert_to_pixmap(int which_pane, const MPMP_INFO *mpmpi,
	      unsigned short  base_colour[3],
	      int sx, int sy, int colour_start);


