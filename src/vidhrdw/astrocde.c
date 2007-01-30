/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/astrocde.h"
#include <math.h> /* for sin() and cos() */


#define SCREEN_WIDTH 320
#define MAX_LINES 204

#define MAX_INT_PER_FRAME 256

static UINT8 interrupt_enable;
static UINT8 interrupt_vector;

UINT8 *wow_videoram;
static UINT8 magic_expand_color, magic_control, magic_expand_count, magic_shift_leftover;
static UINT8 collision;

/* This value affects the star layout, the value is correct since
   it is mentioned in the docs and perfectly matches screen shots.
 */
#define CLOCKS_PER_LINE 455

/* This value affects the star blinking and the sparkle patterns.
   It is just a guess, aiming to a supposed 60Hz refresh rate, and has
   not been verified.
 */
#define CLOCKS_PER_FRAME (CLOCKS_PER_LINE*262)

#define RNG_PERIOD 131071	/* 2^17-1 */
static int *rng;
static int *star;


static UINT8 colors[MAX_INT_PER_FRAME][8];
static UINT8 colorsplit[MAX_INT_PER_FRAME];
static UINT8 BackgroundData,VerticalBlank;

static UINT8 sparkle[MAX_INT_PER_FRAME][4];	/* sparkle[line][0] is star enable */

static void wow_update_line(mame_bitmap *bitmap,int line);
static void profpac_update_line(mame_bitmap *bitmap,int line);

/* This is the handler for reading the display memory */
/* It is switched at init time for different games */
read8_handler astrocde_videoram_r;

PALETTE_INIT( astrocde )
{
	/* This routine builds a palette using a transformation from */
	/* the YUV (Y, B-Y, R-Y) to the RGB color space */

	int i,j;

	float Y, RY, BY;	/* Y, R-Y, and B-Y signals as generated by the game */
						/* Y = Luminance -> (0 to 1) */
						/* R-Y = Color along R-Y axis -> C*(-1 to +1) */
						/* B-Y = Color along B-Y axis -> C*(-1 to +1) */
	float R, G, B;

	float brightest = 1.0;	/* Approx. Luminance values for the extremes -> (0 to 1) */
	float dimmest   = 0.0;
	float C = 0.75;			/* Approx. Chroma intensity */

	/* The astrocade has a 256 color palette                 */
	/* 32 colors, with 8 luminance levels for each color     */
	/* The 32 colors circle around the YUV color space,      */
	/* with the exception of the first 8 which are grayscale */

	/* Note: to simulate a B&W monitor, set C=0 and all      */
	/*       colors will appear as the first 8 grayscales    */

	for(i=0;i<32;i++)
	{
		RY = C*sin(i*2.0*M_PI/32.0);
		if (i == 0)
			BY = 0;
		else
//          BY = C*cos(i*2.0*M_PI/32.0);
			BY = 1.15*cos(i*2.0*M_PI/32.0);


		for(j=0;j<8;j++)
		{
			Y = (j/7.0)*(brightest-dimmest)+dimmest;

			/* Transform to RGB */

			R = (RY+Y)*255;
			G = (Y - 0.299*(RY+Y) - 0.114*(BY+Y))/0.587*255;
			B = (BY+Y)*255;

			/* Clipping, in case of saturation */

			if (R < 0)
				R = 0;
			if (R > 255)
				R = 255;
			if (G < 0)
				G = 0;
			if (G > 255)
				G = 255;
			if (B < 0)
				B = 0;
			if (B > 255)
				B = 255;

			/* Round, and set the value */
			palette_set_color(machine,i*8+j,floor(R+.5),floor(G+.5),floor(B+.5));
		}
	}
}



/****************************************************************************
 * Scanline Interrupt System
 ****************************************************************************/

static UINT8 NextScanInt=0;			/* Normal */
static UINT8 CurrentScan=0;
static UINT8 InterruptFlag=0;

static UINT8 GorfDelay;				/* Gorf */

WRITE8_HANDLER( astrocde_interrupt_vector_w )
{
	interrupt_vector = data;
}


WRITE8_HANDLER( astrocde_interrupt_enable_w )
{
	InterruptFlag = data;

	interrupt_enable = ~data & 0x01;

	/* Gorf Special interrupt */

	if (data & 0x10)
 	{
  		GorfDelay =(CurrentScan + 7) & 0xFF;

		/* Gorf Special *MUST* occur before next scanline interrupt */

		if ((NextScanInt > CurrentScan) && (NextScanInt < GorfDelay))
		{
		  	GorfDelay = NextScanInt - 1;
		}

#ifdef VERBOSE
		logerror("Gorf Delay set to %02x\n",GorfDelay);
#endif

	}

#ifdef VERBOSE
	logerror("Interrupt Flag set to %02x\n",InterruptFlag);
#endif
}

WRITE8_HANDLER( astrocde_interrupt_w )
{
	/* A write to 0F triggers an interrupt at that scanline */

#ifdef VERBOSE
	logerror("Scanline interrupt set to %02x\n",data);
#endif

	NextScanInt = data;
}

static void interrupt_common(void)
{
	int i,next;

	video_screen_update_partial(0, CurrentScan);

	next = (CurrentScan + 1) % MAX_INT_PER_FRAME;
	for (i = 0;i < 8;i++)
		colors[next][i] = colors[CurrentScan][i];
	for (i = 0;i < 4;i++)
		sparkle[next][i] = sparkle[CurrentScan][i];
	colorsplit[next] = colorsplit[CurrentScan];

	CurrentScan = next;
}

INTERRUPT_GEN( wow_interrupt )
{
	interrupt_common();

	/* Scanline interrupt enabled ? */

	if (interrupt_enable && (InterruptFlag & 0x08) && (CurrentScan == NextScanInt))
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, interrupt_vector);
}

/****************************************************************************
 * Gorf - Interrupt routine and Timer hack
 ****************************************************************************/

INTERRUPT_GEN( gorf_interrupt )
{
	interrupt_common();

	/* Gorf Special Bits */

	cpunum_set_input_line(0,0,CLEAR_LINE);

	if (interrupt_enable && (InterruptFlag & 0x10) && (CurrentScan==GorfDelay))
	{
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, interrupt_vector & 0xf0);
	}
	else if ((InterruptFlag & 0x08) && (CurrentScan == NextScanInt))
	{
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, interrupt_vector);
	}
}

/* ======================================================================= */

READ8_HANDLER( wow_video_retrace_r )
{
	return CurrentScan;
}

READ8_HANDLER( wow_intercept_r )
{
	int res;

	res = collision;
	collision = 0;

	return res;
}


/* Switches color registers at this zone - 40 zones (NOT USED) */

WRITE8_HANDLER( astrocde_colour_split_w )
{
	colorsplit[CurrentScan] = 2 * (data & 0x3f);

	BackgroundData = ((data&0xc0) >> 6) * 0x55;
}


/* This selects commercial (high res, arcade) or
                  consumer (low res, astrocade) mode */

WRITE8_HANDLER( astrocde_mode_w )
{
//  astrocade_mode = data & 0x01;
}


WRITE8_HANDLER( astrocde_vertical_blank_w )
{
	if (VerticalBlank != data)
	{
		VerticalBlank = data;
	}
}


WRITE8_HANDLER( astrocde_colour_register_w )
{
	colors[CurrentScan][offset] = data;

#ifdef VERBOSE
	logerror("colors %01x set to %02x\n",offset,data);
#endif
}

WRITE8_HANDLER( astrocde_colour_block_w )
{
	static int color_reg_num = 7;

	astrocde_colour_register_w(color_reg_num,data);

	color_reg_num = (color_reg_num - 1) & 7;
}


READ8_HANDLER( wow_videoram_r )
{
	return wow_videoram[offset];
}

static UINT16 profpac_color_mapping[4];

UINT16 profpac_map_colors(UINT8 data)
{
	UINT16 temp = 0x0000;
	temp |= profpac_color_mapping[data>>6] << 12;
	temp |= profpac_color_mapping[(data>>4)&0x03] << 8;
	temp |= profpac_color_mapping[(data>>2)&0x03] << 4;
	temp |= profpac_color_mapping[data&0x03];
	return temp;
}

static UINT8 profpac_read_page = 0;
static UINT8 profpac_write_page = 0;
static UINT8 profpac_visible_page = 0;

WRITE8_HANDLER( profpac_page_select_w )
{
	profpac_read_page = data&3;
	profpac_write_page = (data>>2)&3;
	profpac_visible_page = (data>>4)&3;
}

