/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  * History *

  MJC - 01.02.98 - Line based dirty colour / dirty rectangle handling

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "includes/astrocde.h"

#include <math.h> /* for sin() and cos() */

unsigned char *astrocade_videoram;
int magic_expand_color, magic_control, magic_expand_flipflop, collision;

static int ColourSplit=0;								/* Colour System vars */
static int Colour[8] = {0,0,0,0,0,0,0,0};

static unsigned int LeftColourCheck=0x00000000;
static unsigned int RightColourCheck=0x00000000;
static unsigned int LeftLineColour[256];
static unsigned int RightLineColour[256];

static int VerticalBlank = 204;
static int BackgroundData = 0;
static int LastShifter = 0;
static int astrocade_mode = 0;

/* These are the bits of the Magic Register */
static const unsigned char SHIFT_MASK  = 0x03;
static const unsigned char ROTATE_MASK = 0x04;
static const unsigned char EXPAND_MASK = 0x08;
static const unsigned char OR_MASK     = 0x10;
static const unsigned char XOR_MASK    = 0x20;
static const unsigned char FLOP_MASK   = 0x40;

/* ======================================================================= */

enum { FAKE_BLK,FAKE_YLW,FAKE_BLU,FAKE_RED,FAKE_WHT };

static unsigned short fake_colortable[] =
{
	FAKE_BLK,FAKE_YLW,FAKE_BLU,FAKE_RED,
	FAKE_BLK,FAKE_WHT,FAKE_BLK,FAKE_RED   /* not used by the game, here only for the dip switch menu */
};

PALETTE_INIT( astrocade )
{
	/* This routine builds a palette using a transformation from */
	/* the YUV (Y, B-Y, R-Y) to the RGB color space */

	/* It also returns a fake colortable, for the menus */

	int i,j;

	float Y, RY, BY;	/* Y, R-Y, and B-Y signals as generated by the game */
						/* Y = Luminance -> (0 to 1) */
						/* R-Y = Color along R-Y axis -> C*(-1 to +1) */
						/* B-Y = Color along B-Y axis -> C*(-1 to +1) */
	float R, G, B;

	float brightest = 1.0;	/* Approx. Luminance values for the extremes -> (0 to 1) */
	float dimmest   = 0.0;
	float C = 0.75;		    /* Approx. Chroma intensity */

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
			BY = C*cos(i*2.0*M_PI/32.0);

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
			palette_set_color(i*8+j, floor(R+.5), floor(G+.5),  floor(B+.5));
		}
	}

	/* Set the fake colortable for the dip switch menus, etc. */

	colortable = fake_colortable;
}

WRITE8_HANDLER ( astrocade_vertical_blank_w )
{
	VerticalBlank = data;
}

 READ8_HANDLER ( astrocade_intercept_r )
{
	int res;

	res = collision;
	collision = 0;

	return res;
}


 READ8_HANDLER ( astrocade_video_retrace_r )
{
	extern int CurrentScan;

    return CurrentScan;
}

/* Switches colour registers at this zone - 40 zones */
/* Also sets the background colors */

WRITE8_HANDLER ( astrocade_colour_split_w )
{
	ColourSplit = data&0x3f;

	if (astrocade_mode == 1)
		ColourSplit <<= 1;

	BackgroundData = ((data&0xc0) >> 6) * 0x55;

#ifdef MAME_DEBUG
    logerror("Colour split set to %02d\n",ColourSplit);
#endif
}

/* This selects commercial (high res, arcade) or
                  consumer (low res, astrocade) mode */

WRITE8_HANDLER ( astrocade_mode_w )
{
	astrocade_mode = data & 0x01;
}

WRITE8_HANDLER ( astrocade_colour_register_w )
{
	if(Colour[offset] != data)
    {
		Colour[offset] = data;

        if(offset>3)
			LeftColourCheck = (Colour[4] << 24) | (Colour[5] << 16) | (Colour[6] < 8) | Colour[7];
        else
        	RightColourCheck = (Colour[0] << 24) | (Colour[1] << 16) | (Colour[2] < 8) | Colour[3];
	}

#ifdef MAME_DEBUG
    logerror("Colour %01x set to %02x\n",offset,data);
#endif
}

WRITE8_HANDLER ( astrocade_colour_block_w )
{
	static int color_reg_num = 7;

	Colour[color_reg_num] = data;

#ifdef MAME_DEBUG
    logerror("Colour block write: color %x set to %x\n", color_reg_num,data);
#endif

	if (color_reg_num == 0)
		color_reg_num = 7;
	else
		color_reg_num--;

	LeftColourCheck = (Colour[4] << 24) | (Colour[5] << 16) | (Colour[6] < 8) | Colour[7];
    RightColourCheck = (Colour[0] << 24) | (Colour[1] << 16) | (Colour[2] < 8) | Colour[3];

}

