/*
 * Copyright 2010-2012 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_PRINTF_H__
#define __BX_PRINTF_H__

#include "bx.h"
#include <alloca.h>
#include <stdarg.h> // va_list
#include <string>

namespace bx
{
	inline int32_t stricmp(const char* _a, const char* _b)
	{
#if BX_COMPILER_MSVC
		return _stricmp(_a, _b);
#else
		return strcasecmp(_a, _b);
#endif // BX_COMPILER_
	}

	/// Find new line. Returns pointer after new line terminator.
	inline const char* strnl(const char* _str)
	{
		const char* eol = strstr(_str, "\n\r");
		if (NULL != eol)
		{
			return eol + 2;
		}

		eol = strstr(_str, "\n");
		if (NULL != eol)
		{
			return eol + 1;
		}

		return eol + strlen(_str);
	}

	/// Find end of line. Retuns pointer to new line terminator.
	inline const char* streol(const char* _str)
	{
		const char* eol = strstr(_str, "\n\r");
		if (NULL != eol)
		{
			return eol;
		}

		eol = strstr(_str, "\n");
		if (NULL != eol)
		{
			return eol;
		}

		return eol + strlen(_str);
	}

	/// Skip whitespace.
	inline const char* strws(const char* _str)
	{
		for (; isspace(*_str); ++_str);
		return _str;
	}

	/// Skip non-whitespace.
	inline const char* strnws(const char* _str)
	{
		for (; !isspace(*_str); ++_str);
		return _str;
	}

	/// Skip word.
	inline const char* strword(const char* _str)
	{
		for (char ch = *_str++; isalnum(ch) || '_' == ch; ch = *_str++);
		return _str-1;
	}

	/// Find matching block.
	inline const char* strmb(const char* _str, char _open, char _close)
	{
		int count = 0;
		for (char ch = *_str++; ch != '\0' && count >= 0; ch = *_str++)
		{
			if (ch == _open)
			{
				count++;
			}
			else if (ch == _close)
			{
				count--;
				if (0 == count)
				{
					return _str-1;
				}
			}
		}

		return NULL;
	}

	/// Cross platform implementation of vsnprintf that returns number of
	/// characters which would have been written to the final string if
	/// enough space had been available.
	inline int32_t vsnprintf(char* _str, size_t _size, const char* _format, va_list _argList)
	{
#if BX_COMPILER_MSVC
		int32_t len = ::vsnprintf(_str, _size, _format, _argList);
		return -1 == len ? ::_vscprintf(_format, _argList) : len;
#else
		return ::vsnprintf(_str, _size, _format, _argList);
#endif // BX_COMPILER_MSVC
	}

	inline int32_t snprintf(char* _str, size_t _size, const char* _format, ...) BX_PRINTF_ARGS(3, 4)
	{
		va_list argList;
		va_start(argList, _format);
		int32_t len = vsnprintf(_str, _size, _format, argList);
		va_end(argList);
		return len;
	}

	inline std::string stringPrintfVargs(const char* _format, va_list _argList)
	{
		char temp[2048];

		char* out = temp;
		int32_t len = bx::vsnprintf(out, sizeof(temp), _format, _argList);
		if ( (int32_t)sizeof(temp) < len)
		{
			out = (char*)alloca(len+1);
			len = bx::vsnprintf(out, len, _format, _argList);
		}
		out[len] = '\0';

		return out;
	}

	inline std::string stringPrintf(const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		std::string str = stringPrintfVargs(_format, argList);
		va_end(argList);
		return str;
	}
} // namespace bx

#endif // __BX_PRINTF_H__
