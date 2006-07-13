/***************************************************************************

	imgtool.c

	Core code for Imgtool

***************************************************************************/

#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "osdepend.h"
#include "imgtoolx.h"
#include "utils.h"
#include "library.h"
#include "modules.h"
#include "pool.h"



/***************************************************************************
    MACROS
***************************************************************************/

/* this is a temporary hack */
#define GET_IMAGE(partition)	((partition)->image)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct _imgtool_image
{
	const imgtool_module *module;
	memory_pool pool;
};

struct _imgtool_partition
{
	imgtool_image *image;
	int partition_index;
};

struct _imgtool_directory
{
	imgtool_image *image;
};



/***************************************************************************
    GLOBALS
***************************************************************************/

imgtool_library *global_imgtool_library;



/***************************************************************************

	Imgtool initialization and basics

***************************************************************************/

/*-------------------------------------------------
    markerrorsource - marks where an error source
-------------------------------------------------*/

static imgtoolerr_t markerrorsource(imgtoolerr_t err)
{
	assert(imgtool_error != NULL);

	switch(err)
	{
		case IMGTOOLERR_OUTOFMEMORY:
		case IMGTOOLERR_UNEXPECTED:
		case IMGTOOLERR_BUFFERTOOSMALL:
			/* Do nothing */
			break;

		case IMGTOOLERR_FILENOTFOUND:
		case IMGTOOLERR_BADFILENAME:
			err |= IMGTOOLERR_SRC_FILEONIMAGE;
			break;

		default:
			err |= IMGTOOLERR_SRC_IMAGEFILE;
			break;
	}
	return err;
}



/*-------------------------------------------------
    internal_error - debug function for raising
	internal errors
-------------------------------------------------*/

static void internal_error(const imgtool_module *module, const char *message)
{
#ifdef MAME_DEBUG
	logerror("%s: %s\n", module->name, message);
#endif
}



/*-------------------------------------------------
    imgtool_init - initializes the imgtool core
-------------------------------------------------*/

void imgtool_init(int omit_untested)
{
	imgtoolerr_t err;
	err = imgtool_create_cannonical_library(omit_untested, &global_imgtool_library);
	assert(err == IMGTOOLERR_SUCCESS);
	if (err == IMGTOOLERR_SUCCESS)
	{
		imgtool_library_sort(global_imgtool_library, ITLS_DESCRIPTION);
	}
}



/*-------------------------------------------------
    imgtool_exit - closes out the imgtool core
-------------------------------------------------*/

void imgtool_exit(void)
{
	if (global_imgtool_library)
	{
		imgtool_library_close(global_imgtool_library);
		global_imgtool_library = NULL;
	}
}



/*-------------------------------------------------
    imgtool_find_module - looks up a module
-------------------------------------------------*/

const imgtool_module *imgtool_find_module(const char *modulename)
{
	return imgtool_library_findmodule(global_imgtool_library, modulename);
}



/*-------------------------------------------------
    imgtool_get_module_features - retrieves a
	structure identifying this module's features
	associated with an image
-------------------------------------------------*/

imgtool_module_features imgtool_get_module_features(const imgtool_module *module)
{
	imgtool_module_features features;
	memset(&features, 0, sizeof(features));

	if (module->create)
		features.supports_create = 1;
	if (module->open)
		features.supports_open = 1;
	if (module->read_file)
		features.supports_reading = 1;
	if (module->write_file)
		features.supports_writing = 1;
	if (module->delete_file)
		features.supports_deletefile = 1;
	if (module->path_separator)
		features.supports_directories = 1;
	if (module->free_space)
		features.supports_freespace = 1;
	if (module->create_dir)
		features.supports_createdir = 1;
	if (module->delete_dir)
		features.supports_deletedir = 1;
	if (module->supports_creation_time)
		features.supports_creation_time = 1;
	if (module->supports_lastmodified_time)
		features.supports_lastmodified_time = 1;
	if (module->read_sector)
		features.supports_readsector = 1;
	if (module->write_sector)
		features.supports_writesector = 1;
	if (module->list_forks)
		features.supports_forks = 1;
	if (module->get_iconinfo)
		features.supports_geticoninfo = 1;
	if (!features.supports_writing && !features.supports_createdir && !features.supports_deletefile && !features.supports_deletedir)
		features.is_read_only = 1;
	return features;
}



/*-------------------------------------------------
    evaluate_module - evaluates a single file to
	determine what module can best handle a file
-------------------------------------------------*/

