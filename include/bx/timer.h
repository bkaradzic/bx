/*
 * Copyright 2010-2011 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_TIMER_H__
#define __BX_TIMER_H__

#include "bx.h"

#if BX_PLATFORM_ANDROID
#	include <time.h> // clock, clock_gettime
#elif BX_PLATFORM_NACL | BX_PLATFORM_LINUX
#	include <sys/time.h> // gettimeofday
#endif // BX_PLATFORM_

namespace bx
{
	inline int64_t getHPCounter()
	{
#if BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360
		LARGE_INTEGER li;
		// Performance counter value may unexpectedly leap forward
		// http://support.microsoft.com/kb/274323
		QueryPerformanceCounter(&li);
		int64_t i64 = li.QuadPart;
#elif BX_PLATFORM_ANDROID
		int64_t i64 = clock();
#else
		struct timeval now;
		gettimeofday(&now, 0);
		int64_t i64 = now.tv_sec*1000000 + now.tv_usec;
#endif // BNET_PLATFORM_
		static int64_t offset = i64;
		return i64 - offset;
	}

	inline int64_t getHPFrequency()
	{
#if BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		return li.QuadPart;
#elif BX_PLATFORM_ANDROID
		return CLOCKS_PER_SEC;
#else
		return 1000000;
#endif // BNET_PLATFORM_
	}

} // namespace bx

#endif // __BX_TIMER_H__
