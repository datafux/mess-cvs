#include <stdarg.h> /* prolly should go in xmame.h */
#include "xmame.h"
#include "driver.h"
#include "audit.h"
#include "info.h"
#include "hash.h"
#include "sound/samples.h"

#ifdef MESS
#include "infomess.h"
#endif

static int frontend_list_clones(const char *gamename);
static int frontend_list_cpu(void);
static int frontend_list_gamelistheader(void);
static int frontend_list_hash(int type);
static void namecopy(char *name_ref, const char *desc, int buf_size);

static int list       = 0;
static int listclones = 1;
static int verbose    = 1;
static int correct    = 0;
static int incorrect  = 0;
static int not_found  = 0;
static int sortby     = 0;

enum {
	/* standard list commands */
	LIST_LIST = 1, LIST_FULL, LIST_GAMES, LIST_DETAILS, LIST_GAMELIST,
	LIST_SOURCEFILE, LIST_COLORS,
#ifdef MESS
	/* MESS-specific commands */
	LIST_MESSDEVICES, 
#endif
	LIST_ROMSIZE, LIST_PALETTESIZE, LIST_ROMS, LIST_CRC, LIST_SHA1, 
	LIST_MD5, LIST_SAMPLES, LIST_SAMDIR, 
	VERIFY_ROMS, VERIFY_ROMSETS, VERIFY_SAMPLES, VERIFY_SAMPLESETS,
	/* internal verification list commands (developers only) */
	LIST_MISSINGROMS, LIST_DUPCRC, LIST_WRONGORIENTATION, LIST_WRONGMERGE,
	LIST_WRONGFPS,
	/* standard list commands which require special handling */
	LIST_CLONES, LIST_XML, LIST_CPU
};
   
/* Mame frontend interface & commandline */
/* parsing rountines by Maurizio Zanello */

struct rc_option frontend_list_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Frontend Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "list", "l", rc_set_int, &list, NULL, LIST_LIST, 0, NULL, "List supported games matching gamename, or all, gamename may contain * and ? wildcards" },
	{ "listfull", "lf", rc_set_int, &list, NULL, LIST_FULL, 0, NULL, "Like -list, with full description" },
	{ "listgames", "lg", rc_set_int, &list, NULL, LIST_GAMES, 0, NULL, "Like -list, with manufacturer and year" },
	{ "listdetails", "ld", rc_set_int, &list, NULL, LIST_DETAILS, 0, NULL, "Like -list, with detailed info" },
	{ "listgamelist", "lgl", rc_set_int, &list, NULL, LIST_GAMELIST, 0, NULL, "Like -list, with specially formatted extra info for generating gamelist.mame" },
	{ "listsourcefile", "lsf", rc_set_int, &list, NULL, LIST_SOURCEFILE, 0, NULL, "Like -list, with driver sourcefile" },
	{ "listcolors", "lcol", rc_set_int, &list, NULL, LIST_COLORS, 0, NULL, "Like -list, with the number of colors used" },
#ifdef MESS
	{ "listdevices", NULL, rc_set_int, &list, NULL, LIST_MESSDEVICES, 0, NULL, "list available devices" },
#endif
	{ "listromsize", "lrs", rc_set_int, &list, NULL, LIST_ROMSIZE, 0, NULL, "Like -list, with the year and size of the roms used" },
	{ "listpalettesize", "lps", rc_set_int, &list, NULL, LIST_PALETTESIZE, 0, NULL, "Like -list, with the year and palette size of the roms used" },
	{ "listroms", "lr", rc_set_int, &list, NULL, LIST_ROMS, 0, NULL, "Like -list, but lists used ROMs" },
	{ "listcrc", "lcrc", rc_set_int, &list, NULL, LIST_CRC, 0, NULL, "Like -list, but lists used ROMs with CRC-32s" },
	{ "listsha1", "lsha1", rc_set_int, &list, NULL, LIST_SHA1, 0, NULL, "Like -list, but lists used ROMs with SHA-1s" },
	{ "listmd5", "lmd5", rc_set_int, &list, NULL, LIST_MD5, 0, NULL, "Like -list, but lists used ROMs with MD5s" },
#if (HAS_SAMPLES)
	{ "listsamples", "ls", rc_set_int, &list, NULL, LIST_SAMPLES, 0, NULL, "Like -list, but lists used audio samples" },
	{ "listsamdir", "lsd", rc_set_int, &list, NULL, LIST_SAMDIR, 0, NULL, "Like -list, but lists dir where samples are taken from" },
#endif   
	{ "verifyroms", "vr", rc_set_int, &list, NULL, VERIFY_ROMS, 0, NULL, "Verify ROMs for games matching gamename, or all, gamename may contain * and ? wildcards" },
	{ "verifyromsets", "vrs", rc_set_int, &list, NULL, VERIFY_ROMSETS, 0, NULL, "Like -verifyroms, but less verbose" },
#if (HAS_SAMPLES)
	{ "verifysamples", "vs", rc_set_int, &list, NULL, VERIFY_SAMPLES, 0, NULL, "Like -verifyroms but verify audio samples instead" },
	{ "verifysamplesets", "vss", rc_set_int, &list, NULL, VERIFY_SAMPLESETS, 0, NULL, "Like -verifysamples, but less verbose" },
