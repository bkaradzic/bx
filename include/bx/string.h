/*
 * Copyright 2010-2012 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_PRINTF_H__
#define __BX_PRINTF_H__

#include "bx.h"
#include <alloca.h>
#include <stdarg.h> // va_list
#include <stdio.h>  // vsnprintf, vsnwprintf
#include <string.h>
#include <string>
#include <wchar.h>  // wchar_t

namespace bx
{
	/// Case insensitive string compare.
	inline int32_t stricmp(const char* _a, const char* _b)
	{
#if BX_COMPILER_MSVC
		return _stricmp(_a, _b);
#else
		return strcasecmp(_a, _b);
#endif // BX_COMPILER_
	}

	///
	inline size_t strnlen(const char* _str, size_t _max)
	{
		const char* end = _str + _max;
		const char* ptr;
		for (ptr = _str; ptr < end && *ptr != '\0'; ++ptr);
		return ptr - _str;
	}

	/// Find substring in string. Limit search to _size.
	inline const char* strnstr(const char* _str, const char* _find, size_t _size)
	{
		char first = *_find;
		if ('\0' == first)
		{
			return _str;
		}

		const char* cmp = _find + 1;
		size_t len = strlen(cmp);
		do 
		{
			for (char match = *_str++; match != first && 0 < _size; match = *_str++, --_size)
			{
				if ('\0' == match)
				{
					return NULL;
				}
			}

			if (0 == _size)
			{
				return NULL;
			}
			
		} while (0 != strncmp(_str, cmp, len) );

		return --_str;
	}

	/// Find new line. Returns pointer after new line terminator.
	inline const char* strnl(const char* _str)
	{
		for (; '\0' != *_str; _str += strnlen(_str, 1024) )
		{
			const char* eol = strnstr(_str, "\r\n", 1024);
			if (NULL != eol)
			{
				return eol + 2;
			}

			eol = strnstr(_str, "\n", 1024);
			if (NULL != eol)
			{
				return eol + 1;
			}
		}

		return _str;
	}

	/// Find end of line. Retuns pointer to new line terminator.
	inline const char* streol(const char* _str)
	{
		for (; '\0' != *_str; _str += strnlen(_str, 1024) )
		{
			const char* eol = strnstr(_str, "\r\n", 1024);
			if (NULL != eol)
			{
				return eol;
			}

			eol = strnstr(_str, "\n", 1024);
			if (NULL != eol)
			{
				return eol;
			}
		}

		return _str;
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

	// Normalize string to sane line endings.
	inline void eolLF(char* _out, size_t _size, const char* _str)
	{
		if (0 < _size)
		{
			char* end = _out + _size - 1;
			for (char ch = *_str++; ch != '\0' && _out < end; ch = *_str++)
			{
				if ('\r' != ch)
				{
					*_out++ = ch;
				}
			}

			*_out = '\0';
		}
	}

	/// Cross platform implementation of vsnprintf that returns number of
	/// characters which would have been written to the final string if
	/// enough space had been available.
	inline int32_t vsnprintf(char* _str, size_t _count, const char* _format, va_list _argList)
	{
#if BX_COMPILER_MSVC
		int32_t len = ::vsnprintf(_str, _count, _format, _argList);
		return -1 == len ? ::_vscprintf(_format, _argList) : len;
#else
		return ::vsnprintf(_str, _count, _format, _argList);
#endif // BX_COMPILER_MSVC
	}

	/// Cross platform implementation of vsnwprintf that returns number of
	/// characters which would have been written to the final string if
	/// enough space had been available.
	inline int32_t vsnwprintf(wchar_t* _str, size_t _count, const wchar_t* _format, va_list _argList)
	{
#if BX_COMPILER_MSVC
		int32_t len = ::_vsnwprintf_s(_str, _count*sizeof(wchar_t), _count, _format, _argList);
		return -1 == len ? ::_vscwprintf(_format, _argList) : len;
#elif defined(__MINGW32__)
		return ::vsnwprintf(_str, _count, _format, _argList);
#else
		return ::vswprintf(_str, _count, _format, _argList);
#endif // BX_COMPILER_MSVC
	}

	inline int32_t snprintf(char* _str, size_t _count, const char* _format, ...) // BX_PRINTF_ARGS(3, 4)
	{
		va_list argList;
		va_start(argList, _format);
		int32_t len = vsnprintf(_str, _count, _format, argList);
		va_end(argList);
		return len;
	}

	inline int32_t swnprintf(wchar_t* _out, size_t _count, const wchar_t* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		int32_t len = vsnwprintf(_out, _count, _format, argList);
		va_end(argList);
		return len;
	}

	inline void stringPrintfVargs(std::string& _out, const char* _format, va_list _argList)
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
		_out.append(out);
	}

	inline void stringPrintf(std::string& _out, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		stringPrintfVargs(_out, _format, argList);
		va_end(argList);
	}

} // namespace bx

#endif // __BX_PRINTF_H__
