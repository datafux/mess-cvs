/***************************************************************************

	imgtool.h

	Main headers for Imgtool core

***************************************************************************/

#ifndef IMGTOOL_H
#define IMGTOOL_H

#include <stdlib.h>
#include <stdio.h>
#include "osdepend.h"
#include "mess.h"
#include "formats/flopimg.h"
#include "opresolv.h"
#include "library.h"
#include "filter.h"

/* ----------------------------------------------------------------------- */

#define EOLN_CR		"\x0d"
#define EOLN_LF		"\x0a"
#define EOLN_CRLF	"\x0d\x0a"

#define FILENAME_BOOTBLOCK	((const char *) 1)

/* ---------------------------------------------------------------------------
 * Image calls
 *
 * These are the calls that front ends should use for manipulating images. You
 * should never call the module functions directly because they may not be
 * implemented (i.e. - the function pointers are NULL). The img_* functions are
 * aware of these issues and will make the appropriate checks as well as
 * marking up return codes with the source.  In addition, some of the img_*
 * calls are high level calls that simply image manipulation
 *
 * Calls that return 'int' that are not explictly noted otherwise return
 * imgtool error codes
 * ---------------------------------------------------------------------------
 */

typedef struct _imgtool_module_features imgtool_module_features;
struct _imgtool_module_features
{
	unsigned int supports_create : 1;
	unsigned int supports_open : 1;
	unsigned int supports_reading : 1;
	unsigned int supports_writing : 1;
	unsigned int supports_deletefile : 1;
	unsigned int supports_directories : 1;
	unsigned int supports_freespace : 1;
	unsigned int supports_createdir : 1;
	unsigned int supports_deletedir : 1;
	unsigned int supports_creation_time : 1;
	unsigned int supports_lastmodified_time : 1;
	unsigned int supports_readsector : 1;
	unsigned int supports_writesector : 1;
	unsigned int supports_forks : 1;
	unsigned int supports_geticoninfo : 1;
	unsigned int is_read_only : 1;
};

/* ----- initialization and basics ----- */
void imgtool_init(int omit_untested);
void imgtool_exit(void);
const imgtool_module *imgtool_find_module(const char *modulename);
imgtool_module_features imgtool_get_module_features(const imgtool_module *module);

/* ----- image management ----- */
imgtoolerr_t imgtool_identify_file(const char *filename, imgtool_module **modules, size_t count);
imgtoolerr_t imgtool_image_open(const imgtool_module *module, const char *filename, int read_or_write, imgtool_image **outimg);
imgtoolerr_t imgtool_image_open_byname(const char *modulename, const char *filename, int read_or_write, imgtool_image **outimg);
imgtoolerr_t imgtool_image_create(const imgtool_module *module, const char *fname, option_resolution *opts, imgtool_image **image);
imgtoolerr_t imgtool_image_create_byname(const char *modulename, const char *fname, option_resolution *opts, imgtool_image **image);
void imgtool_image_close(imgtool_image *image);
imgtoolerr_t imgtool_image_info(imgtool_image *image, char *string, size_t len);
imgtoolerr_t imgtool_image_get_sector_size(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, UINT32 *length);
imgtoolerr_t imgtool_image_read_sector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, void *buffer, size_t len);
imgtoolerr_t imgtool_image_write_sector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, const void *buffer, size_t len);
imgtoolerr_t imgtool_image_get_block_size(imgtool_image *image, UINT32 *length);
imgtoolerr_t imgtool_image_read_block(imgtool_image *image, UINT64 block, void *buffer);
imgtoolerr_t imgtool_image_write_block(imgtool_image *image, UINT64 block, const void *buffer);
void *imgtool_image_malloc(imgtool_image *image, size_t size);
const imgtool_module *imgtool_image_module(imgtool_image *image);
void *imgtool_image_extra_bytes(imgtool_image *image);

