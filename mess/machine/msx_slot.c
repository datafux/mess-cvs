/*
 * msx_slot.c : definitions of the different slots 
 */

#include "driver.h"
#include "includes/msx_slot.h"
#include "includes/msx.h"
#include "includes/wd179x.h"

extern MSX msx1;


MSX_SLOT_INIT(empty)
{
	state->type = SLOT_EMPTY;

	return 0;
}

MSX_SLOT_RESET(empty)
{
	/* reset void */
}

MSX_SLOT_MAP(empty)
{
	cpu_setbank (page * 2 + 1, msx1.empty);
	cpu_setbank (page * 2 + 2, msx1.empty);
}

MSX_SLOT_INIT(rom)
{
	state->type = SLOT_ROM;
	state->mem = mem;
	state->size = size;
	state->start_page = page;

	return 0;
}

MSX_SLOT_RESET(rom)
{
	/* state-less */
}

MSX_SLOT_MAP(rom)
{
	UINT8 *mem = state->mem + (page - state->start_page) * 0x4000;
	
	cpu_setbank (page * 2 + 1, mem);
	cpu_setbank (page * 2 + 2, mem + 0x2000);
}

MSX_SLOT_INIT(ram)
{
	state->mem = auto_malloc (size);
	if (!state->mem) {
		logerror ("ram: error: out of memory\n");
		return 1;
	}
	state->type = SLOT_RAM;
	state->start_page = page;
	state->size = size;

	return 0;
}
	
MSX_SLOT_MAP(ram)
{
	UINT8 *mem = state->mem + (page - state->start_page) * 0x4000;

	msx1.ram_pages[page] = mem;
	cpu_setbank (page * 2 + 1, mem);
	cpu_setbank (page * 2 + 2, mem + 0x2000);
}

MSX_SLOT_RESET(ram)
{
}

MSX_SLOT_INIT(rammm)
{
	int i, mask, nsize;

	nsize = 0x10000; /* 64 kb */
	mask = 3;
	for (i=0; i<6; i++) {
		if (size == nsize) {
			break;
		}
		mask = (mask << 1) | 1;
		nsize <<= 1;
	}
	if (i == 6) {
		logerror ("ram mapper: error: must be 64kb, 128kb, 256kb, 512kb, "
				  "1mb, 2mb or 4mb\n");
		return 1;
	}
	state->mem = auto_malloc (size);
	if (!state->mem) {
		logerror ("ram mapper: error: out of memory\n");
		return 1;
	}

	state->type = SLOT_RAM_MM;
	state->start_page = page;
	state->size = size;
	state->bank_mask = mask;

	return 0;
}

MSX_SLOT_RESET(rammm)
{
	int i;

	for (i=0; i<4; i++) {
		msx1.ram_mapper[i] = 3 - i;
	}
}

MSX_SLOT_MAP(rammm)
{
	UINT8 *mem = state->mem + 
			0x4000 * (msx1.ram_mapper[page] & state->bank_mask);
	
	msx1.ram_pages[page] = mem;
	cpu_setbank (page * 2 + 1, mem);
	cpu_setbank (page * 2 + 2, mem + 0x2000);
}

MSX_SLOT_INIT(msxdos2)
{
	if (size != 0x10000) {
		logerror ("msxdos2: error: rom file must be 64kb\n");
		return 1;
	}
	state->type = SLOT_MSXDOS2;
	state->mem = mem;
	state->size = size;

	return 0;
}

MSX_SLOT_RESET(msxdos2)
{
	state->banks[0] = 0;
}

MSX_SLOT_MAP(msxdos2)
{
	if (page != 1) {
		cpu_setbank (page * 2 + 1, msx1.empty);
		cpu_setbank (page * 2 + 2, msx1.empty);
	} else {
		cpu_setbank (3, state->mem + state->banks[0] * 0x4000);
		cpu_setbank (4, state->mem + state->banks[0] * 0x4000 + 0x2000);
	}
}

MSX_SLOT_WRITE(msxdos2)
{
	if (addr == 0x6000) {
		state->banks[0] = val & 3;
		slot_msxdos2_map (state, 1);
	}
}

MSX_SLOT_INIT(konami)
{
	state->type = SLOT_KONAMI;
	state->mem = mem;
	state->size = size;

	switch (size) {
	case 0x10000:
		state->bank_mask = 7;
		break;
	case 0x20000:
		state->bank_mask = 15;
		break;
	case 0x40000:
		state->bank_mask = 31;
		break;
	case 0x80000:
		state->bank_mask = 63;
		break;
	default:
		logerror ("konami4: error: only 64kb, 128kb, 256kb, 512kb supported\n");
		return 1;
	}

	return 0;
}

MSX_SLOT_RESET(konami)
{
	int i;

	for (i=0; i<4; i++) state->banks[i] = i;
}

MSX_SLOT_MAP(konami)
{
	switch (page) {
	case 0:
		cpu_setbank (1, state->mem + state->banks[0] * 0x2000);
		cpu_setbank (2, state->mem + state->banks[1] * 0x2000);
		break;
	case 1:
		cpu_setbank (3, state->mem + state->banks[0] * 0x2000);
		cpu_setbank (4, state->mem + state->banks[1] * 0x2000);
		break;
	case 2:
		cpu_setbank (5, state->mem + state->banks[2] * 0x2000);
		cpu_setbank (6, state->mem + state->banks[3] * 0x2000);
		break;
	case 3:
		cpu_setbank (7, state->mem + state->banks[2] * 0x2000);
		cpu_setbank (8, state->mem + state->banks[3] * 0x2000);
	}
}

MSX_SLOT_WRITE(konami)
{
	switch (addr) {
	case 0x6000:
		state->banks[1] = val & state->bank_mask;
		slot_konami_map (state, 1);
		if (msx1.state[0] == state) {
			slot_konami_map (state, 0);
		}
		break;
	case 0x8000:
		state->banks[2] = val & state->bank_mask;
		slot_konami_map (state, 2);
		if (msx1.state[3] == state) {
			slot_konami_map (state, 3);
		}
		break;
	case 0xa000:
		state->banks[3] = val & state->bank_mask;
		slot_konami_map (state, 2);
		if (msx1.state[3] == state) {
			slot_konami_map (state, 3);
		}
	}
}

MSX_SLOT_INIT(konami_scc)
{
	switch (size) {
	case 0x10000:
		state->bank_mask = 7;
		break;
	case 0x20000:
		state->bank_mask = 15;
		break;
	case 0x40000:
		state->bank_mask = 31;
		break;
	case 0x80000:
		state->bank_mask = 63;
		break;
	default:
		logerror ("konami5: error: only 64kb, 128kb, 256kb, 512kb supported\n");
		return 1;
	}

	state->type = SLOT_KONAMI_SCC;
	state->mem = mem;
	state->size = size;

	return 0;
}

