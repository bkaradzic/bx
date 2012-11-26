/*
 * Copyright 2010-2012 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_OS_H__
#define __BX_OS_H__

#include "bx.h"

#if BX_PLATFORM_NACL || BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX || BX_PLATFORM_OSX
#	include <sched.h> // sched_yield
#	if BX_PLATFORM_NACL
#		include <sys/nacl_syscalls.h> // nanosleep
#	else
#		include <time.h> // nanosleep
#	endif // BX_PLATFORM_NACL
#endif // BX_PLATFORM_

namespace bx
{
	inline void sleep(uint32_t _ms)
	{
#if BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360
		Sleep(_ms);
#else
		timespec req = {_ms/1000, (_ms%1000)*1000000};
		timespec rem = {0, 0};
		nanosleep(&req, &rem);
#endif // BX_PLATFORM_
	}

	inline void yield()
	{
#if BX_PLATFORM_WINDOWS
		SwitchToThread();
#elif BX_PLATFORM_XBOX360
		Sleep(0);
#else
		sched_yield();
#endif // BX_PLATFORM_
	}

} // namespace bx

#endif // __BX_OS_H__