static imgtoolerr_t evaluate_module(const char *fname,
	const imgtool_module *module, float *result)
{
	imgtoolerr_t err;
	imgtool_image *image = NULL;
	imgtool_partition *partition = NULL;
	imgtool_directory *imageenum = NULL;
	imgtool_dirent ent;
	float current_result;

	*result = 0.0;

	err = imgtool_image_open(module, fname, OSD_FOPEN_READ, &image);
	if (err)
		goto done;

	if (image)
	{
		current_result = module->open_is_strict ? 0.9 : 0.5;

		err = imgtool_partition_open(image, 0, &partition);
		if (err)
			goto done;

		err = imgtool_directory_open(partition, NULL, &imageenum);
		if (err)
			goto done;

		memset(&ent, 0, sizeof(ent));
		do
		{
			err = imgtool_directory_get_next(imageenum, &ent);
			if (err)
				goto done;

			if (ent.corrupt)
				current_result = (current_result * 99 + 1.00) / 100;
			else
				current_result = (current_result + 1.00) / 2;
		}
		while(!ent.eof);

		*result = current_result;
	}

done:
	if (ERRORCODE(err) == IMGTOOLERR_CORRUPTIMAGE)
		err = IMGTOOLERR_SUCCESS;
	if (imageenum)
		imgtool_directory_close(imageenum);
	if (partition)
		imgtool_partition_close(partition);
	if (image)
		imgtool_image_close(image);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_image - retrieves the image
	associated with this partition
-------------------------------------------------*/

imgtool_image *imgtool_partition_image(imgtool_partition *partition)
{
	return partition->image;
}



/*-------------------------------------------------
    imgtool_identify_file - attempts to determine the module
	for any given image
-------------------------------------------------*/

imgtoolerr_t imgtool_identify_file(const char *fname, imgtool_module **modules, size_t count)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	imgtool_library *library = global_imgtool_library;
	imgtool_module *module = NULL;
	imgtool_module *insert_module;
	imgtool_module *temp_module;
	size_t i = 0;
	const char *extension;
	float val, temp_val, *values = NULL;

	if (count <= 0)
	{
		err = IMGTOOLERR_UNEXPECTED;
		goto done;
	}

	for (i = 0; i < count; i++)
		modules[i] = NULL;
	if (count > 1)
		count--;		/* null terminate */

	values = (float *) malloc(count * sizeof(*values));
	if (!values)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	for (i = 0; i < count; i++)
		values[i] = 0.0;

	/* figure out the file extension, if any */
	extension = strrchr(fname, '.');
	if (extension)
		extension++;

	/* iterate through all modules */
	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		if (!extension || findextension(module->extensions, extension))
		{
			err = evaluate_module(fname, module, &val);
			if (err)
				goto done;

			insert_module = module;
			for (i = 0; (val > 0.0) && (i < count); i++)
			{
				if (val > values[i])
				{
					temp_val = values[i];
					temp_module = modules[i];
					values[i] = val;
					modules[i] = insert_module;
					val = temp_val;
					insert_module = temp_module;
				}
			}
		}
	}

	if (!modules[0])
		err = IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_IMAGEFILE;

done:
	if (values)
		free(values);
	return err;
}



/*-------------------------------------------------
    imgtool_image_get_sector_size - gets the size of a sector
	on an image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_get_sector_size(imgtool_image *image, UINT32 track, UINT32 head,
	UINT32 sector, UINT32 *length)
{
	/* implemented? */
	if (!image->module->get_sector_size)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	return image->module->get_sector_size(image, track, head, sector, length);
}



/*-------------------------------------------------
    imgtool_image_read_sector - reads a sector on an image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_read_sector(imgtool_image *image, UINT32 track, UINT32 head,
	UINT32 sector, void *buffer, size_t len)
{
	/* implemented? */
	if (!image->module->read_sector)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	return image->module->read_sector(image, track, head, sector, buffer, len);
}



/*-------------------------------------------------
    imgtool_image_write_sector - writes a sector on an image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_write_sector(imgtool_image *image, UINT32 track, UINT32 head,
	UINT32 sector, const void *buffer, size_t len)
{
	/* implemented? */
	if (!image->module->write_sector)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	return image->module->write_sector(image, track, head, sector, buffer, len);
}



/*-------------------------------------------------
    imgtool_image_get_block_size - gets the size of a standard
	block on an image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_get_block_size(imgtool_image *image, UINT32 *length)
{
	/* implemented? */
	if (image->module->block_size == 0)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	*length = image->module->block_size;
	return IMGTOOLERR_SUCCESS;
}



/*-------------------------------------------------
    imgtool_image_read_block - reads a standard block on an
	image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_read_block(imgtool_image *image, UINT64 block, void *buffer)
{
	/* implemented? */
	if (!image->module->read_block)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	return image->module->read_block(image, buffer, block);
}



/*-------------------------------------------------
    imgtool_image_write_block - writes a standard block on an
	image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_write_block(imgtool_image *image, UINT64 block, const void *buffer)
{
	/* implemented? */
	if (!image->module->write_block)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	return image->module->write_block(image, buffer, block);
}



/*-------------------------------------------------
    imgtool_image_malloc - allocates memory associated with an
	image
-------------------------------------------------*/

void *imgtool_image_malloc(imgtool_image *image, size_t size)
{
	return pool_malloc(&image->pool, size);
}



/*-------------------------------------------------
    imgtool_image_module - returns the module associated with
	this image
-------------------------------------------------*/

const imgtool_module *imgtool_image_module(imgtool_image *img)
{
	return img->module;
}



/*-------------------------------------------------
    imgtool_image_extra_bytes - returns extra bytes on an image
-------------------------------------------------*/

void *imgtool_image_extra_bytes(imgtool_image *img)
{
	assert(img->module->image_extra_bytes > 0);
	return ((UINT8 *) img) + sizeof(*img);
}



/***************************************************************************

	Imgtool partition management

***************************************************************************/

imgtoolerr_t imgtool_partition_open(imgtool_image *image, int partition_index, imgtool_partition **partition)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	imgtool_partition *p;

	// allocate the new partition object
	p = (imgtool_partition *) malloc(sizeof(*p));
	if (!p)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(p, 0, sizeof(*p));

	// fill out the structure
	p->image = image;
	p->partition_index = partition_index;

done:
	*partition = err ? NULL : p;
	return err;
}