#endif
	{ "clones", "cl", rc_bool, &listclones, "1", 0, 0, NULL, "Show / don't show bootlegs/clones in the above list commands" },
	{ "listclones", "lcl", rc_set_int, &list, NULL, LIST_CLONES, 0, NULL, "Like -list, but lists the clones of the specified game" },
	{ "listxml", "lx", rc_set_int, &list, NULL, LIST_XML, 0, NULL, "List all available info on drivers in XML format" },
	{ "listcpu", "lc", rc_set_int, &list, NULL, LIST_CPU, 0, NULL, "List cpu usage statics per year" },
	{ "Internal verification list commands (only for developers)", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
#ifdef MAME_DEBUG /* do not put this into a public release! */
	{ "listmissingroms", "lmr", rc_set_int, &list, NULL, LIST_MISSINGROMS, 0, NULL, "Like -list, but lists ROMs missing" },
#endif
	{ "listdupcrc", "ldc", rc_set_int, &list, NULL, LIST_DUPCRC, 0, NULL, "Like -list, but lists ROMs with identical crc" },
	{ "listwrongorientation", "lwo", rc_set_int, &list, NULL, LIST_WRONGORIENTATION, 0, NULL, "Like -list, but lists any games which use the orientation flags wrongly" },
	{ "listwrongmerge", "lwm", rc_set_int, &list, NULL, LIST_WRONGMERGE, 0, NULL, "Like -list, but lists any games which use the clone_of field wrongly" },
	{ "listwrongfps", "lwf", rc_set_int, &list, NULL, LIST_WRONGFPS, 0, NULL, "Like -list, but lists any games which use the FPS field wrongly" },
	{ "sortname", "srtn", rc_set_int, &sortby, NULL, 1, 0, NULL, "Sort roms by name" },
	{ "sortdriver", "srtd", rc_set_int, &sortby, NULL, 2, 0, NULL, "Sort roms by driver name" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

int CLIB_DECL compare_names(const void *elem1, const void *elem2)
{
	int cmp;
	game_driver *drv1 = *(game_driver **)elem1;
	game_driver *drv2 = *(game_driver **)elem2;
	char name1[200], name2[200];
	namecopy(name1, drv1->description, 200);
	namecopy(name2, drv2->description, 200);
	cmp = strcasecmp(name1, name2);
	if (cmp == 0)
		cmp = strcasecmp(drv1->description, drv2->description);
	return cmp;
}

int CLIB_DECL compare_driver_names(const void *elem1, const void *elem2)
{
	game_driver *drv1 = *(game_driver **)elem1;
	game_driver *drv2 = *(game_driver **)elem2;
	return strcmp(drv1->name, drv2->name);
}

/* compare string[8] using standard(?) wildchars ('?' & '*')          */
/* for this to work correctly, the shells internal wildcard expansion */
/* mechanism has to be disabled, use quotes */
int strwildcmp(const char *sp1, const char *sp2)
{
	char s1[9], s2[9];
	int i, l1, l2;
	char *p;

	strncpy(s1, sp1, 8); s1[8] = 0; if (s1[0] == 0) strcpy(s1, "*");

	strncpy(s2, sp2, 8); s2[8] = 0; if (s2[0] == 0) strcpy(s2, "*");

	p = strchr(s1, '*');
	if (p)
	{
		for (i = p - s1; i < 8; i++) s1[i] = '?';
		s1[8] = 0;
	}

	p = strchr(s2, '*');
	if (p)
	{
		for (i = p - s2; i < 8; i++) s2[i] = '?';
		s2[8] = 0;
	}

	l1 = strlen(s1);
	if (l1 < 8)
	{
		for (i = l1 + 1; i < 8; i++) s1[i] = ' ';
		s1[8] = 0;
	}

	l2 = strlen(s2);
	if (l2 < 8)
	{
		for (i = l2 + 1; i < 8; i++) s2[i] = ' ';
		s2[8] = 0;
	}

	for (i = 0; i < 8; i++)
	{
		if (s1[i] == '?' && s2[i] != '?') s1[i] = s2[i];
		if (s2[i] == '?' && s1[i] != '?') s2[i] = s1[i];
	}

	return strcasecmp(s1, s2);
}

static void namecopy(char *name_ref, const char *desc, int buf_size)
{
	char name[200];

	strncpy(name, desc, 199);
	name[199] = 0;

	/* remove details in parenthesis */
	if (strstr(name, " ("))
		*strstr(name, " (") = 0;

	/* Move leading "The" to the end */
	if (strncmp(name, "The ", 4) == 0)
		snprintf(name_ref, buf_size, "%s, The", name + 4);
	else if (strncmp(name, "Le ", 3) == 0)
		snprintf(name_ref, buf_size, "%s, Le", name + 3);
	else
		snprintf(name_ref, buf_size, "%s", name);
}

static int myprintf(char *fmt, ...)
{
	int i = 0;
	va_list args;

	if(verbose)
	{
		va_start(args, fmt);
		i = vfprintf(stdout_file, fmt, args);
		va_end(args);
		fflush(stdout_file);
	}
	return i;
}

static void frontend_verify(int driver, int rom)
{
	int status;

	if(rom)
		status = audit_verify_roms(driver, (verify_printf_proc)myprintf);
	else
		status = audit_verify_samples(driver, (verify_printf_proc)myprintf);

	if (verbose)
		fprintf(stdout_file, "%s %s ", rom? "romset":"sampleset",
				drivers[driver]->name);
	else
		fprintf(stdout_file, "%-8s  ", drivers[driver]->name);

	switch (status)
	{
		case BEST_AVAILABLE:
			fprintf(stdout_file, "best available\n");
			correct++;
			break;
		case CORRECT:
			fprintf(stdout_file, "correct\n");
			correct++;
			break;
		case NOTFOUND:
		case CLONE_NOTFOUND:
			fprintf(stdout_file, "not found\n");
			not_found++;
			break;
		case INCORRECT:
			fprintf(stdout_file, "incorrect\n");
			incorrect++;
			break;
	}
	fflush(stdout_file);
}

static int frontend_uses_roms(int driver)
{
	const rom_entry *region, *rom;
	int total_roms = 0;

	for (region = rom_first_region(drivers[driver]); region; region = rom_next_region(region))
	{
		for (rom = rom_first_file(region); rom && (ROM_GETNAME(rom) ||
					ROM_GETOFFSET(rom) || ROM_GETLENGTH(rom)); rom = rom_next_file(rom))
		{
			if (ROM_GETNAME(rom) && ROM_GETNAME(rom) != (char *)-1)
			{
				total_roms++;
			}
		}
	}

	return total_roms;
}

char *get_description(int driver)
{
	static char description[BUF_SIZE];

	namecopy(description, drivers[driver]->description, BUF_SIZE);

	/* Print the additional description only if we are listing clones */
	if (listclones)
	{
		char *pdest;
		int result;
		int len = strlen(description);
		pdest = strchr(drivers[driver]->description, '(');
		result = pdest - drivers[driver]->description;
		if (pdest && result > 0 )
			snprintf(description + len, BUF_SIZE - len, " %s", strchr(drivers[driver]->description, '('));
	}

	return description;
}

int frontend_list(const char *gamename)
{
	int i, j = 0;
	const char *header[] = {
		/*** standard list commands ***/
		/* list             */ NAME" currently supports:\n",
		/* listfull         */ "name      description\n"
			"--------  -----------\n",
		/* listgames        */ "year manufacturer                         name\n"
			"---- ------------------------------------ --------------------------------\n",
		/* listdetails      */ "rom name driver     cpu 1    cpu 2    cpu 3    cpu 4    cpu 5    cpu 6    cpu 7    cpu 8     sound 1     sound 2     sound 3     sound 4     sound 5      name\n"
			"-------- ---------- -------- -------- -------- -------- -------- -------- -------- --------  ----------- ----------- ----------- ----------- ----------- --------------------------\n",
		/* listgamelist     */ "",
		/* listsourcefile   */ "name     source file\n"
			"-------- -----------\n",
		/* listcolors       */ "name      colors\n"
			"--------  ------\n",
#ifdef MESS
		/* listdevices      */ "",
#endif
		/* listromsize      */ "name    \tyear \tsize\n"
			"--------\t-----\t----\n",
		/* listpalettesize  */ "name    \tyear \tcolors\n"
			"--------\t-----\t----\n",
		/* listroms         */ "",
		/* listcrc          */ "CRC      filename     description\n"
			"-------- ------------ -----------\n",
		/* listsha1         */ "SHA1                                     filename     description\n"
			"---------------------------------------- ------------ -----------\n",
		/* listmd5          */ "MD5                              filename     description\n"
			"-------------------------------- ------------ -----------\n",
		/* listsamples      */ "",
		/* listsamdir       */ "name      samples dir\n"
			"--------  -----------\n",
		/* verifyroms       */ "",
		/* verifyromsets    */ "name      result\n"
			"--------  ------\n",
		/* verifysamples    */ "",
		/* verifysamplesets */ "name      result\n"
			"--------  ------\n",
		/* listclones       */ "",
		/* listxml          */ "",
		/* listcpu          */ "",
		/*** internal verification list commands (developers only) ***/
		/* listmissingroms  */ "name      clone of  description\n"
			"--------  --------  -----------\n",
		/* listdupcrc       */ "CRC      filename1    name1        filename2    name2\n"
			"-------- ------------ --------     ------------ --------\n",
		/* wrongorientation */ "",
		/* wrongmerge       */ "",
		/* wrongfps         */ "name      resolution  fps\n"
			"--------  ----------  -----------\n"
	};

	machine_config drv;
	const char *all_games = "*";
	int matching     = 0;
	int skipped      = 0;

	/* HACK: some options REQUIRE gamename field to work: default to "*" */
	if (!gamename || (strlen(gamename) == 0))
		gamename = all_games;

	/* 
	 * since the cpuintrf structure is filled dynamically now, we have to 
	 * init first
	 */
	cpuintrf_init();
	sndintrf_init();

	/* sort the list if requested */
	if (sortby)
	{
		int count = 0;

		/* first count the drivers */
		while (drivers[count])
			count++;

		/* qsort as appropriate */
		if (sortby == 1)
			qsort((void *)drivers, count, sizeof(drivers[0]), compare_names);
		else if (sortby == 2)
			qsort((void *)drivers, count, sizeof(drivers[0]), compare_driver_names);
	}

	/* listcommands which require special handling */
	switch (list)
	{
		/* no list requested */
		case 0:
			return 1234;
			/* listclones is a special case since the strwildcmp */
			/* also has to be done on clone_of. */
		case LIST_CLONES:
			return frontend_list_clones(gamename);
		case LIST_XML: /* list all info */
			init_resource_tracking();
			print_mame_xml( stdout_file, drivers, gamename );
			exit_resource_tracking();
			return OSD_OK;
		case LIST_CPU:
			return frontend_list_cpu();
		case LIST_CRC: /* list all CRC32 */
			fprintf(stdout_file, header[list - 1]);
			return frontend_list_hash(HASH_CRC);
		case LIST_SHA1: /* list all SHA1 */
			fprintf(stdout_file, header[list - 1]);
			return frontend_list_hash(HASH_SHA1);
		case LIST_MD5: /* list all MD5 */
			fprintf(stdout_file, header[list - 1]);
			return frontend_list_hash(HASH_MD5);
		case LIST_GAMELIST: /* list all of the games */
			frontend_list_gamelistheader(); /* display the header */
			break;
	}

	fprintf(stdout_file, header[list - 1]);

	for (i = 0; drivers[i]; i++)
	{
		const game_driver *clone_of_i = driver_get_clone(drivers[i]);
		expand_machine_driver(drivers[i]->drv, &drv);	
		if ( (listclones || clone_of_i == NULL ||
					(clone_of_i->flags & NOT_A_DRIVER)) &&
				!strwildcmp(gamename, drivers[i]->name) )
		{
			matching++;

			switch(list)
			{
				/*** standard list commands ***/
				case LIST_LIST: /* simple games list */
					fprintf(stdout_file, "%-8s", drivers[i]->name);
					if (!(matching % 8))
						fprintf(stdout_file, "\n");
					else
						fprintf(stdout_file, "  ");
					break;
				case LIST_FULL: /* games list with descriptions */
					fprintf(stdout_file, "%-10s\"%s\"\n", drivers[i]->name,
							get_description(i));
					break;
				case LIST_GAMES:
					{
						fprintf(stdout_file, "%-5s%-36s %s\n",
								drivers[i]->year,
								drivers[i]->manufacturer, get_description(i));
						break;
					}
				case LIST_DETAILS: /* A detailed MAMELIST.TXT type roms lister */
					/* First, the rom name */
					fprintf(stdout_file, "%-8s ",drivers[i]->name);

					/* source file (skip the leading path) */ 
					fprintf(stdout_file, "%-10s ", strrchr(drivers[i]->source_file, '/') + 1);

					/* Then, cpus */
					for (j = 0; j < MAX_CPU; j++)
					{
						const cpu_config *x_cpu = drv.cpu;
						fprintf(stdout_file, "%-8s ",cputype_name(x_cpu[j].cpu_type));
					}
					fprintf(stdout_file, " ");

					for (j = 0; j < MAX_SOUND; j++)
					{
						const sound_config *x_sound = drv.sound;
						fprintf(stdout_file, "%-11s ", sndtype_name(x_sound[j].sound_type));
					}

					/* Lastly, the name of the game and a \newline */
					fprintf(stdout_file, " %s\n", get_description(i));
					break;
				case LIST_GAMELIST:
					{
						fprintf(stdout_file, "| %-33.33s", get_description(i));

						if (drivers[i]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION))
						{
							const game_driver *maindrv;
							int foundworking;

							if (clone_of_i && !(clone_of_i->flags & NOT_A_DRIVER))
								maindrv = clone_of_i;
							else
								maindrv = drivers[i];

							foundworking = 0;
							j = 0;
							while (drivers[j])
							{
								const game_driver *clone_of_j = driver_get_clone(drivers[j]);
								if (drivers[j] == maindrv || clone_of_j == maindrv)
								{
									if (!(drivers[j]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)))
									{
										foundworking = 1;
										break;
									}
								}
								j++;
							}

							if (foundworking)
								fprintf(stdout_file, "| No(1) ");
							else
								fprintf(stdout_file, "|   No  ");
						}
						else
							fprintf(stdout_file, "|  Yes  ");

						if (drivers[i]->flags & GAME_WRONG_COLORS)
							fprintf(stdout_file, "|   No  ");
						else if (drivers[i]->flags & GAME_IMPERFECT_COLORS)
							fprintf(stdout_file, "| Close ");
						else
							fprintf(stdout_file, "|  Yes  ");

						{
							const char **samplenames = NULL;
#if (HAS_SAMPLES || HAS_VLM5030)
							for (j = 0; drv.sound[j].sound_type && j < MAX_SOUND; j++)
							{
#if (HAS_SAMPLES)
								if (drv.sound[j].sound_type == SOUND_SAMPLES)
								{
									samplenames = ((struct Samplesinterface *)drv.sound[j].config)->samplenames;
									break;
								}
#endif
							}
#endif
							if (drivers[i]->flags & GAME_NO_SOUND)
								fprintf(stdout_file, "|   No  ");
							else if (drivers[i]->flags & GAME_IMPERFECT_SOUND)
							{
								if (samplenames)
									fprintf(stdout_file, "|Part(2)");
								else
									fprintf(stdout_file, "|Partial");
							}
							else
							{
								if (samplenames)
									fprintf(stdout_file, "| Yes(2)");
								else
									fprintf(stdout_file, "|  Yes  ");
							}
						}

						if (drivers[i]->flags & GAME_NO_COCKTAIL)
							fprintf(stdout_file, "|   No  ");
						else
							fprintf(stdout_file, "|  Yes  ");

						fprintf(stdout_file, "| %-8s |\n",drivers[i]->name);

					}
					break;
				case LIST_SOURCEFILE:
					fprintf(stdout_file, "%-8s %s\n", drivers[i]->name,
							drivers[i]->source_file);
					break;
				case LIST_COLORS:
					fprintf(stdout_file, "%-8s  %d\n", drivers[i]->name,
							drv.total_colors);
					break;
#ifdef MESS
				case LIST_MESSDEVICES:
					frontend_listdevices(stdout_file);
					return 0;
					break;
#endif
				case LIST_ROMSIZE:
					{
						const rom_entry *region, *rom, *chunk;

						j = 0;
						for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
							for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
								for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
									j += ROM_GETLENGTH(chunk);

						fprintf(stdout_file, "%-8s\t%-5s\t%u\n", drivers[i]->name, drivers[i]->year, j);
					}
					break;

				case LIST_PALETTESIZE:
					{
						fprintf(stdout_file, "%-8s\t%-5s\t%u\n",drivers[i]->name,drivers[i]->year,drv.total_colors); 
					}
					break;

				case LIST_ROMS: /* game roms list */
					{
						const rom_entry *region, *rom, *chunk;
						char buf[512];

						if (!frontend_uses_roms(i))
						{
							skipped++;
							continue;
						}

						fprintf(stdout_file, "This is the list of the ROMs required for driver \"%s\".\n"
									"Name            Size Checksum\n", drivers[i]->name);
						for (region = drivers[i]->rom; region; region = rom_next_region(region))			
						{
							for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
							{
								const char *name = ROM_GETNAME(rom);
								const char *hash = ROM_GETHASHDATA(rom);
								int length = -1; /* default is for disks! */

								if (ROMREGION_ISROMDATA(region))
								{
									length = 0;
									for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
										length += ROM_GETLENGTH(chunk);
								}

								fprintf(stdout_file, "%-12s ", name);
								if (length >= 0)
									fprintf(stdout_file, "%7d",length);
								else
									fprintf(stdout_file, "       ");

								if (!hash_data_has_info(hash, HASH_INFO_NO_DUMP))
								{
									if (hash_data_has_info(hash, HASH_INFO_BAD_DUMP))
										fprintf(stdout_file, " BAD");

									hash_data_print(hash, 0, buf);
									fprintf(stdout_file, " %s", buf);
								}
								else
									fprintf(stdout_file, " NO GOOD DUMP KNOWN");

								fprintf(stdout_file, "\n");
							}
						}
					}
					break;
#if (HAS_SAMPLES || HAS_VLM5030)
				case LIST_SAMPLES: /* game samples list */
				case LIST_SAMDIR:  /* games list with samples directories */
					{
						int found = 0;

						for (j = 0; drv.sound[j].sound_type && j < MAX_SOUND; j++ )
						{
							const char **samplenames = NULL;
#if (HAS_SAMPLES)
							if( drv.sound[j].sound_type == SOUND_SAMPLES )
								samplenames = ((struct Samplesinterface *)drv.sound[j].config)->samplenames;
#endif                        
							if (samplenames && samplenames[0])
							{
								found = 1;

								if(list == LIST_SAMPLES)
								{
									int k = 0;

									while (samplenames[k] != 0)
									{
										fprintf(stdout_file, "%s\n", samplenames[k]);
										k++;
									}
								}
								else
								{
									fprintf(stdout_file, "%-10s",drivers[i]->name);
									if (samplenames[0][0] == '*')
										fprintf(stdout_file, "%s\n",samplenames[0]+1);
									else
										fprintf(stdout_file, "%s\n",drivers[i]->name);
								}
							}
						}
						if (!found)
							skipped++;
					}
					break;
				case VERIFY_SAMPLESETS:
					verbose = 0;
					/* fall through */
				case VERIFY_SAMPLES:
					{
						const char **samplenames = NULL;

						for( j = 0; drv.sound[j].sound_type && j < MAX_SOUND; j++ )
						{
#if (HAS_SAMPLES)
							if( drv.sound[j].sound_type == SOUND_SAMPLES )
								samplenames = ((struct Samplesinterface *)drv.sound[j].config)->samplenames;
#endif
						}

						/* ignore games that need no samples */
						if (samplenames == NULL || samplenames[0] == NULL)
							skipped++;
						else
							frontend_verify(i, 0);
					}
					break;
#endif
				case VERIFY_ROMSETS:
					verbose = 0;
					/* fall through */
				case VERIFY_ROMS:
					/* ignore games that need no roms */
					if (!frontend_uses_roms(i))
						skipped++;
					else
						frontend_verify(i, 1);
					break;

					/*** internal verification list commands (developers only) ***/
				case LIST_MISSINGROMS:
					if (audit_has_missing_roms (i))
					{
						fprintf(stdout_file, "%-10s%-10s%s\n", drivers[i]->name,
								clone_of_i ? clone_of_i->name : "",
								get_description(i));
						not_found++;
					}
					break;
				case LIST_DUPCRC:
					{
						const rom_entry *region, *rom;
						int found = 0;

						if (!frontend_uses_roms(i))
						{
							skipped++;
							continue;
						}

						for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
							for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
								if (!hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
									for (j = i + 1; drivers[j]; j++)
									{
										const rom_entry *region1, *rom1;

										for (region1 = rom_first_region(drivers[j]); region1; region1 = rom_next_region(region1))
											for (rom1 = rom_first_file(region1); rom1; rom1 = rom_next_file(rom1))
												if (strcmp(ROM_GETNAME(rom), ROM_GETNAME(rom1)) && hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(rom1), 0))
												{
													char temp[512];

													/* Print only the checksums available for both the roms */
													unsigned int functions = 
														hash_data_used_functions(ROM_GETHASHDATA(rom)) &
														hash_data_used_functions(ROM_GETHASHDATA(rom1));

													hash_data_print(ROM_GETHASHDATA(rom), functions, temp);
													fprintf(stdout_file, "%s\n", temp);
													fprintf(stdout_file, "  %-12s %-8s\n", ROM_GETNAME(rom), drivers[i]->name);
													fprintf(stdout_file, "  %-12s %-8s\n", ROM_GETNAME(rom1),drivers[j]->name);
													found = 1;
												}
									}

						if (found)
							incorrect++;
						else
							correct++;
					}
					break;
				case LIST_WRONGORIENTATION: /* list drivers which incorrectly use the orientation and visible area fields */
					if(!(drv.video_attributes & VIDEO_TYPE_VECTOR) &&
							((drv.screen[0].default_visible_area.max_x - drv.screen[0].default_visible_area.min_x + 1) <=
							 (drv.screen[0].default_visible_area.max_y - drv.screen[0].default_visible_area.min_y + 1)) &&
							/* list of valid exceptions */
							strcmp(drivers[i]->name,"crater") &&
							strcmp(drivers[i]->name,"mpatrol") &&
							strcmp(drivers[i]->name,"troangel") &&
							strcmp(drivers[i]->name,"travrusa") &&
							strcmp(drivers[i]->name,"kungfum") &&
							strcmp(drivers[i]->name,"battroad") &&
							strcmp(drivers[i]->name,"vigilant") &&
							strcmp(drivers[i]->name,"sonson") &&
							strcmp(drivers[i]->name,"brkthru") &&
							strcmp(drivers[i]->name,"darwin") &&
							strcmp(drivers[i]->name,"exprraid") &&
							strcmp(drivers[i]->name,"sidetrac") &&
							strcmp(drivers[i]->name,"targ") &&
							strcmp(drivers[i]->name,"spectar") &&
							strcmp(drivers[i]->name,"venture") &&
							strcmp(drivers[i]->name,"mtrap") &&
							strcmp(drivers[i]->name,"pepper2") &&
							strcmp(drivers[i]->name,"hardhat") &&
							strcmp(drivers[i]->name,"fax") &&
							strcmp(drivers[i]->name,"circus") &&
							strcmp(drivers[i]->name,"robotbwl") &&
							strcmp(drivers[i]->name,"crash") &&
							strcmp(drivers[i]->name,"ripcord") &&
							strcmp(drivers[i]->name,"starfire") &&
							strcmp(drivers[i]->name,"fireone") &&
							strcmp(drivers[i]->name,"renegade") &&
							strcmp(drivers[i]->name,"battlane") &&
							strcmp(drivers[i]->name,"megatack") &&
							strcmp(drivers[i]->name,"killcom") &&
							strcmp(drivers[i]->name,"challeng") &&
							strcmp(drivers[i]->name,"kaos") &&
							strcmp(drivers[i]->name,"formatz") &&
							strcmp(drivers[i]->name,"bankp") &&
							strcmp(drivers[i]->name,"liberatr") &&
							strcmp(drivers[i]->name,"toki") &&
							strcmp(drivers[i]->name,"stactics") &&
							strcmp(drivers[i]->name,"sprint1") &&
							strcmp(drivers[i]->name,"sprint2") &&
							strcmp(drivers[i]->name,"nitedrvr") &&
							strcmp(drivers[i]->name,"punchout") &&
							strcmp(drivers[i]->name,"spnchout") &&
							strcmp(drivers[i]->name,"armwrest") &&
							strcmp(drivers[i]->name,"route16") &&
							strcmp(drivers[i]->name,"stratvox") &&
							strcmp(drivers[i]->name,"irobot") &&
							strcmp(drivers[i]->name,"leprechn") &&
							strcmp(drivers[i]->name,"starcrus") &&
							strcmp(drivers[i]->name,"astrof") &&
							strcmp(drivers[i]->name,"tomahawk") &&
							strcmp(drivers[i]->name,"astrocde") &&
							strcmp(drivers[i]->name,"vic20") &&
							strcmp(drivers[i]->name,"vc20") &&
							strcmp(drivers[i]->name,"p2000t") &&
							strcmp(drivers[i]->name,"kim1"))
							{
								fprintf(stdout_file, "%s %dx%d\n",drivers[i]->name,
										drv.screen[0].default_visible_area.max_x - drv.screen[0].default_visible_area.min_x + 1,
										drv.screen[0].default_visible_area.max_y - drv.screen[0].default_visible_area.min_y + 1);
								incorrect++;
							} else correct++;
					break;
				case LIST_WRONGMERGE: /* list duplicate crc-32 with different ROM name in clone sets */
					{
						const rom_entry *region, *rom;
						int found = 0;

						if (!frontend_uses_roms(i))
						{
							skipped++;
							continue;
						}

						for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
							for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
								if (!hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
								{
									for (j = 0; drivers[j]; j++)
									{
										const game_driver *clone_of_j = driver_get_clone(drivers[j]);
										if (j != i && clone_of_j && (clone_of_j->flags & NOT_A_DRIVER) == 0 && (clone_of_j == drivers[i] || (i < j && clone_of_j == clone_of_i)))
										{
											const rom_entry *region1, *rom1;
											int match = 0;

											for (region1 = rom_first_region(drivers[j]); region1; region1 = rom_next_region(region1))
											{
												for (rom1 = rom_first_file(region1); rom1; rom1 = rom_next_file(rom1))
												{
													if (!strcmp(ROM_GETNAME(rom), ROM_GETNAME(rom1)))
													{
														if (!hash_data_has_info(ROM_GETHASHDATA(rom1), HASH_INFO_NO_DUMP) &&
																!hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(rom1), 0))
														{
															char temp[512];

															/* Print only the checksums available for both the roms */
															unsigned int functions = 
																hash_data_used_functions(ROM_GETHASHDATA(rom)) &
																hash_data_used_functions(ROM_GETHASHDATA(rom1));

															fprintf(stdout_file, "%s:\n", ROM_GETNAME(rom));

															hash_data_print(ROM_GETHASHDATA(rom), functions, temp);
															fprintf(stdout_file, "  %-8s: %s\n", drivers[i]->name, temp);

															hash_data_print(ROM_GETHASHDATA(rom1), functions, temp);
															fprintf(stdout_file, "  %-8s: %s\n", drivers[j]->name, temp);
															found = 1;
														}
														else
															match = 1;
													}
												}
											}

											if (match == 0)
											{
												for (region1 = rom_first_region(drivers[j]); region1; region1 = rom_next_region(region1))
												{
													for (rom1 = rom_first_file(region1); rom1; rom1 = rom_next_file(rom1))
													{
														if (strcmp(ROM_GETNAME(rom), ROM_GETNAME(rom1)) && hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(rom1), 0))
														{
															char buf[512];


															hash_data_print(ROM_GETHASHDATA(rom), 0, buf);
															fprintf(stdout_file, "%s\n", buf);
															fprintf(stdout_file, "    %-12s %-8s\n", ROM_GETNAME(rom),drivers[i]->name);
														}
													}
												}
											}

										}
									}
								}

						if (found)
							incorrect++;
						else
							correct++;
					}
					break;
				case LIST_WRONGFPS: /* list drivers with too high frame rate */
					if ((drv.video_attributes & VIDEO_TYPE_VECTOR) == 0 &&
							(clone_of_i == NULL ||
							 (clone_of_i->flags & NOT_A_DRIVER)) &&
							drv.screen[0].refresh_rate > 57 &&
							drv.screen[0].default_visible_area.max_y - drv.screen[0].default_visible_area.min_y + 1 > 244 &&
							drv.screen[0].default_visible_area.max_y - drv.screen[0].default_visible_area.min_y + 1 <= 256)
					{
						fprintf(stdout_file, "%-8s  %-4dx%4d   %fHz\n",
								drivers[i]->name,
								drv.screen[0].default_visible_area.max_x -
								drv.screen[0].default_visible_area.min_x + 1,
								drv.screen[0].default_visible_area.max_y -
								drv.screen[0].default_visible_area.min_y + 1,
								drv.screen[0].refresh_rate);
						incorrect++;
					}
					else
						correct++;
					break;
			}
			fflush(stdout_file);
		}
	}

	/* print footer for those -list options which need one */
	switch(list)
	{
		case LIST_GAMELIST:
			fprintf(stdout_file,
					"+----------------------------------+-------+-------+-------+-------+----------+\n\n"
					"(1) There are variants of the game (usually bootlegs) that work correctly\n"
#if (HAS_SAMPLES)
					"(2) Needs samples provided separately\n"
#endif
			       );
			break;
	}

	if (matching == 0)
	{
		fprintf(stderr_file, "Error: \"%s\" is not supported!\n", gamename);
		return 1;
	}

	fprintf(stdout_file, "\n\n");
	fprintf(stdout_file, "Total Supported: %d", i);
	if (matching != i)
	{
		fprintf(stdout_file, ", Matching \"%s\": %d\n", gamename, matching);
	}
	else
	{
		fprintf(stdout_file, "\n");
	}
	if (skipped) fprintf(stdout_file, "Displayed: %d, Skipped: %d, because they don't use any roms/samples/devices\n", matching-skipped, skipped);
	if (correct+incorrect) fprintf(stdout_file, "Found: %d, of which %d correct and %d incorrect\n", correct+incorrect, correct, incorrect);
	if (not_found) fprintf(stdout_file, "Not found: %d\n", not_found);
	fflush(stdout_file);

	if (incorrect > 0)
		return 2;
	else
		return 0;
}

