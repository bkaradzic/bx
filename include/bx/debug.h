/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_DEBUG_H__
#define __BX_DEBUG_H__

#include "bx.h"

#if BX_PLATFORM_ANDROID
#	include <android/log.h>
#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* _str);
#else
#	include <stdio.h>
#endif // BX_PLATFORM_WINDOWS

namespace bx
{
	inline void debugBreak()
	{
#if BX_COMPILER_MSVC
		__debugbreak();
#elif BX_CPU_ARM
		asm("bkpt 0");
#elif !BX_PLATFORM_NACL && BX_CPU_X86 && (BX_COMPILER_GCC || BX_COMPILER_CLANG)
		// NaCl doesn't like int 3:
		// NativeClient: NaCl module load failed: Validation failure. File violates Native Client safety rules.
		__asm__ ("int $3");
#else // cross platform implementation
		int* int3 = (int*)3L;
		*int3 = 3;
#endif // BX
	}

	inline void debugOutput(const char* _out)
	{
#if BX_PLATFORM_ANDROID
		__android_log_write(ANDROID_LOG_DEBUG, "", _out);
#elif BX_PLATFORM_WINDOWS || BX_PLATFORM_XBOX360
		OutputDebugStringA(_out);
#else
		fputs(_out, stderr);
		fflush(stderr);
#endif // BX_PLATFORM_
	}

} // namespace bx

#endif // __BX_DEBUG_H__
