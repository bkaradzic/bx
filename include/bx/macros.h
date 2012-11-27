/*
 * Copyright 2010-2012 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_MACROS_H__
#define __BX_MACROS_H__

#include "bx.h"

#define BX_VA_ARGS_COUNT_DETAIL(_a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8, _a9, _a10, _a11, _a12, _a13, _a14, _a15, _a16, _last, ...) _last
#define BX_VA_ARGS_COUNT(...) BX_VA_ARGS_COUNT_DETAIL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define BX_MACRO_DISPATCHER_DETAIL1(_func, _argCount) _func ## _argCount
#define BX_MACRO_DISPATCHER_DETAIL2(_func, _argCount) BX_MACRO_DISPATCHER_DETAIL1(_func, _argCount)
#define BX_MACRO_DISPATCHER(_func, ...) BX_MACRO_DISPATCHER_DETAIL2(_func, BX_VA_ARGS_COUNT(__VA_ARGS__) )

#define BX_MAKEFOURCC(_a, _b, _c, _d) ( ( (uint32_t)(_a) | ( (uint32_t)(_b) << 8) | ( (uint32_t)(_c) << 16) | ( (uint32_t)(_d) << 24) ) )

#define BX_STRINGIZE(_x) BX_STRINGIZE_(_x)
#define BX_STRINGIZE_(_x) #_x

#define BX_FILE_LINE_LITERAL "" __FILE__ "(" BX_STRINGIZE(__LINE__) "): "

#define BX_ALIGN_MASK(_value, _mask) ( ( (_value)+(_mask) ) & ( (~0)&(~(_mask) ) ) )
#define BX_ALIGN_16(_value) BX_ALIGN_MASK(_value, 0xf)
#define BX_ALIGN_256(_value) BX_ALIGN_MASK(_value, 0xff)
#define BX_ALIGN_4096(_value) BX_ALIGN_MASK(_value, 0xfff)

#if BX_COMPILER_GCC || BX_COMPILER_CLANG
#	define BX_ALIGN_STRUCT(_align, _struct) _struct __attribute__( (aligned(_align) ) )
#	define BX_ALLOW_UNUSED __attribute__( (unused) )
#	define BX_FORCE_INLINE __extension__ static __inline __attribute__( (__always_inline__) )
#	define BX_FUNCTION __PRETTY_FUNCTION__
#	define BX_NO_INLINE __attribute__( (noinline) )
#	define BX_NO_RETURN __attribute__( (noreturn) )
#	define BX_NO_VTABLE
#	define BX_OVERRIDE
#	define BX_PRINTF_ARGS(_format, _args) __attribute__ ( (format(__printf__, _format, _args) ) )
#	if BX_COMPILER_CLANG
#		define BX_THREAD /* not supported right now */
#	else
#		define BX_THREAD __thread
#	endif // BX_COMPILER_CLANG
#elif BX_COMPILER_MSVC
#	define BX_ALIGN_STRUCT(_align, _struct) __declspec(align(_align) ) _struct
#	define BX_ALLOW_UNUSED
#	define BX_FORCE_INLINE __forceinline
#	define BX_FUNCTION __FUNCTION__
#	define BX_NO_INLINE __declspec(noinline)
#	define BX_NO_RETURN
#	define BX_NO_VTABLE __declspec(novtable)
#	define BX_OVERRIDE override
#	define BX_PRINTF_ARGS(_format, _args)
#	define BX_THREAD __declspec(thread)
#else
#	error "Unknown BX_COMPILER_?"
#endif

#define BX_CACHE_LINE_ALIGN_MARKER() BX_ALIGN_STRUCT(BX_CACHE_LINE_SIZE, struct) {}
#define BX_CACHE_LINE_ALIGN(_def) BX_CACHE_LINE_ALIGN_MARKER(); _def; BX_CACHE_LINE_ALIGN_MARKER()

#define BX_ALIGN_STRUCT_16(_struct) BX_ALIGN_STRUCT(16, _struct)
#define BX_ALIGN_STRUCT_256(_struct) BX_ALIGN_STRUCT(256, _struct)

#define BX_UNUSED(_unused) do { (void)sizeof(_unused); } while(0)

#ifndef BX_CHECK
#	define BX_CHECK(...) do {} while(0)
#endif // BX_CHECK

#ifndef BX_TRACE
#	define BX_TRACE(...) do {} while(0)
#endif // BX_TRACE

#ifndef BX_WARN
#	define BX_WARN(...) do {} while(0)
#endif // BX_CHECK

#ifndef  BX_CONFIG_SPSCQUEUE_USE_MUTEX
#	define BX_CONFIG_SPSCQUEUE_USE_MUTEX 0
#endif // BX_CONFIG_SPSCQUEUE_USE_MUTEX

#ifndef BX_CONFIG_CRT_FILE_READER_WRITER
#	define BX_CONFIG_CRT_FILE_READER_WRITER BX_PLATFORM_WINDOWS|BX_PLATFORM_LINUX|BX_PLATFORM_OSX
#endif // BX_CONFIG_CRT_FILE_READER_WRITER

#endif // __BX_MACROS_H__
