/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_THREAD_H__
#define __BX_THREAD_H__

#if BX_PLATFORM_POSIX
#	include <pthread.h>
#endif // BX_PLATFORM_POSIX

#include "sem.h"

namespace bx
{
	typedef int32_t (*ThreadFn)(void* _userData);

	class Thread
	{
		BX_CLASS_NO_COPY_NO_ASSIGNMENT(Thread);

	public:
		Thread()
#if BX_PLATFORM_WINDOWS|BX_PLATFORM_XBOX360
			: m_handle(INVALID_HANDLE_VALUE)
#elif BX_PLATFORM_POSIX
			: m_handle(0)
#endif // BX_PLATFORM_
			, m_fn(NULL)
			, m_userData(NULL)
			, m_stackSize(0)
			, m_exitCode(0 /*EXIT_SUCCESS*/)
			, m_running(false)
		{
		}

		virtual ~Thread()
		{
			if (m_running)
			{
				shutdown();
			}
		}

		void init(ThreadFn _fn, void* _userData = NULL, uint32_t _stackSize = 0)
		{
			BX_CHECK(!m_running, "Already running!");

			m_fn = _fn;
			m_userData = _userData;
			m_stackSize = _stackSize;
			m_running = true;

#if BX_PLATFORM_WINDOWS|BX_PLATFORM_XBOX360
			m_handle = CreateThread(NULL
				, m_stackSize
				, threadFunc
				, this
				, 0
				, NULL
				);
#elif BX_PLATFORM_POSIX
			int result;
			BX_UNUSED(result);

			pthread_attr_t attr;
			result = pthread_attr_init(&attr);
			BX_CHECK(0 == result, "pthread_attr_init failed! %d", result);

			if (0 != m_stackSize)
			{
				result = pthread_attr_setstacksize(&attr, m_stackSize);
				BX_CHECK(0 == result, "pthread_attr_setstacksize failed! %d", result);
			}

// 			sched_param sched;
// 			sched.sched_priority = 0;
// 			result = pthread_attr_setschedparam(&attr, &sched);
// 			BX_CHECK(0 == result, "pthread_attr_setschedparam failed! %d", result);

			result = pthread_create(&m_handle, &attr, &threadFunc, this);
			BX_CHECK(0 == result, "pthread_attr_setschedparam failed! %d", result);
#endif // BX_PLATFORM_

			m_sem.wait();
		}

		void shutdown()
		{
			BX_CHECK(m_running, "Not running!");
#if BX_PLATFORM_WINDOWS|BX_PLATFORM_XBOX360
			WaitForSingleObject(m_handle, INFINITE);
			GetExitCodeThread(m_handle, (DWORD*)&m_exitCode);
			CloseHandle(m_handle);
			m_handle = INVALID_HANDLE_VALUE;
#elif BX_PLATFORM_POSIX
			union
			{
				void* ptr;
				int32_t i;
			} cast;
			pthread_join(m_handle, &cast.ptr);
			m_exitCode = cast.i;
			m_handle = 0;
#endif // BX_PLATFORM_
			m_running = false;
		}

		bool isRunning() const
		{
			return m_running;
		}

	private:
		int32_t entry()
		{
			m_sem.post();
			return m_fn(m_userData);
		}

#if BX_PLATFORM_WINDOWS|BX_PLATFORM_XBOX360
		static DWORD WINAPI threadFunc(LPVOID _arg)
		{
			Thread* thread = (Thread*)_arg;
			int32_t result = thread->entry();
			return result;
		}
#else
		static void* threadFunc(void* _arg)
		{
			Thread* thread = (Thread*)_arg;
			union
			{
				void* ptr;
				int32_t i;
			} cast;
			cast.i = thread->entry();
			return cast.ptr;
		}
#endif // BX_PLATFORM_

#if BX_PLATFORM_WINDOWS|BX_PLATFORM_XBOX360
		HANDLE m_handle;
#elif BX_PLATFORM_POSIX
		pthread_t m_handle;
#endif // BX_PLATFORM_

		ThreadFn m_fn;
		void* m_userData;
		Semaphore m_sem;
		uint32_t m_stackSize;
		int32_t m_exitCode;
		bool m_running;
	};

} // namespace bx

#endif // __BX_THREAD_H__