MSX_SLOT_RESET(konami_scc)
{
	int i;

	for (i=0; i<4; i++) state->banks[i] = i;
	state->cart.scc.active = 0;
}

READ_HANDLER (konami_scc_bank5)
{
	if (offset < 0x1800) {
		return msx1.state[2]->mem[msx1.state[2]->banks[2] * 0x2000 + offset];
	}
	else if (offset & 0x80) {
#if 0
		if ((offset & 0xff) >= 0xe0) {
			/* write 0xff to deformation register */
		}
#endif
		return 0xff;
	}
	else {
		return K051649_waveform_r (offset & 0x7f);
	}
}

MSX_SLOT_MAP(konami_scc)
{
	switch (page) {
	case 0:
		cpu_setbank (1, state->mem + state->banks[2] * 0x2000);
		cpu_setbank (2, state->mem + state->banks[3] * 0x2000);
		break;
	case 1:
		cpu_setbank (3, state->mem + state->banks[0] * 0x2000);
		cpu_setbank (4, state->mem + state->banks[1] * 0x2000);
		break;
	case 2:
		cpu_setbank (5, state->mem + state->banks[2] * 0x2000);
		cpu_setbank (6, state->mem + state->banks[3] * 0x2000);
		memory_set_bankhandler_r (5, 0, 
				state->cart.scc.active ? konami_scc_bank5 : MRA_BANK5);
		break;
	case 3:
		cpu_setbank (7, state->mem + state->banks[0] * 0x2000);
		cpu_setbank (8, state->mem + state->banks[1] * 0x2000);
	}
}

MSX_SLOT_WRITE(konami_scc)
{
	if (addr >= 0x5000 && addr < 0x5800) {
		state->banks[0] = val & state->bank_mask;
		slot_konami_map (state, 1);
		if (msx1.state[3] == state) {
			slot_konami_map (state, 3);
		}
	} 
	else if (addr >= 0x7000 && addr < 0x7800) {
		state->banks[1] = val & state->bank_mask;
		slot_konami_map (state, 1);
		if (msx1.state[3] == state) {
			slot_konami_map (state, 3);
		}
	} 
	else if (addr >= 0x9000 && addr < 0x9800) {
		state->banks[2] = val & state->bank_mask;
		state->cart.scc.active = ((val & 0x3f) == 0x3f);
		slot_konami_map (state, 2);
		if (msx1.state[0] == state) {
			slot_konami_map (state, 0);
		}
	} 
	else if (state->cart.scc.active && addr >= 0x9800 && addr < 0xa000) {
		int offset = addr & 0xff;

		if (offset < 0x80) {
			K051649_waveform_w (offset, val);
		}
		else if (offset < 0xa0) {
			offset &= 0xf;
			if (offset < 0xa) {
				K051649_frequency_w (offset, val);
			}
			else if (offset < 0xf) {
				K051649_volume_w (offset - 0xa, val);
			}
			else {
				K051649_keyonoff_w (0, val);
			}
		}
#if 0
		else if (offset >= 0xe0) {
			/* deformation register */
		}
#endif
	} 
	else if (addr >= 0xb000 && addr < 0xb800) {
		state->banks[3] = val & state->bank_mask;
		slot_konami_map (state, 2);
		if (msx1.state[0] == state) {
			slot_konami_map (state, 0);
		}
	}
}

MSX_SLOT_INIT(ascii8)
{
	state->type = SLOT_ASCII8;
	state->mem = mem;
	state->size = size;

	switch (size) {
	case 0x10000:
		state->bank_mask = 7;
		break;
	case 0x20000:
		state->bank_mask = 15;
		break;
	case 0x40000:
		state->bank_mask = 31;
		break;
	case 0x80000:
		state->bank_mask = 63;
		break;
	default:
		logerror ("ascii8: error: only 64kb, 128kb, 256kb, 512kb supported\n");
		return 1;
	}

	return 0;
}

MSX_SLOT_RESET(ascii8)
{
	int i;

	for (i=0; i<4; i++) state->banks[i] = i;
}

