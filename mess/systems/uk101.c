/**********************************************************************
UK101 Memory map

	CPU: 6502 @ 1.0Mhz

Interrupts:	None.

Video:		Memory mapped

Sound:		None

Hardware:	MC6850

		0000-0FFF	RAM (standard)
		1000-1FFF	RAM (expanded)
		2000-9FFF	RAM	(emulator only)
		A000-BFFF	ROM (basic)
		C000-CFFF	NOP
		D000-D3FF	RAM (video)
		D400-DEFF	NOP
		DF00-DF00	H/W (Keyboard)
		DF01-EFFF	NOP
		F000-F001	H/W (MC6850)
		F002-F7FF	NOP
		F800-FFFF	ROM (monitor)
**********************************************************************/
#include "driver.h"
#include "inputx.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"
#include "machine/mc6850.h"
#include "includes/uk101.h"

/* memory w/r functions */

ADDRESS_MAP_START( uk101_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_READWRITE( MRA8_BANK1, MWA8_BANK1 )
	AM_RANGE(0x1000, 0x1fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK2 )
	AM_RANGE(0x2000, 0x9fff) AM_READWRITE( MRA8_BANK3, MWA8_BANK3 )
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xcfff) AM_NOP
	AM_RANGE(0xd000, 0xd3ff) AM_READWRITE( videoram_r, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xd400, 0xdeff) AM_NOP
	AM_RANGE(0xdf00, 0xdf00) AM_READWRITE( uk101_keyb_r, uk101_keyb_w )
	AM_RANGE(0xdf01, 0xefff) AM_NOP
	AM_RANGE(0xf000, 0xf001) AM_READWRITE( acia6850_0_r, acia6850_0_w )
	AM_RANGE(0xf002, 0xf7ff) AM_NOP
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END



/*
 Relaxed decoding as implemented in the Superboard II

		0000-0FFF	RAM (standard)
		1000-1FFF	RAM (expanded)
		2000-9FFF	RAM	(emulator only)
		A000-BFFF	ROM (basic)
		C000-CFFF	NOP
		D000-D3FF	RAM (video)
		D400-DBFF	NOP
		DC00-DFFF	H/W (Keyboard)
		E000-EFFF	NOP
		F000-F0FF	H/W (MC6850)
		F100-F7FF	NOP
		F800-FFFF	ROM (monitor)
*/
ADDRESS_MAP_START( superbrd_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_READWRITE( MRA8_BANK1, MWA8_BANK1 )
	AM_RANGE(0x1000, 0x1fff) AM_READWRITE( MRA8_BANK2, MWA8_BANK2 )
	AM_RANGE(0x2000, 0x9fff) AM_READWRITE( MRA8_BANK3, MWA8_BANK3 )
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xcfff) AM_NOP
	AM_RANGE(0xd000, 0xd7ff) AM_READWRITE( videoram_r, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xd800, 0xdbff) AM_NOP
	AM_RANGE(0xdc00, 0xdfff) AM_READWRITE( uk101_keyb_r, superbrd_keyb_w )
	AM_RANGE(0xe000, 0xefff) AM_NOP
	AM_RANGE(0xf000, 0xf0ff) AM_READWRITE( acia6850_0_r, acia6850_0_w )
	AM_RANGE(0xf100, 0xf7ff) AM_NOP
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END


/* graphics output */

struct GfxLayout uk101_charlayout =
{
	8, 16,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 0*8, 1*8, 1*8, 2*8, 2*8, 3*8, 3*8,
	  4*8, 4*8, 5*8, 5*8, 6*8, 6*8, 7*8, 7*8 },
	8 * 8
};

struct GfxLayout superbrd_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static struct	GfxDecodeInfo uk101_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &uk101_charlayout, 0, 1},
	{-1}
};

static struct	GfxDecodeInfo superbrd_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &superbrd_charlayout, 0, 1},
	{-1}
};

