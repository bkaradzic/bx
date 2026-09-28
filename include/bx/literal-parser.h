/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_LITERAL_PARSER_H_HEADER_GUARD
#define BX_LITERAL_PARSER_H_HEADER_GUARD

#include <bx/string.h>

namespace bx
{
	/// Parses numeric literal at the beginning of a string.
	///
	/// Syntax that's accepted is selected by `Profile`, so that numeric literals of different
	/// languages can be read by the same parser. Profiles for a few languages are provided by
	/// `wgsl`, `glsl`, `hlsl`, `c`, `cpp`, and `msl`.
	///
	/// Parsing stops at the first character that can't be part of literal, and trailing input is
	/// not an error. Literal that was matched is returned by `getLiteral`, and it's a view into
	/// input string.
	///
	/// @attention LiteralParser doesn't own input string. Input string must outlive LiteralParser.
	///
	class LiteralParser
	{
		BX_CLASS(LiteralParser
			, NO_DEFAULT_CTOR
			, NO_COPY
			);

	public:
		/// Type of literal that was matched.
		///
		enum Type : uint8_t
		{
			Unknown,     //!< Input doesn't start with literal that profile accepts.

			Binary,      //!< Binary integer, `0b1010`.
			Octal,       //!< Octal integer, `0o777`.
			Decimal,     //!< Decimal integer, `42`.
			Hexadecimal, //!< Hexadecimal integer, `0x2a`.
			Float,       //!< Floating point number, `1.5`, `1e3`, or `0x1.8p3`.
		};

		/// Syntax flags, see `Profile::flags`.
		///
		enum : uint32_t
		{
			Sign               = 1u<<0, //!< Leading `+` or `-` is part of literal.
			HexPrefix          = 1u<<1, //!< Hexadecimal `0x` and `0X` prefix.
			BinPrefix          = 1u<<2, //!< Binary `0b` prefix.
			OctPrefix          = 1u<<3, //!< Octal `0o` prefix.
			HexFloat           = 1u<<4, //!< Hexadecimal float, `0x1.8p3`. It requires `.` or `p`.
			LeadingDot         = 1u<<5, //!< Float without integer part, `.5`.
			TrailingDot        = 1u<<6, //!< Float without fractional part, `2.`.
			UpperExponent      = 1u<<7, //!< `E` and `P` exponent, in addition to `e` and `p`.
			PromoteFloatSuffix = 1u<<8, //!< Float suffix makes decimal integer a float, WGSL `1f`.
			NoLeadingZero      = 1u<<9, //!< Decimal integer may not have leading zero. WGSL rejects `0123`, but `00012.` is a valid float.
		};

		/// Set of suffixes that literal can end with.
		///
		struct Suffixes
		{
			const StringView* data; //!< Suffixes to match. Longest match wins.
			uint16_t          num;  //!< Number of suffixes.
		};

		/// Numeric syntax that LiteralParser accepts.
		///
		struct Profile
		{
			uint32_t flags;       //!< Syntax flags.
			char     separator;   //!< Digit separator, or 0 if there is none. For example `_` or `'`.
			Suffixes intSuffix;   //!< Suffixes accepted on integer literal.
			Suffixes floatSuffix; //!< Suffixes accepted on float literal.
		};

		/// Returns profile that accepts sign, binary, octal and hexadecimal prefix, and `_` digit
		/// separator, with no suffixes.
		///
		static Profile defaults();

		/// Returns profile for WGSL numeric literals.
		///
		static Profile wgsl();

		/// Returns profile for GLSL numeric literals.
		///
		static Profile glsl();

		/// Returns profile for HLSL numeric literals.
		///
		static Profile hlsl();

		/// Returns profile for C numeric literals.
		///
		static Profile c();

		/// Returns profile for C++ numeric literals.
		///
		static Profile cpp();

		/// Returns profile for Metal Shading Language numeric literals.
		///
		static Profile msl();

		/// Constructor. Parses literal at the beginning of `_str`.
		///
		/// @param[in] _str Input string to parse. It's not copied, and it must outlive
		///   LiteralParser.
		/// @param[in] _profile Numeric syntax to accept.
		///
		LiteralParser(const StringView& _str, const Profile& _profile = defaults() );

		/// Returns true if literal was matched.
		///
		bool isOk() const;

		/// Returns literal that was matched, or empty string view if there was no match.
		///
		const StringView& getLiteral() const;

		/// Returns type of literal that was matched, or `Type::Unknown` if there was no match.
		///
		Type getType() const;

	private:
		StringView m_literal;
		Type       m_type;
		bool       m_ok;
	};

} // namespace bx

#include "inline/literal-parser.inl"

#endif // BX_LITERAL_PARSER_H_HEADER_GUARD
