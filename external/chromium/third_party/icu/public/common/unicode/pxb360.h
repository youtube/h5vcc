/*
******************************************************************************
*
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* Note: autoconf creates platform.h from platform.h.in at configure time.
*
******************************************************************************
*
*  FILE NAME : platform.h
*
*   Date        Name        Description
*   05/13/98    nos         Creation (content moved here from ptypes.h).
*   03/02/99    stephen     Added AS400 support.
*   03/30/99    stephen     Added Linux support.
*   04/13/99    stephen     Reworked for autoconf.
******************************************************************************
*/

// forked from pxb1.h, then forced back into big-endian mode.

#ifndef _PLATFORM_H
#define _PLATFORM_H

/**
 * \file
 * \brief Basic types for the platform
 */

/* This file should be included before uvernum.h. */
#if defined(UVERNUM_H)
# error Do not include unicode/uvernum.h before #including unicode/platform.h.  Instead of unicode/uvernum.h, #include unicode/uversion.h
#endif

/** _MSC_VER is used to detect the Microsoft compiler. */
#if defined(_MSC_VER)
#define U_INT64_IS_LONG_LONG 0
#else
#define U_INT64_IS_LONG_LONG 1
#endif

/**
 * Determine wheter to enable auto cleanup of libraries.
 * @internal
 */
#ifndef UCLN_NO_AUTO_CLEANUP
#define UCLN_NO_AUTO_CLEANUP 1
#endif

/**
 * \def U_HAVE_DIRENT_H
 * Define whether dirent.h is available
 * @internal
 */
#ifndef U_HAVE_DIRENT_H
#define U_HAVE_DIRENT_H 1
#endif

/** Define whether inttypes.h is available */
#ifndef U_HAVE_INTTYPES_H
#define U_HAVE_INTTYPES_H 1
#endif

/**
 * Define what support for C++ streams is available.
 *     If U_IOSTREAM_SOURCE is set to 199711, then &lt;iostream&gt; is available
 * (1997711 is the date the ISO/IEC C++ FDIS was published), and then
 * one should qualify streams using the std namespace in ICU header
 * files.
 *     If U_IOSTREAM_SOURCE is set to 198506, then &lt;iostream.h&gt; is
 * available instead (198506 is the date when Stroustrup published
 * "An Extensible I/O Facility for C++" at the summer USENIX conference).
 *     If U_IOSTREAM_SOURCE is 0, then C++ streams are not available and
 * support for them will be silently suppressed in ICU.
 *
 */

#ifndef U_IOSTREAM_SOURCE
#define U_IOSTREAM_SOURCE 199711
#endif

/**
 * \def U_HAVE_STD_STRING
 * Define whether the standard C++ (STL) &lt;string&gt; header is available.
 * For platforms that do not use platform.h and do not define this constant
 * in their platform-specific headers, std_string.h defaults
 * U_HAVE_STD_STRING to 1.
 * @internal
 */
#ifndef U_HAVE_STD_STRING
#define U_HAVE_STD_STRING 1
#endif

/** @{ Determines whether specific types are available */
#ifndef U_HAVE_INT8_T
#define U_HAVE_INT8_T 1
#endif

#ifndef U_HAVE_UINT8_T
#define U_HAVE_UINT8_T 1
#endif

#ifndef U_HAVE_INT16_T
#define U_HAVE_INT16_T 1
#endif

#ifndef U_HAVE_UINT16_T
#define U_HAVE_UINT16_T 1
#endif

#ifndef U_HAVE_INT32_T
#define U_HAVE_INT32_T 1
#endif

#ifndef U_HAVE_UINT32_T
#define U_HAVE_UINT32_T 1
#endif

#ifndef U_HAVE_INT64_T
#define U_HAVE_INT64_T 1
#endif

#ifndef U_HAVE_UINT64_T
#define U_HAVE_UINT64_T 1
#endif

/** @} */

/** Define 64 bit limits */
#if !U_INT64_IS_LONG_LONG
# ifndef INT64_C
#  define INT64_C(x) ((int64_t)x)
# endif
# ifndef UINT64_C
#  define UINT64_C(x) ((uint64_t)x)
# endif
/** else use the umachine.h definition */
#endif

/*===========================================================================*/
/** @{ Compiler and environment features                                     */
/*===========================================================================*/

/* Define whether namespace is supported */
#ifndef U_HAVE_NAMESPACE
#define U_HAVE_NAMESPACE 1
#endif

/** Determines the endianness of the platform */
#define U_IS_BIG_ENDIAN 1

/* 1 or 0 to enable or disable threads.  If undefined, default is: enable threads. */
#ifndef ICU_USE_THREADS
#define ICU_USE_THREADS 1
#endif

/* On strong memory model CPUs (e.g. x86 CPUs), we use a safe & quick double check mutex lock. */
/**
Microsoft can define _M_IX86, _M_AMD64 (before Visual Studio 8) or _M_X64 (starting in Visual Studio 8). 
Intel can define _M_IX86 or _M_X64
*/
#if defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
#define UMTX_STRONG_MEMORY_MODEL 1
#endif

#ifndef U_DEBUG
#define U_DEBUG 0
#endif

#ifndef U_RELEASE
#define U_RELEASE 1
#endif

/* Determine whether to disable renaming or not. This overrides the
   setting in umachine.h which is for all platforms. */
#ifndef U_DISABLE_RENAMING
#define U_DISABLE_RENAMING 0
#endif

/* Determine whether to override new and delete. */
#ifndef U_OVERRIDE_CXX_ALLOCATION
#define U_OVERRIDE_CXX_ALLOCATION 0
#endif
/* Determine whether to override placement new and delete for STL. */
#ifndef U_HAVE_PLACEMENT_NEW
#define U_HAVE_PLACEMENT_NEW 0
#endif

/* Determine whether to enable tracing. */
#ifndef U_ENABLE_TRACING
#define U_ENABLE_TRACING 0
#endif

/**
 * Whether to enable Dynamic loading in ICU
 * @internal
 */
#ifndef U_ENABLE_DYLOAD
#define U_ENABLE_DYLOAD 0
#endif

/**
 * Whether to test Dynamic loading as an OS capabilty
 * @internal
 */
#ifndef U_CHECK_DYLOAD
#define U_CHECK_DYLOAD 1
#endif


/** Do we allow ICU users to use the draft APIs by default? */
#ifndef U_DEFAULT_SHOW_DRAFT
#define U_DEFAULT_SHOW_DRAFT 1
#endif

/** @} */

/*===========================================================================*/
/** @{ Character data types                                                      */
/*===========================================================================*/

#if ((defined(OS390) && (!defined(__CHARSET_LIB) || !__CHARSET_LIB))) || defined(OS400)
#   define U_CHARSET_FAMILY 1
#endif

/** @} */

/*===========================================================================*/
/** @{ Information about wchar support                                           */
/*===========================================================================*/

#ifndef U_HAVE_WCHAR_H
#define U_HAVE_WCHAR_H      1
#endif

#ifndef U_SIZEOF_WCHAR_T
#define U_SIZEOF_WCHAR_T    2
#endif

#ifndef U_HAVE_WCSCPY
#define U_HAVE_WCSCPY       1
#endif

/** @} */

/**
 * \def U_DECLARE_UTF16
 * Do not use this macro. Use the UNICODE_STRING or U_STRING_DECL macros
 * instead.
 * @internal
 */
#if 1
#define U_DECLARE_UTF16(string) L ## string
#endif

/** @} */

/*===========================================================================*/
/** @{ Information about POSIX support                                           */
/*===========================================================================*/

/**
 * @internal
 */
#if 1
#define U_TZSET         _tzset
#endif
/**
 * @internal
 */
#if 1
#define U_TIMEZONE      _timezone
#endif
/**
 * @internal
 */
#if 1
#define U_TZNAME        _tzname
#endif
/**
 * @internal
 */
#if 1
#define U_DAYLIGHT      _daylight
#endif

#define U_HAVE_MMAP 0
#define U_HAVE_POPEN 0

/** @} */

/*===========================================================================*/
/** @{ Symbol import-export control                                              */
/*===========================================================================*/

#ifdef U_STATIC_IMPLEMENTATION
#define U_EXPORT
#else
#define U_EXPORT __declspec(dllexport)
#endif
#define U_EXPORT2 __cdecl
#define U_IMPORT __declspec(dllimport)
/* @} */

/*===========================================================================*/
/** @{ Code alignment and C function inlining                                    */
/*===========================================================================*/

#ifndef U_INLINE
#   ifdef __cplusplus
#       define U_INLINE inline
#   else
#       define U_INLINE __inline
#   endif
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && !defined(_MANAGED)
#define U_ALIGN_CODE(val)    __asm      align val
#else
#define U_ALIGN_CODE(val)
#endif

/** @} */

/*===========================================================================*/
/** @{ Programs used by ICU code                                                 */
/*===========================================================================*/

/**
 * \def U_MAKE
 * What program to execute to run 'make'
 */
#ifndef U_MAKE
#define U_MAKE  "make"
#endif

/** @} */

/*===========================================================================*/
/* Custom icu entry point renaming                                                  */
/*===========================================================================*/

/**
 * Define the library suffix with C syntax.
 * @internal
 */
# define U_LIB_SUFFIX_C_NAME
/**
 * Define the library suffix as a string with C syntax
 * @internal
 */
# define U_LIB_SUFFIX_C_NAME_STRING ""
/**
 * 1 if a custom library suffix is set
 * @internal
 */
# define U_HAVE_LIB_SUFFIX 0

#if U_HAVE_LIB_SUFFIX
# ifndef U_ICU_ENTRY_POINT_RENAME
/* Renaming pattern:    u_strcpy_41_suffix */
#  define U_ICU_ENTRY_POINT_RENAME(x)    x ## _ ## 46 ##
#  define U_DEF_ICUDATA_ENTRY_POINT(major, minor) icudt####major##minor##_dat

# endif
#endif

#endif