void imgtool_partition_close(imgtool_partition *partition)
{
	free(partition);
}



/***************************************************************************

	Imgtool partition operations

***************************************************************************/

/*-------------------------------------------------
    imgtool_partition_get_attribute_name - retrieves the human readable
	name for an attribute
-------------------------------------------------*/

void imgtool_partition_get_attribute_name(const imgtool_module *module, UINT32 attribute, const imgtool_attribute *attr_value,
	char *buffer, size_t buffer_len)
{
	imgtoolerr_t err = IMGTOOLERR_UNIMPLEMENTED;

	buffer[0] = '\0';

	if (attr_value)
	{
		if (module->attr_name)
			err = module->attr_name(attribute, attr_value, buffer, buffer_len);

		if (err == IMGTOOLERR_UNIMPLEMENTED)
		{
			switch(attribute & 0xF0000)
			{
				case IMGTOOLATTR_INT_FIRST:
					snprintf(buffer, buffer_len, "%d", (int) attr_value->i);
					break;
			}
		}
	}
	else
	{
		switch(attribute)
		{
			case IMGTOOLATTR_INT_MAC_TYPE:
				snprintf(buffer, buffer_len, "File type");
				break;
			case IMGTOOLATTR_INT_MAC_CREATOR:
				snprintf(buffer, buffer_len, "File creator");
				break;
			case IMGTOOLATTR_INT_MAC_FINDERFLAGS:
				snprintf(buffer, buffer_len, "Finder flags");
				break;
			case IMGTOOLATTR_INT_MAC_COORDX:
				snprintf(buffer, buffer_len, "X coordinate");
				break;
			case IMGTOOLATTR_INT_MAC_COORDY:
				snprintf(buffer, buffer_len, "Y coordinate");
				break;
			case IMGTOOLATTR_INT_MAC_FINDERFOLDER:
				snprintf(buffer, buffer_len, "Finder folder");
				break;
			case IMGTOOLATTR_INT_MAC_ICONID:
				snprintf(buffer, buffer_len, "Icon ID");
				break;
			case IMGTOOLATTR_INT_MAC_SCRIPTCODE:
				snprintf(buffer, buffer_len, "Script code");
				break;
			case IMGTOOLATTR_INT_MAC_EXTENDEDFLAGS:
				snprintf(buffer, buffer_len, "Extended flags");
				break;
			case IMGTOOLATTR_INT_MAC_COMMENTID:
				snprintf(buffer, buffer_len, "Comment ID");
				break;
			case IMGTOOLATTR_INT_MAC_PUTAWAYDIRECTORY:
				snprintf(buffer, buffer_len, "Putaway directory");
				break;
			case IMGTOOLATTR_TIME_CREATED:
				snprintf(buffer, buffer_len, "Creation time");
				break;
			case IMGTOOLATTR_TIME_LASTMODIFIED:
				snprintf(buffer, buffer_len, "Last modified time");
				break;
		}
	}
}



/*-------------------------------------------------
    imgtool_validitychecks - checks the validity
	of the imgtool modules
-------------------------------------------------*/

int imgtool_validitychecks(void)
{
	int error = 0;
	int val;
	imgtoolerr_t err;
	imgtool_library *library;
	const imgtool_module *module = NULL;
	const struct OptionGuide *guide_entry;
	imgtool_module_features features;

	err = imgtool_create_cannonical_library(TRUE, &library);
	if (err)
		goto done;

	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		features = imgtool_get_module_features(module);

		if (!module->name)
		{
			printf("imgtool module %s has null 'name'\n", module->name);
			error = 1;
		}
		if (!module->description)
		{
			printf("imgtool module %s has null 'description'\n", module->name);
			error = 1;
		}
		if (!module->extensions)
		{
			printf("imgtool module %s has null 'extensions'\n", module->extensions);
			error = 1;
		}

		/* sanity checks on modules that do not support directories */
		if (!module->path_separator)
		{
			if (module->alternate_path_separator)
			{
				printf("imgtool module %s specified alternate_path_separator but not path_separator\n", module->name);
				error = 1;
			}
			if (module->initial_path_separator)
			{
				printf("imgtool module %s specified initial_path_separator without directory support\n", module->name);
				error = 1;
			}
			if (module->create_dir)
			{
				printf("imgtool module %s implements create_dir without directory support\n", module->name);
				error = 1;
			}
			if (module->delete_dir)
			{
				printf("imgtool module %s implements delete_dir without directory support\n", module->name);
				error = 1;
			}
		}

		/* sanity checks on sector operations */
		if (module->read_sector && !module->get_sector_size)
		{
			printf("imgtool module %s implements read_sector without supporting get_sector_size\n", module->name);
			error = 1;
		}
		if (module->write_sector && !module->get_sector_size)
		{
			printf("imgtool module %s implements write_sector without supporting get_sector_size\n", module->name);
			error = 1;
		}

		/* sanity checks on creation options */
		if (module->createimage_optguide || module->createimage_optspec)
		{
			if (!module->create)
			{
				printf("imgtool module %s has creation options without supporting create\n", module->name);
				error = 1;
			}
			if ((!module->createimage_optguide && module->createimage_optspec)
				|| (module->createimage_optguide && !module->createimage_optspec))
			{
				printf("imgtool module %s does has partially incomplete creation options\n", module->name);
				error = 1;
			}

			if (module->createimage_optguide && module->createimage_optspec)
			{
				guide_entry = module->createimage_optguide;
				while(guide_entry->option_type != OPTIONTYPE_END)
				{
					if (option_resolution_contains(module->createimage_optspec, guide_entry->parameter))
					{
						switch(guide_entry->option_type)
						{
							case OPTIONTYPE_INT:
							case OPTIONTYPE_ENUM_BEGIN:
								err = option_resolution_getdefault(module->createimage_optspec,
									guide_entry->parameter, &val);
								if (err)
									goto done;
								break;

							default:
								break;
						}
						if (!guide_entry->identifier)
						{
							printf("imgtool module %s creation option %d has null identifier\n",
								module->name, (int) (guide_entry - module->createimage_optguide));
							error = 1;
						}
						if (!guide_entry->display_name)
						{
							printf("imgtool module %s creation option %d has null display_name\n",
								module->name, (int) (guide_entry - module->createimage_optguide));
							error = 1;
						}
					}
					guide_entry++;
				}
			}
		}
	}

