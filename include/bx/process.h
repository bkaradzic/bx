/*
 * Copyright 2010-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef BX_PROCESS_H_HEADER_GUARD
#define BX_PROCESS_H_HEADER_GUARD

#if BX_PLATFORM_LINUX
#	include <unistd.h>
#endif // BX_PLATFORM_LINUX

namespace bx
{
	///
	inline void* exec(const char*const _argv[])
	{
#if BX_PLATFORM_LINUX
		pid_t pid = fork();

		if (0 == pid)
		{
			int result = execvp(_argv[0], const_cast<char *const*>(&_argv[1]) );
			BX_UNUSED(result);
			return NULL;
		}

		return (void*)uintptr_t(pid);
#else
		return NULL;
#endif // BX_PLATFORM_LINUX
	}

} // namespace bx

#endif // BX_PROCESS_H_HEADER_GUARD
