/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
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
#	include <dlfcn.h>
#endif // BX_PLATFORM_

namespace bx
{
	inline void sleep(uint32_t _ms)
	{
#if BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360
		Sleep(_ms);
#else
		timespec req = {(time_t)_ms/1000, (long)((_ms%1000)*1000000)};
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

	inline void* dlopen(const char* _filePath)
	{
#if BX_PLATFORM_WINDOWS
		return (void*)LoadLibraryA(_filePath);
#else
		return ::dlopen(_filePath, RTLD_LOCAL|RTLD_LAZY);
#endif // BX_PLATFORM_
	}

	inline void dlclose(void* _handle)
	{
#if BX_PLATFORM_WINDOWS
		FreeLibrary( (HMODULE)_handle);
#else
		::dlclose(_lib);
#endif // BX_PLATFORM_
	}

	inline void* dlsym(void* _handle, const char* _symbol)
	{
#if BX_PLATFORM_WINDOWS
		return (void*)GetProcAddress( (HMODULE)_handle, _symbol);
#else
		return dlsym(_handle, _symbol);
#endif // BX_PLATFORM_
	}

	inline void setenv(const char* _name, const char* _value)
	{
#if BX_PLATFORM_WINDOWS
		SetEnvironmentVariableA(_name, _value);
#else
		setenv(_name, _value, 1);
#endif // BX_PLATFORM_
	}

	inline void unsetenv(const char* _name)
	{
#if BX_PLATFORM_WINDOWS
		SetEnvironmentVariableA(_name, NULL);
#else
		unsetenv(_name);
#endif // BX_PLATFORM_
	}

} // namespace bx

#endif // __BX_OS_H__