static UINT8 profpac_read_half = 0;
static UINT8 profpac_write_mode = 0;
static UINT8 profpac_intercept = 0;

READ8_HANDLER( profpac_intercept_r )
{
	return profpac_intercept;
}

static UINT16 *profpac_videoram = 0;
static UINT8 profpac_vw = 0;
static UINT8 profpac_cw = 0;

WRITE8_HANDLER( profpac_screenram_ctrl_w )
{
	switch (offset)
	{
		rgb_t color;

		case 0: /* port 0xC0 - red component */
			color = palette_get_color(Machine, (data>>4)&0x0f);
			palette_set_color(Machine, (data>>4)&0x0f, pal4bit(data), RGB_GREEN(color), RGB_BLUE(color) );
		break;
		case 1: /* port 0xC1 - green component */
			color = palette_get_color(Machine, (data>>4)&0x0f);
			palette_set_color(Machine, (data>>4)&0x0f, RGB_RED(color), pal4bit(data), RGB_BLUE(color) );
		break;
		case 2: /* port 0xC2 - blue component */
			color = palette_get_color(Machine, (data>>4)&0x0f);
			palette_set_color(Machine, (data>>4)&0x0f, RGB_RED(color), RGB_GREEN(color), pal4bit(data) );
		break;
		case 3: /* port 0xC3 - set 2bpp to 4bpp mapping */
			profpac_color_mapping[(data>>4)&0x03] = data&0x0f;
			profpac_intercept = 0x00;
		break;
		case 4: /* port 0xC4 - which half to read on a memory access */
			profpac_vw = data&0x0f; /* refresh write enable lines TBD */
			profpac_read_half = (data>>4)&1;
		break;
		case 5: /* port 0xC5 */
			profpac_cw = data&0x0f; /* bitplane write enable lines */
			profpac_write_mode = (data>>4)&0x03;
		break;
	}
}

READ8_HANDLER( profpac_videoram_r )
{
	UINT16 temp = profpac_videoram[profpac_read_page*0x4000 + offset];

	if (profpac_read_half == 0)
		return ((temp>>6)&0xc0) + ((temp>>4)&0x30) + ((temp>>2)&0x0c) + (temp&0x03);
	else
		return ((temp>>8)&0xc0) + ((temp>>6)&0x30) + ((temp>>4)&0x0c) + ((temp>>2)&0x03);
}

