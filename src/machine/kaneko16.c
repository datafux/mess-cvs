/***************************************************************************


							MCU Code Simulation


***************************************************************************/

#include "driver.h"
#include "machine/random.h"

data16_t *mcu_ram;

/***************************************************************************
								Sand Scorpion
***************************************************************************/

/*
	MCU Tasks:

	- Collision detection (test if 2 rectangles overlap)
	- Multiply 2 words, obtaining a long word
	- Return a random value?
*/

READ16_HANDLER( sandscrp_mcu_ram_r )
{
	switch( offset )
	{
		case 0x04/2:	// Bit 0: collision detection
		{
			/* First rectangle */
			int x_10		=	mcu_ram[0x00/2];
			int x_11		=	mcu_ram[0x02/2] + x_10;
			int y_10		=	mcu_ram[0x04/2];
			int y_11		=	mcu_ram[0x06/2] + y_10;

			/* Second rectangle */
			int x_20		=	mcu_ram[0x08/2];
			int x_21		=	mcu_ram[0x0a/2] + x_20;
			int y_20		=	mcu_ram[0x0c/2];
			int y_21		=	mcu_ram[0x0e/2] + y_20;

			/* Sign extend the words */
			x_10 = (x_10 & 0x7fff) - (x_10 & 0x8000);
			x_11 = (x_11 & 0x7fff) - (x_11 & 0x8000);
			y_10 = (y_10 & 0x7fff) - (y_10 & 0x8000);
			y_11 = (y_11 & 0x7fff) - (y_11 & 0x8000);
			x_20 = (x_20 & 0x7fff) - (x_20 & 0x8000);
			x_21 = (x_21 & 0x7fff) - (x_21 & 0x8000);
			y_20 = (y_20 & 0x7fff) - (y_20 & 0x8000);
			y_21 = (y_21 & 0x7fff) - (y_21 & 0x8000);

			/* Check if they overlap */
			if	(	( x_10 > x_21 ) || ( x_11 < x_20 ) ||
					( y_10 > y_21 ) || ( y_11 < y_20 )	)
				return 0;
			else
				return 1;
		}
		break;

		case 0x10/2:	// Multiply 2 words, obtain a long word.
		case 0x12/2:
		{
			int res = mcu_ram[0x10/2] * mcu_ram[0x12/2];
			if (offset == 0x10/2)	return (res >> 16) & 0xffff;
			else					return (res >>  0) & 0xffff;
		}
		break;

		case 0x14/2:	// Random?
			return (mame_rand() & 0xffff);
	}

	logerror("CPU #0 PC %06X : Unknown MCU word %04X read\n",activecpu_get_pc(),offset*2);
	return mcu_ram[offset];
}

WRITE16_HANDLER( sandscrp_mcu_ram_w )
{
	COMBINE_DATA(&mcu_ram[offset]);
}


/***************************************************************************

								CALC3 MCU:

					Shogun Warriors / Fujiyama Buster
								B.Rap Boys

***************************************************************************/
/*
---------------------------------------------------------------------------
								CALC 3

92	B.Rap Boys				KANEKO CALC3 508 (74 PIN PQFP)
92	Shogun Warriors
	Fujiyama Buster			KANEKO CALC3 508 (74 Pin PQFP)
---------------------------------------------------------------------------

MCU Initialization command:

shogwarr: CPU #0 PC 00037A : MCU executed command: 00FF 0059 019E 030A FFFE 0042 0020 7FE0
fjbuster: CPU #0 PC 00037A : MCU executed command: 00FF 0059 019E 030A FFFE 0042 0020 7FE0
brapboys: CPU #0 PC 000BAE : MCU executed command: 00FF 00C2 0042 0830 082E 00C8 0020 0872

shogwarr/fjbuster:

00FF : busy flag, MCU clears it when cmd finished (main program loops until it's cleared)
0059 : MCU writes DSW -> $102e15
019E : ??? -> $102e14, compared with -1 once, very interesting, see $1063e6 - IT2
030A : location where MCU will get its parameters from now on
FFFE : probably polled by MCU, needs to be kept alive (cleared by main cpu - IT2)
0042 : MCU writes its checksum
00207FE0 : may serves for relocating code (written as .l)
*/