WRITE8_HANDLER ( astrocade_videoram_w )
{
	if ((offset < 0x1000) && (astrocade_videoram[offset] != data))
	{
		astrocade_videoram[offset] = data;
    }
}


WRITE8_HANDLER ( astrocade_magic_expand_color_w )
{
#ifdef MAME_DEBUG
//	logerror("%04x: magic_expand_color = %02x\n",cpu_getpc(),data);
#endif

	magic_expand_color = data;
}


WRITE8_HANDLER ( astrocade_magic_control_w )
{
#ifdef MAME_DEBUG
//	logerror("%04x: magic_control = %02x\n",cpu_getpc(),data);
#endif
	magic_expand_flipflop = 0;	/* initialize the expand nibble */
	LastShifter = 0;			/* clear the shifter */

	magic_control = data;
}

WRITE8_HANDLER ( astrocade_magicram_w )
{
	unsigned int data1,shift,bits,bibits,stib,k,old_data;

#ifdef MAME_DEBUG
//	logerror("%04x: magicram_w(%04x) = %02x, magic_register = %02x\n",cpu_getpc(),offset,data,magic_control);
#endif

	if (magic_control & EXPAND_MASK)	/* expand mode */
	{
		bits = data;

		/* if flip-flop set, expand lower half */
		/* otherwise do upper half */

		if (magic_expand_flipflop)
			bits <<= 4;

		/* now what we want to expand is in bits 4-7 */

		bibits = 0;
		for (k = 0;k < 4;k++)
		{
			bibits <<= 2;
			if (bits & 0x80)
				bibits |= (magic_expand_color >> 2) & 0x03;
			else
				bibits |= magic_expand_color & 0x03;
			bits <<= 1;
		}

		data = bibits;
	}

	/* rotating or shifting */

	data1 = 0;

	if (magic_control & ROTATE_MASK)
	{
		/* Rotate not implemented */
		/* (Only functional in commercial mode) */
	}
	else
	{
		shift = (magic_control & SHIFT_MASK);

		if (shift)
		{
			while (shift > 0)
			{
				data1 >>= 2;
				data1 |= (data & 0x03) << 6;

				data >>= 2;

				shift--;
			}

			data |= LastShifter;
			LastShifter = data1;

		}
	}

	/* flopping */

	if (magic_control & FLOP_MASK)	/* copy backwards */
	{
		bits = data;
		stib = 0;
		for (k = 0;k < 4;k++)
		{
			stib >>= 2;
			stib |= (bits & 0xc0);
			bits <<= 2;
		}

		data = stib;

		bits = data1;
		stib = 0;
		for (k = 0;k < 4;k++)
		{
			stib >>= 2;
			stib |= (bits & 0xc0);
			bits <<= 2;
		}

		data1 = stib;
	}

	/* OR or XOR */

	if (magic_control & 0x30)
	{
		old_data = astrocade_videoram[offset];
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

	if (magic_control & XOR_MASK)
		data ^= astrocade_videoram[offset];	    /* draw in XOR mode */
	else if (magic_control & OR_MASK)
		data |= astrocade_videoram[offset];		/* draw in OR mode */

	/* else draw in copy mode */

	astrocade_videoram_w(offset,data);

	magic_expand_flipflop ^= 1;
}

void astrocade_copy_line(int line)
{
	/* Copy one line to bitmap, using current colour register settings */

    int memloc;
    int i,x,num_bytes;
    int data,color;

	if (astrocade_mode == 0)
	{
		memloc = line/2 * 40;
		num_bytes = 40;
	}
	else
	{
		num_bytes = 80;
		memloc = line * 80;
	}

	LeftLineColour[line]  = LeftColourCheck;
	RightLineColour[line]  = RightColourCheck;

	for(i=0;i<num_bytes;i++,memloc++)
	{
		if (line < VerticalBlank)
			data = astrocade_videoram[memloc];
		else
			data = BackgroundData;

		for(x=i*4+3;x>=i*4;x--)
		{
			color = data & 03;

			if (i<ColourSplit)
				color += 4;

			if (astrocade_mode == 0)
			{
				plot_pixel(tmpbitmap,2*x,line,Machine->pens[Colour[color]]);
				plot_pixel(tmpbitmap,2*x+1,line,Machine->pens[Colour[color]]);
			}
			else
				plot_pixel(tmpbitmap,x,line,Machine->pens[Colour[color]]);

			data >>= 2;
		}

	}
}

