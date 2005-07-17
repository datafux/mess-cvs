/***************************************************************************

	systems/apple3.c

	Apple ///

	Driver is not working yet; seems to get caught in an infinite loop on
	startup.  Special thanks to Chris Smolinski (author of the Sara emulator)
	for his input about this poorly known system.

***************************************************************************/

#include "driver.h"
#include "includes/apple3.h"
#include "includes/apple2.h"
#include "devices/mflopimg.h"
#include "formats/ap2_dsk.h"
#include "machine/6522via.h"
#include "machine/appldriv.h"
#include "inputx.h"


static ADDRESS_MAP_START( apple3_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x00FF) AM_READWRITE(apple3_00xx_r, apple3_00xx_w)
	AM_RANGE(0x0100, 0x01FF) AM_RAMBANK(2)
	AM_RANGE(0x0200, 0x1FFF) AM_RAMBANK(3)
	AM_RANGE(0x2000, 0x9FFF) AM_RAMBANK(4)
	AM_RANGE(0xA000, 0xBFFF) AM_RAMBANK(5)
ADDRESS_MAP_END



extern PALETTE_INIT( apple2 );

static MACHINE_DRIVER_START( apple3 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 2000000)        /* 2 Mhz */
	MDRV_CPU_PROGRAM_MAP(apple3_map, 0)
	MDRV_CPU_PERIODIC_INT(apple3_interrupt, TIME_IN_HZ(192))
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( apple3 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(280*2, 192)
	MDRV_VISIBLE_AREA(0, (280*2)-1,0,192-1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(512)
	MDRV_PALETTE_INIT( apple2 )

	MDRV_VIDEO_START( apple3 )
	MDRV_VIDEO_UPDATE( apple3 )
MACHINE_DRIVER_END


static INPUT_PORTS_START( apple3 )
    PORT_START_TAG("keyb_0")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Delete")	PORT_CODE(KEYCODE_BACKSPACE)PORT_CHAR(8)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\x1b")		PORT_CODE(KEYCODE_LEFT)		PORT_CHAR(UCHAR_MAMEKEY(LEFT))
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab")		PORT_CODE(KEYCODE_TAB)		PORT_CHAR(9)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\x19")		PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(UCHAR_MAMEKEY(DOWN))
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\x18")		PORT_CODE(KEYCODE_UP)		PORT_CHAR(UCHAR_MAMEKEY(UP))
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return")	PORT_CODE(KEYCODE_ENTER)	PORT_CHAR(13)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\x1a")		PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Esc")		PORT_CODE(KEYCODE_ESC)		PORT_CHAR(27)

    PORT_START_TAG("keyb_1")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)	PORT_CHAR(' ')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)	PORT_CHAR('\'') PORT_CHAR('\"')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)	PORT_CHAR(',') PORT_CHAR('<')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('-') PORT_CHAR('_')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)	PORT_CHAR('.') PORT_CHAR('>')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)	PORT_CHAR('/') PORT_CHAR('?')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)		PORT_CHAR('0') PORT_CHAR(')')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)		PORT_CHAR('1') PORT_CHAR('!')

    PORT_START_TAG("keyb_2")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)	PORT_CHAR('2') PORT_CHAR('@')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)	PORT_CHAR('3') PORT_CHAR('#')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)	PORT_CHAR('4') PORT_CHAR('$')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)	PORT_CHAR('5') PORT_CHAR('%')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)	PORT_CHAR('6') PORT_CHAR('^')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)	PORT_CHAR('7') PORT_CHAR('&')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)	PORT_CHAR('8') PORT_CHAR('*')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)	PORT_CHAR('9') PORT_CHAR('(')

    PORT_START_TAG("keyb_3")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR(':')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=') PORT_CHAR('+')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[') PORT_CHAR('{')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\\') PORT_CHAR('|')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)		PORT_CHAR('`') PORT_CHAR('~')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('A') PORT_CHAR('a')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('B') PORT_CHAR('b')

    PORT_START_TAG("keyb_4")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)	PORT_CHAR('C') PORT_CHAR('c')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)	PORT_CHAR('D') PORT_CHAR('d')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)	PORT_CHAR('E') PORT_CHAR('e')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)	PORT_CHAR('F') PORT_CHAR('f')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)	PORT_CHAR('G') PORT_CHAR('g')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)	PORT_CHAR('H') PORT_CHAR('h')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)	PORT_CHAR('I') PORT_CHAR('i')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)	PORT_CHAR('J') PORT_CHAR('j')

    PORT_START_TAG("keyb_5")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)	PORT_CHAR('K') PORT_CHAR('k')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)	PORT_CHAR('L') PORT_CHAR('l')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)	PORT_CHAR('M') PORT_CHAR('m')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)	PORT_CHAR('N') PORT_CHAR('n')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)	PORT_CHAR('O') PORT_CHAR('o')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)	PORT_CHAR('P') PORT_CHAR('p')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)	PORT_CHAR('Q') PORT_CHAR('q')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)	PORT_CHAR('R') PORT_CHAR('r')

    PORT_START_TAG("keyb_6")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 	PORT_CHAR('S') PORT_CHAR('s')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 	PORT_CHAR('T') PORT_CHAR('t')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 	PORT_CHAR('U') PORT_CHAR('u')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 	PORT_CHAR('V') PORT_CHAR('v')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 	PORT_CHAR('W') PORT_CHAR('w')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 	PORT_CHAR('X') PORT_CHAR('x')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 	PORT_CHAR('Y') PORT_CHAR('y')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)	PORT_CHAR('Z') PORT_CHAR('z')

    PORT_START_TAG("keyb_special")
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Shift")	PORT_CODE(KEYCODE_LSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Shift")	PORT_CODE(KEYCODE_RSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Control")		PORT_CODE(KEYCODE_LCONTROL)	PORT_CHAR(UCHAR_SHIFT_2)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)  PORT_PLAYER(1)			PORT_CODE(KEYCODE_0_PAD)	PORT_CODE(JOYCODE_1_BUTTON1)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2)  PORT_PLAYER(1)			PORT_CODE(KEYCODE_ENTER_PAD)PORT_CODE(JOYCODE_1_BUTTON2)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1)  PORT_PLAYER(2)			PORT_CODE(JOYCODE_2_BUTTON1)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RESET")		PORT_CODE(KEYCODE_F12)
INPUT_PORTS_END

ROM_START(apple3)
    ROM_REGION(0x1000,REGION_CPU1,0)
	ROM_LOAD( "apple3.rom", 0x0000, 0x1000, CRC(55e8eec9) SHA1(579ee4cd2b208d62915a0aa482ddc2744ff5e967))
ROM_END

static const char *apple2_floppy_getname(const struct IODevice *dev, int id, char *buf, size_t bufsize)
{
	snprintf(buf, bufsize, "Slot 6 Disk #%d", id + 1);
	return buf;
}

static void apple3_floppy_getinfo(struct IODevice *dev)
{
	/* floppy */
	apple525_device_getinfo(dev, 1, 4);
	dev->count = 4;
	dev->name = apple2_floppy_getname;
}

SYSTEM_CONFIG_START(apple3)
	CONFIG_RAM_DEFAULT(0x80000)
	CONFIG_DEVICE(apple3_floppy_getinfo)
	CONFIG_QUEUE_CHARS( AY3600 )
	CONFIG_ACCEPT_CHAR( AY3600 )
SYSTEM_CONFIG_END

/*     YEAR		NAME		PARENT	COMPAT	MACHINE    INPUT	INIT    CONFIG	COMPANY				FULLNAME */
COMPX( 1980,	apple3,		0,		0,		apple3,    apple3,	apple3,	apple3,	"Apple Computer",	"Apple ///", GAME_NOT_WORKING )

