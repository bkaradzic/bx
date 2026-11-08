/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_URL_H_HEADER_GUARD
#define BX_URL_H_HEADER_GUARD

#include "filepath.h"
#include "string.h"

namespace bx
{
	/// Splits URL into tokens. Tokens are views into the parsed string, and they
	/// don't own any storage.
	///
	class UrlView
	{
	public:
		/// URL token.
		///
		enum Enum
		{
			Scheme,   //!< Scheme, without `://` separator.
			UserName, //!< User name from authority's user info.
			Password, //!< Password from authority's user info.
			Host,     //!< Host name, or IP address. IPv6 address is in `[` and `]`.
			Port,     //!< Port, as it appears in URL, without `:` separator.
			Path,     //!< Path, including leading `/`.
			Query,    //!< Query, without leading `?`.
			Fragment, //!< Fragment, without leading `#`.

			Count
		};

		/// Constructor. All tokens are empty until `parse` succeeds.
		///
		UrlView();

		/// Reset all tokens to empty string view.
		///
		void clear();

		/// Parse URL into tokens.
		///
		/// @param[in] _url URL to parse: `scheme://authority/path?query#fragment`, `scheme:path`,
		///   or, without scheme, `host:port/path`, and `/path`. Text before `:` is scheme, unless
		///   only digits follow it up to the end, or `/`, that's port. It's not copied, and it
		///   must outlive UrlView.
		///
		/// @returns True if URL is parsed successfully. Otherwise tokens are
		///   left in unspecified state, and must not be used.
		///
		bool parse(const StringView& _url);

		/// Get token.
		///
		/// @param[in] _token Token to get.
		///
		/// @returns Token, or empty string view if URL doesn't contain it.
		///
		const StringView& get(Enum _token) const;

	private:
		StringView m_tokens[Count];
	};

	/// Characters `urlEncode` passes through unescaped, in addition to the
	/// unreserved set `A-Z a-z 0-9 - _ . ~`.
	///
	struct UrlEncoding
	{
		/// Encoding.
		///
		enum Enum
		{
			Component, //!< Nothing. Use for single path segment, query value, etc.
			Path,      //!< `/` separators. Use for whole path.

			Count
		};
	};

	/// Percent-encode string.
	///
	/// @param[out] _out Output buffer. It's always zero terminated, unless
	///   `_max` is zero. Encoding stops on truncation, and escape sequence is
	///   never split.
	/// @param[in] _max Size of output buffer, including zero terminator.
	/// @param[in] _str String to encode.
	/// @param[in] _encoding Characters to pass through unescaped.
	///
	/// @returns Number of characters written, excluding zero terminator.
	///
	int32_t urlEncode(
		  char* _out
		, int32_t _max
		, const StringView& _str
		, UrlEncoding::Enum _encoding = UrlEncoding::Component
		);

	/// Decode percent-encoded string. What's not a valid escape sequence stays as it is.
	///
	/// @param[out] _out Output buffer, it can be the same memory as `_str`. It's always zero
	///   terminated, unless `_max` is zero. Decoding stops on truncation.
	/// @param[in] _max Size of output buffer, including zero terminator.
	/// @param[in] _str String to decode.
	///
	/// @returns Number of characters written, excluding zero terminator. Decoded text can have
	///   zero in it (`%00`).
	///
	int32_t urlDecode(
		  char* _out
		, int32_t _max
		, const StringView& _str
		);

	/// Convert `file` URL to file path.
	///
	/// @param[out] _outFilePath File path. On Windows it starts with drive letter, `/d:/a` is
	///   `d:/a`.
	/// @param[in] _url URL.
	///
	/// @returns True if URL is local file (host is empty or `localhost`), and path is shorter
	///   than `kMaxFilePath`. Path with zero in it (`%00`) is refused.
	///
	bool urlToFilePath(FilePath& _outFilePath, const StringView& _url);

	/// Convert absolute file path to `file` URL.
	///
	/// @param[out] _out Output buffer, zero terminated.
	/// @param[in] _max Size of output buffer, including zero terminator. It must be larger than
	///   8 + 3 times the length of path.
	/// @param[in] _filePath Absolute file path.
	///
	/// @returns Number of characters written, excluding zero terminator. 0 if buffer is too
	///   small, or path is not shorter than `kMaxFilePath`.
	///
	int32_t urlFromFilePath(
		  char* _out
		, int32_t _max
		, const StringView& _filePath
		);

} // namespace bx

#endif // BX_URL_H_HEADER_GUARD