done:
	if (err)
	{
		printf("imgtool: %s\n", imgtool_error(err));
		error = 1;
	}
	imgtool_exit();
	return error;
}



/*-------------------------------------------------
    imgtool_temp_str - provides a temporary string
	buffer used for string passing
-------------------------------------------------*/

char *imgtool_temp_str(void)
{
	static int index;
	static char temp_string_pool[32][256];
	return temp_string_pool[index++ % (sizeof(temp_string_pool) / sizeof(temp_string_pool[0]))];
}



/***************************************************************************

	Image handling functions

***************************************************************************/

static imgtoolerr_t internal_open(const imgtool_module *module, const char *fname,
	int read_or_write, option_resolution *createopts, imgtool_image **outimg)
{
	imgtoolerr_t err;
	imgtool_stream *f = NULL;
	imgtool_image *image = NULL;
	size_t size;

	if (outimg)
		*outimg = NULL;

	/* is the requested functionality implemented? */
	if ((read_or_write == OSD_FOPEN_RW_CREATE) ? !module->create : !module->open)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* open the stream */
	f = stream_open(fname, read_or_write);
	if (!f)
	{
		err = IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_IMAGEFILE;
		goto done;
	}

	/* setup the image structure */
	size = sizeof(struct _imgtool_image) + module->image_extra_bytes;
	image = (imgtool_image *) malloc(size);
	if (!image)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(image, '\0', size);
	pool_init(&image->pool);
	image->module = module;
	
	/* actually call create or open */
	if (read_or_write == OSD_FOPEN_RW_CREATE)
		err = module->create(image, f, createopts);
	else
		err = module->open(image, f);
	if (err)
	{
		err = markerrorsource(err);
		goto done;
	}

done:
	if (err)
	{
		if (f)
			stream_close(f);
		if (image)
		{
			free(image);
			image = NULL;
		}
	}

	if (outimg)
		*outimg = image;
	else if (image)
		imgtool_image_close(image);
	return err;
}



/*-------------------------------------------------
    imgtool_image_open - open an image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_open(const imgtool_module *module, const char *fname, int read_or_write, imgtool_image **outimg)
{
	read_or_write = read_or_write ? OSD_FOPEN_RW : OSD_FOPEN_READ;
	return internal_open(module, fname, read_or_write, NULL, outimg);
}



/*-------------------------------------------------
    imgtool_image_open_byname - open an image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_open_byname(const char *modulename, const char *fname, int read_or_write, imgtool_image **outimg)
{
	const imgtool_module *module;

	module = imgtool_find_module(modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return imgtool_image_open(module, fname, read_or_write, outimg);
}



/*-------------------------------------------------
    imgtool_image_close - close an image
-------------------------------------------------*/

void imgtool_image_close(imgtool_image *image)
{
	if (image->module->close)
		image->module->close(image);
	pool_exit(&image->pool);
	free(image);
}



/*-------------------------------------------------
    imgtool_image_create - creates an image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_create(const imgtool_module *module, const char *fname,
	option_resolution *opts, imgtool_image **image)
{
	imgtoolerr_t err;
	option_resolution *alloc_resolution = NULL;

	/* allocate dummy options if necessary */
	if (!opts && module->createimage_optguide)
	{
		alloc_resolution = option_resolution_create(module->createimage_optguide, module->createimage_optspec);
		if (!alloc_resolution)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		opts = alloc_resolution;
	}
	if (opts)
		option_resolution_finish(opts);

	err = internal_open(module, fname, OSD_FOPEN_RW_CREATE, opts, image);
	if (err)
		goto done;

done:
	if (alloc_resolution)
		option_resolution_close(alloc_resolution);
	return err;
}



/*-------------------------------------------------
    imgtool_image_create_byname - creates an image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_create_byname(const char *modulename, const char *fname,
	option_resolution *opts, imgtool_image **image)
{
	const imgtool_module *module;

	module = imgtool_find_module(modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return imgtool_image_create(module, fname, opts, image);
}



/*-------------------------------------------------
    imgtool_image_info - returns format specific information
	about an image
-------------------------------------------------*/

imgtoolerr_t imgtool_image_info(imgtool_image *image, char *string, size_t len)
{
	if (len > 0)
	{
		string[0] = '\0';
		if (image->module->info)
			image->module->info(image, string, len);
	}
	return IMGTOOLERR_SUCCESS;
}



#define PATH_MUSTBEDIR		0x00000001
#define PATH_LEAVENULLALONE	0x00000002
#define PATH_CANBEBOOTBLOCK	0x00000004

