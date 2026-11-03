/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_ERROR_H_HEADER_GUARD
#	error "Must be included from bx/error!"
#endif // BX_ERROR_H_HEADER_GUARD

#include <bx/debug.h>

namespace bx
{
	static_assert(sizeof(Error) == 2*sizeof(void*) + 4*sizeof(uint32_t), "Error has padding.");

	inline Error::Error()
		: m_filePath("")
		, m_msg("")
		, m_line(0)
		, m_msgLen(0)
		, m_code(0)
		, m_context(0)
	{
	}

	inline void Error::reset()
	{
		m_code    = 0;
		m_context = 0;
		m_msg     = "";
		m_msgLen  = 0;
	}

	inline void Error::setError(ErrorResult _errorResult, const StringLiteral& _msg, const Location& _location)
	{
		BX_ASSERT(0 != _errorResult.code, "Invalid ErrorResult passed to setError!");

		if (!isOk() )
		{
			return;
		}

		m_filePath = _location.filePath;
		m_line     = _location.line;
		m_code     = _errorResult.code;
		m_msg      = _msg.getCPtr();
		m_msgLen   = _msg.getLength();
	}

	inline bool Error::isOk() const
	{
		return 0 == m_code;
	}

	inline ErrorResult Error::get() const
	{
		ErrorResult result = { m_code };
		return result;
	}

	inline StringLiteral Error::getMessage() const
	{
		return StringLiteral(m_msg, m_msgLen);
	}

	inline Location Error::getLocation() const
	{
		return Location(m_filePath, m_line);
	}

	inline uint32_t Error::getContext() const
	{
		return m_context;
	}

	inline bool Error::operator==(const ErrorResult& _rhs) const
	{
		return _rhs.code == m_code;
	}

	inline bool Error::operator!=(const ErrorResult& _rhs) const
	{
		return _rhs.code != m_code;
	}

	inline ErrorIgnore::operator Error*()
	{
		return this;
	}

	inline ErrorAssert::~ErrorAssert()
	{
		const StringLiteral msg = getMessage();
		BX_UNUSED(msg);

		BX_ASSERT_LOC(getLocation(), isOk(), "ErrorAssert: 0x%08x `%S`"
			, get().code
			, &msg
			);
	}

	inline ErrorFatal::operator Error*()
	{
		return this;
	}

	inline ErrorFatal::~ErrorFatal()
	{
		const StringLiteral msg = getMessage();
		_BX_ASSERT_LOC(getLocation(), isOk(), "ErrorFatal: 0x%08x `%S`"
			, get().code
			, &msg
			);
	}

	inline ErrorAssert::operator Error*()
	{
		return this;
	}

	inline ErrorScope::ErrorScope(Error* _err, const StringLiteral& _name)
		: m_err(_err)
		, m_name(_name)
	{
		BX_UNUSED(m_err);
		BX_ASSERT(NULL != _err, "_err can't be NULL");
	}

	inline ErrorScope::~ErrorScope()
	{
		const StringLiteral msg = m_err->getMessage();
		BX_UNUSED(msg);

		if (m_name.isEmpty() )
		{
			BX_ASSERT_LOC(
				  m_err->getLocation()
				, m_err->isOk()
				, "ErrorScope: 0x%08x `%S`"
				, m_err->get().code
				, &msg
				);
		}
		else
		{
			BX_ASSERT_LOC(
				  m_err->getLocation()
				, m_err->isOk()
				, "ErrorScope: %S - 0x%08x `%S`"
				, &m_name
				, m_err->get().code
				, &msg
				);
		}
	}

	inline const StringLiteral& ErrorScope::getName() const
	{
		return m_name;
	}

} // namespace bx
