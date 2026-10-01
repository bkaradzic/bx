/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/literal-parser.h>
#include <bx/scanner.h>

namespace bx
{
	namespace
	{
		static constexpr StringView kWgslInt[]   = { "i", "u", "li", "lu" };
		static constexpr StringView kWgslFloat[] = { "f", "h", "lf" };
		static constexpr StringView kGlslInt[]   = { "u", "U", "lu", "LU" };
		static constexpr StringView kGlslFloat[] = { "f", "F", "lf", "LF" };
		static constexpr StringView kHlslInt[]   = { "u", "U", "l", "L", "ll", "LL", "ul", "lu", "uL", "Lu", "UL", "ull", "llu", "ULL", "LLU" };
		static constexpr StringView kHlslFloat[] = { "f", "F", "h", "H" };
		static constexpr StringView kCInt[]      = { "u", "U", "l", "L", "ll", "LL", "ul", "lu", "uL", "Lu", "ull", "llu", "ULL", "LLU" };
		static constexpr StringView kCFloat[]    = { "f", "F", "l", "L" };
		static constexpr StringView kMslFloat[]  = { "f", "F", "h", "H" };

		using LiteralProfile  = LiteralParser::Profile;
		using LiteralSuffixes = LiteralParser::Suffixes;
		using LiteralType     = LiteralParser::Type;

		char peekChar(Scanner& _scanner)
		{
			const StringView view = _scanner.peek();

			return view.isEmpty() ? '\0' : *view.getPtr();
		}

		bool acceptDigits(Scanner& _scanner, CharTestFn _fn, char _separator)
		{
			bool ok = false;
			for (char ch = peekChar(_scanner)
				; _fn(ch) || (0 != _separator && _separator == ch)
				; ch = peekChar(_scanner) )
			{
				ok |= _fn(ch);
				_scanner.accept();
			}

			return ok;
		}

		bool acceptSuffix(Scanner& _scanner, const LiteralSuffixes& _suffixes)
		{
			int32_t best    = -1;
			int32_t bestLen = 0;
			for (uint16_t ii = 0; ii < _suffixes.num; ++ii)
			{
				const int32_t len = _suffixes.data[ii].getLength();
				if (len > bestLen
				&&  !_scanner.peek(_suffixes.data[ii]).isEmpty() )
				{
					best    = ii;
					bestLen = len;
				}
			}

			if (0 <= best)
			{
				_scanner.accept(_suffixes.data[best]);

				return true;
			}

			return false;
		}

		enum ExponentResult
		{
			ExponentNone,
			ExponentOk,
			ExponentBad,
		};

		ExponentResult acceptExponent(Scanner& _scanner, const LiteralProfile& _profile, CharTestFn _digits)
		{
			const char ch = peekChar(_scanner);
			const bool isExponent = 'e' == ch
				|| (0 != (_profile.flags & LiteralParser::UpperExponent) && 'E' == ch)
				;

			if (!isExponent)
			{
				return ExponentNone;
			}

			_scanner.accept();
			_scanner.accept('-', '+');

			return acceptDigits(_scanner, _digits, _profile.separator) ? ExponentOk : ExponentBad;
		}

		LiteralType parseDecimalFloat(Scanner& _scanner, const LiteralProfile& _profile)
		{
			bool dot  = false;
			bool frac = false;
			if (!_scanner.accept('.').isEmpty() )
			{
				dot  = true;
				frac = acceptDigits(_scanner, isNumeric, _profile.separator);
			}

			const ExponentResult exponent = acceptExponent(_scanner, _profile, isNumeric);
			if (ExponentBad == exponent)
			{
				return LiteralType::Unknown;
			}

			const bool ok = frac
				|| ExponentOk == exponent
				|| (dot && 0 != (_profile.flags & LiteralParser::TrailingDot) )
				;

			if (!ok)
			{
				return LiteralType::Unknown;
			}

			acceptSuffix(_scanner, _profile.floatSuffix);

			return LiteralType::Float;
		}

		LiteralType parseAfterInteger(Scanner& _scanner, const LiteralProfile& _profile)
		{
			const char ch = peekChar(_scanner);
			const bool isExponent = 'e' == ch
				|| (0 != (_profile.flags & LiteralParser::UpperExponent) && 'E' == ch)
				;

			if ('.' == ch
			||  isExponent)
			{
				return parseDecimalFloat(_scanner, _profile);
			}

			if (0 != (_profile.flags & LiteralParser::PromoteFloatSuffix)
			&&  acceptSuffix(_scanner, _profile.floatSuffix) )
			{
				return LiteralType::Float;
			}

			acceptSuffix(_scanner, _profile.intSuffix);

			return LiteralType::Decimal;
		}

		LiteralType parseHex(Scanner& _scanner, const LiteralProfile& _profile)
		{
			const bool anyHex = acceptDigits(_scanner, isHexNum, _profile.separator);

			bool isFloat     = false;
			bool anyHexFrac  = false;
			bool anyExponent = false;
			if (0 != (_profile.flags & LiteralParser::HexFloat) )
			{
				if (!_scanner.accept('.').isEmpty() )
				{
					isFloat    = true;
					anyHexFrac = acceptDigits(_scanner, isHexNum, _profile.separator);
				}

				const char ch = peekChar(_scanner);
				if ('p' == ch
				||  'P' == ch)
				{
					isFloat     = true;
					anyExponent = true;
					_scanner.accept();
					_scanner.accept('-', '+');

					if (!acceptDigits(_scanner, isNumeric, _profile.separator) )
					{
						return LiteralType::Unknown;
					}
				}
			}

			if (isFloat)
			{
				// `0x.p2` has no significand digits.
				if (!anyHex
				&&  !anyHexFrac)
				{
					return LiteralType::Unknown;
				}

				if (anyExponent)
				{
					acceptSuffix(_scanner, _profile.floatSuffix);
				}

				return LiteralType::Float;
			}

			// `0x` with no digits.
			if (!anyHex)
			{
				return LiteralType::Unknown;
			}

			acceptSuffix(_scanner, _profile.intSuffix);

			return LiteralType::Hexadecimal;
		}

