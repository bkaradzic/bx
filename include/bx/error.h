/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_ERROR_H_HEADER_GUARD
#define BX_ERROR_H_HEADER_GUARD

#include "string.h"

///
#define BX_ERROR_SET(_err, _result, _msg)    \
	BX_MACRO_BLOCK_BEGIN                     \
		(_err)->setError(_result,  "" _msg); \
	BX_MACRO_BLOCK_END

///
#define BX_ERROR_USE_TEMP_WHEN_NULL(_err)                           \
	const bx::Error tmpError; /* It should not be used directly! */ \
	_err = NULL == _err ? const_cast<bx::Error*>(&tmpError) : _err

///
#define BX_ERROR_SCOPE(_err, ...)                                  \
	BX_ERROR_USE_TEMP_WHEN_NULL(_err);                             \
	bx::ErrorScope bxErrorScope(const_cast<bx::Error*>(&tmpError), "" __VA_ARGS__)

///
#define BX_ERROR_RESULT(_err, _code)                       \
	static_assert(_code != 0, "ErrorCode 0 is reserved!"); \
	static constexpr bx::ErrorResult _err = { _code }

/// Set error and begin its context: BX_ERROR_CONTEXT_BEGIN(_err, _result, _msg, _size[, _skip]).
#define BX_ERROR_CONTEXT_BEGIN(_err, _result, _msg, ...) \
	BX_MACRO_BLOCK_BEGIN                                 \
		bx::ErrorContext bxErrorContext(_err, _result, _msg, __VA_ARGS__)

/// Error context writer.
#define BX_ERROR_CONTEXT_WRITER() \
		(&bxErrorContext)

/// Write to error context.
#define BX_ERROR_CONTEXT_WRITE(...) \
		bx::write(BX_ERROR_CONTEXT_WRITER(), bx::ErrorIgnore(), __VA_ARGS__)

/// End error context.
#define BX_ERROR_CONTEXT_END() \
	BX_MACRO_BLOCK_END

namespace bx
{
	///
	struct ErrorResult
	{
		uint32_t code;
	};

	///
	class Error
	{
		BX_CLASS(Error
			, NO_COPY
			);

	public:
		///
		Error();

		///
		void reset();

		///
		void setError(ErrorResult _errorResult, const StringLiteral& _msg, const Location& _location = Location::current() );

		///
		bool isOk() const;

		///
		ErrorResult get() const;

		///
		StringLiteral getMessage() const;

		///
		Location getLocation() const;

		/// The error context attached to this error, or 0 when there is none.
		uint32_t getContext() const;

		///
		bool operator==(const ErrorResult& _rhs) const;

		///
		bool operator!=(const ErrorResult& _rhs) const;

	private:
		friend class ErrorContext;

		const char* m_filePath;
		const char* m_msg;
		uint32_t    m_line;
		int32_t     m_msgLen;
		uint32_t    m_code;
		uint32_t    m_context;
	};

	/// Do nothing even if error is set.
	class ErrorIgnore final : public Error
	{
	public:
		///
		operator Error*();
	};

	/// In debug build assert if error is set.
	class ErrorAssert final : public Error
	{
	public:
		///
		~ErrorAssert();

		///
		operator Error*();
	};

	/// Exit application if error is set.
	class ErrorFatal final : public Error
	{
	public:
		///
		~ErrorFatal();

		///
		operator Error*();
	};

	///
	class ErrorScope
	{
		BX_CLASS(ErrorScope
			, NO_DEFAULT_CTOR
			, NO_COPY
			);

	public:
		///
		ErrorScope(Error* _err, const StringLiteral& _name);

		///
		~ErrorScope();

		///
		const StringLiteral& getName() const;

	private:
		Error* m_err;
		const StringLiteral m_name;
	};

} // namespace bx

#include "inline/error.inl"

#endif // BX_ERROR_H_HEADER_GUARD
