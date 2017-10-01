/*
 * Copyright 2011-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bnet#license-bsd-2-clause
 */

#include <bx/url.h>

namespace bx
{
	UrlView::UrlView()
	{
	}

	void UrlView::clear()
	{
		for (uint32_t ii = 0; ii < UrlToken::Count; ++ii)
		{
			m_tokens[ii].clear();
		}
	}

	bool UrlView::parse(const StringView& _url)
	{
		clear();

		const char* start = _url.getPtr();
		const char* term  = _url.getTerm();
		const char* schemeEnd = strFind(StringView(start, term), "://");
		const char* hostStart = NULL != schemeEnd ? schemeEnd+3 : start;
		const char* pathStart = strFind(StringView(hostStart, term), '/');

		if (NULL == schemeEnd
		&&  NULL == pathStart)
		{
			return false;
		}

		if (NULL != schemeEnd
		&& (NULL == pathStart || pathStart > schemeEnd) )
		{
			StringView scheme(start, schemeEnd);

			if (!isAlpha(scheme) )
			{
				return false;
			}

			m_tokens[UrlToken::Scheme].set(scheme);
		}

		if (NULL != pathStart)
		{
			const char* queryStart    = strFind(StringView(pathStart, term), '?');
			const char* fragmentStart = strFind(StringView(pathStart, term), '#');

			if (NULL != fragmentStart
			&&  fragmentStart < queryStart)
			{
				return false;
			}

			m_tokens[UrlToken::Path].set(pathStart
				, NULL != queryStart    ? queryStart
				: NULL != fragmentStart ? fragmentStart
				: term
				);

			if (NULL != queryStart)
			{
				m_tokens[UrlToken::Query].set(queryStart+1
					, NULL != fragmentStart ? fragmentStart
					: term
					);
			}

			if (NULL != fragmentStart)
			{
				m_tokens[UrlToken::Fragment].set(fragmentStart+1, term);
			}

			term = pathStart;
		}

		const char* userPassEnd   = strFind(StringView(hostStart, term), '@');
		const char* userPassStart = NULL != userPassEnd ? hostStart : NULL;
		hostStart = NULL != userPassEnd ? userPassEnd+1 : hostStart;
		const char* portStart = strFind(StringView(hostStart, term), ':');

		m_tokens[UrlToken::Host].set(hostStart, NULL != portStart ? portStart : term);

		if (NULL != portStart)
		{
			m_tokens[UrlToken::Port].set(portStart+1, term);
		}

		if (NULL != userPassStart)
		{
			const char* passStart = strFind(StringView(userPassStart, userPassEnd), ':');

			m_tokens[UrlToken::UserName].set(userPassStart
				, NULL != passStart ? passStart
				: userPassEnd
				);

			if (NULL != passStart)
			{
				m_tokens[UrlToken::Password].set(passStart+1, userPassEnd);
			}
		}

		return true;
	}

	const StringView& UrlView::get(UrlToken::Enum _token) const
	{
		return m_tokens[_token];
	}

	static char toHex(char _nible)
	{
		return "0123456789ABCDEF"[_nible&0xf];
	}

	// https://secure.wikimedia.org/wikipedia/en/wiki/URL_encoding
	void urlEncode(const char* _str, char* _buf, uint32_t _bufSize)
	{
		_bufSize--; // need space for zero terminator

		uint32_t ii = 0;
		for (char ch = *_str++
			; '\0' != ch && ii < _bufSize
			; ch = *_str++
			)
		{
			if (isAlphaNum(ch)
			||  ch == '-'
			||  ch == '_'
			||  ch == '.'
			||  ch == '~')
			{
				_buf[ii++] = ch;
			}
			else if (ii+3 < _bufSize)
			{
				_buf[ii++] = '%';
				_buf[ii++] = toHex(ch>>4);
				_buf[ii++] = toHex(ch);
			}
		}

		_buf[ii] = '\0';
	}

} // namespace bx
