/*
 * Copyright 2010-2011 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_MACROS_H__
#define __BX_MACROS_H__

#include "bx.h"

#define BX_VA_ARGS_COUNT_DETAIL(_a1, _a2, _a3, _a4, _a5, _a6, _a7, _a8, _a9, _a10, _a11, _a12, _a13, _a14, _a15, _a16, _last, ...) _last
#define BX_VA_ARGS_COUNT(...) BX_VA_ARGS_COUNT_DETAIL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define BX_MACRO_DISPATCHER_DETAIL1(_func, _argCount) _func ## _argCount
#define BX_MACRO_DISPATCHER_DETAIL2(_func, _argCount) BX_MACRO_DISPATCHER_DETAIL1(_func, _argCount)
#define BX_MACRO_DISPATCHER(_func, ...) BX_MACRO_DISPATCHER_DETAIL2(_func, VA_ARGS_COUNT(__VA_ARGS__) )

#define BX_STRINGIZE(_x) BX_STRINGIZE_(_x)
#define BX_STRINGIZE_(_x) #_x

#define BX_FILE_LINE_LITERAL "" __FILE__ "(" BX_STRINGIZE(__LINE__) "): "

#define BX_ALIGN_MASK(_value, _mask) ( ( (_value)+(_mask) ) & ( (~0)&(~(_mask) ) ) )
#define BX_ALIGN_16(_value) BX_ALIGN_MASK(_value, 0xf)
#define BX_ALIGN_256(_value) BX_ALIGN_MASK(_value, 0xff)

#if BX_COMPILER_GCC || BX_COMPILER_CLANG
#	define BX_ALIGN_STRUCT(_align, _struct) _struct __attribute__( (aligned(_align) ) )
#	define BX_FUNCTION __PRETTY_FUNCTION__
#	define BX_NO_INLINE __attribute__( (noinline) )
#	define BX_FORCE_INLINE __extension__ static __inline __attribute__( (__always_inline__) )
#	if BX_COMPILER_CLANG
#		define BX_THREAD /* not supported right now */
#	else
#		define BX_THREAD __thread
#	endif // BX_COMPILER_CLANG
#elif BX_COMPILER_MSVC
#	define BX_ALIGN_STRUCT(_align, _struct) __declspec(align(_align) ) _struct
#	define BX_FUNCTION __FUNCTION__
#	define BX_NO_INLINE __declspec(noinline)
#	define BX_FORCE_INLINE __forceinline
#	define BX_THREAD __declspec(thread)
#else
#	error "Unknown BX_COMPILER_?"
#endif

#define BX_ALIGN_STRUCT_16(_struct) BX_ALIGN_STRUCT(16, _struct)
#define BX_ALIGN_STRUCT_256(_struct) BX_ALIGN_STRUCT(256, _struct)

#ifndef BX_CHECK
#	define BX_CHECK(...) do {} while(0)
#endif // BX_CHECK

#ifndef BX_TRACE
#	define BX_TRACE(...) do {} while(0)
#endif // BX_TRACE

#ifndef  BX_CONFIG_SPSCQUEUE_USE_MUTEX
#	define BX_CONFIG_SPSCQUEUE_USE_MUTEX 0
#endif // BX_CONFIG_SPSCQUEUE_USE_MUTEX

#endif // __BX_MACROS_H__