/*-------------------------------------------------
    cannonicalize_path - normalizes a path string
	into a NUL delimited list
-------------------------------------------------*/

static imgtoolerr_t cannonicalize_path(imgtool_image *image, UINT32 flags,
	const char **path, char **alloc_path)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	char *new_path = NULL;
	char path_separator, alt_path_separator;
	const char *s;
	int in_path_separator, i, j;
	
	path_separator = image->module->path_separator;
	alt_path_separator = image->module->alternate_path_separator;

	/* is this path NULL?  if so, is that ignored? */
	if (!*path && (flags & PATH_LEAVENULLALONE))
		goto done;

	/* is this the special filename for bootblocks? */
	if (*path == FILENAME_BOOTBLOCK)
	{
		if (!(flags & PATH_CANBEBOOTBLOCK))
			err = IMGTOOLERR_UNEXPECTED;
		else if (!image->module->supports_bootblock)
			err = IMGTOOLERR_FILENOTFOUND;
		goto done;
	}

	if (path_separator == '\0')
	{
		if (flags & PATH_MUSTBEDIR)
		{
			/* do we specify a path when paths are not supported? */
			if (*path && **path)
			{
				err = IMGTOOLERR_CANNOTUSEPATH | IMGTOOLERR_SRC_FUNCTIONALITY;
				goto done;
			}
			*path = NULL;	/* normalize empty path */
		}
	}
	else
	{
		s = *path ? *path : "";

		/* allocate space for a new cannonical path */
		new_path = malloc(strlen(s) + 4);
		if (!new_path)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}

		/* copy the path */
		in_path_separator = TRUE;
		i = j = 0;
		do
		{
			if ((s[i] != '\0') && (s[i] != path_separator) && (s[i] != alt_path_separator))
			{
				new_path[j++] = s[i];
				in_path_separator = FALSE;
			}
			else if (!in_path_separator)
			{
				new_path[j++] = '\0';
				in_path_separator = TRUE;
			}
		}
		while(s[i++] != '\0');
		new_path[j++] = '\0';
		new_path[j++] = '\0';
		*path = new_path;
	}

done:
	*alloc_path = new_path;
	return err;
}



static imgtoolerr_t cannonicalize_fork(imgtool_image *image, const char **fork)
{
	/* does this module support forks? */
	if (image->module->list_forks)
	{
		/* this module supports forks; make sure that fork is non-NULL */
		if (!*fork)
			*fork = "";
	}
	else
	{
		/* this module does not support forks; make sure that fork is NULL */
		if (*fork)
			return IMGTOOLERR_NOFORKS;
	}
	return IMGTOOLERR_SUCCESS;
}



/*-------------------------------------------------
    imgtool_partition_get_directory_entry - retrieves
	the nth directory entry within a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_get_directory_entry(imgtool_partition *partition, const char *path, int index, imgtool_dirent *ent)
{
	imgtoolerr_t err;
	imgtool_directory *imgenum = NULL;

	if (index < 0)
	{
		err = IMGTOOLERR_PARAMTOOSMALL;
		goto done;
	}

	err = imgtool_directory_open(partition, path, &imgenum);
	if (err)
		goto done;

	do
	{
		err = imgtool_directory_get_next(imgenum, ent);
		if (err)
			goto done;

		if (ent->eof)
		{
			err = IMGTOOLERR_FILENOTFOUND;
			goto done;
		}
	}
	while(index--);

done:
	if (err)
		memset(ent->filename, 0, sizeof(ent->filename));
	if (imgenum)
		imgtool_directory_close(imgenum);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_get_file_size - returns free
	space on a partition, in bytes
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_get_file_size(imgtool_partition *partition, const char *fname, UINT64 *filesize)
{
	int err;
	imgtool_directory *imgenum;
	imgtool_dirent ent;
	const char *path;

	path = NULL;	/* TODO: Need to parse off the path */

	*filesize = -1;
	memset(&ent, 0, sizeof(ent));

	err = imgtool_directory_open(partition, path, &imgenum);
	if (err)
		goto done;

	do
	{
		err = imgtool_directory_get_next(imgenum, &ent);
		if (err)
			goto done;

		if (!mame_stricmp(fname, ent.filename))
		{
			*filesize = ent.filesize;
			goto done;
		}
	}
	while(ent.filename[0]);

	err = IMGTOOLERR_FILENOTFOUND;

done:
	if (imgenum)
		imgtool_directory_close(imgenum);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_list_file_attributes - identifies
	all attributes on a file
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_list_file_attributes(imgtool_partition *partition, const char *path, UINT32 *attrs, size_t len)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	char *alloc_path = NULL;

	memset(attrs, 0, sizeof(*attrs) * len);

	if (!image->module->list_attrs)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_LEAVENULLALONE, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->list_attrs(image, path, attrs, len);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_get_file_attributes - retrieves
	attributes on a file
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_get_file_attributes(imgtool_partition *partition, const char *path, const UINT32 *attrs, imgtool_attribute *values)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	char *alloc_path = NULL;

	if (!image->module->get_attrs)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_LEAVENULLALONE, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->get_attrs(image, path, attrs, values);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_put_file_attributes - sets
	attributes on a file
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_put_file_attributes(imgtool_partition *partition, const char *path, const UINT32 *attrs, const imgtool_attribute *values)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	char *alloc_path = NULL;

	if (!image->module->set_attrs)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_LEAVENULLALONE, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->set_attrs(image, path, attrs, values);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_get_file_attributes - retrieves
	an attribute on a single file
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_get_file_attribute(imgtool_partition *partition, const char *path, UINT32 attr, imgtool_attribute *value)
{
	UINT32 attrs[2];
	attrs[0] = attr;
	attrs[1] = 0;
	return imgtool_partition_get_file_attributes(partition, path, attrs, value);
}



