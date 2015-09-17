/*
 * Copyright 2010-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef BX_CPU_H_HEADER_GUARD
#define BX_CPU_H_HEADER_GUARD

#include "bx.h"

#if BX_COMPILER_MSVC
#	if BX_PLATFORM_XBOX360
#		include <ppcintrinsics.h>
#		include <xtl.h>
#	else
#		include <math.h> // math.h is included because VS bitches:
						 // warning C4985: 'ceil': attributes not present on previous declaration.
						 // must be included before intrin.h.
#		include <intrin.h>
#		include <windows.h>
#	endif // !BX_PLATFORM_XBOX360
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
	inline void readBarrier()
	{
#if BX_COMPILER_MSVC
		_ReadBarrier();
#else
		asm volatile("":::"memory");
#endif // BX_COMPILER
	}

	///
	inline void writeBarrier()
	{
#if BX_COMPILER_MSVC
		_WriteBarrier();
#else
		asm volatile("":::"memory");
#endif // BX_COMPILER
	}

	///
	inline void readWriteBarrier()
	{
#if BX_COMPILER_MSVC
		_ReadWriteBarrier();
#else
		asm volatile("":::"memory");
#endif // BX_COMPILER
	}

	///
	inline void memoryBarrier()
	{
#if BX_PLATFORM_XBOX360
		__lwsync();
#elif BX_PLATFORM_WINRT
		MemoryBarrier();
#elif BX_COMPILER_MSVC
		_mm_mfence();
#else
		__sync_synchronize();
//		asm volatile("mfence":::"memory");
#endif // BX_COMPILER
	}

	inline int32_t atomicFetchAndAdd(volatile int32_t* _ptr, int32_t _add)
	{
#if BX_COMPILER_MSVC
		return _InterlockedExchangeAdd( (volatile long*)_ptr, _add);
#else
		return __sync_fetch_and_add(_ptr, _add);
#endif // BX_COMPILER_
	}

	inline int32_t atomicAddAndFetch(volatile int32_t* _ptr, int32_t _add)
	{
#if BX_COMPILER_MSVC
		return atomicFetchAndAdd(_ptr, _add) + _add;
#else
		return __sync_add_and_fetch(_ptr, _add);
#endif // BX_COMPILER_
	}

	inline int32_t atomicFetchAndSub(volatile int32_t* _ptr, int32_t _sub)
	{
#if BX_COMPILER_MSVC
		return atomicFetchAndAdd(_ptr, -_sub);
#else
		return __sync_fetch_and_sub(_ptr, _sub);
#endif // BX_COMPILER_
	}

	inline int32_t atomicSubAndFetch(volatile int32_t* _ptr, int32_t _sub)
	{
#if BX_COMPILER_MSVC
		return atomicFetchAndAdd(_ptr, -_sub) - _sub;
#else
		return __sync_sub_and_fetch(_ptr, _sub);
#endif // BX_COMPILER_
	}

	/// Returns the resulting incremented value.
	inline int32_t atomicInc(volatile int32_t* _ptr)
	{
		return atomicAddAndFetch(_ptr, 1);
	}

	/// Returns the resulting decremented value.
	inline int32_t atomicDec(volatile int32_t* _ptr)
	{
		return atomicSubAndFetch(_ptr, 1);
	}

	///
	inline int32_t atomicCompareAndSwap(volatile void* _ptr, int32_t _old, int32_t _new)
	{
#if BX_COMPILER_MSVC
		return _InterlockedCompareExchange( (volatile LONG*)(_ptr), _new, _old);
#else
		return __sync_val_compare_and_swap( (volatile int32_t*)_ptr, _old, _new);
#endif // BX_COMPILER
	}

	///
	inline void* atomicExchangePtr(void** _ptr, void* _new)
	{
#if BX_COMPILER_MSVC
		return InterlockedExchangePointer(_ptr, _new);
#else
		return __sync_lock_test_and_set(_ptr, _new);
#endif // BX_COMPILER
	}

	///
	inline int32_t atomicTestAndInc(volatile void* _ptr, int32_t _test)
	{
		int32_t oldVal;
		int32_t newVal = *(int32_t volatile*)_ptr;
		do
		{
			oldVal = newVal;
			newVal = atomicCompareAndSwap(_ptr, oldVal, newVal >= _test ? _test : newVal+1);

		} while (oldVal != newVal);

		return oldVal;
	}

	///
	inline int32_t atomicTestAndDec(volatile void* _ptr, int32_t _test)
	{
		int32_t oldVal;
		int32_t newVal = *(int32_t volatile*)_ptr;
		do
		{
			oldVal = newVal;
			newVal = atomicCompareAndSwap(_ptr, oldVal, newVal <= _test ? _test : newVal-1);

		} while (oldVal != newVal);

		return oldVal;
	}

} // namespace bx

#endif // BX_CPU_H_HEADER_GUARD
