/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_SPANALLOCATOR_H_HEADER_GUARD
#define BX_SPANALLOCATOR_H_HEADER_GUARD

#include <bx/mutex.h>

#if BX_CRT_NONE
#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_WINRT || BX_PLATFORM_XBOXONE
#	include <windows.h>
#elif BX_PLATFORM_POSIX
#	include <pthread.h>
#endif // BX_PLATFORM_*

namespace bx
{
	namespace
	{
		typedef void (*ThreadExitFn)(void* _heap);

		inline uint64_t sliceMask(uint32_t _first, uint32_t _num)
		{
			return ( (UINT64_C(1)<<_num) - 1)<<_first;
		}

		template<typename Ty>
		inline Mutex& getSegmentMapMutex()
		{
			alignas(Mutex) static uint8_t s_mutexStorage[sizeof(Mutex)];
			static Mutex* s_mutex = BX_PLACEMENT_NEW(s_mutexStorage, Mutex);
			return *s_mutex;
		}

#if BX_CRT_NONE
		constexpr uintptr_t kInvalidExitKey = UINTPTR_MAX;
#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_WINRT || BX_PLATFORM_XBOXONE
		constexpr uintptr_t kInvalidExitKey = FLS_OUT_OF_INDEXES;

		template<ThreadExitFn ThreadExitFnT>
		void NTAPI threadExitCallback(void* _heap)
		{
			ThreadExitFnT(_heap);
		}
#else
		constexpr uintptr_t kInvalidExitKey = UINTPTR_MAX;
#endif // BX_PLATFORM_*

		template<ThreadExitFn ThreadExitFnT>
		inline uintptr_t createExitKey()
		{
#if BX_CRT_NONE
			return kInvalidExitKey;
#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_WINRT || BX_PLATFORM_XBOXONE
			return FlsAlloc(threadExitCallback<ThreadExitFnT>);
#elif BX_PLATFORM_POSIX
			pthread_key_t key;
			return 0 == pthread_key_create(&key, ThreadExitFnT) ? uintptr_t(key) : kInvalidExitKey;
#else
			return kInvalidExitKey;
#endif // BX_PLATFORM_*
		}

		inline void destroyExitKey(uintptr_t _key)
		{
			if (kInvalidExitKey == _key)
			{
				return;
			}

#if BX_CRT_NONE
#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_WINRT || BX_PLATFORM_XBOXONE
			FlsFree(DWORD(_key) );
#elif BX_PLATFORM_POSIX
			pthread_key_delete(pthread_key_t(_key) );
#endif // BX_PLATFORM_*
		}

		inline void setExitValue(uintptr_t _key, void* _value)
		{
			if (kInvalidExitKey == _key)
			{
				return;
			}

#if BX_CRT_NONE
#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_WINRT || BX_PLATFORM_XBOXONE
			FlsSetValue(DWORD(_key), _value);
#elif BX_PLATFORM_POSIX
			pthread_setspecific(pthread_key_t(_key), _value);
#endif // BX_PLATFORM_*
		}

	} // namespace

} // namespace bx

#endif // BX_SPANALLOCATOR_H_HEADER_GUARD