/*-------------------------------------------------
    imgtool_partition_put_file_attribute - sets
	attributes on a single file
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_put_file_attribute(imgtool_partition *partition, const char *path, UINT32 attr, imgtool_attribute value)
{
	UINT32 attrs[2];
	attrs[0] = attr;
	attrs[1] = 0;
	return imgtool_partition_put_file_attributes(partition, path, attrs, &value);
}



/*-------------------------------------------------
    imgtool_partition_get_icon_info - retrieves the
	icon for a file stored on a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_get_icon_info(imgtool_partition *partition, const char *path, imgtool_iconinfo *iconinfo)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	char *alloc_path = NULL;

	if (!image->module->get_iconinfo)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, 0, &path, &alloc_path);
	if (err)
		goto done;

	memset(iconinfo, 0, sizeof(*iconinfo));
	err = image->module->get_iconinfo(image, path, iconinfo);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_suggest_file_filters - suggests
	a list of filters appropriate for a file on a
	partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_suggest_file_filters(imgtool_partition *partition, const char *path,
	imgtool_stream *stream, imgtool_transfer_suggestion *suggestions, size_t suggestions_length)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	int i, j;
	char *alloc_path = NULL;
	imgtoolerr_t (*check_stream)(imgtool_stream *stream, imgtool_suggestion_viability_t *viability);
	size_t position;

	/* clear out buffer */
	memset(suggestions, 0, sizeof(*suggestions) * suggestions_length);

	if (!image->module->suggest_transfer)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_LEAVENULLALONE, &path, &alloc_path);
	if (err)
		goto done;

	/* invoke the module's suggest call */
	err = image->module->suggest_transfer(image, path, suggestions, suggestions_length);
	if (err)
		goto done;

	/* Loop on resulting suggestions, and do the following:
	 * 1.  Call check_stream if present, and remove disqualified streams
	 * 2.  Fill in missing descriptions
	 */
	i = j = 0;
	while(suggestions[i].viability)
	{
		if (stream && suggestions[i].filter)
		{
			check_stream = (imgtoolerr_t (*)(imgtool_stream *, imgtool_suggestion_viability_t *)) filter_get_info_fct(suggestions[i].filter, FILTINFO_PTR_CHECKSTREAM);
			if (check_stream)
			{
				position = stream_tell(stream);
				err = check_stream(stream, &suggestions[i].viability);
				stream_seek(stream, position, SEEK_SET);
				if (err)
					goto done;
			}
		}

		/* the check_stream proc can remove the option by clearing out the viability */
		if (suggestions[i].viability)
		{
			/* we may have to move this suggestion, if one was removed */
			if (i != j)
				memcpy(&suggestions[j], &suggestions[i], sizeof(*suggestions));

			/* if the description is missing, fill it in */
			if (!suggestions[j].description)
			{
				if (suggestions[j].filter)
					suggestions[j].description = filter_get_info_string(suggestions[i].filter, FILTINFO_STR_HUMANNAME);
				else
					suggestions[j].description = "Raw";
			}

			j++;
		}
		i++;
	}
	suggestions[j].viability = 0;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_get_chain - retrieves the block
	chain for a file or directory on a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_get_chain(imgtool_partition *partition, const char *path, imgtool_chainent *chain, size_t chain_size)
{
	size_t i;
	imgtool_image *image = GET_IMAGE(partition);

	assert(chain_size > 0);

	if (!image->module->get_chain)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	/* initialize the chain array, so the module's get_chain function can be lazy */
	for (i = 0; i < chain_size; i++)
	{
		chain[i].level = 0;
		chain[i].block = ~0;
	}

	return image->module->get_chain(image, path, chain, chain_size - 1);
}



/*-------------------------------------------------
    imgtool_partition_get_chain_string - retrieves
	the block chain for a file or directory on a
	partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_get_chain_string(imgtool_partition *partition, const char *path, char *buffer, size_t buffer_len)
{
	imgtoolerr_t err;
	imgtool_chainent chain[512];
	UINT64 last_block;
	UINT8 cur_level = 0;
	int len, i;
	int comma_needed = FALSE;

	/* determine the last block identifier */
	chain[0].block = ~0;
	last_block = chain[0].block;

	err = imgtool_partition_get_chain(partition, path, chain, sizeof(chain) / sizeof(chain[0]));
	if (err)
		return err;

	len = snprintf(buffer, buffer_len, "[");
	buffer += len;
	buffer_len -= len;

	for (i = 0; chain[i].block != last_block; i++)
	{
		while(cur_level < chain[i].level)
		{
			len = snprintf(buffer, buffer_len, " [");
			buffer += len;
			buffer_len -= len;
			cur_level++;
			comma_needed = FALSE;
		}
		while(cur_level > chain[i].level)
		{
			len = snprintf(buffer, buffer_len, "]");
			buffer += len;
			buffer_len -= len;
			cur_level--;
		}

		if (comma_needed)
		{
			len = snprintf(buffer, buffer_len, ", ");
			buffer += len;
			buffer_len -= len;
		}

		len = snprintf(buffer, buffer_len, "%u", (unsigned) chain[i].block);
		buffer += len;
		buffer_len -= len;
		comma_needed = TRUE;
	}

	do
	{
		len = snprintf(buffer, buffer_len, "]");
		buffer += len;
		buffer_len -= len;
	}
	while(cur_level-- > 0);

	return IMGTOOLERR_SUCCESS;
}



