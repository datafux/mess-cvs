/***************************************************************************

	Sega System 18 hardware

***************************************************************************/

#include "driver.h"
#include "segaic16.h"



/*************************************
 *
 *	Debugging
 *
 *************************************/

#define PRINT_UNUSUAL_MODES		(0)
#define DEBUG_VDP				(0)



/*************************************
 *
 *	Statics
 *
 *************************************/

static struct tilemap *textmap;
static struct mame_bitmap *tempbitmap;

static UINT8 tile_bank[8];
static UINT8 sprite_bank[16];

static UINT8 draw_enable;
static UINT8 screen_flip;
static UINT8 grayscale_enable;
static UINT8 vdp_enable;
static UINT8 vdp_mixing;

static UINT16 latched_xscroll[4], latched_yscroll[4], latched_pageselect[4];



/*************************************
 *
 *	Prototypes
 *
 *************************************/

extern int start_system18_vdp(void);
extern void update_system18_vdp(struct mame_bitmap *bitmap, const struct rectangle *cliprect);

static void latch_tilemap_values(int param);
static void get_tile_info(int tile_index);
static void get_text_info(int tile_index);



/*************************************
 *
 *	Video startup
 *
 *************************************/

VIDEO_START( system18 )
{
	int i;

	/* create the tilemap for the text layer */
	textmap = tilemap_create(get_text_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,28);
	if (!textmap)
		return 1;

	/* configure it */
	tilemap_set_transparent_pen(textmap, 0);
	tilemap_set_scrolldx(textmap, -24*8, -24*8);
	tilemap_set_scrollx(textmap, 0, 0);

	/* create the tilemaps for the bg/fg layers */
	if (!segaic16_init_virtual_tilemaps(16, 0, get_tile_info))
		return 1;

	/* create the VDP */
	if (start_system18_vdp())
		return 1;

	/* create a temp bitmap to draw the VDP data into */
	tempbitmap = bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 16);
	if (!tempbitmap)
		return 1;

	/* initialize globals */
	draw_enable = 1;
	screen_flip = 0;
	for (i = 0; i < 16; i++)
		sprite_bank[i] = i;
	for (i = 0; i < 8; i++)
		tile_bank[i] = i;

	/* compute palette info */
	segaic16_init_palette(2048);
	return 0;
}


void system18_reset_video(void)
{
	/* set a timer to latch values on scanline 261 */
	timer_set(cpu_getscanlinetime(261), 0, latch_tilemap_values);
}



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_tile_info(int tile_index)
{
/*
	MSB          LSB
	p--------------- Priority flag
	-??------------- Unknown
	---bbb---------- Tile bank select (0-7)
	---ccccccc------ Palette (0-127)
	------nnnnnnnnnn Tile index (0-1023)
*/
	UINT16 data = segaic16_tileram[segaic16_tilemap_page * (64*32) + tile_index];
	int bank = tile_bank[(data >> 10) & 7];
	int color = (data >> 6) & 0x7f;
	int code = data & 0x03ff;

	SET_TILE_INFO(0, bank * 0x400 + code, color, 0);
	tile_info.priority = (data >> 15) & 1;
}



/*************************************
 *
 *	Textmap callbacks
 *
 *************************************/

static void get_text_info(int tile_index)
{
/*
	MSB          LSB
	p--------------- Priority flag
	-???------------ Unknown
	----ccc--------- Palette (0-7)
	-------nnnnnnnnn Tile index (0-511)
*/
	UINT16 data = segaic16_textram[tile_index];
	int bank = tile_bank[0];
	int color = (data >> 9) & 0x07;
	int code = data & 0x1ff;

	SET_TILE_INFO(0, bank * 0x400 + code, color, 0);
	tile_info.priority = (data >> 15) & 1;
}



/*************************************
 *
 *	Miscellaneous setters
 *
 *************************************/

void system18_set_draw_enable(int enable)
{
	enable = (enable != 0);
	if (draw_enable != enable)
	{
		force_partial_update(cpu_getscanline());
		draw_enable = enable;
	}
}