static int frontend_list_clones(const char *gamename)
{
	/* listclones is a special case since the strwildcmp */
	/* also has to be done on clone_of. */
	int i;

	fprintf(stdout_file, "Name:    Clone of:\n");
	for (i = 0; drivers[i]; i++)
	{
		const game_driver *clone_of = driver_get_clone(drivers[i]);
		if(clone_of &&
				!(clone_of->flags & NOT_A_DRIVER) &&
				( !strwildcmp(gamename,drivers[i]->name) ||
				  !strwildcmp(gamename,clone_of->name)))
			fprintf(stdout_file, "%-8s %-8s\n",drivers[i]->name,clone_of->name);
	}
	return 0;
}

static int frontend_list_cpu(void)
{
	machine_config drv;
	int i, j;
	int year;

	for (j = 0; j < 3; j++)
		fprintf(stdout_file, "\t%d", 8 << j);

	fprintf(stdout_file, "\n");

	for (year = 1980; year <= 2000; year++)
	{
		int count[CPU_COUNT];
		int count_buswidth[3];

		for (j = 0; j < CPU_COUNT; j++)
			count[j] = 0;
		for (j = 0; j < 3; j++)
			count_buswidth[j] = 0;

		i = 0;

		while (drivers[i])
		{
			const game_driver *clone_of = driver_get_clone(drivers[i]);
			expand_machine_driver(drivers[i]->drv, &drv);	
			if (clone_of == NULL || (clone_of->flags & NOT_A_DRIVER))
			{
				const cpu_config *x_cpu = drv.cpu;

				if (atoi(drivers[i]->year) == year)
				{
					/*              for (j = 0; j < MAX_CPU; j++) */
					j = 0;  /* count only the main cpu */
					{
						count[x_cpu[j].cpu_type]++;
						switch (cputype_databus_width(x_cpu[j].cpu_type, ADDRESS_SPACE_PROGRAM))
						{
							case  8: count_buswidth[0]++; break;
							case 16: count_buswidth[1]++; break; 
							case 32: count_buswidth[2]++; break;
						}
					}
				}
			}
			i++;
		}

		fprintf(stdout_file, "%d", year);
		for (j = 0; j < 3; j++)
			fprintf(stdout_file, "\t%d", count_buswidth[j]);

		fprintf(stdout_file, "\n");
	}
	return OSD_OK;
}

