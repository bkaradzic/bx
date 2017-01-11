/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_STRING_H_HEADER_GUARD
#define BX_STRING_H_HEADER_GUARD

#include "bx.h"
#include <alloca.h>
#include <stdarg.h> // va_list
#include <wchar.h>  // wchar_t

#include <bx/allocator.h>
#include <bx/hash.h>

#ifndef va_copy
#	define va_copy(_a, _b) (_a) = (_b)
#endif // va_copy

namespace bx
{
	///
	bool toBool(const char* _str);

	/// Case insensitive string compare.
	int32_t stricmp(const char* _a, const char* _b);

	///
	size_t strnlen(const char* _str, size_t _max);

	/// Copy _num characters from string _src to _dst buffer of maximum _dstSize capacity
	/// including zero terminator. Copy will be terminated with '\0'.
	size_t strlncpy(char* _dst, size_t _dstSize, const char* _src, size_t _num = -1);

	/// Find substring in string. Limit search to _size.
	const char* strnstr(const char* _str, const char* _find, size_t _size);

	/// Find substring in string. Case insensitive.
	const char* stristr(const char* _str, const char* _find);

	/// Find substring in string. Case insensitive. Limit search to _max.
	const char* stristr(const char* _str, const char* _find, size_t _max);

	/// Find new line. Returns pointer after new line terminator.
	const char* strnl(const char* _str);

	/// Find end of line. Retuns pointer to new line terminator.
	const char* streol(const char* _str);

	/// Skip whitespace.
	const char* strws(const char* _str);

	/// Skip non-whitespace.
	const char* strnws(const char* _str);

	/// Skip word.
	const char* strword(const char* _str);

	/// Find matching block.
	const char* strmb(const char* _str, char _open, char _close);

	// Normalize string to sane line endings.
	void eolLF(char* _out, size_t _size, const char* _str);

	// Finds identifier.
	const char* findIdentifierMatch(const char* _str, const char* _word);

	// Finds any identifier from NULL terminated array of identifiers.
	const char* findIdentifierMatch(const char* _str, const char* _words[]);

	/// Cross platform implementation of vsnprintf that returns number of
	/// characters which would have been written to the final string if
	/// enough space had been available.
	int32_t vsnprintf(char* _str, size_t _count, const char* _format, va_list _argList);

	/// Cross platform implementation of vsnwprintf that returns number of
	/// characters which would have been written to the final string if
	/// enough space had been available.
	int32_t vsnwprintf(wchar_t* _str, size_t _count, const wchar_t* _format, va_list _argList);

	///
	int32_t snprintf(char* _str, size_t _count, const char* _format, ...);

	///
	int32_t swnprintf(wchar_t* _out, size_t _count, const wchar_t* _format, ...);

	///
	template <typename Ty>
	inline void stringPrintfVargs(Ty& _out, const char* _format, va_list _argList)
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

	///
	template <typename Ty>
	inline void stringPrintf(Ty& _out, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		stringPrintfVargs(_out, _format, argList);
		va_end(argList);
	}

	/// Replace all instances of substring.
	template <typename Ty>
	inline Ty replaceAll(const Ty& _str, const char* _from, const char* _to)
	{
		Ty str = _str;
		size_t startPos = 0;
		const size_t fromLen = strlen(_from);
		const size_t toLen   = strlen(_to);
		while ( (startPos = str.find(_from, startPos) ) != Ty::npos)
		{
			str.replace(startPos, fromLen, _to);
			startPos += toLen;
		}

		return str;
	}

	/// Extract base file name from file path.
	const char* baseName(const char* _filePath);

	/// Convert size in bytes to human readable string.
	void prettify(char* _out, size_t _count, uint64_t _size);

	/// Copy src to string dst of size siz.  At most siz-1 characters
	/// will be copied.  Always NUL terminates (unless siz == 0).
	/// Returns strlen(src); if retval >= siz, truncation occurred.
	size_t strlcpy(char* _dst, const char* _src, size_t _siz);

	/// Appends src to string dst of size siz (unlike strncat, siz is the
	/// full size of dst, not space left).  At most siz-1 characters
	/// will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
	/// Returns strlen(src) + MIN(siz, strlen(initial dst)).
	/// If retval >= siz, truncation occurred.
	size_t strlcat(char* _dst, const char* _src, size_t _siz);

	/// Non-zero-terminated string view.
	class StringView
	{
	public:
		StringView()
		{
			clear();
		}

		StringView(const StringView& _rhs)
		{
			set(_rhs.m_ptr, _rhs.m_len);
		}

		StringView& operator=(const StringView& _rhs)
		{
			set(_rhs.m_ptr, _rhs.m_len);
			return *this;
		}

		StringView(const char* _ptr, uint32_t _len = UINT16_MAX)
		{
			set(_ptr, _len);
		}

		void set(const char* _ptr, uint32_t _len = UINT16_MAX)
		{
			clear();

			if (NULL != _ptr)
			{
				uint32_t len = uint32_t(strnlen(_ptr, _len) );
				if (0 != len)
				{
					m_len = len;
					m_ptr = _ptr;
				}
			}
		}

		void clear()
		{
			m_ptr = "";
			m_len = 0;
		}

		const char* getPtr() const
		{
			return m_ptr;
		}

		const char* getTerm() const
		{
			return m_ptr + m_len;
		}

		bool isEmpty() const
		{
			return 0 == m_len;
		}

		uint32_t getLength() const
		{
			return m_len;
		}

	protected:
		const char* m_ptr;
		uint32_t    m_len;
	};

	inline uint32_t hashMurmur2A(const StringView& _data)
	{
		return hashMurmur2A(_data.getPtr(), _data.getLength() );
	}

	inline uint32_t hashMurmur2A(const char* _data)
	{
		return hashMurmur2A(StringView(_data) );
	}

	/// ASCII string
	template<bx::AllocatorI** AllocatorT>
	class StringT : public StringView
	{
	public:
		StringT()
			: StringView()
		{
		}

		StringT(const StringT<AllocatorT>& _rhs)
			: StringView()
		{
			set(_rhs.m_ptr, _rhs.m_len);
		}

		StringT<AllocatorT>& operator=(const StringT<AllocatorT>& _rhs)
		{
			set(_rhs.m_ptr, _rhs.m_len);
			return *this;
		}

		StringT(const char* _ptr, uint32_t _len = UINT32_MAX)
		{
			set(_ptr, _len);
		}

		StringT(const StringView& _rhs)
		{
			set(_rhs.getPtr(), _rhs.getLength() );
		}

		~StringT()
		{
			clear();
		}

		void set(const char* _ptr, uint32_t _len = UINT32_MAX)
		{
			clear();
			append(_ptr, _len);
		}

		void append(const char* _ptr, uint32_t _len = UINT32_MAX)
		{
			if (0 != _len)
			{
				uint32_t old = m_len;
				uint32_t len = m_len + uint32_t(strnlen(_ptr, _len) );
				char* ptr = (char*)BX_REALLOC(*AllocatorT, 0 != m_len ? const_cast<char*>(m_ptr) : NULL, len+1);
				m_len = len;
				strlncpy(ptr + old, len-old+1, _ptr, _len);

				*const_cast<char**>(&m_ptr) = ptr;
			}
		}

		void clear()
		{
			if (0 != m_len)
			{
				BX_FREE(*AllocatorT, const_cast<char*>(m_ptr) );

				StringView::clear();
			}
		}
	};

} // namespace bx

#endif // BX_STRING_H_HEADER_GUARD