void system18_set_screen_flip(int flip)
{
	flip = (flip != 0);
	if (screen_flip != flip)
	{
		force_partial_update(cpu_getscanline());
		screen_flip = flip;
	}
}


void system18_set_tile_bank(int which, int bank)
{
	if (bank != tile_bank[which])
	{
		force_partial_update(cpu_getscanline());
		tile_bank[which] = bank;

		/* mark all tilemaps dirty */
		tilemap_mark_all_tiles_dirty(NULL);
	}
}


void system18_set_grayscale(int enable)
{
	enable = (enable != 0);
	if (enable != grayscale_enable)
	{
		force_partial_update(cpu_getscanline());
		grayscale_enable = enable;
//		printf("Grayscale = %02X\n", enable);
	}
}


void system18_set_vdp_enable(int enable)
{
	enable = (enable != 0);
	if (enable != vdp_enable)
	{
		force_partial_update(cpu_getscanline());
		vdp_enable = enable;
#if DEBUG_VDP
		printf("VDP enable = %02X\n", enable);
#endif
	}
}


void system18_set_vdp_mixing(int mixing)
{
	if (mixing != vdp_mixing)
	{
		force_partial_update(cpu_getscanline());
		vdp_mixing = mixing;
#if DEBUG_VDP
		printf("VDP mixing = %02X\n", mixing);
#endif
	}
}


void system18_set_sprite_bank(int which, int bank)
{
	if (sprite_bank[which * 2] != bank * 2)
	{
		force_partial_update(cpu_getscanline());
		sprite_bank[which * 2 + 0] = bank * 2 + 0;
		sprite_bank[which * 2 + 1] = bank * 2 + 1;
	}
}



/*************************************
 *
 *	Tilemap accessors
 *
 *************************************/

WRITE16_HANDLER( system18_textram_w )
{
	/* column/rowscroll need immediate updates */
	if (offset >= 0xf00/2)
		force_partial_update(cpu_getscanline());

	COMBINE_DATA(&segaic16_textram[offset]);
	tilemap_mark_tile_dirty(textmap, offset);
}



/*************************************
 *
 *	Latch screen-wide tilemap values
 *
 *************************************/

static void latch_tilemap_values(int param)
{
	int i;

	/* latch the scroll and page select values */
	for (i = 0; i < 4; i++)
	{
		latched_pageselect[i] = segaic16_textram[0xe80/2 + i];
		latched_yscroll[i] = segaic16_textram[0xe90/2 + i];
		latched_xscroll[i] = segaic16_textram[0xe98/2 + i];
	}
	
	/* set a timer to do this again next frame */
	timer_set(cpu_getscanlinetime(261), 0, latch_tilemap_values);
}



/*************************************
 *
 *	Draw a single tilemap layer
 *
 *************************************/

