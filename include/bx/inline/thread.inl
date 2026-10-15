/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_THREAD_H_HEADER_GUARD
#	error "Must be included from bx/thread.h!"
#endif // BX_THREAD_H_HEADER_GUARD

#define BX_TLS_DATA_TEB (BX_PLATFORM_WINDOWS && BX_CPU_X86 && !BX_CRT_NONE)

#if BX_TLS_DATA_TEB
#	if BX_COMPILER_MSVC
#		if BX_ARCH_64BIT
extern "C" unsigned __int64 __readgsqword(unsigned long _offset);
#			pragma intrinsic(__readgsqword)
#		else
extern "C" unsigned long __readfsdword(unsigned long _offset);
#			pragma intrinsic(__readfsdword)
#		endif // BX_ARCH_64BIT
#	endif // BX_COMPILER_MSVC

namespace bx
{
	constexpr uint32_t kTebTlsSlotsOffset          = BX_ARCH_64BIT ? 0x1480 : 0xe10;
	constexpr uint32_t kTebTlsExpansionSlotsOffset = BX_ARCH_64BIT ? 0x1780 : 0xf94;
	constexpr uint32_t kTebTlsSlotsNum             = 64;

#	if BX_COMPILER_MSVC && BX_ARCH_64BIT
	inline void* readTeb(uint32_t _offset)
	{
		return (void*)__readgsqword(_offset);
	}
#	elif BX_COMPILER_MSVC
	inline void* readTeb(uint32_t _offset)
	{
		return (void*)__readfsdword(_offset);
	}
#	elif BX_ARCH_64BIT
	inline void* readTeb(uint32_t _offset)
	{
		void* ptr;
		__asm__ volatile ("movq %%gs:(%1), %0" : "=r" (ptr) : "r" (uintptr_t(_offset) ) );
		return ptr;
	}
#	else
	inline void* readTeb(uint32_t _offset)
	{
		void* ptr;
		__asm__ volatile ("movl %%fs:(%1), %0" : "=r" (ptr) : "r" (uintptr_t(_offset) ) );
		return ptr;
	}
#	endif // BX_COMPILER_*

	inline void* TlsData::get() const
	{
		const uint32_t slot = *(const uint32_t*)m_internal;

		if (slot < kTebTlsSlotsNum) [[likely]]
		{
			return readTeb(kTebTlsSlotsOffset + slot*sizeof(void*) );
		}

		void** expansionSlots = (void**)readTeb(kTebTlsExpansionSlotsOffset);

		return NULL == expansionSlots
			? NULL
			: expansionSlots[slot - kTebTlsSlotsNum]
			;
	}

} // namespace bx
#endif // BX_TLS_DATA_TEB