void calc3_mcu_run(void);

static int calc3_mcu_status, calc3_mcu_command_offset;


void calc3_mcu_init(void)
{
	calc3_mcu_status = 0;
	calc3_mcu_command_offset = 0;
}

WRITE16_HANDLER( calc3_mcu_ram_w )
{
	COMBINE_DATA(&mcu_ram[offset]);
	calc3_mcu_run();
}

#define CALC3_MCU_COM_W(_n_)				\
WRITE16_HANDLER( calc3_mcu_com##_n_##_w )	\
{											\
	calc3_mcu_status |= (1 << _n_);			\
	calc3_mcu_run();						\
}

CALC3_MCU_COM_W(0)
CALC3_MCU_COM_W(1)
CALC3_MCU_COM_W(2)
CALC3_MCU_COM_W(3)

/***************************************************************************
								Shogun Warriors
***************************************************************************/

/* Preliminary simulation: the game doesn't work */

/*
	MCU Tasks:

	- Read the DSWs
	- Supply code snippets to the 68000
*/

void calc3_mcu_run(void)
{
	data16_t mcu_command;

	if ( calc3_mcu_status != (1|2|4|8) )	return;

	mcu_command = mcu_ram[calc3_mcu_command_offset + 0];

	if (mcu_command == 0) return;

	logerror("CPU #0 PC %06X : MCU executed command at %04X: %04X\n",
	 	activecpu_get_pc(),calc3_mcu_command_offset*2,mcu_command);

	switch (mcu_command)
	{

		case 0x00ff:
		{
			int param1 = mcu_ram[calc3_mcu_command_offset + 1];
			int param2 = mcu_ram[calc3_mcu_command_offset + 2];
			int param3 = mcu_ram[calc3_mcu_command_offset + 3];
//			int param4 = mcu_ram[calc3_mcu_command_offset + 4];
			int param5 = mcu_ram[calc3_mcu_command_offset + 5];
//			int param6 = mcu_ram[calc3_mcu_command_offset + 6];
//			int param7 = mcu_ram[calc3_mcu_command_offset + 7];

			// clear old command (handshake to main cpu)
			mcu_ram[calc3_mcu_command_offset] = 0x0000;

			// execute the command:

			mcu_ram[param1 / 2] = ~readinputport(4);	// DSW
			mcu_ram[param2 / 2] = 0xffff;				// ? -1 / anything else

			calc3_mcu_command_offset = param3 / 2;	// where next command will be written?
			// param 4?
			mcu_ram[param5 / 2] = 0x8ee4;				// MCU Rom Checksum!
			// param 6&7 = address.l

/*

First code snippet provided by the MCU:

207FE0: 48E7 FFFE                movem.l D0-D7/A0-A6, -(A7)

207FE4: 3039 00A8 0000           move.w  $a80000.l, D0
207FEA: 4279 0020 FFFE           clr.w   $20fffe.l

207FF0: 41F9 0020 0000           lea     $200000.l, A0
207FF6: 7000                     moveq   #$0, D0

207FF8: 43E8 01C6                lea     ($1c6,A0), A1
207FFC: 7E02                     moveq   #$2, D7
207FFE: D059                     add.w   (A1)+, D0
208000: 51CF FFFC                dbra    D7, 207ffe

208004: 43E9 0002                lea     ($2,A1), A1
208008: 7E04                     moveq   #$4, D7
20800A: D059                     add.w   (A1)+, D0
20800C: 51CF FFFC                dbra    D7, 20800a

208010: 4640                     not.w   D0
208012: 5340                     subq.w  #1, D0
208014: 0068 0030 0216           ori.w   #$30, ($216,A0)

20801A: B07A 009A                cmp.w   ($9a,PC), D0; ($2080b6)
20801E: 670A                     beq     20802a

208020: 0268 000F 0216           andi.w  #$f, ($216,A0)
208026: 4268 0218                clr.w   ($218,A0)

20802A: 5468 0216                addq.w  #2, ($216,A0)
20802E: 42A8 030C                clr.l   ($30c,A0)
208032: 117C 0020 030C           move.b  #$20, ($30c,A0)

208038: 3E3C 0001                move.w  #$1, D7

20803C: 0C68 0008 0218           cmpi.w  #$8, ($218,A0)
208042: 6C00 0068                bge     2080ac

208046: 117C 0080 0310           move.b  #$80, ($310,A0)
20804C: 117C 0008 0311           move.b  #$8, ($311,A0)
208052: 317C 7800 0312           move.w  #$7800, ($312,A0)
208058: 5247                     addq.w  #1, D7
20805A: 0C68 0040 0216           cmpi.w  #$40, ($216,A0)
208060: 6D08                     blt     20806a

208062: 5468 0218                addq.w  #2, ($218,A0)
208066: 6000 0044                bra     2080ac

20806A: 117C 0041 0314           move.b  #$41, ($314,A0)

208070: 0C39 0001 0010 2E12      cmpi.b  #$1, $102e12.l
208078: 6606                     bne     208080

20807A: 117C 0040 0314           move.b  #$40, ($314,A0)

208080: 117C 000C 0315           move.b  #$c, ($315,A0)
208086: 317C 7000 0316           move.w  #$7000, ($316,A0)
20808C: 5247                     addq.w  #1, D7

20808E: 0839 0001 0010 2E15      btst    #$1, $102e15.l	; service mode
208096: 6714                     beq     2080ac

208098: 117C 0058 0318           move.b  #$58, ($318,A0)
20809E: 117C 0006 0319           move.b  #$6, ($319,A0)
2080A4: 317C 6800 031A           move.w  #$6800, ($31a,A0)
2080AA: 5247                     addq.w  #1, D7

2080AC: 3147 030A                move.w  D7, ($30a,A0)
2080B0: 4CDF 7FFF                movem.l (A7)+, D0-D7/A0-A6
2080B4: 4E73                     rte

2080B6: C747
*/
		}
		break;


		case 0x0001:
		{
//			int param1 = mcu_ram[calc3_mcu_command_offset + 1];
			int param2 = mcu_ram[calc3_mcu_command_offset + 2];

			// clear old command (handshake to main cpu)
			mcu_ram[calc3_mcu_command_offset] = 0x0000;

			// execute the command:

			// param1 ?
			mcu_ram[param2/2 + 0] = 0x0000;		// ?
			mcu_ram[param2/2 + 1] = 0x0000;		// ?
			mcu_ram[param2/2 + 2] = 0x0000;		// ?
			mcu_ram[param2/2 + 3] = 0x0000;		// ? addr.l
			mcu_ram[param2/2 + 4] = 0x00e0;		// 0000e0: 4e73 rte

		}
		break;


		case 0x0002:
		{
//			int param1 = mcu_ram[calc3_mcu_command_offset + 1];
//			int param2 = mcu_ram[calc3_mcu_command_offset + 2];
//			int param3 = mcu_ram[calc3_mcu_command_offset + 3];
//			int param4 = mcu_ram[calc3_mcu_command_offset + 4];
//			int param5 = mcu_ram[calc3_mcu_command_offset + 5];
//			int param6 = mcu_ram[calc3_mcu_command_offset + 6];
//			int param7 = mcu_ram[calc3_mcu_command_offset + 7];

			// clear old command (handshake to main cpu)
			mcu_ram[calc3_mcu_command_offset] = 0x0000;

			// execute the command:

		}
		break;

	}

}


/***************************************************************************

								TOYBOX MCU:

								Blood Warrior
							Great 1000 Miles Rally
									...

***************************************************************************/
/*
---------------------------------------------------------------------------
								TOYBOX

94	Bonks Adventure				TOYBOX?		       TBSOP01
94	Blood Warrior				TOYBOX?		       TBS0P01 452 9339PK001
94	Great 1000 Miles Rally		TOYBOX													"MM0525-TOYBOX199","USMM0713-TB1994 "
95	Great 1000 Miles Rally 2	TOYBOX		KANEKO TBSOP02 454 9451MK002 (74 pin PQFP)	"USMM0713-TB1994 "
95  Jackie Chan					TOYBOX													"USMM0713-TB1994 "
95  Gals Panic 3				TOYBOX?		       TBSOP01
---------------------------------------------------------------------------

All the considerations are based on the analysis of jchan, and to a fewer extent galpani3, and make references to the current driver sources:

MCU triggering:
---------------

the 4 JCHAN_MCU_COM_W(...) are in fact 2 groups:

AM_RANGE(0x330000, 0x330001) AM_WRITE(jchan_mcu_com0_w)	// _[ these 2 are set to 0xFFFF
AM_RANGE(0x340000, 0x340001) AM_WRITE(jchan_mcu_com1_w)	//  [ for MCU to execute cmd

AM_RANGE(0x350000, 0x350001) AM_WRITE(jchan_mcu_com2_w)	// _[ these 2 are set to 0xFFFF
AM_RANGE(0x360000, 0x360001) AM_WRITE(jchan_mcu_com3_w)	//  [ for MCU to return its status


MCU parameters:
---------------

mcu_command = mcu_ram[0x0010/2];	// command nb
mcu_offset  = mcu_ram[0x0012/2]/2;	// offset in shared RAM where MCU will write
mcu_subcmd  = mcu_ram[0x0014/2];	// sub-command parameter, happens only for command #4


	the only MCU commands found in program code are:
	- 0x04: protection: provide data (see below) and code <<<---!!!
	- 0x03: read DSW
	- 0x02: load game settings \ stored in ATMEL AT93C46 chip,
	- 0x42: save game settings / 128 bytes serial EEPROM


Current feeling of devs is that this EEPROM might also play a role in the protection scheme,
but I (SV) feel that it is very unlikely because of the following, which has been verified:
if the checksum test fails at most 3 times, then the initial settings, stored in main68k ROM,
are loaded in RAM then saved with cmd 0x42 (see code @ $5196 & $50d4)
Note that this is valid for jchan only, other games haven't been looked at.

Others:
-------

There is one interesting MCU cmd $4 in jchan:
-> sub-cmd $3d, MCU writes the string "USMM0713-TB1994 "

The very same string is written by gtmr games (gtmre/gtmrusa/gtmr2) but apparently with no sub-cmd: this string is
probably the MCU model string, so this one should be in internal MCU ROM (another one for gtmr is "MM0525-TOYBOX199")

TODO: look at this one since this remark is only driver-based.
*/

void toybox_mcu_run(void);
void bloodwar_mcu_run(void);
void gtmr_mcu_run(void);

static data16_t toybox_mcu_com[4];

void toybox_mcu_init(void)
{
	memset(toybox_mcu_com, 0, 4 * sizeof( data16_t) );
}

#define TOYBOX_MCU_COM_W(_n_)							\
WRITE16_HANDLER( toybox_mcu_com##_n_##_w )				\
{														\
	COMBINE_DATA(&toybox_mcu_com[_n_]);					\
	if (toybox_mcu_com[0] != 0xFFFF)	return;			\
	if (toybox_mcu_com[1] != 0xFFFF)	return;			\
	if (toybox_mcu_com[2] != 0xFFFF)	return;			\
	if (toybox_mcu_com[3] != 0xFFFF)	return;			\
														\
	memset(toybox_mcu_com, 0, 4 * sizeof( data16_t ) );	\
	toybox_mcu_run();									\
}

TOYBOX_MCU_COM_W(0)
TOYBOX_MCU_COM_W(1)
TOYBOX_MCU_COM_W(2)
TOYBOX_MCU_COM_W(3)

extern const struct GameDriver driver_bloodwar;
extern const struct GameDriver driver_bonkadv;
extern const struct GameDriver driver_gtmr;
extern const struct GameDriver driver_gtmre;
extern const struct GameDriver driver_gtmrusa;
extern const struct GameDriver driver_gtmr2;
extern const struct GameDriver driver_gtmr2a;

void toybox_mcu_run(void)
{
	if ( (Machine->gamedrv == &driver_gtmr)    ||
		 (Machine->gamedrv == &driver_gtmre)   ||
		 (Machine->gamedrv == &driver_gtmrusa) ||
		 (Machine->gamedrv == &driver_gtmr2)   ||
		 (Machine->gamedrv == &driver_gtmr2a) )
	{
		gtmr_mcu_run();
	}
	else
	if ( (Machine->gamedrv == &driver_bloodwar) ||
		 (Machine->gamedrv == &driver_bonkadv) )
	{
		bloodwar_mcu_run();
	}
}


/***************************************************************************
								Blood Warrior
***************************************************************************/

void bloodwar_mcu_run(void)
{
	data16_t mcu_command	=	mcu_ram[0x0010/2];
	data16_t mcu_offset		=	mcu_ram[0x0012/2] / 2;
	data16_t mcu_data		=	mcu_ram[0x0014/2];

	logerror("CPU #0 PC %06X : MCU executed command: %04X %04X %04X\n",activecpu_get_pc(),mcu_command,mcu_offset*2,mcu_data);

	switch (mcu_command >> 8)
	{
#if 0
		case 0x02:	// TEST
		{
			/* MCU writes the string " ATOP 1993.12 " to shared ram */
			mcu_ram[mcu_offset + 0x70/2 + 0] = 0x2041;
			mcu_ram[mcu_offset + 0x70/2 + 1] = 0x544F;
			mcu_ram[mcu_offset + 0x70/2 + 2] = 0x5020;
			mcu_ram[mcu_offset + 0x70/2 + 3] = 0x3139;
			mcu_ram[mcu_offset + 0x70/2 + 4] = 0x3933;
			mcu_ram[mcu_offset + 0x70/2 + 5] = 0x2E31;
			mcu_ram[mcu_offset + 0x70/2 + 6] = 0x3220;
			mcu_ram[mcu_offset + 0x70/2 + 7] = 0xff00;

			mcu_ram[mcu_offset + 0x10/2 + 0] = 0x0000;
			mcu_ram[mcu_offset + 0x12/2 + 0] = 0x0000;
		}
		break;
#endif

		case 0x02:	// Read from NVRAM
		{
			if (!strcmp(Machine->gamedrv->name,"bonkadv"))
				memcpy(&mcu_ram[mcu_offset],memory_region(REGION_USER1),128);
			else
			{
				mame_file *f;
				if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,0)) != 0)
				{
					mame_fread(f,&mcu_ram[mcu_offset], 128);
					mame_fclose(f);
				}
				else
					memcpy(&mcu_ram[mcu_offset],memory_region(REGION_USER1),128);
			}
		}
		break;

		case 0x42:	// Write to NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,1)) != 0)
			{
				mame_fwrite(f,&mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x03:	// DSW
		{
			mcu_ram[mcu_offset] = readinputport(4);
		}
		break;

		case 0x04:	// Protection
		{
			switch(mcu_data)
			{
				// unknown data
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x04:
				case 0x05:
				case 0x06:
				case 0x07:
				case 0x08:
				case 0x09:
				break;

				// palette data
				case 0x0a:
				case 0x0b:
				case 0x0c:
				case 0x0d:
				case 0x0e:
				case 0x0f:
				case 0x10:
				case 0x11:
				case 0x12:
				case 0x13:
				case 0x14:
				case 0x15:
				case 0x16:
				case 0x17:
				case 0x18:
				case 0x19:
				{
					mcu_ram[mcu_offset + 0] = 0x0001;	// number of palettes (>=1)
					// palette data follows (each palette is 0x200 byes long)
					mcu_ram[mcu_offset + 1] = 0x8000;	// a negative word will end the palette
				}
				break;

				// tilemap data
				case 0x1c:
				case 0x1d:
				case 0x1e:
				case 0x1f:
				case 0x20:
				case 0x21:
				case 0x22:
				case 0x23:
				case 0x24:
				{
					// tile data (ff means no tiles) followed by routine index
					mcu_ram[mcu_offset + 0] = 0xff00;
				}
				break;

				// unknown long
				case 0x25:
				case 0x26:
				case 0x27:
				case 0x28:
				case 0x29:
				case 0x2a:
				case 0x2b:
				case 0x2c:
				case 0x2d:
				{
					mcu_ram[mcu_offset + 0] = 0x0000;
					mcu_ram[mcu_offset + 1] = 0x0000;
				}
				break;

				default:
					logerror("UNKNOWN PARAMETER %02X TO COMMAND 4\n",mcu_data);
			}
		}
		break;

		default:
			logerror("UNKNOWN COMMAND\n");
		break;
	}
}

/***************************************************************************
							Great 1000 Miles Rally
***************************************************************************/

/*
	MCU Tasks:

	- Write and ID string to shared RAM.
	- Access the EEPROM
	- Read the DSWs
*/

void gtmr_mcu_run(void)
{
	data16_t mcu_command	=	mcu_ram[0x0010/2];
	data16_t mcu_offset		=	mcu_ram[0x0012/2] / 2;
	data16_t mcu_data		=	mcu_ram[0x0014/2];

	logerror("CPU #0 PC %06X : MCU executed command: %04X %04X %04X\n",activecpu_get_pc(),mcu_command,mcu_offset*2,mcu_data);

	switch (mcu_command >> 8)
	{

		case 0x02:	// Read from NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,0)) != 0)
			{
				mame_fread(f,&mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x42:	// Write to NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,1)) != 0)
			{
				mame_fwrite(f,&mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x03:	// DSW
		{
			mcu_ram[mcu_offset] = readinputport(4);
		}
		break;

		case 0x04:	// TEST (2 versions)
		{
			if (Machine->gamedrv == &driver_gtmr)
			{
				/* MCU writes the string "MM0525-TOYBOX199" to shared ram */
				mcu_ram[mcu_offset+0] = 0x4d4d;
				mcu_ram[mcu_offset+1] = 0x3035;
				mcu_ram[mcu_offset+2] = 0x3235;
				mcu_ram[mcu_offset+3] = 0x2d54;
				mcu_ram[mcu_offset+4] = 0x4f59;
				mcu_ram[mcu_offset+5] = 0x424f;
				mcu_ram[mcu_offset+6] = 0x5831;
				mcu_ram[mcu_offset+7] = 0x3939;
			}
			else if ( (Machine->gamedrv == &driver_gtmre)  ||
					  (Machine->gamedrv == &driver_gtmrusa) ||
					  (Machine->gamedrv == &driver_gtmr2) ||
					  (Machine->gamedrv == &driver_gtmr2a) )

			{
				/* MCU writes the string "USMM0713-TB1994 " to shared ram */
				mcu_ram[mcu_offset+0] = 0x5553;
				mcu_ram[mcu_offset+1] = 0x4d4d;
				mcu_ram[mcu_offset+2] = 0x3037;
				mcu_ram[mcu_offset+3] = 0x3133;
				mcu_ram[mcu_offset+4] = 0x2d54;
				mcu_ram[mcu_offset+5] = 0x4231;
				mcu_ram[mcu_offset+6] = 0x3939;
				mcu_ram[mcu_offset+7] = 0x3420;
			}
		}
		break;
	}

}