static void draw_layer(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int which, int flags, int priority)
{
	UINT16 xscroll, yscroll, pages;
	int x, y;

	/* get global values */
	xscroll = latched_xscroll[which];
	yscroll = latched_yscroll[which];
	pages = latched_pageselect[which];

	/* column scroll? */
	if (yscroll & 0x8000)
	{
		if (PRINT_UNUSUAL_MODES) printf("Column AND row scroll\n");

		/* loop over row chunks */
		for (y = cliprect->min_y & ~7; y <= cliprect->max_y; y += 8)
		{
			struct rectangle rowcolclip;

			/* adjust to clip this row only */
			rowcolclip.min_y = (y < cliprect->min_y) ? cliprect->min_y : y;
			rowcolclip.max_y = (y + 7 > cliprect->max_y) ? cliprect->max_y : y + 7;

			/* loop over column chunks */
			for (x = ((cliprect->min_x + 9) & ~15) - 9; x <= cliprect->max_x; x += 16)
			{
				UINT16 effxscroll, effyscroll, rowscroll;
				UINT16 effpages = pages;

				/* adjust to clip this column only */
				rowcolclip.min_x = (x < cliprect->min_x) ? cliprect->min_x : x;
				rowcolclip.max_x = (x + 15 > cliprect->max_x) ? cliprect->max_x : x + 15;

				/* get the effective scroll values */
				rowscroll = segaic16_textram[0xf80/2 + 0x40/2 * which + y/8];
				effxscroll = (xscroll & 0x8000) ? rowscroll : xscroll;
				effyscroll = segaic16_textram[0xf16/2 + 0x40/2 * which + (x+9)/16];

				/* are we using an alternate? */
				if (rowscroll & 0x8000)
				{
					effxscroll = latched_xscroll[which + 2];
					effyscroll = latched_yscroll[which + 2];
					effpages = latched_pageselect[which + 2];
				}

				/* draw the chunk */
				effxscroll = (0xc0 - effxscroll) & 0x3ff;
				effyscroll = effyscroll & 0x1ff;
				segaic16_draw_virtual_tilemap(bitmap, &rowcolclip, effpages, effxscroll, effyscroll, flags, priority);
			}
		}
	}
	else
	{
		if (PRINT_UNUSUAL_MODES) printf("Row scroll\n");

		/* loop over row chunks */
		for (y = cliprect->min_y & ~7; y <= cliprect->max_y; y += 8)
		{
			struct rectangle rowclip = *cliprect;
			UINT16 effxscroll, effyscroll, rowscroll;
			UINT16 effpages = pages;

			/* adjust to clip this row only */
			rowclip.min_y = (y < cliprect->min_y) ? cliprect->min_y : y;
			rowclip.max_y = (y + 7 > cliprect->max_y) ? cliprect->max_y : y + 7;

			/* get the effective scroll values */
			rowscroll = segaic16_textram[0xf80/2 + 0x40/2 * which + y/8];
			effxscroll = (xscroll & 0x8000) ? rowscroll : xscroll;
			effyscroll = yscroll;

			/* are we using an alternate? */
			if (rowscroll & 0x8000)
			{
				effxscroll = latched_xscroll[which + 2];
				effyscroll = latched_yscroll[which + 2];
				effpages = latched_pageselect[which + 2];
			}

			/* draw the chunk */
			effxscroll = (0xc0 - effxscroll) & 0x3ff;
			effyscroll = effyscroll & 0x1ff;
			segaic16_draw_virtual_tilemap(bitmap, &rowclip, effpages, effxscroll, effyscroll, flags, priority);
		}
	}
}



/*************************************
 *
 *	Draw a single sprite
 *
 *************************************/

/*
	A note about zooming:

	The current implementation is a guess at how the hardware works. Hopefully
	we will eventually get some good tests run on the hardware to understand
	which rows/columns are skipped during a zoom operation.

	The ring sprites in hwchamp are excellent testbeds for the zooming.
*/

#define draw_pixel() 														\
	/* only draw if onscreen, not 0 or 15 */								\
	if (x >= cliprect->min_x && pix != 0 && pix != 15)						\
	{																		\
		/* are we high enough priority to be visible? */					\
		if (sprpri > pri[x])												\
		{																	\
			/* shadow/hilight mode? */										\
			if (color == 1024 + (0x3f << 4))								\
			{																\
				if (dest[x] < 0x400)	/* don't affect VDP */				\
					dest[x] += (paletteram16[dest[x]] & 0x8000) ? 4096 : 2048;\
			}																\
																			\
			/* regular draw */												\
			else															\
				dest[x] = pix | color;										\
		}																	\
																			\
		/* always mark priority so no one else draws here */				\
		pri[x] = 0xff;														\
	}																		\

