/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bnet#license-bsd-2-clause
 */

#include <bx/scanner.h>
#include <bx/url.h>

namespace bx
{
	static bool isNotAuthorityEnd(char _ch)
	{
		return '/' != _ch
			&& '?' != _ch
			&& '#' != _ch
			;
	}

	static bool isNotSchemeEnd(char _ch)
	{
		return ':' != _ch
			&& isNotAuthorityEnd(_ch)
			;
	}

	static bool isNotColon(char _ch)
	{
		return ':' != _ch;
	}

	static bool isNotFragment(char _ch)
	{
		return '#' != _ch;
	}

	static bool isNotQueryOrFragment(char _ch)
	{
		return '?' != _ch
			&& isNotFragment(_ch)
			;
	}

	static bool isSchemeChar(char _ch)
	{
		return isAlphaNum(_ch)
			|| '+' == _ch
			|| '-' == _ch
			|| '.' == _ch
			;
	}

	static bool isSchemeName(const StringView& _str)
	{
		Scanner scanner(_str);

		if (scanner.accept(isAlpha).isEmpty() )
		{
			return false;
		}

		scanner.acceptWhile(isSchemeChar);

		return scanner.isDone();
	}

	static bool isPortThenPath(const StringView& _str)
	{
		Scanner scanner(_str);

		return !scanner.acceptWhile(isNumeric).isEmpty()
			&& (scanner.isDone() || !scanner.peek('/').isEmpty() )
			;
	}

	UrlView::UrlView()
	{
	}

	void UrlView::clear()
	{
		for (uint32_t ii = 0; ii < Count; ++ii)
		{
			m_tokens[ii].clear();
		}
	}

	bool UrlView::parse(const StringView& _url)
	{
		clear();

		Scanner scanner(_url);
		Scanner lookahead(_url);

		const StringView name = lookahead.acceptWhile(isNotSchemeEnd);

		bool hasScheme = false;

		if (!lookahead.accept(':').isEmpty() )
		{
			const bool slashes = !lookahead.peek("//").isEmpty();

			if (!isSchemeName(name) )
			{
				if (slashes)
				{
					return false;
				}
			}
			else if (slashes
			||  !isPortThenPath(lookahead.getTail() ) )
			{
				hasScheme = true;
				m_tokens[Scheme].set(name);
				scanner.seek(lookahead.getCursor() );
			}
		}

		StringView authority = scanner.getCursor();

		if (!hasScheme)
		{
			authority = scanner.acceptWhile(isNotAuthorityEnd);

			if (scanner.peek('/').isEmpty() )
			{
				return false;
			}
		}
		else if (!scanner.accept("//").isEmpty() )
		{
			authority = scanner.acceptWhile(isNotAuthorityEnd);
		}

		m_tokens[Path].set(scanner.acceptWhile(isNotQueryOrFragment) );

		if (!scanner.accept('?').isEmpty() )
		{
			m_tokens[Query].set(scanner.acceptWhile(isNotFragment) );
		}

		if (!scanner.accept('#').isEmpty() )
		{
			m_tokens[Fragment].set(scanner.acceptAll() );
		}

		StringView hostPort = authority;

		const StringView at = strRFind(authority, '@');

		if (!at.isEmpty() )
		{
			Scanner userInfoScanner(StringView(authority.getPtr(), at.getPtr() ) );

			m_tokens[UserName].set(userInfoScanner.acceptWhile(isNotColon) );

			if (!userInfoScanner.accept(':').isEmpty() )
			{
				m_tokens[Password].set(userInfoScanner.acceptAll() );
			}

			hostPort.set(at.getTerm(), authority.getTerm() );
		}

		Scanner hostScanner(hostPort);

		const StringView open = hostScanner.accept('[');

		if (open.isEmpty() )
		{
			m_tokens[Host].set(hostScanner.acceptWhile(isNotColon) );
		}
		else
		{
			hostScanner.acceptUntil("]");

			const StringView close = hostScanner.accept(']');

			if (close.isEmpty() )
			{
				return false;
			}

			m_tokens[Host].set(open.getPtr(), close.getTerm() );
		}

		if (!hostScanner.accept(':').isEmpty() )
		{
			const StringView port = hostScanner.acceptAll();

			if (!port.isEmpty()
			&&  !isNumeric(port) )
			{
				return false;
			}

			m_tokens[Port].set(port);
		}

		return hostScanner.isDone();
	}

	const StringView& UrlView::get(Enum _token) const
	{
		return m_tokens[_token];
	}

	constexpr int32_t kUrlEscapeSize   = 3;
	constexpr char    kUrlFilePrefix[] = "file://";

