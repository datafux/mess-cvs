/***********************************************************

  utils.h

  Nifty utility code

***********************************************************/

#ifndef UTILS_H
#define UTILS_H

#include <string.h>

/* -----------------------------------------------------------------------
 * osdutils.h is a header file that gives overrides for functions we
 * define below, so if a native implementation is available, we can use
 * it
 * ----------------------------------------------------------------------- */

#include "osdutils.h"

/* -----------------------------------------------------------------------
 * GCC related optimizations
 * ----------------------------------------------------------------------- */

#ifdef __GNUC__
#define FUNCATTR_MALLOC		__attribute__ ((malloc))

#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define FUNCATTR_PURE		__attribute__ ((pure))
#endif

#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5)
#define FUNCATTR_CONST		__attribute__ ((const))
#endif
#endif /* __GNUC__ */

#ifndef FUNCATTR_MALLOC
#define FUNCATTR_MALLOC
#endif

#ifndef FUNCATTR_PURE
#define FUNCATTR_PURE
#endif

#ifndef FUNCATTR_CONST
#define FUNCATTR_CONST
#endif

/* -----------------------------------------------------------------------
 * strncpyz
 * strncatz
 *
 * strncpy done right :-)
 * ----------------------------------------------------------------------- */
char *strncpyz(char *dest, const char *source, size_t len);
char *strncatz(char *dest, const char *source, size_t len);

/* -----------------------------------------------------------------------
 * rtrim
 *
 * Removes all trailing spaces from a string
 * ----------------------------------------------------------------------- */
void rtrim(char *buf);

/* -----------------------------------------------------------------------
 * strcmpi
 * strncmpi
 *
 * Case insensitive compares.  If your platform has this function then
 * #define it in "osdutils.h"
 * ----------------------------------------------------------------------- */

#ifndef strcmpi
int strcmpi(const char *dst, const char *src);
#endif /* strcmpi */

#ifndef strncmpi
int strncmpi(const char *dst, const char *src, size_t n);
#endif /* strcmpi */

/* -----------------------------------------------------------------------
 * osd_basename
 *
 * Given a pathname, returns the partially qualified path.  If your
 * platform has this function then #define it in "osdutils.h"
 * ----------------------------------------------------------------------- */

#ifndef osd_basename
char *osd_basename (char *name);
#endif /* osd_basename */

/* -----------------------------------------------------------------------
 * osd_mkdir
 *
 * A platform independant mkdir().  No default implementation exists, but
 * so far, only imgtool uses this.  Either way, a 'prototype' is given here
 * for informational purposes.
 * ----------------------------------------------------------------------- */

/* void osd_mkdir(const char *dir); */


/* -----------------------------------------------------------------------
 * memset16
 *
 * 16 bit memset
 * ----------------------------------------------------------------------- */
#ifndef memset16
void *memset16 (void *dest, int value, size_t size);
#endif /* memset16 */

/* -----------------------------------------------------------------------
 * Miscellaneous
 * ----------------------------------------------------------------------- */

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR	'/'
#endif

char *stripspace(const char *src);

#endif /* UTILS_H */