/* All this information comes from decoding the PLA at U39 on the screen ram board */
WRITE8_HANDLER( profpac_videoram_w )
{
	UINT16 address = profpac_write_page*0x4000 + offset;
	UINT16 current_value = profpac_videoram[address];
	UINT16 new_value = profpac_map_colors(data);
	UINT16 composite_value = profpac_map_colors(data);

	const UINT16 mask_table[16] =
	{
		0x0000, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777,
		0x8888, 0x9999, 0xaaaa, 0xbbbb, 0xcccc, 0xdddd, 0xeeee, 0xffff
	};

	composite_value = 0;

	/* There are 4 write modes: overwrite, xor, overlay, or underlay */

	switch(profpac_write_mode)
	{
		case 0: /* normal write */
			composite_value = new_value;
			break;
		case 1: /* xor write */
			composite_value = current_value ^ new_value;
			break;
		case 2: /* overlay write */
			composite_value = 0;
			if ((new_value&0xf000) != 0x0000)
				composite_value |= (profpac_color_mapping[data>>6] << 12);
			else
				composite_value |= (current_value&0xf000);
			if ((new_value&0x0f00) != 0x0000)
				composite_value |= (profpac_color_mapping[(data>>4)&0x03] << 8);
			else
				composite_value |= (current_value&0x0f00);
			if ((new_value&0x00f0) != 0x0000)
				composite_value |= (profpac_color_mapping[(data>>2)&0x03] << 4);
			else
				composite_value |= (current_value&0x00f0);
			if ((new_value&0x000f) != 0x0000)
				composite_value |= (profpac_color_mapping[data&0x03]);
			else
				composite_value |= (current_value&0x000f);
			break;
		case 3: /* underlay write */
			if ((current_value&0xf000) == 0x0000)
				composite_value |= (profpac_color_mapping[data>>6] << 12);
			else
				composite_value |= (current_value&0xf000);
			if ((current_value&0x0f00) == 0x0000)
				composite_value |= (profpac_color_mapping[(data>>4)&0x03] << 8);
			else
				composite_value |= (current_value&0x0f00);
			if ((current_value&0x00f0) == 0x0000)
				composite_value |= (profpac_color_mapping[(data>>2)&0x03] << 4);
			else
				composite_value |= (current_value&0x00f0);
			if ((current_value&0x000f) == 0x0000)
				composite_value |= (profpac_color_mapping[data&0x03]);
			else
				composite_value |= (current_value&0x000f);
			break;
	}
	profpac_videoram[address] = composite_value & mask_table[profpac_cw];
	profpac_videoram[address] |= current_value & ~mask_table[profpac_cw];

	/* Intercept (collision) stuff */

	/* There are 3 bits on the register, which are set by various combinations of writes */

	if ((((current_value&0xf000) == 0x2000) && ((new_value&0x8000) == 0x8000)) ||
	    (((current_value&0xf000) == 0x3000) && ((new_value&0xc000) == 0x4000)) ||
	    (((current_value&0x0f00) == 0x0200) && ((new_value&0x0800) == 0x0800)) ||
	    (((current_value&0x0f00) == 0x0300) && ((new_value&0x0c00) == 0x0400)) ||
	    (((current_value&0x00f0) == 0x0020) && ((new_value&0x0080) == 0x0080)) ||
	    (((current_value&0x00f0) == 0x0030) && ((new_value&0x00c0) == 0x0040)) ||
	    (((current_value&0x000f) == 0x0002) && ((new_value&0x0008) == 0x0008)) ||
	    (((current_value&0x000f) == 0x0003) && ((new_value&0x000c) == 0x0004)))
	    profpac_intercept |= 0x01;

	if ((((new_value&0xf000) != 0x0000) && ((current_value&0xc000) == 0x4000)) ||
	    (((new_value&0x0f00) != 0x0000) && ((current_value&0x0c00) == 0x0400)) ||
	    (((new_value&0x00f0) != 0x0000) && ((current_value&0x00c0) == 0x0040)) ||
	    (((new_value&0x000f) != 0x0000) && ((current_value&0x000c) == 0x0004)))
	    profpac_intercept |= 0x02;

	if ((((new_value&0xf000) != 0x0000) && ((current_value&0x8000) == 0x8000)) ||
	    (((new_value&0x0f00) != 0x0000) && ((current_value&0x0800) == 0x0800)) ||
	    (((new_value&0x00f0) != 0x0000) && ((current_value&0x0080) == 0x0080)) ||
	    (((new_value&0x000f) != 0x0000) && ((current_value&0x0008) == 0x0008)))
	    profpac_intercept |= 0x04;
}

WRITE8_HANDLER( astrocde_magic_expand_color_w )
{
	magic_expand_color = data;
}


WRITE8_HANDLER( astrocde_magic_control_w )
{
	magic_control = data;

	magic_expand_count = 0;	/* reset flip-flop for expand mode on write to this register */
	magic_shift_leftover = 0;	/* reset shift buffer on write to this register */

	if (magic_control & 0x04)
		popmessage("unsupported MAGIC ROTATE mode");
}


static void copywithflip(int offset,int data)
{
	int shift,data1;
	UINT8 old_data;

	if (magic_control & 0x40)	/* copy backwards */
	{
		int bits,stib,k;

		bits = data;
		stib = 0;
		for (k = 0;k < 4;k++)
		{
			stib >>= 2;
			stib |= (bits & 0xc0);
			bits <<= 2;
		}

		data = stib;
	}

	shift = magic_control & 3;
	data1 = 0;
	if (magic_control & 0x40)	/* copy backwards */
	{
		while (shift > 0)
		{
			data1 <<= 2;
			data1 |= (data & 0xc0) >> 6;
			data <<= 2;
			shift--;
		}
	}
	else
	{
		while (shift > 0)
		{
			data1 >>= 2;
			data1 |= (data & 0x03) << 6;
			data >>= 2;
			shift--;
		}
	}
	data |= magic_shift_leftover;
	magic_shift_leftover = data1;

	if (magic_control & 0x30)
	{
		old_data = program_read_byte(0x4000+offset);
		collision &= 0x0f;

		if (data | old_data)
		{
			if ((data & 0x03) && (old_data & 0x03))
				collision |= 0x88;
			if ((data & 0x0c) && (old_data & 0x0c))
				collision |= 0x44;
			if ((data & 0x30) && (old_data & 0x30))
				collision |= 0x22;
			if ((data & 0xc0) && (old_data & 0xc0))
				collision |= 0x11;
		}
	}

	if (magic_control & 0x20) data ^= program_read_byte(0x4000+offset);	/* draw in XOR mode */
	else if (magic_control & 0x10) data |= program_read_byte(0x4000+offset);;	/* draw in OR mode */
	program_write_byte(0x4000+offset,data);
}


WRITE8_HANDLER( wow_magicram_w )
{
	if (magic_control & 0x08)	/* expand mode */
	{
		int bits,bibits,k;

		bits = data;
		if (magic_expand_count) bits <<= 4;
		bibits = 0;
		for (k = 0;k < 4;k++)
		{
			bibits <<= 2;
			if (bits & 0x80) bibits |= (magic_expand_color >> 2) & 0x03;
			else bibits |= magic_expand_color & 0x03;
			bits <<= 1;
		}

		copywithflip(offset,bibits);

		magic_expand_count ^= 1;
	}
	else copywithflip(offset,data);
}


static int src;
static int mode;	/*  bit 0 = direction
                        bit 1 = expand mode
                        bit 2 = constant
                        bit 3 = flush
                        bit 4 = flip
                        bit 5 = flop */
static int skip;	/* bytes to skip after row copy */
static int dest;
static int length;	/* row length */
static int loops;	/* rows to copy - 1 */

WRITE8_HANDLER( astrocde_pattern_board_w )
{
	switch (offset)
	{
		case 0:
			src = data;
			break;
		case 1:
			src = src + data * 256;
			break;
		case 2:
			mode = data & 0x3f;			/* register is 6 bit wide */
			break;
		case 3:
			skip = data;
			break;
		case 4:
			dest = skip + data * 256;	/* register 3 is shared between skip and dest */
			break;
		case 5:
			length = data;
			break;
		case 6:
			loops = data;
			break;
	}

	if (offset == 6)	/* trigger blit */
	{
		int i,j;

#ifdef VERBOSE
//      logerror("%04x: blit src %04x mode %02x skip %d dest %04x length %d loops %d\n",
//          activecpu_get_pc(),src,mode,skip,dest,length,loops);
#endif

		/* Kludge: have to steal some cycles from the Z80 otherwise text
           scrolling in Gorf is too fast. */
		activecpu_adjust_icount(- 4 * (length+1) * (loops+1));

		for (i = 0; i <= loops;i++)
		{
			for (j = 0;j <= length;j++)
			{
				if (!(mode & 0x08) || j < length)
				{
					if (mode & 0x01)			/* Direction */
						program_write_byte(src,program_read_byte(dest));
					else
						if (dest >= 0) program_write_byte(dest,program_read_byte(src));	/* ASG 971005 */
				}
				/* close out writes in case of shift... I think this is wrong */
				else if (j == length)
					if (dest >= 0) program_write_byte(dest,0);

				if ((j & 1) || !(mode & 0x02))  /* Expand Mode - don't increment source on odd loops */
					if (mode & 0x04) src++;		/* Constant mode - don't increment at all! */

				if (mode & 0x20) dest++;		/* copy forwards */
				else dest--;					/* backwards */
			}

			if ((j & 1) && (mode & 0x02))		/* always increment source at end of line */
				if (mode & 0x04) src++;			/* Constant mode - don't increment at all! */

			if ((mode & 0x08) && (mode & 0x04)) /* Correct src if in flush mode */
				src--;                          /* and NOT in Constant mode */

			if (mode & 0x20) dest--;			/* copy forwards */
			else dest++;						/* backwards */

			dest += (int)((signed char)skip);	/* extend the sign of the skip register */

			/* Note: actually the hardware doesn't handle the sign of the skip register, */
			/* when incrementing the destination address the carry bit is taken from the */
			/* mode register. To faithfully emulate the hardware I should do: */
#if 0
			{
				int lo,hi;

				lo = dest & 0x00ff;
				hi = dest & 0xff00;
				lo += skip;
				if (mode & 0x10)
				{
					if (lo < 0x100) hi -= 0x100;
				}
				else
				{
					if (lo > 0xff) hi += 0x100;
				}
				dest = hi | (lo & 0xff);
			}
#endif
		}
	}
}