/*-------------------------------------------------
    imgtool_partition_get_free_space - returns the
	amount of free space on a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_get_free_space(imgtool_partition *partition, UINT64 *sz)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	UINT64 size;

	if (!image->module->free_space)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = image->module->free_space(image, &size);
	if (err)
		return err | IMGTOOLERR_SRC_IMAGEFILE;

	if (sz)
		*sz = size;
	return IMGTOOLERR_SUCCESS;
}



/*-------------------------------------------------
    imgtool_partition_read_file - starts reading
	from a file on a partition with a stream
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_read_file(imgtool_partition *partition, const char *filename, const char *fork, imgtool_stream *destf, filter_getinfoproc filter)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	imgtool_stream *newstream = NULL;
	char *alloc_path = NULL;
	union filterinfo u;

	if (!image->module->read_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_CANBEBOOTBLOCK, &filename, &alloc_path);
	if (err)
		goto done;

	err = cannonicalize_fork(image, &fork);
	if (err)
		goto done;

	if (filter)
	{
		/* use a filter */
		filter(FILTINFO_PTR_READFILE, &u);
		if (!u.read_file)
		{
			err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
			goto done;
		}

		err = u.read_file(image, filename, fork, destf);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}
	else
	{
		/* invoke the actual module */
		err = image->module->read_file(image, filename, fork, destf);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}

done:
	if (newstream)
		stream_close(newstream);
	if (alloc_path)
		free(alloc_path);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_write_file - starts writing
	to a new file on an image with a stream
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_write_file(imgtool_partition *partition, const char *filename, const char *fork, imgtool_stream *sourcef, option_resolution *opts, filter_getinfoproc filter)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	char *buf = NULL;
	char *s;
	imgtool_stream *newstream = NULL;
	option_resolution *alloc_resolution = NULL;
	char *alloc_path = NULL;
	UINT64 free_space;
	UINT64 file_size;
	union filterinfo u;

	if (!image->module->write_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* Does this image module prefer upper case file names? */
	if (image->module->prefer_ucase)
	{
		buf = malloc(strlen(filename) + 1);
		if (!buf)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		strcpy(buf, filename);
		for (s = buf; *s; s++)
			*s = toupper(*s);
		filename = buf;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_CANBEBOOTBLOCK, &filename, &alloc_path);
	if (err)
		goto done;

	err = cannonicalize_fork(image, &fork);
	if (err)
		goto done;

	/* allocate dummy options if necessary */
	if (!opts && image->module->writefile_optguide)
	{
		alloc_resolution = option_resolution_create(image->module->writefile_optguide, image->module->writefile_optspec);
		if (!alloc_resolution)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		opts = alloc_resolution;
	}
	if (opts)
		option_resolution_finish(opts);

	/* if free_space is implemented; do a quick check to see if space is available */
	if (image->module->free_space)
	{
		err = image->module->free_space(image, &free_space);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}

		file_size = stream_size(sourcef);

		if (file_size > free_space)
		{
			err = markerrorsource(IMGTOOLERR_NOSPACE);
			goto done;
		}
	}

	if (filter)
	{
		/* use a filter */
		filter(FILTINFO_PTR_WRITEFILE, &u);
		if (!u.write_file)
		{
			err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
			goto done;
		}

		err = u.write_file(image, filename, fork, sourcef, opts);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}
	else
	{
		/* actually invoke the write file handler */
		err = image->module->write_file(image, filename, fork, sourcef, opts);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}

