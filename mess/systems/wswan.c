/***************************************************************************

  wswan.c

  Driver file to handle emulation of the Bandai WonderSwan
  By:

  Anthony Kruize

  Based on the WStech documentation by Judge and Dox.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/wswan.h"
#include "devices/cartslot.h"
#include "sound/custom.h"

static ADDRESS_MAP_START (wswan_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x03fff) AM_RAM			/* 16kb RAM + 16kb 4 colour tiles */
	AM_RANGE(0x04000, 0x0ffff) AM_NOP			/* Not used */
	AM_RANGE(0x10000, 0x1ffff) AM_RAMBANK(1)	/* SRAM bank */
	AM_RANGE(0x20000, 0x2ffff) AM_RAMBANK(2)	/* ROM bank 1 */
	AM_RANGE(0x30000, 0x3ffff) AM_RAMBANK(3)	/* ROM bank 2 */
	AM_RANGE(0x40000, 0x4ffff) AM_RAMBANK(4)	/* ROM bank 3 */
	AM_RANGE(0x50000, 0x5ffff) AM_RAMBANK(5)	/* ROM bank 4 */
	AM_RANGE(0x60000, 0x6ffff) AM_RAMBANK(6)	/* ROM bank 5 */
	AM_RANGE(0x70000, 0x7ffff) AM_RAMBANK(7)	/* ROM bank 6 */
	AM_RANGE(0x80000, 0x8ffff) AM_RAMBANK(8)	/* ROM bank 7 */
	AM_RANGE(0x90000, 0x9ffff) AM_RAMBANK(9)	/* ROM bank 8 */
	AM_RANGE(0xA0000, 0xAffff) AM_RAMBANK(10)	/* ROM bank 9 */
	AM_RANGE(0xB0000, 0xBffff) AM_RAMBANK(11)	/* ROM bank 10 */
	AM_RANGE(0xC0000, 0xCffff) AM_RAMBANK(12)	/* ROM bank 11 */
	AM_RANGE(0xD0000, 0xDffff) AM_RAMBANK(13)	/* ROM bank 12 */
	AM_RANGE(0xE0000, 0xEffff) AM_RAMBANK(14)	/* ROM bank 13 */
	AM_RANGE(0xF0000, 0xFffff) AM_RAMBANK(15)	/* ROM bank 14 */
ADDRESS_MAP_END

static ADDRESS_MAP_START (wsc_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x00000, 0x03fff) AM_RAM			/* 16kb RAM + 16kb 4 colour tiles */
	AM_RANGE(0x04000, 0x0ffff) AM_RAM			/* 16 colour tiles + palettes */
	AM_RANGE(0x10000, 0x1ffff) AM_RAMBANK(1)	/* SRAM bank */
	AM_RANGE(0x20000, 0x2ffff) AM_RAMBANK(2)	/* ROM bank 1 */
	AM_RANGE(0x30000, 0x3ffff) AM_RAMBANK(3)	/* ROM bank 2 */
	AM_RANGE(0x40000, 0x4ffff) AM_RAMBANK(4)	/* ROM bank 3 */
	AM_RANGE(0x50000, 0x5ffff) AM_RAMBANK(5)	/* ROM bank 4 */
	AM_RANGE(0x60000, 0x6ffff) AM_RAMBANK(6)	/* ROM bank 5 */
	AM_RANGE(0x70000, 0x7ffff) AM_RAMBANK(7)	/* ROM bank 6 */
	AM_RANGE(0x80000, 0x8ffff) AM_RAMBANK(8)	/* ROM bank 7 */
	AM_RANGE(0x90000, 0x9ffff) AM_RAMBANK(9)	/* ROM bank 8 */
	AM_RANGE(0xA0000, 0xAffff) AM_RAMBANK(10)	/* ROM bank 9 */
	AM_RANGE(0xB0000, 0xBffff) AM_RAMBANK(11)	/* ROM bank 10 */
	AM_RANGE(0xC0000, 0xCffff) AM_RAMBANK(12)	/* ROM bank 11 */
	AM_RANGE(0xD0000, 0xDffff) AM_RAMBANK(13)	/* ROM bank 12 */
	AM_RANGE(0xE0000, 0xEffff) AM_RAMBANK(14)	/* ROM bank 13 */
	AM_RANGE(0xF0000, 0xFffff) AM_RAMBANK(15)	/* ROM bank 14 */
ADDRESS_MAP_END

static ADDRESS_MAP_START (wswan_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x00, 0xff) AM_READWRITE( wswan_port_r, wswan_port_w )	/* I/O ports */
ADDRESS_MAP_END

INPUT_PORTS_START( wswan )
	PORT_START /* IN 0 : cursors */
	PORT_BIT( 0x1, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("Up") 
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("Down") 
	PORT_BIT( 0x8, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("Left") 
	PORT_BIT( 0x2, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("Right") 
	PORT_START /* IN 1 : Buttons */
	PORT_BIT( 0x2, IP_ACTIVE_HIGH, IPT_START1) PORT_NAME("Start") 
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Button A") 
	PORT_BIT( 0x8, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Button B") 
INPUT_PORTS_END

static gfx_decode gfxdecodeinfo[] =
{ { -1 } /* end of array */ };

/* WonderSwan can display 16 shades of grey */
static PALETTE_INIT( wswan )
{
	int ii;
	for( ii = 0; ii < 16; ii++ )
	{
		UINT8 shade = ii * (256 / 16);
		palette_set_color( 15 - ii, shade, shade, shade );
	}
}

static struct CustomSound_interface wswan_sound_interface =
{
	wswan_sh_start
};

static MACHINE_DRIVER_START( wswan )
	/* Basic machine hardware */
	/* FIXME: CPU should be a V30MZ not a V30! */
	MDRV_CPU_ADD_TAG("main", V30, 3072000)		/* 3.072 Mhz */
	MDRV_CPU_PROGRAM_MAP(wswan_mem, 0)
	MDRV_CPU_IO_MAP(wswan_io, 0)
	MDRV_CPU_VBLANK_INT(wswan_scanline_interrupt, 158/*159?*/)	/* 1 int each scanline */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( wswan )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(28*8, 18*8)
	MDRV_VISIBLE_AREA(0*8, 28*8-1, 0*8, 18*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(4*16)
	MDRV_PALETTE_INIT(wswan)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(wswan_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wscolor )
	MDRV_IMPORT_FROM(wswan)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(wsc_mem, 0)
	MDRV_PALETTE_LENGTH(4096)
MACHINE_DRIVER_END

static void wswan_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_wswan_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "ws,wsc"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(wswan)
	CONFIG_DEVICE(wswan_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( wswan )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
ROM_END

ROM_START( wscolor )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
ROM_END

/*     YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY   FULLNAME*/
CONS( 1999, wswan,   0,      0,      wswan,   wswan, 0,    wswan,  "Bandai", "WonderSwan",       GAME_NOT_WORKING )
CONS( 2000, wscolor, wswan,  0,      wscolor, wswan, 0,    wswan,  "Bandai", "WonderSwan Color", GAME_NOT_WORKING )
