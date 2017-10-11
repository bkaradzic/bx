/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_CPU_H_HEADER_GUARD
#define BX_CPU_H_HEADER_GUARD

#include "bx.h"

#if BX_COMPILER_MSVC
#	include <math.h> // math.h is included because VS bitches:
					 // warning C4985: 'ceil': attributes not present on previous declaration.
					 // must be included before intrin.h.
#	include <intrin.h>
#	include <windows.h>
#	if BX_PLATFORM_WINRT
#		define _InterlockedExchangeAdd64 InterlockedExchangeAdd64
#	endif // BX_PLATFORM_WINRT
extern "C" void _ReadBarrier();
extern "C" void _WriteBarrier();
extern "C" void _ReadWriteBarrier();
#	pragma intrinsic(_ReadBarrier)
#	pragma intrinsic(_WriteBarrier)
#	pragma intrinsic(_ReadWriteBarrier)
#	pragma intrinsic(_InterlockedExchangeAdd)
#	pragma intrinsic(_InterlockedCompareExchange)
#endif // BX_COMPILER_MSVC

namespace bx
{
	///
	void readBarrier();

	///
	void writeBarrier();

	///
	void readWriteBarrier();

	///
	void memoryBarrier();

	///
	template<typename Ty>
	Ty atomicFetchAndAdd(volatile Ty* _ptr, Ty _value);

	///
	template<typename Ty>
	Ty atomicAddAndFetch(volatile Ty* _ptr, Ty _value);

	///
	template<typename Ty>
	Ty atomicFetchAndSub(volatile Ty* _ptr, Ty _value);

	///
	template<typename Ty>
	Ty atomicSubAndFetch(volatile Ty* _ptr, Ty _value);

	///
	template<typename Ty>
	Ty atomicCompareAndSwap(volatile Ty* _ptr, Ty _old, Ty _new);

	///
	template<typename Ty>
	Ty atomicFetchTestAndAdd(volatile Ty* _ptr, Ty _test, Ty _value);

	///
	template<typename Ty>
	Ty atomicFetchTestAndSub(volatile Ty* _ptr, Ty _test, Ty _value);

	///
	void* atomicExchangePtr(void** _ptr, void* _new);

} // namespace bx

#include "inline/cpu.inl"

#endif // BX_CPU_H_HEADER_GUARD