static int frontend_list_gamelistheader(void)
{
	fprintf(stdout_file,
			"This is the complete list of games supported by %s %s.\n",
			NAME, build_version);
	if (!listclones)
		fprintf(stdout_file,
				"Variants of the same game are not included, you can use the -listclones command\n"
				"to get a list of the alternate versions of a given game.\n");
	fprintf(stdout_file, "\n"
			"This list is generated automatically and is not 100%% accurate (particularly in\n"
			"the Screen Flip column). Please let us know of any errors so we can correct\n"
			"them.\n"
			"\n"
			"Here are the meanings of the columns:\n");
	fprintf(stdout_file, "\n"
			"Working\n"
			"=======\n"
			"  NO: Emulation is still in progress; the game does not work correctly. This\n"
			"  means anything from major problems to a black screen.\n");
	fprintf(stdout_file, "\n"
			"Correct Colors\n"
			"==============\n"
			"    YES: Colors should be identical to the original.\n"
			"  CLOSE: Colors are nearly correct.\n"
			"     NO: Colors are completely wrong. \n");
	fprintf(stdout_file, "\n"
			"  Note: In some cases, the color PROMs for some games are not yet available.\n"
			"  This causes a NO GOOD DUMP KNOWN message on startup (and, of course, the game\n"
			"  has wrong colors). The game will still say YES in this column, however,\n"
			"  because the code to handle the color PROMs has been added to the driver. When\n"
			"  the PROMs are available, the colors will be correct.\n");
	fprintf(stdout_file, "\n"
			"Sound\n"
			"=====\n"
			"  PARTIAL: Sound support is incomplete or not entirely accurate. \n"
			"\n"
			"  Note: Some original games contain analog sound circuitry, which is difficult\n"
			"  to emulate. Thereforce, these emulated sounds may be significantly different.\n");
	fprintf(stdout_file, "\n"
			"Screen Flip\n"
			"===========\n"
			"  Many games were offered in cocktail-table models, allowing two players to sit\n"
			"  across from each other; the game's image flips 180 degrees for each player's\n"
			"  turn. Some games also have a \"Flip Screen\" DIP switch setting to turn the\n"
			"  picture (particularly useful with vertical games).\n"
			"  In many cases, this feature has not yet been emulated.\n");
	fprintf(stdout_file, "\n"
			"Internal Name\n"
			"=============\n"
			"  This is the unique name that must be used when running the game from a\n"
			"  command line.\n"
			"\n"
			"  Note: Each game's ROM set must be placed in the ROM path, either in a .zip\n"
			"  file or in a subdirectory with the game's Internal Name. The former is\n"
			"  suggested, because the files will be identified by their CRC instead of\n"
			"  requiring specific names.\n\n");
	fprintf(stdout_file, "+----------------------------------+-------+-------+-------+-------+----------+\n");
	fprintf(stdout_file, "|                                  |       |Correct|       |Screen | Internal |\n");
	fprintf(stdout_file, "| Game Name                        |Working|Colors | Sound | Flip  |   Name   |\n");
	fprintf(stdout_file, "+----------------------------------+-------+-------+-------+-------+----------+\n");
	return OSD_OK;
}

static int frontend_list_hash(int type)
{
	int i;
	for (i = 0; drivers[i]; i++)
	{
		const rom_entry *region, *rom;
		for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				char chksum[256];

				if (hash_data_extract_printable_checksum(ROM_GETHASHDATA(rom), type, chksum))
					fprintf(stdout_file, "%s %-12s %s\n", chksum, ROM_GETNAME(rom), drivers[i]->description);
			}
	}

	return OSD_OK;
}

/*============================================================ */
/*	osd_display_loading_rom_message */
/*============================================================ */

/* 
 * Called while loading ROMs.  It is called a last time with name == 0 to 
 * signal that the ROM loading process is finished.  return non-zero to abort 
 * loading
 */
int osd_display_loading_rom_message(const char *name,
		rom_load_data *romdata)
{
	static int count = 0;
	
	if (name)
		fprintf(stderr_file,"loading rom %d: %-32s\n", count, name);
	else
		fprintf(stderr_file,"done\n");
	
	fflush(stderr_file);
	count++;

	return 0;
}