done:
	if (buf)
		free(buf);
	if (alloc_path)
		free(alloc_path);
	if (newstream)
		stream_close(newstream);
	if (alloc_resolution)
		option_resolution_close(alloc_resolution);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_get_file - read a file from
	an image, storing it into a native file
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_get_file(imgtool_partition *partition, const char *filename, const char *fork,
	const char *dest, filter_getinfoproc filter)
{
	imgtoolerr_t err;
	imgtool_stream *f;
	char *alloc_dest = NULL;
	const char *filter_extension = NULL;

	if (!dest)
	{
		if (filter)
			filter_extension = filter_get_info_string(filter, FILTINFO_STR_EXTENSION);

		if (filter_extension)
		{
			alloc_dest = malloc(strlen(filename) + 1 + strlen(filter_extension) + 1);
			if (!alloc_dest)
				return IMGTOOLERR_OUTOFMEMORY;

			sprintf(alloc_dest, "%s.%s", filename, filter_extension);
			dest = alloc_dest;
		}
		else
		{
			if (filename == FILENAME_BOOTBLOCK)
				dest = "boot.bin";
			else
				dest = filename;
		}
	}

	f = stream_open(dest, OSD_FOPEN_WRITE);
	if (!f)
	{
		err = IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;
		goto done;
	}

	err = imgtool_partition_read_file(partition, filename, fork, f, filter);
	if (err)
		goto done;

done:
	if (f)
		stream_close(f);
	if (alloc_dest)
		free(alloc_dest);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_put_file - read a native file
	and store it on a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_put_file(imgtool_partition *partition, const char *newfname, const char *fork,
	const char *source, option_resolution *opts, filter_getinfoproc filter)
{
	imgtoolerr_t err;
	imgtool_stream *f;

	if (!newfname)
		newfname = (const char *) osd_basename((char *) source);

	f = stream_open(source, OSD_FOPEN_READ);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = imgtool_partition_write_file(partition, newfname, fork, f, opts, filter);
	stream_close(f);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_delete_file - delete a file
	on a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_delete_file(imgtool_partition *partition, const char *fname)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	char *alloc_path = NULL;

	if (!image->module->delete_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, 0, &fname, &alloc_path);
	if (err)
		goto done;

	err = image->module->delete_file(image, fname);
	if (err)
	{
		err = markerrorsource(err);
		goto done;
	}

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_list_file_forks - lists all
	forks on an image
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_list_file_forks(imgtool_partition *partition, const char *path, imgtool_forkent *ents, size_t len)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	char *alloc_path = NULL;

	if (!image->module->list_forks)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, 0, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->list_forks(image, path, ents, len);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_create_directory - creates a
	directory on a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_create_directory(imgtool_partition *partition, const char *path)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	char *alloc_path = NULL;

	/* implemented? */
	if (!image->module->create_dir)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_MUSTBEDIR, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->create_dir(image, path);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



/*-------------------------------------------------
    imgtool_partition_delete_directory - deletes a
	directory on a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_partition_delete_directory(imgtool_partition *partition, const char *path)
{
	imgtoolerr_t err;
	imgtool_image *image = GET_IMAGE(partition);
	char *alloc_path = NULL;

	/* implemented? */
	if (!image->module->delete_dir)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_MUSTBEDIR, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->delete_dir(image, path);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



/***************************************************************************

	Directory handling functions

***************************************************************************/

/*-------------------------------------------------
    imgtool_directory_open - begins
	enumerating files on a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_directory_open(imgtool_partition *partition, const char *path, imgtool_directory **outenum)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	imgtool_image *image = GET_IMAGE(partition);
	imgtool_directory *enumeration = NULL;
	char *alloc_path = NULL;
	size_t size;

	/* sanity checks */
	assert(partition);
	assert(outenum);

	*outenum = NULL;

	if (!image->module->next_enum)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	err = cannonicalize_path(image, PATH_MUSTBEDIR, &path, &alloc_path);
	if (err)
		goto done;

	size = sizeof(struct _imgtool_directory) + imgtool_image_module(image)->imageenum_extra_bytes;
	enumeration = (imgtool_directory *) malloc(size);
	if (!enumeration)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(enumeration, '\0', size);
	enumeration->image = image;

	if (image->module->begin_enum)
	{
		err = image->module->begin_enum(enumeration, path);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}

done:
	if (alloc_path)
		free(alloc_path);
	if (err && enumeration)
	{
		free(enumeration);
		enumeration = NULL;
	}
	*outenum = enumeration;
	return err;
}



/*-------------------------------------------------
    imgtool_directory_close - closes a directory
-------------------------------------------------*/

void imgtool_directory_close(imgtool_directory *enumeration)
{
	const imgtool_module *module;
	module = imgtool_directory_module(enumeration);
	if (module->close_enum)
		module->close_enum(enumeration);
	free(enumeration);
}



/*-------------------------------------------------
    imgtool_directory_get_next - continues
	enumerating files within a partition
-------------------------------------------------*/

imgtoolerr_t imgtool_directory_get_next(imgtool_directory *enumeration, imgtool_dirent *ent)
{
	imgtoolerr_t err;
	const imgtool_module *module;

	module = imgtool_directory_module(enumeration);

	/* This makes it so that drivers don't have to take care of clearing
	 * the attributes if they don't apply
	 */
	memset(ent, 0, sizeof(*ent));

	err = module->next_enum(enumeration, ent);
	if (err)
		return markerrorsource(err);

	/* don't trust the module! */
	if (!module->supports_creation_time && (ent->creation_time != 0))
	{
		internal_error(module, "next_enum() specified creation_time, which is marked as unsupported by this module");
		return IMGTOOLERR_UNEXPECTED;
	}
	if (!module->supports_lastmodified_time && (ent->lastmodified_time != 0))
	{
		internal_error(module, "next_enum() specified lastmodified_time, which is marked as unsupported by this module");
		return IMGTOOLERR_UNEXPECTED;
	}
	if (!module->path_separator && ent->directory)
	{
		internal_error(module, "next_enum() returned a directory, which is marked as unsupported by this module");
		return IMGTOOLERR_UNEXPECTED;
	}
	return IMGTOOLERR_SUCCESS;
}



/*-------------------------------------------------
    imgtool_directory_module - returns the module associated
	with this directory
-------------------------------------------------*/

const imgtool_module *imgtool_directory_module(imgtool_directory *enumeration)
{
	return enumeration->image->module;
}



/*-------------------------------------------------
    imgtool_directory_extrabytes - returns extra
	bytes on a directory
-------------------------------------------------*/

void *imgtool_directory_extrabytes(imgtool_directory *enumeration)
{
	assert(enumeration->image->module->imageenum_extra_bytes > 0);
	return ((UINT8 *) enumeration) + sizeof(*enumeration);
}



/*-------------------------------------------------
    imgtool_directory_image - returns the image
	associated with this directory
-------------------------------------------------*/

imgtool_image *imgtool_directory_image(imgtool_directory *enumeration)
{
	return enumeration->image;
}
