/*
 * Copyright 2010-2012 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_SEM_H__
#define __BX_SEM_H__

#include "bx.h"

#if BX_PLATFORM_POSIX
#	include <semaphore.h>
#	include <time.h>
#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360
#	include <windows.h>
#	include <limits.h>
#endif // BX_PLATFORM_

namespace bx
{
#if BX_PLATFORM_POSIX
	class Semaphore
	{
	public:
		Semaphore()
		{
			sem_init(&m_handle, 0, 0);
		}

		~Semaphore()
		{
			sem_destroy(&m_handle);
		}

		void post(uint32_t _count = 1)
		{
			for (uint32_t ii = 0; ii < _count; ++ii)
			{
				sem_post(&m_handle);
			}
		}

		bool wait(int32_t _msecs = -1)
		{
#if BX_PLATFORM_NACL || BX_PLATFORM_OSX
			BX_CHECK(-1 == _msecs, "NaCl and OSX don't support sem_timedwait at this moment.");
			return 0 == sem_wait(&m_handle);
#else
			if (0 > _msecs)
			{
				return 0 == sem_wait(&m_handle);
			}

			timespec ts;
			ts.tv_sec = _msecs/1000;
			ts.tv_nsec = (_msecs%1000)*1000;
			return 0 == sem_timedwait(&m_handle, &ts);
#endif // BX_PLATFORM_
		}

	private:
		Semaphore(const Semaphore& _rhs); // no copy constructor
		Semaphore& operator=(const Semaphore& _rhs); // no assignment operator

		sem_t m_handle;
	};

#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360

	class Semaphore
	{
	public:
		Semaphore()
		{
			m_handle = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
			BX_CHECK(NULL != m_handle, "Failed to create Semaphore!");
		}

		~Semaphore()
		{
			CloseHandle(m_handle);
		}

		void post(uint32_t _count = 1) const
		{
			ReleaseSemaphore(m_handle, _count, NULL);
		}

		bool wait(int32_t _msecs = -1) const
		{
			DWORD milliseconds = (0 > _msecs) ? INFINITE : _msecs;
			return WAIT_OBJECT_0 == WaitForSingleObject(m_handle, milliseconds);
		}

	private:
		Semaphore(const Semaphore& _rhs); // no copy constructor
		Semaphore& operator=(const Semaphore& _rhs); // no assignment operator

		HANDLE m_handle;
	};

#endif // BX_PLATFORM_

} // namespace bx

#endif // __BX_SEM_H__