	static char toHex(uint8_t _nible)
	{
		return "0123456789ABCDEF"[_nible&0xf];
	}

	// https://secure.wikimedia.org/wikipedia/en/wiki/URL_encoding
	int32_t urlEncode(char* _out, int32_t _max, const StringView& _str, UrlEncoding::Enum _encoding)
	{
		if (0 >= _max)
		{
			return 0;
		}

		const char* str = _str.getPtr();

		const int32_t max = _max-1; // need space for zero terminator
		int32_t len = 0;

		for (int32_t ii = 0, num = _str.getLength(); ii < num; ++ii)
		{
			const char ch = str[ii];

			if (isAlphaNum(ch)
			||  '-' == ch
			||  '_' == ch
			||  '.' == ch
			||  '~' == ch
			|| ('/' == ch && UrlEncoding::Path == _encoding) )
			{
				if (max < len+1)
				{
					break;
				}

				_out[len++] = ch;
			}
			else
			{
				// Escape sequence must not be split by truncation.
				if (max < len+kUrlEscapeSize)
				{
					break;
				}

				_out[len++] = '%';
				_out[len++] = toHex(uint8_t(ch)>>4);
				_out[len++] = toHex(uint8_t(ch) );
			}
		}

		_out[len] = '\0';

		return len;
	}

	static uint8_t fromHex(char _ch)
	{
		return uint8_t(isNumeric(_ch) ? _ch - '0' : toLower(_ch) - 'a' + 10);
	}

	int32_t urlDecode(char* _out, int32_t _max, const StringView& _str)
	{
		if (0 >= _max)
		{
			return 0;
		}

		const char* str = _str.getPtr();

		const int32_t max = _max-1;
		const int32_t num = _str.getLength();

		int32_t len = 0;

		for (int32_t ii = 0; ii < num && len < max; ++ii, ++len)
		{
			if ('%' == str[ii]
			&&  ii+2 < num
			&&  isHexNum(str[ii+1])
			&&  isHexNum(str[ii+2]) )
			{
				_out[len] = char(fromHex(str[ii+1])<<4 | fromHex(str[ii+2]) );
				ii += 2;
			}
			else
			{
				_out[len] = str[ii];
			}
		}

		_out[len] = '\0';

		return len;
	}

	static bool hasDriveLetter(const char* _path)
	{
		return isAlpha(_path[0])
			&& ':' == _path[1]
			;
	}

	bool urlToFilePath(FilePath& _outFilePath, const StringView& _url)
	{
		UrlView url;

		if (!url.parse(_url)
		||  0 != strCmpI(url.get(UrlView::Scheme), "file") )
		{
			return false;
		}

		const StringView& host = url.get(UrlView::Host);
		const StringView& path = url.get(UrlView::Path);

		char decoded[kMaxFilePath*3];

		if (path.isEmpty()
		||  '/' != *path.getPtr()
		||  int32_t(BX_COUNTOF(decoded) ) <= path.getLength()
		|| (!host.isEmpty() && 0 != strCmpI(host, "localhost") ) )
		{
			return false;
		}

		const int32_t len = urlDecode(decoded, BX_COUNTOF(decoded), path);

		if (kMaxFilePath <= len
		||  len != strLen(decoded) )
		{
			return false;
		}

		const bool drive = BX_ENABLED(BX_PLATFORM_WINDOWS)
			&& 3 <= len
			&& hasDriveLetter(&decoded[1])
			;

		_outFilePath.set(StringView(&decoded[drive ? 1 : 0], &decoded[len]) );

		return true;
	}

	int32_t urlFromFilePath(char* _out, int32_t _max, const StringView& _filePath)
	{
		if (0 < _max)
		{
			_out[0] = '\0';
		}

		if (kMaxFilePath <= _filePath.getLength() )
		{
			return 0;
		}

		const FilePath filePath(_filePath);

		StringView path(filePath.getCPtr() );

		const int32_t worstCaseSize = int32_t(BX_COUNTOF(kUrlFilePrefix) ) + kUrlEscapeSize*path.getLength();

		if (_max < worstCaseSize)
		{
			return 0;
		}

		int32_t len = strCopy(_out, _max, kUrlFilePrefix);

		if (BX_ENABLED(BX_PLATFORM_WINDOWS)
		&&  2 <= path.getLength()
		&&  hasDriveLetter(path.getPtr() ) )
		{
			_out[len++] = '/';
			_out[len++] = path.getPtr()[0];
			_out[len++] = ':';
			path.set(path.getPtr() + 2, path.getTerm() );
		}

		return len + urlEncode(&_out[len], _max - len, path, UrlEncoding::Path);
	}

} // namespace bx
