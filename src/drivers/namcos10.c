/***************************************************************************

  Namco System 10 - Arcade PSX Hardware
  =====================================
  Driver by smf

  Only one rom for Mr Driller 2 is dumped, it is encrypted using an xor and then a bitswap.
  All it currently does is try to access the controller port.

*/

#include "driver.h"
#include "cpu/mips/psx.h"
#include "includes/psx.h"

static INTERRUPT_GEN( namcos10_vblank )
{
}

static ADDRESS_MAP_START( namcos10_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_WRITE(MWA32_RAM)    /* ram */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_WRITE(MWA32_BANK1)  /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x1f801008, 0x1f80100b) AM_WRITE(MWA32_RAM)    /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x1f801040, 0x1f80105f) AM_WRITE(psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x1f801070, 0x1f801077) AM_WRITE(psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_WRITE(psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80113f) AM_WRITE(psx_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_WRITE(psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_WRITE(psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x1f802020, 0x1f802033) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x1fc00000, 0x1fffffff) AM_WRITE(MWA32_ROM)    /* bios mirror */
	AM_RANGE(0x80000000, 0x803fffff) AM_WRITE(MWA32_BANK3)  /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fffffff) AM_WRITE(MWA32_ROM)    /* bios mirror */
	AM_RANGE(0xa0000000, 0xa03fffff) AM_WRITE(MWA32_BANK5)  /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfffffff) AM_WRITE(MWA32_ROM)    /* bios */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITE(MWA32_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( namcos10_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_READ(MRA32_RAM)    /* ram */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_READ(MRA32_BANK1)  /* scratchpad */
	AM_RANGE(0x1f801008, 0x1f80100b) AM_READ(MRA32_RAM)    /* ?? */
	AM_RANGE(0x1f801010, 0x1f801013) AM_READ(MRA32_NOP)
	AM_RANGE(0x1f801014, 0x1f801017) AM_READ(MRA32_NOP)
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READ(psx_sio_r)
	AM_RANGE(0x1f801070, 0x1f801077) AM_READ(psx_irq_r)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READ(psx_dma_r)
	AM_RANGE(0x1f801100, 0x1f80113f) AM_READ(psx_counter_r)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READ(psx_gpu_r)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READ(psx_mdec_r)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READ(MRA32_NOP)
	AM_RANGE(0x1f802020, 0x1f802033) AM_READ(MRA32_RAM)
	AM_RANGE(0x1fc00000, 0x1fffffff) AM_READ(MRA32_BANK2)  /* bios mirror */
	AM_RANGE(0x80000000, 0x807fffff) AM_READ(MRA32_BANK3)  /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fffffff) AM_READ(MRA32_BANK4)  /* bios mirror */
	AM_RANGE(0xa0000000, 0xa07fffff) AM_READ(MRA32_BANK5)  /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfffffff) AM_READ(MRA32_BANK6)  /* bios */
ADDRESS_MAP_END

static DRIVER_INIT( namcos10 )
{
	int i;
	data16_t *RAM = (data16_t *)memory_region( REGION_USER2 );

	for( i = 0; i < memory_region_length( REGION_USER2 ) / 2; i++ )
	{
		RAM[ i ] = BITSWAP16( RAM[ i ] ^ 0xaaaa,
			0xc, 0xd, 0xf, 0xe, 0xb, 0xa, 0x9, 0x8,
			0x7, 0x6, 0x4, 0x1, 0x2, 0x5, 0x0, 0x3 );
	}

	cpu_setbank( 1, memory_region( REGION_USER1 ) );
	cpu_setbank( 2, memory_region( REGION_USER2 ) );
	cpu_setbank( 3, memory_region( REGION_CPU1 ) );
	cpu_setbank( 4, memory_region( REGION_USER2 ) );
	cpu_setbank( 5, memory_region( REGION_CPU1 ) );
	cpu_setbank( 6, memory_region( REGION_USER2 ) );
}

MACHINE_INIT( namcos10 )
{
	psx_machine_init();
}

static MACHINE_DRIVER_START( namcos10 )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( namcos10_readmem, namcos10_writemem )
	MDRV_CPU_VBLANK_INT( namcos10_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( namcos10 )
	MDRV_NVRAM_HANDLER( generic_0fill )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
MACHINE_DRIVER_END

INPUT_PORTS_START( namcos10 )
	/* IN 0 */
	PORT_START
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x7fff, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 )
	PORT_BIT( 0x000f, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_SERVICE2 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x000f, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 3 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
INPUT_PORTS_END

ROM_START( mrdrilr2 )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x800000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD( "dr21vera.1a",  0x000000, 0x800000, CRC(f93532a2) SHA1(8b72f2868978be1f0e0abd11425a3c8b2b0c4e99) )
ROM_END

GAMEX( 2000, mrdrilr2,  0,        namcos10, namcos10, namcos10, ROT0, "Namco", "Mr Driller 2", GAME_NOT_WORKING | GAME_NO_SOUND )
