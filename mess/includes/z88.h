#include "driver.h"
#include "osdepend.h"
#define Z88_NUM_COLOURS 2

#define Z88_SCREEN_WIDTH        640
#define Z88_SCREEN_HEIGHT       64

int z88_vh_start(void);
void z88_vh_stop(void);
void z88_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void z88_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom);
