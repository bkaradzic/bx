/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_THREAD_H_HEADER_GUARD
#define BX_THREAD_H_HEADER_GUARD

#include "bx.h"
#include "semaphore.h"

namespace bx
{
	///
	typedef int32_t (*ThreadFn)(void* _userData);

	///
	class Thread
	{
		BX_CLASS(Thread
			, NO_COPY
			, NO_ASSIGNMENT
			);

	public:
		///
		Thread();

		///
		virtual ~Thread();

		///
		void init(ThreadFn _fn, void* _userData = NULL, uint32_t _stackSize = 0, const char* _name = NULL);

		///
		void shutdown();

		///
		bool isRunning() const;

		///
		int32_t getExitCode() const;

		///
		void setThreadName(const char* _name);

		void setAffinityMask(const uint32_t _coreAffinityMask)
		{
#if BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360
			DWORD_PTR result;
			BX_UNUSED(result);

			result = SetThreadAffinityMask(m_handle, _coreAffinityMask);
			BX_CHECK(0 != result, "SetThreadAffinityMask failed!");
#elif BX_PLATFORM_WINRT
			// We use ThreadPool::RunAsync to run the thread, we cannot set thread affinity!
			BX_UNUSED(_coreAffinityMask);
#elif BX_PLATFORM_POSIX
			int result;
			BX_UNUSED(result);

			cpu_set_t cpuset;
			CPU_ZERO(&cpuset);
			for(uint32_t cpuId=0; cpuId < 32; cpuId++)
			{
				if(_coreAffinityMask & (1 << cpuId))
					CPU_SET(cpuId, &cpuset);
			}
			result = pthread_setaffinity_np(m_handle, sizeof(cpu_set_t), &cpuset);
			BX_CHECK(0 == result, "pthread_setaffinity_np failed! %d", result);
#endif // BX_PLATFORM_
		}

	private:
		friend struct ThreadInternal;
		int32_t entry();

		BX_ALIGN_DECL(16, uint8_t) m_internal[64];

		ThreadFn  m_fn;
		void*     m_userData;
		Semaphore m_sem;
		uint32_t  m_stackSize;
		int32_t   m_exitCode;
		bool      m_running;
	};

	///
	class TlsData
	{
	public:
		///
		TlsData();

		///
		~TlsData();

		///
		void* get() const;

		///
		void set(void* _ptr);

	private:
		BX_ALIGN_DECL(16, uint8_t) m_internal[64];
	};

} // namespace bx

#endif // BX_THREAD_H_HEADER_GUARD
