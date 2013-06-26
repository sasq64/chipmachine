/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* debug facilities */
/* #undef DEBUG */

/* debug facilities for file68 */
/* #undef DEBUG_FILE68 */
/* Defined if file68 supports audio (via libao) */
/* #undef FILE68_AO */

/* Defined if file68 supports remote files (via libcurl) */
/* #undef FILE68_CURL */

/* Defined if file68 supports !ice depacker (via unice68) */
#define FILE68_UNICE68 1

/* Defined if file68 supports deflate (via zlib) */
#define FILE68_Z 1

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Support __declspec() */
#define HAVE_DECLSPEC 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `free' function. */
#define HAVE_FREE 1

/* Define to 1 if you have the `getenv' function. */
#define HAVE_GETENV 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `malloc' function. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `sleep' function. */
#define HAVE_SLEEP 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Support visibility __attribute__ */
/* #undef HAVE_VISIBILITY */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `vsprintf' function. */
#define HAVE_VSPRINTF 1

/* Define to 1 if you have the <windows.h> header file. */
/* #undef HAVE_WINDOWS_H */

/* Define to 1 if you have the <winreg.h> header file. */
/* #undef HAVE_WINREG_H */

/* Disable file decriptor stream support */
/* #undef ISTREAM68_NO_FD */

/* Disable FILE* stream support */
/* #undef ISTREAM68_NO_FILE */

/* Disable memory stream support */
/* #undef ISTREAM68_NO_MEM */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if assertions should be disabled. */
#define NDEBUG 1

/* suppress all debug facilities for file68 */
#define NDEBUG_FILE68 1

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "libfile68"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "benjihan -4t- sourceforge"

/* Package short description */
#define PACKAGE_DESC "sc68 file and utility library. It is part of the sc68 project. Visit <http://sc68.atari.org>"

/* Define to the full name of this package. */
#define PACKAGE_NAME "file68"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "file68 3.0.0a"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libfile68"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://sc68.atari.org"

/* Version number */
#define PACKAGE_VERNUM 300

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.0.0a"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Using ao library */
/* #undef USE_AO */

/* Using curl library */
/* #undef USE_CURL */

/* Using Windows registry */
/* #undef USE_REGISTRY68 */

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Using unice68 library */
#define USE_UNICE68 1

/* Using z library */
#define USE_Z 1

/* Version number of package */
#define VERSION "3.0.0a"

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to the equivalent of the C99 'restrict' keyword, or to
   nothing if this is not supported.  Do not define if restrict is
   supported directly.  */
#define restrict __restrict
/* Work around a bug in Sun C++: it does not support _Restrict or
   __restrict__, even though the corresponding Sun C compiler ends up with
   "#define restrict _Restrict" or "#define restrict __restrict__" in the
   previous line.  Perhaps some future version of Sun C++ will work with
   restrict; if so, hopefully it defines __RESTRICT like Sun C does.  */
#if defined __SUNPRO_CC && !defined __RESTRICT
# define _Restrict
# define __restrict__
#endif

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */
