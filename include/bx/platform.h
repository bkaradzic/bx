/*
 * Copyright 2010-2012 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_PLATFORM_H__
#define __BX_PLATFORM_H__

#define BX_COMPILER_CLANG 0
#define BX_COMPILER_GCC 0
#define BX_COMPILER_MSVC 0

#define BX_PLATFORM_ANDROID 0
#define BX_PLATFORM_LINUX 0
#define BX_PLATFORM_NACL 0
#define BX_PLATFORM_WINDOWS 0
#define BX_PLATFORM_XBOX360 0

#define BX_CPU_ARM 0
#define BX_CPU_PPC 0
#define BX_CPU_X86 0

#define BX_CPU_ENDIAN_BIG 0
#define BX_CPU_ENDIAN_LITTLE 0

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Compilers
#if defined(_MSC_VER)
#	undef BX_COMPILER_MSVC
#	define BX_COMPILER_MSVC 1
#elif defined(__clang__)
// clang defines __GNUC__
#	undef BX_COMPILER_CLANG
#	define BX_COMPILER_CLANG 1
#elif defined(__GNUC__)
#	undef BX_COMPILER_GCC
#	define BX_COMPILER_GCC 1
#else
#	error "BX_COMPILER_* is not defined!"
#endif //

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Operating_Systems
#if defined(_XBOX_VER)
#	undef BX_PLATFORM_XBOX360
#	define BX_PLATFORM_XBOX360 1
#elif defined(_WIN32) || defined(_WIN64)
#	undef BX_PLATFORM_WINDOWS
#	define BX_PLATFORM_WINDOWS 1
#elif defined(__native_client__)
// NaCl compiler defines __linux__
#	undef BX_PLATFORM_NACL
#	define BX_PLATFORM_NACL 1
#elif defined(__ANDROID__)
// Android compiler defines __linux__
#	undef BX_PLATFORM_ANDROID
#	define BX_PLATFORM_ANDROID 1
#elif defined(__linux__)
#	undef BX_PLATFORM_LINUX
#	define BX_PLATFORM_LINUX 1
#else
#	error "BX_PLATFORM_* is not defined!"
#endif //

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Architectures
#if defined(__arm__)
#	undef BX_CPU_ARM
#	define BX_CPU_ARM 1
#	define BX_CACHE_LINE_SIZE 64
#elif defined(_M_PPC) || defined(__powerpc__) || defined(__powerpc64__)
#	undef BX_CPU_PPC
#	define BX_CPU_PPC 1
#	define BX_CACHE_LINE_SIZE 128
#elif defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#	undef BX_CPU_X86
#	define BX_CPU_X86 1
#	define BX_CACHE_LINE_SIZE 64
#endif // 

#if BX_CPU_PPC
#	undef BX_CPU_ENDIAN_BIG
#	define BX_CPU_ENDIAN_BIG 1
#else
#	undef BX_CPU_ENDIAN_LITTLE
#	define BX_CPU_ENDIAN_LITTLE 1
#endif // BX_PLATFORM_

#endif // __BX_PLATFORM_H__