static void init_star_field(void)
{
	int generator;
	int count,x,y;

	generator = 0;

	/* this 17-bit shifter with XOR feedback has a period of 2^17-1 iterations */
	for (count = 0;count < RNG_PERIOD;count++)
	{
		int bit1,bit2;

		generator <<= 1;
		bit1 = (~generator >> 17) & 1;
		bit2 = (generator >> 5) & 1;

		if (bit1 ^ bit2) generator |= 1;

		rng[count] = generator & 0x1ffff;
	}


	/* calculate stars positions */
	count = 0;
	for (y = 0;y < MAX_LINES;y++)
	{
		for (x = -16;x < CLOCKS_PER_LINE-16;x++)	/* perfect values determined with screen shots */
		{
			if (x >= Machine->screen[0].visarea.min_x &&
				x <= Machine->screen[0].visarea.max_x &&
				y >= Machine->screen[0].visarea.min_y &&
				y <= Machine->screen[0].visarea.max_y)
			{
				if ((rng[count] & 0x1fe00) == 0x0fe00)
					star[x+SCREEN_WIDTH*y] = 1;
				else
					star[x+SCREEN_WIDTH*y] = 0;
			}

			count++;
		}
	}


	/* now convert the rng values to Y adjustments that will be used at runtime */
	for (count = 0;count < RNG_PERIOD;count++)
	{
		int r;

		r = rng[count];
		rng[count] = (((r >> 12) & 1) << 3) +
					 (((r >>  8) & 1) << 2) +
					 (((r >>  4) & 1) << 1) +
					 (((r >>  0) & 1) << 0);
	}
}


/* GORF Special Registers
 *
 * These are data writes, done by IN commands
 *
 * The data is placed on the upper bits 8-11 bits of the address bus (B)
 * and is used to drive 2 8 bit addressable latches to control :-
 *
 * IO 15
 *   0 coin counter
 *   1 coin counter
 *   2 Star enable (never written to)
 *   3 Sparkle 1
 *   4 Sparkle 2
 *   5 Sparkle 3
 *   6 Second Amp On/Off ?
 *   7 Drv7
 *
 * IO 16
 *   0 Space Cadet Lamp
 *   1 Space Captain Lamp
 *   2 Space Colonel Lamp
 *   3 Space General Lamp
 *   4 Space Warrior Lamp
 *   5 Space Avanger Lamp
 *   6
 *   7 ?
 *
 */

READ8_HANDLER( gorf_io_1_r )
{
	UINT8 data = offset >> 8;
	offset &= 0xff;

	offset = (offset << 3) + (data >> 1);
	data = ~data & 0x01;

	switch (offset)
	{
	case 0x00: coin_counter_w(0,data); break;
	case 0x01: coin_counter_w(1,data); break;
	case 0x02: sparkle[CurrentScan][0] = data; break;
	case 0x03: sparkle[CurrentScan][1] = data; break;
	case 0x04: sparkle[CurrentScan][2] = data; break;
	case 0x05: sparkle[CurrentScan][3] = data; break;
#ifdef VERBOSE
	default:
		logerror("%04x: Latch IO1 %02x set to %d\n",activecpu_get_pc(),offset,data);
#endif
	}

	return 0;
}

READ8_HANDLER( gorf_io_2_r )
{
	UINT8 data = offset >> 8;
	offset &= 0xff;

	offset = (offset << 3) + (data >> 1);
	data = ~data & 0x01;

	switch (offset)
	{
	case 0x00: output_set_value("lamp0", !data); break;
	case 0x01: output_set_value("lamp1", !data); break;
	case 0x02: output_set_value("lamp2", !data); break;
	case 0x03: output_set_value("lamp3", !data); break;
	case 0x04: output_set_value("lamp4", !data); break;
	case 0x05: output_set_value("lamp5", !data); break;
#ifdef VERBOSE
	default:
		logerror("%04x: Latch IO2 %02x set to %d\n",activecpu_get_pc(),offset,data);
#endif
	}

	return 0;
}

