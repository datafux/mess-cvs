/************************************************************************
Philips P2000 1 Memory map

	CPU: Z80
		0000-0fff	ROM
		1000-4fff	ROM (appl)
		5000-57ff	RAM (Screen T ver)
		5000-5fff	RAM (Screen M ver)
		6000-9fff	RAM (system)
		a000-ffff	RAM (extension)

	Interrupts:

	Ports:
		00-09		Keyboard input
		10-1f		Output ports
		20-2f		Input ports
		30-3f		Scroll reg (T ver)
		50-5f		Beeper
		70-7f		DISAS (M ver)
		88-8B		CTC
		8C-90		Floppy ctrl
		94			RAM Bank select

	Display: SAA5050

************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/saa5050.h"
#include "includes/p2000t.h"

/* port i/o functions */

static ADDRESS_MAP_START( p2000t_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f) AM_READ( p2000t_port_000f_r)
	AM_RANGE(0x10, 0x1f) AM_WRITE( p2000t_port_101f_w)
	AM_RANGE(0x20, 0x2f) AM_READ( p2000t_port_202f_r)
	AM_RANGE(0x30, 0x3f) AM_WRITE( p2000t_port_303f_w)
	AM_RANGE(0x50, 0x5f) AM_WRITE( p2000t_port_505f_w)
	AM_RANGE(0x70, 0x7f) AM_WRITE( p2000t_port_707f_w)
	AM_RANGE(0x88, 0x8b) AM_WRITE( p2000t_port_888b_w)
	AM_RANGE(0x8c, 0x90) AM_WRITE( p2000t_port_8c90_w)
	AM_RANGE(0x94, 0x94) AM_WRITE( p2000t_port_9494_w)
ADDRESS_MAP_END

/* Memory w/r functions */

static ADDRESS_MAP_START( p2000t_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_READ( MRA8_ROM)
	AM_RANGE(0x1000, 0x4fff) AM_READ( MRA8_RAM)
	AM_RANGE(0x5000, 0x57ff) AM_READ( videoram_r)
	AM_RANGE(0x5800, 0x9fff) AM_READ( MRA8_RAM)
	AM_RANGE(0xa000, 0xffff) AM_READ( MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( p2000t_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_WRITE( MWA8_ROM)
	AM_RANGE(0x1000, 0x4fff) AM_WRITE( MWA8_RAM)
	AM_RANGE(0x5000, 0x57ff) AM_WRITE( videoram_w) AM_BASE( &videoram) AM_SIZE( &videoram_size)
	AM_RANGE(0x5800, 0x9fff) AM_WRITE( MWA8_RAM)
	AM_RANGE(0xa000, 0xffff) AM_WRITE( MWA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( p2000m_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_READ( MRA8_ROM)
	AM_RANGE(0x1000, 0x4fff) AM_READ( MRA8_RAM)
	AM_RANGE(0x5000, 0x5fff) AM_READ( videoram_r)
	AM_RANGE(0x6000, 0x9fff) AM_READ( MRA8_RAM)
	AM_RANGE(0xa000, 0xffff) AM_READ( MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( p2000m_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x0fff) AM_WRITE( MWA8_ROM)
	AM_RANGE(0x1000, 0x4fff) AM_WRITE( MWA8_RAM)
	AM_RANGE(0x5000, 0x5fff) AM_WRITE( videoram_w) AM_BASE( &videoram) AM_SIZE( &videoram_size)
	AM_RANGE(0x6000, 0x9fff) AM_WRITE( MWA8_RAM)
	AM_RANGE(0xa000, 0xffff) AM_WRITE( MWA8_NOP)
ADDRESS_MAP_END

/* graphics output */

static gfx_layout p2000m_charlayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8,
	  5*8, 6*8, 7*8, 8*8, 9*8 },
	8 * 10
};

static gfx_decode p2000m_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &p2000m_charlayout, 0, 128 },
	{ -1 }
};

static unsigned char p2000m_palette[2 * 3] =
{
	0x00, 0x00, 0x00,
	0xff, 0xff, 0xff
};

static	unsigned	short	p2000m_colortable[2 * 2] =
{
	1,0, 0,1
};

static PALETTE_INIT( p2000m )
{
	palette_set_colors(0, p2000m_palette, sizeof(p2000m_palette) / 3);
	memcpy(colortable, p2000m_colortable, sizeof (p2000m_colortable));
}

/* Keyboard input */

INPUT_PORTS_START (p2000t)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad .") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 00") PORT_CODE(KEYCODE_ENTER_PAD)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("#") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift Lock") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("<") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Code") PORT_CODE(KEYCODE_HOME)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clrln") PORT_CODE(KEYCODE_END)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad +") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad -") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_QUOTE)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("->") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Pad 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1/4") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_COLON)
	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LShift") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("deadkey")
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("deadkey")
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("deadkey")
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("deadkey")
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("deadkey")
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("deadkey")
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RShift") PORT_CODE(KEYCODE_RSHIFT)
INPUT_PORTS_END



static INTERRUPT_GEN( p2000_interrupt )
{
	cpunum_set_input_line(0, 0, PULSE_LINE);
}

/* Machine definition */
static MACHINE_DRIVER_START( p2000t )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 2500000)
	MDRV_CPU_PROGRAM_MAP(p2000t_readmem, p2000t_writemem)
	MDRV_CPU_IO_MAP(p2000t_io, 0)
	MDRV_CPU_VBLANK_INT(p2000_interrupt, 1)

    /* video hardware */
	MDRV_IMPORT_FROM( vh_saa5050 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


/* Machine definition */
static MACHINE_DRIVER_START( p2000m )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 2500000)
	MDRV_CPU_PROGRAM_MAP(p2000m_readmem, p2000m_writemem)
	MDRV_CPU_IO_MAP(p2000t_io, 0)
	MDRV_CPU_VBLANK_INT(p2000_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80 * 6, 24 * 10)
	MDRV_VISIBLE_AREA(0, 80 * 6 - 1, 0, 24 * 10 - 1)
	MDRV_GFXDECODE( p2000m_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT(p2000m)

	MDRV_VIDEO_START(p2000m)
	MDRV_VIDEO_UPDATE(p2000m)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


ROM_START(p2000t)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("p2000.rom", 0x0000, 0x1000, CRC(650784a3))
	ROM_LOAD("basic.rom", 0x1000, 0x4000, CRC(9d9d38f9))
	ROM_REGION(0x01000, REGION_GFX1,0)
	ROM_LOAD("p2000.chr", 0x0140, 0x08c0, BAD_DUMP CRC(78c17e3e))
ROM_END

ROM_START(p2000m)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("p2000.rom", 0x0000, 0x1000, CRC(650784a3))
	ROM_LOAD("basic.rom", 0x1000, 0x4000, CRC(9d9d38f9))
	ROM_REGION(0x01000, REGION_GFX1,0)
	ROM_LOAD("p2000.chr", 0x0140, 0x08c0, BAD_DUMP CRC(78c17e3e))
ROM_END

/*		YEAR	NAME		PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG  COMPANY		FULLNAME */
COMP (	1980,	p2000t,		0,		0,		p2000t,		p2000t,		0,		NULL,	"Philips",	"Philips P2000T" , 0)
COMP (	1980,	p2000m,		p2000t,	0,		p2000m,		p2000t,		0,		NULL,	"Philips",	"Philips P2000M" , 0)

