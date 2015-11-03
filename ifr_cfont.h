
int init_cfont(void);
long	text_width_cfont(FONT_TYPE which_font, const char *text, int len);
unsigned char *text_to_bitmap_cfont(FONT_TYPE which_font, const char *text,
									        int *width_return, int *height_return);
unsigned char *text_to_bitmap_multiline_cfont(char *text, 
				int linecharwid, int *width_return, int *height_return);
void deinit_cfont(void);