static void draw_one_sprite(struct mame_bitmap *bitmap, const struct rectangle *cliprect, UINT16 *data)
{
	int bottom  = data[0] >> 8;
	int top     = data[0] & 0xff;
	int xpos    = (data[1] & 0x1ff) - 0xb8;
	int hide    = data[2] & 0x4000;
	int flip    = data[2] & 0x100;
	int pitch   = (INT8)(data[2] & 0xff);
	UINT16 addr = data[3];
	int bank    = sprite_bank[(data[4] >> 8) & 0xf];
	int sprpri  = 1 << ((data[4] >> 6) & 0x3);
	int color   = 1024 + ((data[4] & 0x3f) << 4);
	int vzoom   = (data[5] >> 5) & 0x1f;
	int hzoom   = data[5] & 0x1f;
	int x, y, pix, numbanks;
	UINT16 *spritedata;

	/* initialize the end address to the start address */
	data[7] = addr;

	/* if hidden, or top greater than/equal to bottom, or invalid bank, punt */
	if (hide || (top >= bottom) || bank == 255)
		return;
	
	/* clamp to within the memory region size */
	numbanks = memory_region_length(REGION_GFX2) / 0x20000;
	if (numbanks)
		bank %= numbanks;
	spritedata = (UINT16 *)memory_region(REGION_GFX2) + 0x10000 * bank;

	/* reset the yzoom counter */
	data[5] &= 0x03ff;

	/* for the non-flipped case, we start one row ahead */
	if (!flip)
		addr += pitch;

	/* loop from top to bottom */
	for (y = top; y < bottom; y++)
	{
		/* skip drawing if not within the cliprect */
		if (y >= cliprect->min_y && y <= cliprect->max_y)
		{
			UINT16 *dest = (UINT16 *)bitmap->line[y];
			UINT8 *pri = (UINT8 *)priority_bitmap->line[y];
			int xacc = 0x20;

			/* non-flipped case */
			if (!flip)
			{
				/* start at the word before because we preincrement below */
				data[7] = addr - 1;
				for (x = xpos; x <= cliprect->max_x; )
				{
					UINT16 pixels = spritedata[++data[7]];

					/* draw four pixels */
					pix = (pixels >> 12) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  8) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  4) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  0) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;

					/* stop if the last pixel in the group was 0xf */
					if (pix == 15)
						break;
				}
			}

			/* flipped case */
			else
			{
				/* start at the word after because we predecrement below */
				data[7] = addr + pitch + 1;
				for (x = xpos; x <= cliprect->max_x; )
				{
					UINT16 pixels = spritedata[--data[7]];

					/* draw four pixels */
					pix = (pixels >>  0) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  4) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >>  8) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;
					pix = (pixels >> 12) & 0xf; if (xacc < 0x40) { draw_pixel(); x++; } else xacc -= 0x40; xacc += hzoom;

					/* stop if the last pixel in the group was 0xf */
					if (pix == 15)
						break;
				}
			}
		}

		/* advance a row */
		addr += pitch;

		/* accumulate zoom factors; if we carry into the high bit, skip an extra row */
		data[5] += vzoom << 10;
		if (data[5] & 0x8000)
		{
			addr += pitch;
			data[5] &= ~0x8000;
		}
	}
}



/*************************************
 *
 *	Sprite drawing
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	UINT16 *cursprite;

	/* first scan forward to find the end of the list */
	for (cursprite = segaic16_spriteram; cursprite < segaic16_spriteram + 0x7ff/2; cursprite += 8)
		if (cursprite[2] & 0x8000)
			break;

	/* now scan backwards and render the sprites in order */
	for (cursprite -= 8; cursprite >= segaic16_spriteram; cursprite -= 8)
		draw_one_sprite(bitmap, cliprect, cursprite);
}



/*************************************
 *
 *	VDP drawing
 *
 *************************************/

static void draw_vdp(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority)
{
	int x, y;

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *src = (UINT16 *)tempbitmap->line[y];
		UINT16 *dst = (UINT16 *)bitmap->line[y];
		UINT8 *pri = (UINT8 *)priority_bitmap->line[y];

		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			UINT16 pix = src[x];
			if (pix != 0xffff)
			{
				dst[x] = pix;
				pri[x] |= priority;
			}
		}
	}
}



/*************************************
 *
 *	Video update
 *
 *************************************/