		LiteralType parseLiteral(Scanner& _scanner, const LiteralProfile& _profile)
		{
			if (0 != (_profile.flags & LiteralParser::Sign) )
			{
				_scanner.accept('-', '+');
			}

			if (!_scanner.accept('0').isEmpty() )
			{
				if (0 != (_profile.flags & LiteralParser::BinPrefix)
				&&  !_scanner.accept('b').isEmpty() )
				{
					if (!acceptDigits(_scanner, isBinNum, _profile.separator) )
					{
						return LiteralType::Unknown;
					}

					acceptSuffix(_scanner, _profile.intSuffix);

					return LiteralType::Binary;
				}

				if (0 != (_profile.flags & LiteralParser::OctPrefix)
				&&  !_scanner.accept('o').isEmpty() )
				{
					if (!acceptDigits(_scanner, isOctNum, _profile.separator) )
					{
						return LiteralType::Unknown;
					}

					acceptSuffix(_scanner, _profile.intSuffix);

					return LiteralType::Octal;
				}

				if (0 != (_profile.flags & LiteralParser::HexPrefix)
				&&  !_scanner.accept('x', 'X').isEmpty() )
				{
					return parseHex(_scanner, _profile);
				}

				const bool        leadingZeros = acceptDigits(_scanner, isNumeric, _profile.separator);
				const LiteralType type         = parseAfterInteger(_scanner, _profile);

				if (leadingZeros
				&&  LiteralType::Decimal == type
				&&  0 != (_profile.flags & LiteralParser::NoLeadingZero) )
				{
					return LiteralType::Unknown;
				}

				return type;
			}

			if (isNumeric(peekChar(_scanner) ) )
			{
				acceptDigits(_scanner, isNumeric, _profile.separator);

				return parseAfterInteger(_scanner, _profile);
			}

			if (0 != (_profile.flags & LiteralParser::LeadingDot)
			&&  '.' == peekChar(_scanner) )
			{
				_scanner.accept('.');

				// `.` must be followed by a digit.
				if (!acceptDigits(_scanner, isNumeric, _profile.separator) )
				{
					return LiteralType::Unknown;
				}

				if (ExponentBad == acceptExponent(_scanner, _profile, isNumeric) )
				{
					return LiteralType::Unknown;
				}

				acceptSuffix(_scanner, _profile.floatSuffix);

				return LiteralType::Float;
			}

			return LiteralType::Unknown;
		}

	} // namespace

	LiteralParser::Profile LiteralParser::defaults()
	{
		return
		{
			Sign | BinPrefix | OctPrefix | HexPrefix,
			'_',
			{ NULL, 0 },
			{ NULL, 0 },
		};
	}

	LiteralParser::Profile LiteralParser::wgsl()
	{
		return
		{
			HexPrefix | HexFloat | LeadingDot | TrailingDot | UpperExponent | PromoteFloatSuffix | NoLeadingZero,
			0,
			{ kWgslInt,   uint16_t(BX_COUNTOF(kWgslInt)   ) },
			{ kWgslFloat, uint16_t(BX_COUNTOF(kWgslFloat) ) },
		};
	}

	LiteralParser::Profile LiteralParser::glsl()
	{
		return
		{
			HexPrefix | LeadingDot | TrailingDot | UpperExponent,
			0,
			{ kGlslInt,   uint16_t(BX_COUNTOF(kGlslInt)   ) },
			{ kGlslFloat, uint16_t(BX_COUNTOF(kGlslFloat) ) },
		};
	}

	LiteralParser::Profile LiteralParser::hlsl()
	{
		return
		{
			HexPrefix | LeadingDot | TrailingDot | UpperExponent | PromoteFloatSuffix,
			0,
			{ kHlslInt,   uint16_t(BX_COUNTOF(kHlslInt)   ) },
			{ kHlslFloat, uint16_t(BX_COUNTOF(kHlslFloat) ) },
		};
	}

	LiteralParser::Profile LiteralParser::c()
	{
		return
		{
			HexPrefix | HexFloat | LeadingDot | TrailingDot | UpperExponent,
			0,
			{ kCInt,   uint16_t(BX_COUNTOF(kCInt)   ) },
			{ kCFloat, uint16_t(BX_COUNTOF(kCFloat) ) },
		};
	}

	LiteralParser::Profile LiteralParser::cpp()
	{
		Profile profile   = c();
		profile.flags    |= BinPrefix;
		profile.separator = '\'';

		return profile;
	}

	LiteralParser::Profile LiteralParser::msl()
	{
		Profile profile = cpp();
		profile.floatSuffix = { kMslFloat, uint16_t(BX_COUNTOF(kMslFloat) ) };

		return profile;
	}

	LiteralParser::LiteralParser(const StringView& _str, const Profile& _profile)
		: m_type(Type::Unknown)
		, m_ok(false)
	{
		Scanner scanner(_str);
		const StringView start = scanner.getCursor();

		m_type    = parseLiteral(scanner, _profile);
		m_ok      = Type::Unknown != m_type;
		m_literal = m_ok
			? scanner.between(start)
			: StringView()
			;
	}

} // namespace bx
