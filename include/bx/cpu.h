/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_CPU_H_HEADER_GUARD
#define BX_CPU_H_HEADER_GUARD

#include "bx.h"

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
	template<typename Ty>
	Ty atomicFetchAndAddsat(volatile Ty* _ptr, Ty _value, Ty _max);

	///
	template<typename Ty>
	Ty atomicFetchAndSubsat(volatile Ty* _ptr, Ty _value, Ty _min);

	///
	void* atomicExchangePtr(void** _ptr, void* _new);

	/// Sequentially consistent atomic load of 32-bit, or 64-bit value.
	template<typename Ty>
	Ty atomicLoad(const volatile Ty* _ptr);

	/// Sequentially consistent atomic store of 32-bit, or 64-bit value.
	template<typename Ty>
	void atomicStore(volatile Ty* _ptr, Ty _value);

	/// Atomic store without ordering, for values that don't need to be immediately
	/// visible to other threads.
	template<typename Ty>
	void atomicStoreRelaxed(volatile Ty* _ptr, Ty _value);

	/// Atomic exchange of 32-bit, or 64-bit value. Returns previous value.
	template<typename Ty>
	Ty atomicExchange(volatile Ty* _ptr, Ty _new);

	/// Atomic compare and swap of pointer sized integer. Returns previous value.
	uintptr_t atomicCompareAndSwapPtr(volatile uintptr_t* _ptr, uintptr_t _old, uintptr_t _new);

	/// Hint to CPU that caller is spin waiting.
	void cpuRelax();

} // namespace bx

#include "inline/cpu.inl"

#endif // BX_CPU_H_HEADER_GUARD