static unsigned char uk101_palette[2 * 3] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0xff, 0xff	/* White */
};

static unsigned short uk101_colortable[] =
{
	0,1
};

static PALETTE_INIT( uk101 )
{
	palette_set_colors(0, uk101_palette, sizeof(uk101_palette) / 3);
	memcpy(colortable, uk101_colortable, sizeof (uk101_colortable));
}

/* keyboard input */
INPUT_PORTS_START( uk101 )
	PORT_START	/* 0: DF00 & 0x80 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE )
	PORT_START /* 1: DF00 & 0x40 */
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Backspace", KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_COLON, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE )
	PORT_START /* 2: DF00 & 0x20 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Enter", KEYCODE_ENTER, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE )
	PORT_START /* 3: DF00 & 0x10 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE )
	PORT_START /* 4: DF00 & 0x08 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE )
	PORT_START /* 5: DF00 & 0x04 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE )
	PORT_START /* 6: DF00 & 0x02 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "'", KEYCODE_QUOTE, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE )
	PORT_START /* 7: DF00 & 0x01 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD | IPF_TOGGLE, "Caps Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Right Shift", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Left Shift", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BIT( 0x18, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Escape", KEYCODE_ESC, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Control", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Control", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "~", KEYCODE_TILDE, IP_JOY_NONE )
INPUT_PORTS_END

/* Superboard keyboard

Matrix (see http://www.technology.niagarac.on.ca/people/mcsele/images/OSI600SchematicsPg11-12.jpg)

    C7 C6 C5 C4 C3 C2 C1 C0
R7   1  2  3  4  5  6  7
R6   8  9  0  :  - Ro
R5   .  L  O Lf Cr
R4   W  E  R  T  Y  U  I
R3   S  D  F  G  H  J  K
R2   X  C  V  B  N  M  ,
R1   Q  A  Z Sp  /  ;  P
R0  Rp Ct Ec       Ls Rs Sl  

Cr = RETURN, Ct = CTRL, Ec = ESC, Lf= LINE FEED, Ls = left SHIFT, Ro = RUB OUT
Rp = REPEAT, Rs = right SHIFT, Sl = SHIFT LOCK, Sp = space bar 

Layout (see http://www.technology.niagarac.on.ca/people/mcsele/images/OSISuperboardII.jpg)

! " # $ % & ' ( )   * = RUB
1 2 3 4 5 6 7 8 9 0 : - OUT

                        LINE
ESC Q W E R T Y U I O P FEED RETURN

                       + SHIFT
CTRL A S D F G H J K L ; LOCK  REPEAT BREAK

                    < > ?
SHIFT Z X C V B N M , . / SHIFT

     ---SPACE BAR---
*/
INPUT_PORTS_START( superbrd )
	PORT_START	/* 0: DF00 & 0x80 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_KEY2( 0x02, IP_ACTIVE_LOW, "7  '", KEYCODE_7, CODE_NONE,	'7',	'\'')	/* 7 ' */
	PORT_KEY2( 0x04, IP_ACTIVE_LOW, "6  &", KEYCODE_6, CODE_NONE,	'6',	'&')	/* 6 & */
	PORT_KEY2( 0x08, IP_ACTIVE_LOW, "5  %", KEYCODE_5, CODE_NONE,	'5',	'%')	/* 5 % */
	PORT_KEY2( 0x10, IP_ACTIVE_LOW, "4  $", KEYCODE_4, CODE_NONE,	'4',	'$')	/* 4 $ */
	PORT_KEY2( 0x20, IP_ACTIVE_LOW, "3  #", KEYCODE_3, CODE_NONE,	'3',	'#')	/* 3 # */
	PORT_KEY2( 0x40, IP_ACTIVE_LOW, "2  \"", KEYCODE_2, CODE_NONE,	'2',	'\"')	/* 2 " */
	PORT_KEY2( 0x80, IP_ACTIVE_LOW, "1  !", KEYCODE_1, CODE_NONE,	'1',	'!')	/* 1 ! */
	PORT_START /* 1: DF00 & 0x40 */
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_KEY1( 0x04, IP_ACTIVE_LOW, "RUBOUT", KEYCODE_BACKSPACE, CODE_NONE,	8)	/* RUBOUT */
	PORT_KEY2( 0x08, IP_ACTIVE_LOW, "-  =", KEYCODE_EQUALS, CODE_NONE,		'-',	'=') /* - = */
	PORT_KEY2( 0x10, IP_ACTIVE_LOW, ":  *", KEYCODE_MINUS, CODE_NONE,		':',	'*') /* : * */
	PORT_KEY1( 0x20, IP_ACTIVE_LOW, "0   ", KEYCODE_0, CODE_NONE,			'0')
	PORT_KEY2( 0x40, IP_ACTIVE_LOW, "9  )", KEYCODE_9, CODE_NONE,			'9',	')')	/* 9 ) */
	PORT_KEY2( 0x80, IP_ACTIVE_LOW, "8  (", KEYCODE_8, CODE_NONE,			'8',	'(')	/* 8 ( */
	PORT_START /* 2: DF00 & 0x20 */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_KEY1( 0x08, IP_ACTIVE_LOW, "ENTER",		KEYCODE_ENTER,		CODE_NONE,		13)
	PORT_KEY1( 0x10, IP_ACTIVE_LOW, "LINE FEED", KEYCODE_OPENBRACE, CODE_NONE, 10) /* LINE FEED */
	PORT_KEY1( 0x20, IP_ACTIVE_LOW, "O", KEYCODE_O, CODE_NONE,				'O')
	PORT_KEY1( 0x40, IP_ACTIVE_LOW, "L", KEYCODE_L, CODE_NONE,				'L')
	PORT_KEY2( 0x80, IP_ACTIVE_LOW, ".  >", KEYCODE_STOP, CODE_NONE,			'.',	'>') /* . > */
	PORT_START /* 3: DF00 & 0x10 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_KEY1( 0x02, IP_ACTIVE_LOW, "I", KEYCODE_I, CODE_NONE,				'I')
	PORT_KEY1( 0x04, IP_ACTIVE_LOW, "U", KEYCODE_U, CODE_NONE,				'U')
	PORT_KEY1( 0x08, IP_ACTIVE_LOW, "Y", KEYCODE_Y, CODE_NONE,				'Y')
	PORT_KEY1( 0x10, IP_ACTIVE_LOW, "T", KEYCODE_T, CODE_NONE,				'T')
	PORT_KEY1( 0x20, IP_ACTIVE_LOW, "R", KEYCODE_R, CODE_NONE,				'R')
	PORT_KEY1( 0x40, IP_ACTIVE_LOW, "E", KEYCODE_E, CODE_NONE,				'E')
	PORT_KEY1( 0x80, IP_ACTIVE_LOW, "W", KEYCODE_W, CODE_NONE,				'W')
	PORT_START /* 4: DF00 & 0x08 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_KEY1( 0x02, IP_ACTIVE_LOW, "K", KEYCODE_K, CODE_NONE,				'K')
	PORT_KEY1( 0x04, IP_ACTIVE_LOW, "J", KEYCODE_J, CODE_NONE,				'J')
	PORT_KEY1( 0x08, IP_ACTIVE_LOW, "H", KEYCODE_H, CODE_NONE,				'H')
	PORT_KEY1( 0x10, IP_ACTIVE_LOW, "G", KEYCODE_G, CODE_NONE,				'G')
	PORT_KEY1( 0x20, IP_ACTIVE_LOW, "F", KEYCODE_F, CODE_NONE,				'F')
	PORT_KEY1( 0x40, IP_ACTIVE_LOW, "D", KEYCODE_D, CODE_NONE,				'D')
	PORT_KEY1( 0x80, IP_ACTIVE_LOW, "S", KEYCODE_S, CODE_NONE,				'S')
	PORT_START /* 5: DF00 & 0x04 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_KEY2( 0x02, IP_ACTIVE_LOW, ",  <", KEYCODE_COMMA, CODE_NONE,		',',	'<') /* , < */
	PORT_KEY1( 0x04, IP_ACTIVE_LOW, "M", KEYCODE_M, CODE_NONE,				'M')
	PORT_KEY1( 0x08, IP_ACTIVE_LOW, "N", KEYCODE_N, CODE_NONE,				'N')
	PORT_KEY1( 0x10, IP_ACTIVE_LOW, "B", KEYCODE_B, CODE_NONE,				'B')
	PORT_KEY1( 0x20, IP_ACTIVE_LOW, "V", KEYCODE_V, CODE_NONE,				'V')
	PORT_KEY1( 0x40, IP_ACTIVE_LOW, "C", KEYCODE_C, CODE_NONE,				'C')
	PORT_KEY1( 0x80, IP_ACTIVE_LOW, "X", KEYCODE_X, CODE_NONE,				'X')
	PORT_START /* 6: DF00 & 0x02 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_KEY1( 0x02, IP_ACTIVE_LOW, "P", KEYCODE_P, CODE_NONE,				'P')
	PORT_KEY2( 0x04, IP_ACTIVE_LOW, ";  +", KEYCODE_COLON, CODE_NONE,		';',	'+') /* ; + */
	PORT_KEY2( 0x08, IP_ACTIVE_LOW, "/  ?", KEYCODE_SLASH, CODE_NONE,		'/',	'?') /* / ? */
	PORT_KEY1( 0x10, IP_ACTIVE_LOW, "SPACE", KEYCODE_SPACE, CODE_NONE,		' ')
	PORT_KEY1( 0x20, IP_ACTIVE_LOW, "Z", KEYCODE_Z, CODE_NONE,				'Z')
	PORT_KEY1( 0x40, IP_ACTIVE_LOW, "A", KEYCODE_A, CODE_NONE,				'A')
	PORT_KEY1( 0x80, IP_ACTIVE_LOW, "Q", KEYCODE_Q, CODE_NONE,				'Q')
	PORT_START /* 7: DF00 & 0x01 */
	/* PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD | IPF_TOGGLE, "Caps Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE ) */
	/* PORT_KEY1( 0x01, IP_ACTIVE_LOW, "SHIFT LOCK", KEYCODE_CAPSLOCK, CODE_NONE,		UCHAR_MAMEKEY(CAPSLOCK)) */
	PORT_BIT_NAME(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD | IPF_TOGGLE, "SHIFT LOCK")	
	PORT_CODE(KEYCODE_QUOTE, CODE_NONE)
	PORT_UCHAR(UCHAR_MAMEKEY(CAPSLOCK))	
	PORT_KEY1( 0x02, IP_ACTIVE_LOW, "R-SHIFT",	KEYCODE_RSHIFT,		CODE_NONE,	UCHAR_SHIFT_1)
	PORT_KEY1( 0x04, IP_ACTIVE_LOW, "L-SHIFT",	KEYCODE_LSHIFT,		CODE_NONE,	UCHAR_SHIFT_1)
	PORT_BIT( 0x18, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_KEY1( 0x20, IP_ACTIVE_LOW, "ESC",		KEYCODE_TAB,		CODE_NONE,	27)
	PORT_KEY1( 0x40, IP_ACTIVE_LOW, "CTRL",		KEYCODE_CAPSLOCK,	CODE_NONE,	UCHAR_SHIFT_2)
	PORT_KEY1( 0x80, IP_ACTIVE_LOW, "REPEAT",	KEYCODE_BACKSLASH, 		CODE_NONE,	'\\') /* REPEAT */
INPUT_PORTS_END

static INTERRUPT_GEN( uk101_interrupt )
{
	cpu_set_irq_line(0, 0, PULSE_LINE);
}

/* machine definition */
static MACHINE_DRIVER_START( uk101 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 1000000)
	MDRV_CPU_PROGRAM_MAP( uk101_mem, 0 )
	MDRV_CPU_VBLANK_INT(uk101_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32 * 8, 25 * 16)
	MDRV_VISIBLE_AREA(0, 32 * 8 - 1, 0, 25 * 16 - 1)
	MDRV_GFXDECODE( uk101_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(2)
	MDRV_PALETTE_INIT( uk101 )

	MDRV_VIDEO_START( generic )
	MDRV_VIDEO_UPDATE( uk101 )
MACHINE_DRIVER_END

static struct DACinterface superbrd_DAC_interface =
{
    1,          /* number of DACs */
    { 100 }     /* volume */
};

static MACHINE_DRIVER_START( superbrd )
	MDRV_IMPORT_FROM( uk101 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_GFXDECODE( superbrd_gfxdecodeinfo )
	MDRV_CPU_PROGRAM_MAP( superbrd_mem, 0 )
	MDRV_SCREEN_SIZE(32 * 8, 32 * 8)
	MDRV_VISIBLE_AREA(0, 32 * 8 - 1, 0, 32 * 8 - 1)
	MDRV_VIDEO_UPDATE( superbrd )
	MDRV_INTERLEAVE(0)
	/* sound hardware */
	MDRV_SOUND_ADD(DAC, superbrd_DAC_interface)
MACHINE_DRIVER_END

ROM_START(uk101)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("basuk01.rom", 0xa000, 0x0800, CRC(9d3caa92))
	ROM_LOAD("basuk02.rom", 0xa800, 0x0800, CRC(0039ef6a))
	ROM_LOAD("basuk03.rom", 0xb000, 0x0800, CRC(0d011242))
	ROM_LOAD("basuk04.rom", 0xb800, 0x0800, CRC(667223e8))
	ROM_LOAD("monuk02.rom", 0xf800, 0x0800, CRC(04ac5822))
	ROM_REGION(0x800, REGION_GFX1,0)
	ROM_LOAD("chguk101.rom", 0x0000, 0x0800, CRC(fce2c84a))
ROM_END

ROM_START(superbrd)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("basus01.rom", 0xa000, 0x0800, CRC(f4f5dec0))
	ROM_LOAD("basuk02.rom", 0xa800, 0x0800, CRC(0039ef6a))
	ROM_LOAD("basus03.rom", 0xb000, 0x0800, CRC(ca25f8c1))
	ROM_LOAD("basus04.rom", 0xb800, 0x0800, CRC(8ee6030e))
	ROM_LOAD("monde01.rom", 0xf800, 0x0800, CRC(95a44d2e))
	ROM_REGION(0x800, REGION_GFX1,0)
	ROM_LOAD("chgsup2.rom", 0x0000, 0x0800, CRC(735f5e0a))
ROM_END

SYSTEM_CONFIG_START(uk101)
	CONFIG_DEVICE_LEGACY(IO_CASSETTE, 1, "bas\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, NULL, NULL, device_load_uk101_cassette, device_unload_uk101_cassette, NULL)
	CONFIG_RAM_DEFAULT(4 * 1024)
	CONFIG_RAM(8 * 1024)
	CONFIG_RAM(40 * 1024)
SYSTEM_CONFIG_END

/*    YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT	INIT	CONFIG  COMPANY				FULLNAME */
COMP( 1979,	uk101,		0,		0,		uk101,		uk101,	uk101,	uk101,	"Compukit",			"UK101" )
COMP( 1979, superbrd,	uk101,	0,		superbrd,	superbrd,	uk101,	uk101,	"Ohio Scientific",	"Superboard II" )

