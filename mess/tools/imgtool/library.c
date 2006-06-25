/****************************************************************************

	library.c

	Code relevant to the Imgtool library; analgous to the MESS/MAME driver
	list.

****************************************************************************/

#include <assert.h>
#include <string.h>

#include "osdepend.h"
#include "library.h"
#include "pool.h"
#include "mame.h"

struct _imgtool_library
{
	memory_pool pool;
	imgtool_module *first;
	imgtool_module *last;
};



imgtool_library *imgtool_library_create(void)
{
	imgtool_library *library;

	library = malloc(sizeof(struct _imgtool_library));
	if (!library)
		return NULL;
	memset(library, 0, sizeof(*library));

	pool_init(&library->pool);
	return library;
}



void imgtool_library_close(imgtool_library *library)
{
	pool_exit(&library->pool);
	free(library);
}



static void imgtool_library_add_class(imgtool_library *library, const imgtool_class *imgclass)
{
	imgtool_module *module;

	/* allocate the module and place it in the chain */
	module = auto_malloc(sizeof(*module));
	memset(module, 0, sizeof(*module));
	module->previous = library->last;
	if (library->last)
		library->last->next = module;
	else
		library->first = module;
	library->last = module;

	module->imgclass					= *imgclass;
	module->name						= auto_strdup(imgtool_get_info_string(imgclass, IMGTOOLINFO_STR_NAME));
	module->description					= auto_strdup(imgtool_get_info_string(imgclass, IMGTOOLINFO_STR_DESCRIPTION));
	module->extensions					= auto_strdup(imgtool_get_info_string(imgclass, IMGTOOLINFO_STR_FILE_EXTENSIONS));
	module->eoln						= auto_strdup_allow_null(imgtool_get_info_ptr(imgclass, IMGTOOLINFO_STR_EOLN));
	module->path_separator				= (char) imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_PATH_SEPARATOR);
	module->alternate_path_separator	= (char) imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_ALTERNATE_PATH_SEPARATOR);
	module->prefer_ucase				= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_PREFER_UCASE) ? 1 : 0;
	module->initial_path_separator		= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_INITIAL_PATH_SEPARATOR) ? 1 : 0;
	module->open_is_strict				= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_OPEN_IS_STRICT) ? 1 : 0;
	module->supports_creation_time		= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_SUPPORTS_CREATION_TIME) ? 1 : 0;
	module->supports_lastmodified_time	= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_SUPPORTS_LASTMODIFIED_TIME) ? 1 : 0;
	module->tracks_are_called_cylinders	= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_TRACKS_ARE_CALLED_CYLINDERS) ? 1 : 0;
	module->writing_untested			= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_WRITING_UNTESTED) ? 1 : 0;
	module->creation_untested			= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_CREATION_UNTESTED) ? 1 : 0;
	module->supports_bootblock			= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_SUPPORTS_BOOTBLOCK) ? 1 : 0;
	module->open						= (imgtoolerr_t (*)(imgtool_image *, imgtool_stream *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_OPEN);
	module->create						= (imgtoolerr_t (*)(imgtool_image *, imgtool_stream *, option_resolution *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_CREATE);
	module->close						= (void (*)(imgtool_image *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_CLOSE);
	module->info						= (void (*)(imgtool_image *, char *, size_t)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_INFO);
	module->begin_enum					= (imgtoolerr_t (*)(imgtool_imageenum *, const char *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_BEGIN_ENUM);
	module->next_enum					= (imgtoolerr_t (*)(imgtool_imageenum *, imgtool_dirent *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_NEXT_ENUM);
	module->close_enum					= (void (*)(imgtool_imageenum *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_CLOSE_ENUM);
	module->free_space					= (imgtoolerr_t (*)(imgtool_image *, UINT64 *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_FREE_SPACE);
	module->read_file					= (imgtoolerr_t (*)(imgtool_image *, const char *, const char *, imgtool_stream *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_READ_FILE);
	module->write_file					= (imgtoolerr_t (*)(imgtool_image *, const char *, const char *, imgtool_stream *, option_resolution *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_WRITE_FILE);
	module->delete_file					= (imgtoolerr_t (*)(imgtool_image *, const char *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_DELETE_FILE);
	module->list_forks					= (imgtoolerr_t (*)(imgtool_image *, const char *, imgtool_forkent *, size_t)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_LIST_FORKS);
	module->create_dir					= (imgtoolerr_t (*)(imgtool_image *, const char *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_CREATE_DIR);
	module->delete_dir					= (imgtoolerr_t (*)(imgtool_image *, const char *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_DELETE_DIR);
	module->list_attrs					= (imgtoolerr_t (*)(imgtool_image *, const char *, UINT32 *, size_t)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_LIST_ATTRS);
	module->get_attrs					= (imgtoolerr_t (*)(imgtool_image *, const char *, const UINT32 *, imgtool_attribute *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_GET_ATTRS);
	module->set_attrs					= (imgtoolerr_t (*)(imgtool_image *, const char *, const UINT32 *, const imgtool_attribute *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_SET_ATTRS);
	module->attr_name					= (imgtoolerr_t (*)(UINT32, const imgtool_attribute *, char *, size_t)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_ATTR_NAME);
	module->get_iconinfo				= (imgtoolerr_t (*)(imgtool_image *, const char *, imgtool_iconinfo *)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_GET_ICON_INFO);
	module->suggest_transfer			= (imgtoolerr_t (*)(imgtool_image *, const char *, imgtool_transfer_suggestion *, size_t))  imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_SUGGEST_TRANSFER);
	module->get_chain					= (imgtoolerr_t (*)(imgtool_image *, const char *, imgtool_chainent *, size_t)) imgtool_get_info_fct(imgclass, IMGTOOLINFO_PTR_GET_CHAIN);
	module->createimage_optguide		= (const struct OptionGuide *) imgtool_get_info_ptr(imgclass, IMGTOOLINFO_PTR_CREATEIMAGE_OPTGUIDE);
	module->createimage_optspec			= auto_strdup_allow_null(imgtool_get_info_ptr(imgclass, IMGTOOLINFO_STR_CREATEIMAGE_OPTSPEC));
	module->writefile_optguide			= (const struct OptionGuide *) imgtool_get_info_ptr(imgclass, IMGTOOLINFO_PTR_WRITEFILE_OPTGUIDE);
	module->writefile_optspec			= auto_strdup_allow_null(imgtool_get_info_ptr(imgclass, IMGTOOLINFO_STR_WRITEFILE_OPTSPEC));
	module->image_extra_bytes			+= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_IMAGE_EXTRA_BYTES);
	module->imageenum_extra_bytes		+= imgtool_get_info_int(imgclass, IMGTOOLINFO_INT_ENUM_EXTRA_BYTES);
}



void imgtool_library_add(imgtool_library *library, imgtool_get_info get_info)
{
	int (*make_class)(int index, imgtool_class *imgclass);
	imgtool_class imgclass;
	int i, result;

	/* try this class */
	memset(&imgclass, 0, sizeof(imgclass));
	imgclass.get_info = get_info;

	/* do we have derived getinfo functions? */
	make_class = (int (*)(int index, imgtool_class *imgclass))
		imgtool_get_info_fct(&imgclass, IMGTOOLINFO_PTR_MAKE_CLASS);

	if (make_class)
	{
		i = 0;
		do
		{
			/* clear out the class */
			memset(&imgclass, 0, sizeof(imgclass));
			imgclass.get_info = get_info;

			/* make the class */
			result = make_class(i++, &imgclass);
			if (result)
				imgtool_library_add_class(library, &imgclass);
		}
		while(result);
	}
	else
	{
		imgtool_library_add_class(library, &imgclass);
	}
}



const imgtool_module *imgtool_library_unlink(imgtool_library *library,
	const char *module)
{
	imgtool_module *m;
	imgtool_module **previous;
	imgtool_module **next;

	for (m = library->first; m; m = m->next)
	{
		if (!mame_stricmp(m->name, module))
		{
			previous = m->previous ? &m->previous->next : &library->first;
			next = m->next ? &m->next->previous : &library->last;
			*previous = m->next;
			*next = m->previous;
			m->previous = NULL;
			m->next = NULL;
			return m;
		}
	}
	return NULL;
}



static int module_compare(const imgtool_module *m1,
	const imgtool_module *m2, imgtool_libsort_t sort)
{
	int rc = 0;
	switch(sort)
	{
		case ITLS_NAME:
			rc = strcmp(m1->name, m2->name);
			break;
		case ITLS_DESCRIPTION:
			rc = mame_stricmp(m1->name, m2->name);
			break;
	}
	return rc;
}



void imgtool_library_sort(imgtool_library *library, imgtool_libsort_t sort)
{
	imgtool_module *m1;
	imgtool_module *m2;
	imgtool_module *target;
	imgtool_module **before;
	imgtool_module **after;

	for (m1 = library->first; m1; m1 = m1->next)
	{
		target = m1;
		for (m2 = m1->next; m2; m2 = m2->next)
		{
			while(module_compare(target, m2, sort) > 0)
				target = m2;
		}

		if (target != m1)
		{
			/* unlink the target */
			before = target->previous ? &target->previous->next : &library->first;
			after = target->next ? &target->next->previous : &library->last;
			*before = target->next;
			*after = target->previous;

			/* now place the target before m1 */
			target->previous = m1->previous;
			target->next = m1;
			before = m1->previous ? &m1->previous->next : &library->first;
			*before = target;
			m1->previous = target;

			/* since we changed the order, we have to replace ourselves */
			m1 = target;
		}
	}
}



const imgtool_module *imgtool_library_findmodule(
	imgtool_library *library, const char *module_name)
{
	const imgtool_module *module;

	assert(library);
	module = library->first;
	while(module && module_name && strcmp(module->name, module_name))
		module = module->next;
	return module;
}



imgtool_module *imgtool_library_iterate(imgtool_library *library, const imgtool_module *module)
{
	return module ? module->next : library->first;
}



imgtool_module *imgtool_library_index(imgtool_library *library, int i)
{
	imgtool_module *module;
	module = library->first;
	while(module && i--)
		module = module->next;
	return module;
}



void *imgtool_library_alloc(imgtool_library *library, size_t mem)
{
	return pool_malloc(&library->pool, mem);
}



char *imgtool_library_strdup(imgtool_library *library, const char *s)
{
	return s ? pool_strdup(&library->pool, s) : NULL;
}