/* Wizard of Wor Special Registers
 *
 * These are data writes, done by IN commands
 *
 * The data is placed on the upper bits 8-11 bits of the address bus (A)
 * and is used to drive 1 8 bit addressable latches to control :-
 *
 * IO 15
 *   0 coin counter
 *   1 coin counter
 *   2 Star enable (never written to)
 *   3 Sparkle 1
 *   4 Sparkle 2
 *   5 Sparkle 3
 *   6 n.c.
 *   7 coin counter
 *
 */

READ8_HANDLER( wow_io_r )
{
	UINT8 data = offset >> 8;
	offset &= 0xff;

	offset = (offset << 3) + (data >> 1);
	data = ~data & 0x01;

	switch (offset)
	{
		case 0: coin_counter_w(0,data); break;
		case 1: coin_counter_w(1,data); break;
		case 2: sparkle[CurrentScan][0] = data; break;
		case 3: sparkle[CurrentScan][1] = data; break;
		case 4: sparkle[CurrentScan][2] = data; break;
		case 5: sparkle[CurrentScan][3] = data; break;
		case 7: coin_counter_w(2,data); break;
	}

#ifdef VERBOSE
	logerror("%04x: Latch IO %02x set to %d\n",activecpu_get_pc(),offset,data);
#endif

	return 0;
}

void astrocade_state_save_register(void)
{
	state_save_register_global(interrupt_enable);
	state_save_register_global(interrupt_vector);
	state_save_register_global(magic_expand_color);
	state_save_register_global(magic_control);
	state_save_register_global(magic_expand_count);
	state_save_register_global(magic_shift_leftover);
	state_save_register_global_2d_array(colors);
	state_save_register_global_array(colorsplit);
	state_save_register_global(BackgroundData);
	state_save_register_global(VerticalBlank);
	state_save_register_global_2d_array(sparkle);

	state_save_register_global(NextScanInt);
	state_save_register_global(CurrentScan);
	state_save_register_global(InterruptFlag);
	state_save_register_global(GorfDelay);

	state_save_register_global(src);
	state_save_register_global(mode);
	state_save_register_global(skip);
	state_save_register_global(dest);
	state_save_register_global(length);
	state_save_register_global(loops);
}

/****************************************************************************/

VIDEO_START( astrocde )
{
	astrocde_videoram_r = wow_videoram_r;

	rng = auto_malloc(RNG_PERIOD * sizeof(rng[0]));
	star = auto_malloc(SCREEN_WIDTH * MAX_LINES * sizeof(star[0]));

	memset(sparkle,0,sizeof(sparkle));
	CurrentScan = 0;

	astrocade_state_save_register();

	return 0;
}

VIDEO_START( astrocde_stars )
{
	int res;

	res = video_start_astrocde(machine);

	sparkle[0][0] = 1;	/* wow doesn't initialize this */
	init_star_field();

	return res;
}

/****************************************************************************/

void wow_update_line(mame_bitmap *bitmap,int line)
{
	/* Copy one line to bitmap, using current color register settings */

	int memloc;
	int i,x;
	int data,color;
	int rngoffs;
	UINT8 scanline[80*4+3];

	if (line >= MAX_LINES) return;

	rngoffs = MOD_U32_U64_U32( MUL_U64_U32_U32(
			cpu_getcurrentframe() % RNG_PERIOD, CLOCKS_PER_FRAME), RNG_PERIOD);

	memloc = line * 80;

	for (i = 0; i < 80; i++, memloc++)
	{
		if (line < VerticalBlank)
			data = astrocde_videoram_r(memloc);
		else
			data = BackgroundData;

		for (x = i*4+3; x >= i*4; x--)
		{
			int pen,scol;

			color = data & 0x03;
			if (i < colorsplit[line]) color += 4;

			if ((data & 0x03) == 0)
			{
				if (sparkle[line][0])
				{
					if (star[x+SCREEN_WIDTH*line])
					{
						scol = rng[(rngoffs + x+CLOCKS_PER_LINE*line) % RNG_PERIOD];
						pen = (colors[line][color]&~7) + scol/2;
					}
					else
						pen = 0;
				}
				else
					pen = colors[line][color];
			}
			else
			{
				if (sparkle[line][data & 0x03])
				{
					scol = rng[(rngoffs + x+CLOCKS_PER_LINE*line) % RNG_PERIOD];
					pen = (colors[line][color]&~7) + scol/2;
				}
				else
					pen = colors[line][color];
			}

			scanline[x] = pen;
			data >>= 2;
		}
	}

	draw_scanline8(bitmap, 0, line, 80*4+3, scanline, Machine->pens, -1);
}