MSX_SLOT_MAP(ascii8)
{
	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		cpu_setbank (3, state->mem + state->banks[0] * 0x2000);
		cpu_setbank (4, state->mem + state->banks[1] * 0x2000);
		break;
	case 2:
		cpu_setbank (5, state->mem + state->banks[2] * 0x2000);
		cpu_setbank (6, state->mem + state->banks[3] * 0x2000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(ascii8)
{
	int bank;

	if (addr >= 0x2000 && addr < 0x4000) {
		bank = (addr / 0x800) & 3;

		state->banks[bank] = val & state->bank_mask;
		if (bank <= 1) {
			slot_ascii8_map (state, 1);
		}
		else if (msx1.state[2] == state) {
			slot_ascii8_map (state, 2);
		}
	}
}

MSX_SLOT_INIT(ascii16)
{
	state->type = SLOT_ASCII16;
	state->mem = mem;
	state->size = size;

	switch (size) {
	case 0x10000:
		state->bank_mask = 3;
		break;
	case 0x20000:
		state->bank_mask = 7;
		break;
	case 0x40000:
		state->bank_mask = 15;
		break;
	case 0x80000:
		state->bank_mask = 31;
		break;
	default:
		logerror ("ascii16: error: only 64kb, 128kb, 256kb, 512kb supported\n");
		return 1;
	}

	return 0;
}

MSX_SLOT_RESET(ascii16)
{
	int i;

	for (i=0; i<2; i++) state->banks[i] = i;
}

MSX_SLOT_MAP(ascii16)
{
	UINT8 *mem;

	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		mem = state->mem + state->banks[0] * 0x4000;
		cpu_setbank (3, mem);
		cpu_setbank (4, mem + 0x2000);
		break;
	case 2:
		mem = state->mem + state->banks[1] * 0x4000;
		cpu_setbank (5, mem);
		cpu_setbank (6, mem + 0x2000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(ascii16)
{
	if (addr >= 0x6000 && addr < 0x6800) {
		state->banks[0] = val & state->bank_mask;
		slot_ascii16_map (state, 1);
	}
	else if (addr >= 0x7000 && addr < 0x7800) {
		state->banks[1] = val & state->bank_mask;
		if (msx1.state[2] == state) {
			slot_ascii16_map (state, 2);
		}
	}
}

MSX_SLOT_INIT(ascii8_sram)
{
	state->cart.sram.mem = auto_malloc (0x2000);
	if (!state->cart.sram.mem) {
		logerror ("ascii8_sram: error: failed to malloc sram memory\n");
		return 1;
	}
	memset (state->cart.sram.mem, 0, 0x2000);
	state->type = SLOT_ASCII8_SRAM;
	state->mem = mem;
	state->size = size;

	switch (size) {
	case 0x10000:
		state->bank_mask = 7;
		state->cart.sram.sram_mask = 8;
		state->cart.sram.empty_mask = 0xf0;
		break;
	case 0x20000:
		state->bank_mask = 15;
		state->cart.sram.sram_mask = 16;
		state->cart.sram.empty_mask = 0xe0;
		break;
	case 0x40000:
		state->bank_mask = 31;
		state->cart.sram.sram_mask = 32;
		state->cart.sram.empty_mask = 0xc0;
		break;
	case 0x80000:
		state->bank_mask = 63;
		state->cart.sram.sram_mask = 64;
		state->cart.sram.empty_mask = 0x80;
		break;
	case 0x100000: /* Royal Blood (1MB) */
		state->bank_mask = 127;
		state->cart.sram.sram_mask = 128;
		state->cart.sram.empty_mask = 0;
		break;
	default:
		logerror ("ascii8_sram: error: only exactly 64kb, 128kb, 256kb, "
				  "512kb and 1mb supported\n");
		return 1;
	}

	return 0;
}

MSX_SLOT_RESET(ascii8_sram)
{
	int i;

	for (i=0; i<4; i++) state->banks[i] = i;
}

static UINT8 *ascii8_sram_bank_select (slot_state *state, int bankno)
{
	int bank = state->banks[bankno];

	if (bank & state->cart.sram.empty_mask) {
		return msx1.empty;
	}
	else if (bank & state->cart.sram.sram_mask) {
		return state->cart.sram.mem;
	}
	else {
		return state->mem + (bank & state->banks[bankno]) * 0x2000;
	}
}

MSX_SLOT_MAP(ascii8_sram)
{
	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		cpu_setbank (3, ascii8_sram_bank_select (state, 0));
		cpu_setbank (4, ascii8_sram_bank_select (state, 1));
		break;
	case 2:
		cpu_setbank (5, ascii8_sram_bank_select (state, 2));
		cpu_setbank (6, ascii8_sram_bank_select (state, 3));
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(ascii8_sram)
{
	int bank;

	if (addr >= 0x2000 && addr < 0x4000) {
		bank = (addr / 0x800) & 3;

		state->banks[bank] = val & state->bank_mask;
		if (bank <= 1) {
			slot_ascii8_map (state, 1);
		}
		else if (msx1.state[2] == state) {
			slot_ascii8_map (state, 2);
		}
	}
	if (addr >= 0x8000 && addr < 0xc000) {
		bank = addr < 0xa000 ? 2 : 3;
		if (state->banks[bank] & state->cart.sram.sram_mask) {
			state->cart.sram.mem[addr & 0x1fff] = val;
		}
	}
}

MSX_SLOT_LOADSRAM(ascii8_sram)
{
	void *f;

	if (!state->sramfile) {
		logerror ("ascii8_sram: error: no sram filename provided\n");
		return 1;
	}

	f = mame_fopen (Machine->gamedrv->name, state->sramfile, 
					FILETYPE_MEMCARD, 0);
	if (f) {
		if (mame_fread (f, state->cart.sram.mem, 0x2000) == 0x2000) {
			mame_fclose (f);
			logerror ("ascii8_sram: info: sram loaded\n");
			return 0;
		}
		mame_fclose (f);
		memset (state->cart.sram.mem, 0, 0x2000);
		logerror ("ascii8_sram: warning: could not read sram file\n");
		return 1;
	}

	logerror ("ascii8_sram: warning: could not open sram file for reading\n");

	return 1;
}

MSX_SLOT_SAVESRAM(ascii8_sram)
{
	void *f;

	if (!state->sramfile) {
		return 0;
	}

	f = mame_fopen (Machine->gamedrv->name, state->sramfile,
					FILETYPE_MEMCARD, 1);
	if (f) {
		mame_fwrite (f, state->cart.sram.mem, 0x2000);
		mame_fclose (f);
		logerror ("ascii8_sram: info: sram saved\n");

		return 0;
	}

	logerror ("ascii8_sram: warning: could not open sram file for saving\n");

	return 1;
}

MSX_SLOT_INIT(ascii16_sram)
{
	state->cart.sram.mem = auto_malloc (0x4000);
	if (!state->cart.sram.mem) {
		logerror ("ascii16_sram: error: failed to malloc sram memory\n");
		return 1;
	}
	memset (state->cart.sram.mem, 0, 0x4000);
	state->type = SLOT_ASCII16_SRAM;
	state->mem = mem;
	state->size = size;

	switch (size) {
	case 0x10000:
		state->bank_mask = 3;
		state->cart.sram.sram_mask = 4;
		state->cart.sram.empty_mask = 0xf8;
		break;
	case 0x20000:
		state->bank_mask = 7;
		state->cart.sram.sram_mask = 8;
		state->cart.sram.empty_mask = 0xf0;
		break;
	case 0x40000:
		state->bank_mask = 15;
		state->cart.sram.sram_mask = 16;
		state->cart.sram.empty_mask = 0xe0;
		break;
	case 0x80000:
		state->bank_mask = 31;
		state->cart.sram.sram_mask = 32;
		state->cart.sram.empty_mask = 0xc0;
		break;
	case 0x100000: 
		state->bank_mask = 63;
		state->cart.sram.sram_mask = 64;
		state->cart.sram.empty_mask = 0xc0;
		break;
	default:
		logerror ("ascii16_sram: error: only exactly 64kb, 128kb, 256kb, "
				  "512kb and 1mb supported\n");
		return 1;
	}

	return 0;
}

MSX_SLOT_RESET(ascii16_sram)
{
	int i;

	for (i=0; i<4; i++) state->banks[i] = i;
}
static UINT8 *ascii16_sram_bank_select (slot_state *state, int bankno)
{
	int bank = state->banks[bankno];

	if (bank & state->cart.sram.empty_mask) {
		return msx1.empty;
	}
	else if (bank & state->cart.sram.sram_mask) {
		return state->cart.sram.mem;
	}
	else {
		return state->mem + (bank & state->banks[bankno]) * 0x4000;
	}
}

MSX_SLOT_MAP(ascii16_sram)
{
	UINT8 *mem;

	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		mem = ascii16_sram_bank_select (state, 0);
		cpu_setbank (3, mem);
		cpu_setbank (4, mem + 0x2000);
		break;
	case 2:
		mem = ascii16_sram_bank_select (state, 1);
		cpu_setbank (5, mem);
		cpu_setbank (6, mem + 0x2000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(ascii16_sram)
{
	if (addr >= 0x6000 && addr < 0x6800) {
		state->banks[0] = val;
		slot_ascii16_map (state, 1);
	}
	else if (addr >= 0x7000 && addr < 0x7800) {
		state->banks[1] = val;
		if (msx1.state[2] == state) {
			slot_ascii16_map (state, 2);
		}
	}
	else if (addr >= 0x8000 && addr < 0xc000) {
		if (state->banks[1] & state->cart.sram.sram_mask) {
			int offset, i;

			offset = addr & 0x07ff;
			for (i=0; i<8; i++) {
				state->cart.sram.mem[offset] = val;
				offset += 0x0800;
			}
		}
	}
}

MSX_SLOT_LOADSRAM(ascii16_sram)
{
	void *f;
	UINT8 *p;

	if (!state->sramfile) {
		logerror ("ascii16_sram: error: no sram filename provided\n");
		return 1;
	}

	f = mame_fopen (Machine->gamedrv->name, state->sramfile, 
					FILETYPE_MEMCARD, 0);
	if (f) {
		p = state->cart.sram.mem;

		if (mame_fread (f, state->cart.sram.mem, 0x200) == 0x000) {
			int offset, i;
			
			mame_fclose (f);

			offset = 0;
			for (i=0; i<7; i++) {
				memcpy (p + 0x800, p, 0x800);
				p += 0x800;
			}

			logerror ("ascii16_sram: info: sram loaded\n");
			return 0;
		}
		mame_fclose (f);
		memset (state->cart.sram.mem, 0, 0x4000);
		logerror ("ascii16_sram: warning: could not read sram file\n");
		return 1;
	}

	logerror ("ascii16_sram: warning: could not open sram file for reading\n");

	return 1;
}

MSX_SLOT_SAVESRAM(ascii16_sram)
{
	void *f;

	if (!state->sramfile) {
		return 0;
	}

	f = mame_fopen (Machine->gamedrv->name, state->sramfile,
					FILETYPE_MEMCARD, 1);
	if (f) {
		mame_fwrite (f, state->cart.sram.mem, 0x200);
		mame_fclose (f);
		logerror ("ascii16_sram: info: sram saved\n");

		return 0;
	}

	logerror ("ascii16_sram: warning: could not open sram file for saving\n");

	return 1;
}

MSX_SLOT_INIT(rtype)
{
	if (state->size != 0x60000) {
		logerror ("rtype: error: rom file should be exactly 384kb\n");
		return 1;
	}

	state->type = SLOT_RTYPE;
	state->mem = mem;
	state->size = size;

	return 0;
}

MSX_SLOT_RESET(rtype)
{
	state->banks[0] = 15;
	state->banks[1] = 15;
}

MSX_SLOT_MAP(rtype)
{
	UINT8 *mem;

	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		mem = state->mem + state->banks[0] * 0x4000;
		cpu_setbank (3, mem);
		cpu_setbank (4, mem + 0x2000);
		break;
	case 2:
		mem = state->mem + state->banks[1] * 0x4000;
		cpu_setbank (5, mem);
		cpu_setbank (6, mem + 0x2000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(rtype)
{
	if (addr >= 0x7000 && addr < 0x8000) {
		int data ;

		if (val & 0x10) {
			data = 0x10 | (val & 7);
		}
		else {
			data = val & 0x0f;
		}
		state->banks[1] = data;
		if (msx1.state[2] == state) {
			slot_rtype_map (state, 2);
		}
	}
}

MSX_SLOT_INIT(gmaster2)
{
	UINT8 *p;

	if (size != 0x20000) {
		logerror ("gmaster2: error: rom file should be 128kb\n");
		return 1;
	}
	state->type = SLOT_GAMEMASTER2;
	state->size = size;
	state->mem = mem;

	p = auto_malloc (0x4000);
	if (!p) {
		logerror ("gmaster2: error: out of memory\n");
		return 1;
	}

	memset (p, 0, 0x4000);
	state->cart.sram.mem = p;

	return 0;
}

MSX_SLOT_RESET(gmaster2)
{
	int i;

	for (i=0; i<4; i++) {
		state->banks[i] = i;
	}
}

MSX_SLOT_MAP(gmaster2)
{
	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		cpu_setbank (3, state->mem); /* bank 0 is hardwired */
		if (state->banks[1] > 15) {
			cpu_setbank (4, state->cart.sram.mem + 
					(state->banks[2] - 16) * 0x2000);
		}
		else {
			cpu_setbank (4, state->mem + state->banks[1] * 0x2000);
		}
		break;
	case 2:
		if (state->banks[2] > 15) {
			cpu_setbank (5, state->cart.sram.mem +
					(state->banks[2] - 16) * 0x2000);
		}
		else {
			cpu_setbank (5, state->mem + state->banks[2] * 0x2000);
		}
		if (state->banks[3] > 15) {
			cpu_setbank (6, state->cart.sram.mem +
					(state->banks[2] - 16) * 0x2000);
		}
		else {
			cpu_setbank (5, state->mem + state->banks[3] * 0x2000);
		}
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(gmaster2)
{
	if (addr >= 0x6000 && addr < 0x7000) {
		if (val & 0x10) {
			val = val & 0x20 ? 17 : 16;
		}
		else {
			val = val & 15;
		}
		state->banks[1] = val;
		slot_gmaster2_map (state, 1);
	}
	else if (addr >= 0x8000 && addr < 0x9000) {
		if (val & 0x10) {
			val = val & 0x20 ? 17 : 16;
		}
		else {
			val = val & 15;
		}
		state->banks[2] = val;
		slot_gmaster2_map (state, 2);
	}
	else if (addr >= 0xa000 && addr < 0xb000) {
		if (val & 0x10) {
			val = val & 0x20 ? 17 : 16;
		}
		else {
			val = val & 15;
		}
		state->banks[3] = val;
		slot_gmaster2_map (state, 2);
	}
	else if (addr >= 0xb000 && addr < 0xc000) {
		addr &= 0x0fff;
		switch (state->banks[3]) {
		case 16:
			state->cart.sram.mem[addr] = val;
			state->cart.sram.mem[addr + 0x1000] = val;
			break;
		case 17:
			state->cart.sram.mem[addr + 0x2000] = val;
			state->cart.sram.mem[addr + 0x3000] = val;
			break;
		}
	}
}

MSX_SLOT_LOADSRAM(gmaster2)
{
	void *f;
	UINT8 *p;

	p = state->cart.sram.mem;
	f = mame_fopen (Machine->gamedrv->name, state->sramfile, 
					FILETYPE_MEMCARD, 0);
	if (f) {
		if (mame_fread (f, p + 0x1000, 0x2000) == 0x2000) {
			memcpy (p, p + 0x1000, 0x1000);
			memcpy (p + 0x3000, p + 0x2000, 0x1000);
			mame_fclose (f);
			logerror ("gmaster2: info: sram loaded\n");
			return 0;
		}
		mame_fclose (f);
		memset (p, 0, 0x4000);
		logerror ("gmaster2: warning: could not read sram file\n");
		return 1;
	}

	logerror ("gmaster2: warning: could not open sram file for reading\n");

	return 1;
}

MSX_SLOT_SAVESRAM(gmaster2)
{
	void *f;

	f = mame_fopen (Machine->gamedrv->name, state->sramfile,
					FILETYPE_MEMCARD, 1);
	if (f) {
		mame_fwrite (f, state->cart.sram.mem + 0x1000, 0x2000);
		mame_fclose (f);
		logerror ("gmaster2: info: sram saved\n");

		return 0;
	}

	logerror ("gmaster2: warning: could not open sram file for saving\n");

	return 1;
}

MSX_SLOT_INIT(diskrom)
{
	if (size != 0x4000) {
		logerror ("diskrom: error: the diskrom should be 16kb\n");
		return 1;
	}

	state->type = SLOT_DISK_ROM;
	state->mem = mem;
	state->size = size;

	return 0;
}

MSX_SLOT_RESET(diskrom)
{
	/* improve: reset disk controller */
}

static READ_HANDLER (msx_diskrom_page1_r)
{
	switch (offset) {
	case 0x1ff8: return wd179x_status_r (0);
	case 0x1ff9: return wd179x_track_r (0);
	case 0x1ffa: return wd179x_sector_r (0);
	case 0x1ffb: return wd179x_data_r (0);
	case 0x1fff: return msx1.dsk_stat;
	default: return msx1.state[1]->mem[offset + 0x2000];
	}
}
																				
static READ_HANDLER (msx_diskrom_page2_r)
{
	if (offset >= 0x1ff8) {
		switch (offset) {
		case 0x1ff8: return wd179x_status_r (0);
		case 0x1ff9: return wd179x_track_r (0);
		case 0x1ffa: return wd179x_sector_r (0);
		case 0x1ffb: return wd179x_data_r (0);
		case 0x1fff: return msx1.dsk_stat;
		default: return msx1.state[2]->mem[offset + 0x2000];
		}
	} 
	else {
		return 0xff;
	}
}
																				

MSX_SLOT_MAP(diskrom)
{
	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		cpu_setbank (3, state->mem);
		cpu_setbank (4, state->mem + 0x2000);
		memory_set_bankhandler_r (4, 0, msx_diskrom_page1_r);
		break;
	case 2:
		cpu_setbank (5, msx1.empty);
		cpu_setbank (6, msx1.empty);
		memory_set_bankhandler_r (6, 0, msx_diskrom_page2_r);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(diskrom)
{
	if (addr >= 0xa000 && addr < 0xc000) {
		addr -= 0x4000;
	}

	switch (addr) {
	case 0x7ff8:
		wd179x_command_w (0, val);
		break;
	case 0x7ff9:
		wd179x_track_w (0, val);
		break;
	case 0x7ffa:
		wd179x_sector_w (0, val);
		break;
	case 0x7ffb:
		wd179x_data_w (0, val);
		break;
	case 0x7ffc:
		wd179x_set_side (val & 1);
		state->mem[0x3ffc] = val | 0xfe;
	case 0x7ffd:
		wd179x_set_drive (val & 1);
		if ((state->mem[0x3ffd] ^ val) & 2) {
			set_led_status (0, !(val & 2));
		}
		state->mem[0x3ffd] = val | 0x7c;
		break;
	}
}

MSX_SLOT_INIT(synthesizer)
{
	if (size != 0x8000) {
		logerror ("synthesizer: error: rom file must be 32kb\n");
		return 1;
	}
	state->type = SLOT_SYNTHESIZER;
	state->mem = mem;
	state->size = size;

	return 0;
}

MSX_SLOT_RESET(synthesizer)
{
	/* empty */
}

MSX_SLOT_MAP(synthesizer)
{
	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		cpu_setbank (3, state->mem);
		cpu_setbank (4, state->mem + 0x2000);
		break;
	case 2:
		cpu_setbank (5, state->mem + 0x4000);
		cpu_setbank (6, state->mem + 0x6000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(synthesizer)
{
	if (addr >= 0x4000 && addr < 0x8000 && !(addr & 0x0010)) {
		DAC_data_w (0, val);
	}
}

MSX_SLOT_INIT(majutsushi)
{
	if (size != 0x20000) {
		logerror ("majutsushi: error: rom file must be 128kb\n");
		return 1;
	}
	state->type = SLOT_MAJUTSUSHI;
	state->mem = mem;
	state->size = size;
	state->bank_mask = 0x0f;

	return 0;
}

MSX_SLOT_RESET(majutsushi)
{
	int i;

	for (i=0; i<4; i++) {
		state->banks[i] = i;
	}
} 

MSX_SLOT_MAP(majutsushi)
{
	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		cpu_setbank (3, state->mem + state->banks[0] * 0x2000);
		cpu_setbank (4, state->mem + state->banks[1] * 0x2000);
		break;
	case 2:
		cpu_setbank (5, state->mem + state->banks[2] * 0x2000);
		cpu_setbank (6, state->mem + state->banks[3] * 0x2000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(majutsushi)
{
	if (addr >= 0x5000 && addr < 0x6000) {
		DAC_data_w (0, val);
	}
	else if (addr >= 0x6000 && addr < 0x8000) {
		state->banks[1] = val & 0x0f;
		slot_majutsushi_map (state, 1);
	}
	else if (addr >= 0x8000 && addr < 0xa000) {
		state->banks[2] = val & 0x0f;
		slot_majutsushi_map (state, 2);
	}
	else if (addr >= 0xa000 && addr < 0xc000) {
		state->banks[3] = val & 0x0f;
		slot_majutsushi_map (state, 2);
	}
}

MSX_SLOT_INIT(fmpac)
{
	UINT8 *p;

	if (size != 0x10000) {
		logerror ("fmpac: error: rom file must be 64kb\n");
		return 1;
	}

	if (!strncmp ((char*)mem + 0x18, "PAC2", 4)) {
		state->cart.fmpac.sram_support = 1;
		p = auto_malloc (0x4000);
		if (!p) {
			logerror ("fmpac: error: out of memory\n");
			return 1;
		}
		memset (p, 0, 0x2000);
		memset (p + 0x2000, 0xff, 0x2000);
		state->cart.fmpac.mem = p;
	}
	else {
		state->cart.fmpac.sram_support = 0;
		state->cart.fmpac.mem = NULL;
	}

	state->type = SLOT_FMPAC;
	state->size = 0x10000;
	state->mem = mem;
	state->bank_mask = 3;

	return 0;
}

MSX_SLOT_RESET(fmpac)
{
	state->banks[0] = 0;
	state->cart.fmpac.sram_active = 0;
	state->cart.fmpac.opll_active = 0;
	/* IMPROVE: reset sound chip */
}

MSX_SLOT_MAP(fmpac)
{
	if (page != 1) {
		cpu_setbank (page * 2 + 1, msx1.empty);
		cpu_setbank (page * 2 + 2, msx1.empty);
	}
	else {
		if (state->cart.fmpac.sram_active) {
			cpu_setbank (3, state->cart.fmpac.mem);
			cpu_setbank (4, state->cart.fmpac.mem + 0x2000);
		}
		else {
			cpu_setbank (3, state->mem + state->banks[0] * 0x4000);
			cpu_setbank (4, state->mem + state->banks[0] * 0x4000 + 0x2000);
		}
	}
}

MSX_SLOT_WRITE(fmpac)
{
	if (state->cart.fmpac.sram_support) {
		if (addr >= 0x4000 && addr < 0x7ffe && state->cart.fmpac.sram_active) {
			state->cart.fmpac.mem[addr & 0x1fff] = val;
		}
		if (addr == 0x7ffe || addr == 0x7fff) {
			state->cart.fmpac.mem[addr & 0x1fff] = val;

			if (state->cart.fmpac.mem[0x1ffe] == 0x4d &&
				state->cart.fmpac.mem[0x1fff] == 0x69) {
				state->cart.fmpac.sram_active = 1;
			}
			else {
				state->cart.fmpac.sram_active = 0;
			}
		}
	}

	if (addr == 0x7ff4 && state->cart.fmpac.opll_active) {
		YM2413_register_port_0_w (0, val);
	}
	else if (addr == 0x7ff5 && state->cart.fmpac.opll_active) {
		YM2413_data_port_0_w (0, val);
	}
	else if (addr == 0x7ff6) {
		int data = val & 0x11;
		state->mem[0x3ff6] = data;
		state->mem[0x7ff6] = data;
		state->mem[0xbff6] = data;
		state->mem[0xfff6] = data;
		state->cart.fmpac.mem[0x3ff6] = data;
		state->cart.fmpac.opll_active = val & 1;
		msx1.opll_active = val & 1;
		logerror ("FM-PAC: OPLL %sactivated\n", val & 1 ? "" : "de");
	}
	else if (addr == 0x7ff7) {
		state->banks[0] = val & 3;
		state->cart.fmpac.mem[0x3ff7] = val & 3;
		slot_fmpac_map (state, 1);
	}
}

static char PAC_HEADER[] = "PAC2 BACKUP DATA";
#define PAC_HEADER_LEN (16)

MSX_SLOT_LOADSRAM(fmpac)
{
	void *f;
	char buf[PAC_HEADER_LEN];

	if (!state->cart.fmpac.sram_support) {
		logerror ("Your fmpac.rom does not support sram\n");
		return 1;
	}

	if (!state->sramfile) {
		logerror ("No sram filename provided\n");
		return 1;
	}

	f = mame_fopen (Machine->gamedrv->name, state->sramfile, 
				FILETYPE_MEMCARD, 0);
	if (f) {
		if ((mame_fread (f, buf, PAC_HEADER_LEN) == PAC_HEADER_LEN) &&
			!strncmp (buf, PAC_HEADER, PAC_HEADER_LEN) &&
			mame_fread (f, state->cart.fmpac.mem, 0x1ffe)) {
			logerror ("fmpac: info: sram loaded\n");
			mame_fclose (f);
			return 0;
		}
		else {
			logerror ("fmpac: warning: failed to load sram\n");
			mame_fclose (f);
			return 1;
		}
	}

	logerror ("fmpac: warning: could not open sram file\n");
	return 1;
}

MSX_SLOT_SAVESRAM(fmpac)
{
	void *f;

	if (!state->cart.fmpac.sram_support|| !state->sramfile) {
		return 0;
	}

	f = mame_fopen (Machine->gamedrv->name, state->sramfile,
				FILETYPE_MEMCARD, 1);
	if (f) {
		if ((mame_fwrite (f, PAC_HEADER, PAC_HEADER_LEN) == PAC_HEADER_LEN) &&
			(mame_fwrite (f, state->cart.fmpac.mem, 0x1ffe) == 0x1ffe)) {
			logerror ("fmpac: info: sram saved\n");
			mame_fclose (f);
			return 0;
		}
		else {
			logerror ("fmpac: warning: sram save to file failed\n");
			mame_fclose (f);
			return 1;
		}
	}

	logerror ("fmpac: warning: could not open sram file for writing\n");
	
	return 1;
}

MSX_SLOT_INIT(superloadrunner)
{
	if (size != 0x20000) {
		logerror ("superloadrunner: error: rom file should be exactly "
				  "128kb\n");
		return 1;
	}
	state->type = SLOT_SUPERLOADRUNNER;
	state->mem = mem;
	state->size = size;
	state->start_page = page;
	state->bank_mask = 7;

	return 0;
}

MSX_SLOT_RESET(superloadrunner)
{
	msx1.superloadrunner_bank = 0;
}

MSX_SLOT_MAP(superloadrunner)
{
	UINT8 *mem;

	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		cpu_setbank (3, state->mem);
		cpu_setbank (4, state->mem + 0x2000);
		break;
	case 2:
		mem = state->mem + 
				(msx1.superloadrunner_bank & state->bank_mask) * 0x4000;
		cpu_setbank (5, mem);
		cpu_setbank (6, mem + 0x2000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_INIT(crossblaim)
{
	if (size != 0x10000) {
		logerror ("crossblaim: error: rom file should be exactly 64kb\n");
		return 1;
	}
	state->type = SLOT_CROSS_BLAIM;
	state->mem = mem;
	state->size = size;

	return 0;
}

MSX_SLOT_RESET(crossblaim)
{
	int i;

	for (i=0; i<2; i++) {
		state->banks[i] = i;
	}
}

MSX_SLOT_MAP(crossblaim)
{
	UINT8 *mem;

	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		mem = state->mem + state->banks[0] * 0x4000;
		cpu_setbank (3, mem);
		cpu_setbank (4, mem + 0x2000);
		break;
	case 2:
		mem = state->mem + state->banks[1] * 0x4000;
		cpu_setbank (5, mem);
		cpu_setbank (6, mem + 0x2000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(crossblaim)
{
	if (addr == 0x4045) {
		state->banks[1] = val & 3;
		if (msx1.state[2] == state) {
			slot_crossblaim_map (state, 2);
		}
	}
}

MSX_SLOT_INIT(korean80in1)
{
	state->type = SLOT_KOREAN_80IN1;
	state->mem = mem;
	state->size = size;

	switch (size) {
	case 0x10000:
		state->bank_mask = 7;
		break;
	case 0x20000:
		state->bank_mask = 15;
		break;
	case 0x40000:
		state->bank_mask = 31;
		break;
	case 0x80000:
		state->bank_mask = 63;
		break;
	default:
		logerror ("korean-80in1: error: only 64kb, 128kb, 256kb, 512kb "
				  "supported\n");
		return 1;
	}

	return 0;
}

MSX_SLOT_RESET(korean80in1)
{
	int i;

	for (i=0; i<4; i++) {
		state->banks[i] = i;
	}
}

MSX_SLOT_MAP(korean80in1)
{
	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		cpu_setbank (3, state->mem + state->banks[0] * 0x2000);
		cpu_setbank (4, state->mem + state->banks[1] * 0x2000);
		break;
	case 2:
		cpu_setbank (5, state->mem + state->banks[2] * 0x2000);
		cpu_setbank (6, state->mem + state->banks[3] * 0x2000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(korean80in1)
{
	int bank;

	if (addr >= 0x4000 && addr < 0x4004) {
		bank = addr & 3;

		state->banks[bank] = val & state->bank_mask;
		if (bank <= 1) {
			slot_korean80in1_map (state, 1);
		}
		else if (msx1.state[2] == state) {
			slot_korean80in1_map (state, 2);
		}
	}
}

MSX_SLOT_INIT(korean126in1)
{
	state->type = SLOT_KOREAN_126IN1;
	state->mem = mem;
	state->size = size;

	switch (size) {
	case 0x10000:
		state->bank_mask = 3;
		break;
	case 0x20000:
		state->bank_mask = 7;
		break;
	case 0x40000:
		state->bank_mask = 15;
		break;
	case 0x80000:
		state->bank_mask = 31;
		break;
	default:
		logerror ("korean-126in1: error: only 64kb, 128kb, 256kb, 512kb "
				  "supported\n");
		return 1;
	}

	return 0;
}

MSX_SLOT_RESET(korean126in1)
{
	int i;

	for (i=0; i<2; i++) state->banks[i] = i;
}

MSX_SLOT_MAP(korean126in1)
{
	UINT8 *mem;

	switch (page) {
	case 0:
		cpu_setbank (1, msx1.empty);
		cpu_setbank (2, msx1.empty);
		break;
	case 1:
		mem = state->mem + state->banks[0] * 0x4000;
		cpu_setbank (3, mem);
		cpu_setbank (4, mem + 0x2000);
		break;
	case 2:
		mem = state->mem + state->banks[1] * 0x4000;
		cpu_setbank (5, mem);
		cpu_setbank (6, mem + 0x2000);
		break;
	case 3:
		cpu_setbank (7, msx1.empty);
		cpu_setbank (8, msx1.empty);
	}
}

MSX_SLOT_WRITE(korean126in1)
{
	if (addr >= 0x4000 && addr < 0x4002) {
		int bank = addr & 1;
		state->banks[bank] = val & state->bank_mask;
		if (bank == 0) {
			slot_korean126in1_map (state, 1);
		}
		else if (msx1.state[2] == state) {
			slot_korean126in1_map (state, 2);
		}
	}
}
MSX_SLOT_INIT(soundcartridge)
{
	UINT8 *p;

	p = auto_malloc (0x10000);
	if (!p) {
		logerror ("soundcartridge: error: failed to malloc ram\n");
		return 1;
	}
	memset (p, 0, 0x10000);

	state->mem = p;
	state->size = 0x10000;
	state->type = SLOT_SOUNDCARTRIDGE;

	return 0;
}

MSX_SLOT_RESET(soundcartridge)
{
	int i;

	for (i=0; i<4; i++) {
		state->banks[i] = i;
		state->cart.sccp.ram_mode[i] = 0;
		state->cart.sccp.banks_saved[i] = i;
	}
	state->cart.sccp.mode = 0;
	state->cart.sccp.scc_active = 0;
	state->cart.sccp.sccp_active = 0;
}

READ_HANDLER (soundcartridge_scc)
{
	int reg;

	if (offset < 0x1800 || offset >= 0x1fdf) {
		return msx1.state[2]->mem[msx1.state[2]->banks[2] * 0x2000 + offset];
	}

	reg = offset & 0xff;

	if (reg < 0x80) {
		return K051649_waveform_r (reg);
	}
	else if (reg < 0xa0) {
		/* nothing */
	}
	else if (reg < 0xc0) {
		/* read wave 5 */
		return K051649_waveform_r (0x80 + (reg & 0x1f));
	}
#if 0
	else if (reg < 0xe0) {
		/* write 0xff to deformation register */
	}
#endif

	return 0xff;
}

READ_HANDLER (soundcartridge_sccp)
{
	int reg;

	if (offset < 0x1800 || offset >= 0x1fdf) {
		return msx1.state[2]->mem[msx1.state[2]->banks[3] * 0x2000 + offset];
	}

	reg = offset & 0xff;

	if (reg < 0xa0) {
		return K051649_waveform_r (reg);
	}
#if 0
	else if (reg >= 0xc0 && reg < 0xe0) {
		/* write 0xff to deformation register */
	}
#endif

	return 0xff;
}

MSX_SLOT_MAP(soundcartridge)
{
	switch (page) {
	case 0:
		cpu_setbank (1, state->mem + state->banks[2] * 0x2000);
		cpu_setbank (2, state->mem + state->banks[3] * 0x2000);
		break;
	case 1:
		cpu_setbank (3, state->mem + state->banks[0] * 0x2000);
		cpu_setbank (4, state->mem + state->banks[1] * 0x2000);
		break;
	case 2:
		cpu_setbank (5, state->mem + state->banks[2] * 0x2000);
		cpu_setbank (6, state->mem + state->banks[3] * 0x2000);
	    memory_set_bankhandler_r (5, 0, 
			state->cart.sccp.scc_active ? soundcartridge_scc : MRA_BANK5);

	    memory_set_bankhandler_r (6, 0, 
			state->cart.sccp.sccp_active ? soundcartridge_sccp : MRA_BANK6);
		break;
	case 3:
		cpu_setbank (7, state->mem + state->banks[0] * 0x2000);
		cpu_setbank (8, state->mem + state->banks[1] * 0x2000);
		break;
	}
}

MSX_SLOT_WRITE(soundcartridge)
{
	int i;

	if (addr < 0x4000) {
		return;
	}
	else if (addr < 0x6000) {
		if (state->cart.sccp.ram_mode[0]) {
			state->mem[state->banks[0] * 0x2000 + (addr & 0x1fff)] = val;
		}
		else if (addr >= 0x5000 && addr < 0x5800) {
			state->banks[0] = val & 7;
			state->cart.sccp.banks_saved[0] = val;
			slot_soundcartridge_map (state, 1);
			if (msx1.state[3] == state) {
				slot_soundcartridge_map (state, 3);
			}
		}
	}
	else if (addr < 0x8000) {
		if (state->cart.sccp.ram_mode[1]) {
			state->mem[state->banks[1] * 0x2000 + (addr & 0x1fff)] = val;
		}
		else if (addr >= 0x7000 && addr < 0x7800) {
			state->banks[1] = val & 7;
			state->cart.sccp.banks_saved[1] = val;
			if (msx1.state[3] == state) {
				slot_soundcartridge_map (state, 3);
			}
			slot_soundcartridge_map (state, 1);
		}
	}
	else if (addr < 0xa000) {
		if (state->cart.sccp.ram_mode[2]) {
			state->mem[state->banks[2] * 0x2000 + (addr & 0x1fff)] = val;
		}
		else if (addr >= 0x9000 && addr < 0x9800) {
			state->banks[2] = val & 7;
			state->cart.sccp.banks_saved[2] = val;
			state->cart.sccp.scc_active = 
					(((val & 0x3f) == 0x3f) && !(state->cart.sccp.mode & 0x20));

			slot_soundcartridge_map (state, 2);
			if (msx1.state[0] == state) {
				slot_soundcartridge_map (state, 0);
			}
		}
		else if (addr >= 0x9800 && state->cart.sccp.scc_active) {
			int offset = addr & 0xff;

			if (offset < 0x80) {
				K051649_waveform_w (offset, val);
			}
			else if (offset < 0xa0) {
				offset &= 0xf;

				if (offset < 0xa) {
					K051649_frequency_w (offset, val);
				}
				else if (offset < 0x0f) {
					K051649_volume_w (offset - 0xa, val);
				}
				else if (offset == 0x0f) {
					K051649_keyonoff_w (0, val);
				}
			}
#if 0
			else if (offset < 0xe0) {
				/* write to deformation register */
			}
#endif
		}
	}
	else if (addr < 0xbffe) {
		if (state->cart.sccp.ram_mode[3]) {
			state->mem[state->banks[3] * 0x2000 + (addr & 0x1fff)] = val;
		}
		else if (addr >= 0xb000 && addr < 0xb800) {
			state->cart.sccp.banks_saved[3] = val;
			state->banks[3] = val & 7;
			state->cart.sccp.sccp_active = 
					(val & 0x80) && (state->cart.sccp.mode & 0x20);
			slot_soundcartridge_map (state, 2);
			if (msx1.state[0] == state) {
				slot_soundcartridge_map (state, 0);
			}
		}
		else if (addr >= 0xb800 && state->cart.sccp.sccp_active) {
			int offset = addr & 0xff;

			if (offset < 0xa0) {
				K052539_waveform_w (offset, val);
			}
			else if (offset < 0xc0) {
				offset &= 0x0f;

				if (offset < 0x0a) {
					K051649_frequency_w (offset, val);
				}
				else if (offset < 0x0f) {
					K051649_volume_w (offset - 0x0a, val);
				}
				else if (offset == 0x0f) {
					K051649_keyonoff_w (0, val);
				}
			}
#if 0
			else if (offset < 0xe0) {
				/* write to deformation register */
			}
#endif
		}
	}
	else if (addr < 0xc000) {
		/* write to mode register */
		if ((state->cart.sccp.mode ^ val) & 0x20) {
			logerror ("soundcartrige: changed to %s mode\n",
							val & 0x20 ? "scc+" : "scc");
		}
		state->cart.sccp.mode = val;
		if (val & 0x10) {
			/* all ram mode */
			for (i=0; i<4; i++) {
				state->cart.sccp.ram_mode[i] = 1;
			}
			state->cart.sccp.scc_active = 0;
			state->cart.sccp.sccp_active = 0;
		}
		else {
			state->cart.sccp.ram_mode[0] = val & 1;
			state->cart.sccp.ram_mode[1] = val & 2;
			state->cart.sccp.ram_mode[2] = (val & 4) && (val & 0x20);
			state->cart.sccp.ram_mode[3] = 0;

		}

		state->cart.sccp.scc_active = 
			(((state->cart.sccp.banks_saved[2] & 0x3f) == 0x3f) && 
		 	!(val & 0x20));

		state->cart.sccp.sccp_active = 
				((state->cart.sccp.banks_saved[3] & 0x80) && (val & 0x20));

		slot_soundcartridge_map (state, 2);
	}
}

MSX_SLOT_START
	MSX_SLOT_ROM (SLOT_EMPTY, empty)
	MSX_SLOT (SLOT_MSXDOS2, msxdos2)
	MSX_SLOT (SLOT_KONAMI_SCC, konami_scc)
	MSX_SLOT (SLOT_KONAMI, konami)
	MSX_SLOT (SLOT_ASCII8, ascii8)
	MSX_SLOT (SLOT_ASCII16, ascii16)
	MSX_SLOT_SRAM (SLOT_GAMEMASTER2, gmaster2)
	MSX_SLOT_SRAM (SLOT_ASCII8_SRAM, ascii8_sram)
	MSX_SLOT_SRAM (SLOT_ASCII16_SRAM, ascii16_sram)
	MSX_SLOT (SLOT_RTYPE, rtype)
	MSX_SLOT (SLOT_MAJUTSUSHI, majutsushi)
	MSX_SLOT_SRAM (SLOT_FMPAC, fmpac)
	MSX_SLOT_ROM (SLOT_SUPERLOADRUNNER, superloadrunner)
	MSX_SLOT (SLOT_SYNTHESIZER, synthesizer)
	MSX_SLOT (SLOT_CROSS_BLAIM, crossblaim)
	MSX_SLOT (SLOT_DISK_ROM, diskrom)
	MSX_SLOT (SLOT_KOREAN_80IN1, korean80in1)
	MSX_SLOT (SLOT_KOREAN_126IN1, korean126in1)
	MSX_SLOT (SLOT_SOUNDCARTRIDGE, soundcartridge)
	MSX_SLOT_ROM (SLOT_ROM, rom)
	MSX_SLOT_RAM (SLOT_RAM, ram)
	MSX_SLOT_RAM (SLOT_RAM_MM, rammm)
MSX_SLOT_END