VIDEO_UPDATE( system18 )
{
	int vdppri, vdplayer;

/*
	Current understanding of VDP mixing:

	mixing = 0x00:
		astorm: layer = 0, pri = 0x00 or 0x01
		lghost: layer = 0, pri = 0x00 or 0x01
		
	mixing = 0x01:
		ddcrew: layer = 0, pri = 0x00 or 0x01 or 0x02

	mixing = 0x02:
		never seen

	mixing = 0x03:
		ddcrew: layer = 1, pri = 0x00 or 0x01 or 0x02

	mixing = 0x04:
		astorm: layer = 2 or 3, pri = 0x00 or 0x01 or 0x02
		mwalk:  layer = 2 or 3, pri = 0x00 or 0x01 or 0x02

	mixing = 0x05:
		ddcrew: layer = 2, pri = 0x04
		wwally: layer = 2, pri = 0x04

	mixing = 0x06:
		never seen

	mixing = 0x07:
		cltchitr: layer = 1 or 2 or 3, pri = 0x02 or 0x04 or 0x08
		mwalk:    layer = 3, pri = 0x04 or 0x08
*/
	vdplayer = (vdp_mixing >> 1) & 3;
	vdppri = (vdp_mixing & 1) ? (1 << vdplayer) : 0;
	
#if DEBUG_VDP
	if (code_pressed(KEYCODE_Q)) vdplayer = 0;
	if (code_pressed(KEYCODE_W)) vdplayer = 1;
	if (code_pressed(KEYCODE_E)) vdplayer = 2;
	if (code_pressed(KEYCODE_R)) vdplayer = 3;
	if (code_pressed(KEYCODE_A)) vdppri = 0x00;
	if (code_pressed(KEYCODE_S)) vdppri = 0x01;
	if (code_pressed(KEYCODE_D)) vdppri = 0x02;
	if (code_pressed(KEYCODE_F)) vdppri = 0x04;
	if (code_pressed(KEYCODE_G)) vdppri = 0x08;
#endif
	
	/* if no drawing is happening, fill with black and get out */
	if (!draw_enable)
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}

	/* if the VDP is enabled, update our tempbitmap */
	if (vdp_enable)
		update_system18_vdp(tempbitmap, cliprect);

	/* reset priorities */
	fillbitmap(priority_bitmap, 0, cliprect);

	/* draw background opaquely first, not setting any priorities */
	draw_layer(bitmap, cliprect, 1, 0 | TILEMAP_IGNORE_TRANSPARENCY, 0x00);
	draw_layer(bitmap, cliprect, 1, 1 | TILEMAP_IGNORE_TRANSPARENCY, 0x00);
	if (vdp_enable && vdplayer == 0) draw_vdp(bitmap, cliprect, vdppri);

	/* draw background again to draw non-transparent pixels over the VDP and set the priority */
	draw_layer(bitmap, cliprect, 1, 0, 0x01);
	draw_layer(bitmap, cliprect, 1, 1, 0x02);
	if (vdp_enable && vdplayer == 1) draw_vdp(bitmap, cliprect, vdppri);

	/* draw foreground */
	draw_layer(bitmap, cliprect, 0, 0, 0x02);
	draw_layer(bitmap, cliprect, 0, 1, 0x04);
	if (vdp_enable && vdplayer == 2) draw_vdp(bitmap, cliprect, vdppri);

	/* text layer */
	tilemap_draw(bitmap, cliprect, textmap, 0, 0x04);
	tilemap_draw(bitmap, cliprect, textmap, 1, 0x08);
	if (vdp_enable && vdplayer == 3) draw_vdp(bitmap, cliprect, vdppri);

	/* draw the sprites */
	draw_sprites(bitmap, cliprect);

#if DEBUG_VDP
	if (vdp_enable && code_pressed(KEYCODE_V))
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		update_system18_vdp(bitmap, cliprect);
	}
	if (vdp_enable && code_pressed(KEYCODE_B))
	{
		FILE *f = fopen("vdp.bin", "w");
		fwrite(tempbitmap->line[0], 1, ((UINT8 *)tempbitmap->line[1] - (UINT8 *)tempbitmap->line[0]) * tempbitmap->height, f);
		fclose(f);
	}
#endif
}