VIDEO_START( profpac )
{
	profpac_videoram = auto_malloc(0x4000* 4 * sizeof(UINT16));

	astrocde_videoram_r = profpac_videoram_r;

	CurrentScan = 0;

	astrocade_state_save_register();

	state_save_register_global_array(profpac_color_mapping);
	state_save_register_global(profpac_read_page);
	state_save_register_global(profpac_write_page);
	state_save_register_global(profpac_visible_page);
	state_save_register_global(profpac_read_half);
	state_save_register_global(profpac_write_mode);
	state_save_register_global(profpac_intercept);
	state_save_register_global_pointer(profpac_videoram, 0x4000 * 4);
	state_save_register_global(profpac_vw);
	state_save_register_global(profpac_cw);

	return 0;
}

void profpac_update_line(mame_bitmap *bitmap,int line)
{
	int i,j;
	UINT8 scanline[80*4];

	if (line >= MAX_LINES) return;

	for(i=0;i<80;i++)
	{
		for(j=0;j<4;j++)
		{
			scanline[i*4+j] =
				profpac_videoram[profpac_visible_page*0x4000+i+line*80]>>((3-j)*4)&0x0f;
		}
	}

	draw_scanline8(bitmap, 0, line, 80*4, scanline, Machine->pens, -1);
}


VIDEO_UPDATE( astrocde )
{
	int i;

	for (i = cliprect->min_y;i <= cliprect->max_y;i++)
		wow_update_line(bitmap,i);
	return 0;
}

VIDEO_UPDATE( profpac )
{
	int i;

	for (i = cliprect->min_y;i <= cliprect->max_y;i++)
		profpac_update_line(bitmap,i);
	return 0;
}

VIDEO_UPDATE( seawolf2 )
{
	int centre;
	int player = program_read_byte(0xc1fb);


	video_update_astrocde(machine,screen,bitmap,cliprect);


	/* Draw a sight */

	if(player != 0)	/* Number of Players */
	{
		/* Player 1 */

		centre = 317 - ((input_port_0_r(0) & 0x3f)-18) * 10;

		if (centre<2)   centre=2;
		if (centre>317) centre=317;

//      draw_crosshair(bitmap,centre,35,&Machine->screen[0].visarea,0);

		/* Player 2 */

		if(player == 2)
		{
			centre = 316 - ((input_port_1_r(0) & 0x3f)-18) * 10;

			if (centre<1)   centre=1;
			if (centre>316) centre=316;

//          draw_crosshair(bitmap,centre,33,&Machine->screen[0].visarea,1);
		}
	}
	return 0;
}

READ8_HANDLER( robby_io_r )
{
	UINT8 data = (offset >> 8)&1;
	offset = (offset >> 9)&7;

	switch(offset)
	{
	case 0x00: coin_counter_w(0,data); break;
	case 0x01: coin_counter_w(1,data); break;
	case 0x02: coin_counter_w(2,data); break;
	case 0x06: set_led_status(0,data); break;
	case 0x07: set_led_status(1,data); break;
	}
	return 0;
}

READ8_HANDLER( profpac_io_1_r )
{
	if(offset & 0x0100) coin_counter_w(0,1); else coin_counter_w(0,0);
	if(offset & 0x0200) coin_counter_w(1,1); else coin_counter_w(1,0);
	if(offset & 0x0400) set_led_status(0,1); else set_led_status(0,0);
	if(offset & 0x0800) set_led_status(1,1); else set_led_status(1,0);
	return 0;
}

READ8_HANDLER( profpac_io_2_r )
{
	if(offset & 0x0100) output_set_value("left lamp A", 1); else output_set_value("left lamp A", 0);
	if(offset & 0x0200) output_set_value("left lamp B", 1); else output_set_value("left lamp B", 0);
	if(offset & 0x0400) output_set_value("left lamp C", 1); else output_set_value("left lamp C", 0);
	if(offset & 0x1000) output_set_value("right lamp A", 1); else output_set_value("right lamp A", 0);
	if(offset & 0x2000) output_set_value("right lamp B", 1); else output_set_value("right lamp B", 0);
	if(offset & 0x4000) output_set_value("right lamp C", 1); else output_set_value("right lamp C", 0);
	return 0;
}