/* ----- partition management ----- */
imgtoolerr_t imgtool_partition_open(imgtool_image *image, int index, imgtool_partition **partition);
imgtoolerr_t imgtool_partition_close(imgtool_partition *partition);

/* ----- partition operations ----- */
imgtoolerr_t imgtool_partition_get_directory_entry(imgtool_partition *partition, const char *path, int index, imgtool_dirent *ent);
imgtoolerr_t imgtool_partition_get_file_size(imgtool_partition *partition, const char *filename, UINT64 *filesize);
imgtoolerr_t imgtool_partition_get_free_space(imgtool_partition *partition, UINT64 *sz);
imgtoolerr_t imgtool_partition_read_file(imgtool_partition *partition, const char *filename, const char *fork, imgtool_stream *destf, filter_getinfoproc filter);
imgtoolerr_t imgtool_partition_write_file(imgtool_partition *partition, const char *filename, const char *fork, imgtool_stream *sourcef, option_resolution *resolution, filter_getinfoproc filter);
imgtoolerr_t imgtool_partition_get_file(imgtool_partition *partition, const char *filename, const char *fork, const char *dest, filter_getinfoproc filter);
imgtoolerr_t imgtool_partition_put_file(imgtool_partition *partition, const char *newfname, const char *fork, const char *source, option_resolution *opts, filter_getinfoproc filter);
imgtoolerr_t imgtool_partition_delete_file(imgtool_partition *partition, const char *fname);
imgtoolerr_t imgtool_partition_list_file_forks(imgtool_partition *partition, const char *path, imgtool_forkent *ents, size_t len);
imgtoolerr_t imgtool_partition_create_directory(imgtool_partition *partition, const char *path);
imgtoolerr_t imgtool_partition_delete_directory(imgtool_partition *partition, const char *path);
imgtoolerr_t imgtool_partition_list_file_attributes(imgtool_partition *partition, const char *path, UINT32 *attrs, size_t len);
imgtoolerr_t imgtool_partition_get_file_attributes(imgtool_partition *partition, const char *path, const UINT32 *attrs, imgtool_attribute *values);
imgtoolerr_t imgtool_partition_put_file_attributes(imgtool_partition *partition, const char *path, const UINT32 *attrs, const imgtool_attribute *values);
imgtoolerr_t imgtool_partition_get_file_attribute(imgtool_partition *partition, const char *path, UINT32 attr, imgtool_attribute *value);
imgtoolerr_t imgtool_partition_put_file_attribute(imgtool_partition *partition, const char *path, UINT32 attr, imgtool_attribute value);
void         imgtool_partition_get_attribute_name(const imgtool_module *module, UINT32 attribute, const imgtool_attribute *attr_value, char *buffer, size_t buffer_len);
imgtoolerr_t imgtool_partition_get_icon_info(imgtool_partition *partition, const char *path, imgtool_iconinfo *iconinfo);
imgtoolerr_t imgtool_partition_suggest_file_filters(imgtool_partition *partition, const char *path, imgtool_stream *stream, imgtool_transfer_suggestion *suggestions, size_t suggestions_length);
imgtoolerr_t imgtool_partition_get_chain(imgtool_partition *partition, const char *path, imgtool_chainent *chain, size_t chain_size);
imgtoolerr_t imgtool_partition_get_chain_string(imgtool_partition *partition, const char *path, char *buffer, size_t buffer_len);

/* ----- directory management ----- */
imgtoolerr_t imgtool_directory_open(imgtool_partition *partition, const char *path, imgtool_directory **outenum);
void         imgtool_directory_close(imgtool_directory *enumeration);
imgtoolerr_t imgtool_directory_get_next(imgtool_directory *enumeration, imgtool_dirent *ent);
const imgtool_module *imgtool_directory_module(imgtool_directory *enumeration);
void *imgtool_directory_extrabytes(imgtool_directory *enumeration);
imgtool_image *imgtool_directory_image(imgtool_directory *enumeration);

int imgtool_validitychecks(void);

#endif /* IMGTOOL_H */
