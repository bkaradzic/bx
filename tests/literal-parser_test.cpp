/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"

#include <bx/literal-parser.h>

bool isNumber(bx::StringView _str)
{
	const char* str = _str.getPtr();
	int32_t len = _str.getLength();

	const char* end = str + len;
	const char* p = str;

	if (p < end && (*p == '-' || *p == '+'))
	{
		p++;
	}

	bool has_digit = false;

	while (p < end && isdigit((unsigned char)*p))
	{
		has_digit = true;
		p++;
	}

	if (!has_digit) return false;

	if (p < end && *p == '.')
	{
		p++;
		has_digit = false;

		while (p < end && isdigit((unsigned char)*p))
		{
			has_digit = true;
			p++;
		}

		if (!has_digit) return false;
	}

	if (p < end && (*p == 'e' || *p == 'E'))
	{
		p++;

		if (p < end && (*p == '-' || *p == '+'))
		{
			p++;
		}

		has_digit = false;

		while (p < end && isdigit((unsigned char)*p))
		{
			has_digit = true;
			p++;
		}

		if (!has_digit) return false;
	}

	return p == end;
}

bool testNumber(const bx::StringView& _input, const bx::StringView& _output, bx::LiteralParser::Type _type = bx::LiteralParser::Type::Unknown)
{
	bx::LiteralParser p(_input);

	const bx::StringView literal = p.getLiteral();

	bool ok = true
		&& p.isOk()
		&& _type == p.getType()
		&& bx::isEqual(literal, _output)
		;

	if (!ok
	&&  _type != bx::LiteralParser::Type::Unknown)
	{
		DBG("%d: %S, expected %d: %S", p.getType(), &literal, _type, &_output);
	}

	BX_TRACE("is_number %d, %d <- %S"
		, isNumber(literal)
		, ok
		, &literal
		);

	return ok;
}

bool testNumber(const bx::StringView& _inputAndOutput, bx::LiteralParser::Type _type = bx::LiteralParser::Type::Unknown)
{
	return testNumber(_inputAndOutput, _inputAndOutput, _type);
}

TEST_CASE("LiteralParser", "[parser]")
{
	// error
	REQUIRE(!testNumber("") );
	REQUIRE(!testNumber(".") );
	REQUIRE(!testNumber("abvgd") );
	REQUIRE(!testNumber("-e0") );
	REQUIRE(!testNumber("-e-89") );
	REQUIRE(!testNumber("0b") );
	REQUIRE(!testNumber("0o") );
	REQUIRE(!testNumber("0x") );
	REQUIRE(!testNumber("0e") );
	REQUIRE(!testNumber("-0.e-") );
	REQUIRE(!testNumber("1.38913e-") );

	// decimal
	REQUIRE(testNumber("0", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("00", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("000", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("0000", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("00_00", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("0_0_0_0", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("0;", "0", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("0-", "0", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("0-e0", "0", bx::LiteralParser::Type::Decimal) );

	// binary
	REQUIRE(testNumber("0b0001001110001001", bx::LiteralParser::Type::Binary) );
	REQUIRE(testNumber("0b0001_0011_1000_1001", bx::LiteralParser::Type::Binary) );
	REQUIRE(testNumber("0b0;", "0b0", bx::LiteralParser::Type::Binary) );
	REQUIRE(testNumber("0b0+", "0b0", bx::LiteralParser::Type::Binary) );

	// octal
	REQUIRE(testNumber("02555", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("0o2555", bx::LiteralParser::Type::Octal) );
	REQUIRE(testNumber("0o25_55", bx::LiteralParser::Type::Octal) );

	// hex
	REQUIRE(testNumber("0x1389", bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0x1389", bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0x1389", bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0x0;", "0x0", bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0x0+", "0x0", bx::LiteralParser::Type::Hexadecimal) );

	// integer
	REQUIRE(testNumber("1389", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("13_89", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("0;", "0", bx::LiteralParser::Type::Decimal) );
	REQUIRE(testNumber("0+", "0", bx::LiteralParser::Type::Decimal) );

	// floating point
	REQUIRE(testNumber("0.1389", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("-0.1389", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("+0.1389", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("0.e-89", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("-0.e-89", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("+0.e-89", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("138e-9", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("1.38913e-89", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("1.389e+001", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("1.389e-002", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("1.389e-002;", "1.389e-002", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("-0.e-89;", "-0.e-89", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("-0.e-89+", "-0.e-89", bx::LiteralParser::Type::Float) );
	REQUIRE(testNumber("-0.e-89e", "-0.e-89", bx::LiteralParser::Type::Float) );

	// hex with letters (lower/upper/mixed) and digit separators
	REQUIRE(testNumber("0xabcdef",   bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0xABCDEF",   bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0xDeadC0de", bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0xff",       bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0x13_89",    bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0xFF_FF",    bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0xdead;", "0xdead", bx::LiteralParser::Type::Hexadecimal) );
	REQUIRE(testNumber("0xC0DE-", "0xC0DE", bx::LiteralParser::Type::Hexadecimal) );

	// more errors: a lone sign or a leading separator is not a number
	REQUIRE(!testNumber("+") );
	REQUIRE(!testNumber("-") );
	REQUIRE(!testNumber("_5") );
	REQUIRE(!testNumber("0xg") );
}

static bool testWgsl(const bx::StringView& _input, const bx::StringView& _output, bx::LiteralParser::Type _type)
{
	bx::LiteralParser p(_input, bx::LiteralParser::wgsl() );

	return p.isOk()
		&& _type == p.getType()
		&& bx::isEqual(p.getLiteral(), _output)
		;
}

static bool testWgsl(const bx::StringView& _inputAndOutput, bx::LiteralParser::Type _type)
{
	return testWgsl(_inputAndOutput, _inputAndOutput, _type);
}

TEST_CASE("LiteralParser WGSL profile", "[parser]")
{
	typedef bx::LiteralParser LP;

	// integers (+ WGSL i/u/li/lu suffixes) and hex
	REQUIRE(testWgsl("0",     LP::Decimal) );
	REQUIRE(testWgsl("42",    LP::Decimal) );
	REQUIRE(testWgsl("1u",    LP::Decimal) );
	REQUIRE(testWgsl("1i",    LP::Decimal) );
	REQUIRE(testWgsl("1li",   LP::Decimal) );
	REQUIRE(testWgsl("1lu",   LP::Decimal) );
	REQUIRE(testWgsl("0xFF",  LP::Hexadecimal) );
	REQUIRE(testWgsl("0xffu", LP::Hexadecimal) );

	// floats: fraction/exponent, edge dots, suffixes, hex float
	REQUIRE(testWgsl("1.0",           LP::Float) );
	REQUIRE(testWgsl(".5",            LP::Float) );
	REQUIRE(testWgsl("2.",            LP::Float) );
	REQUIRE(testWgsl("1e5",           LP::Float) );
	REQUIRE(testWgsl("3.14e-2",       LP::Float) );
	REQUIRE(testWgsl("1.5f",          LP::Float) );
	REQUIRE(testWgsl("1f",            LP::Float) ); // bare integer + float suffix -> float
	REQUIRE(testWgsl("1h",            LP::Float) );
	REQUIRE(testWgsl("1lf",           LP::Float) );
	REQUIRE(testWgsl("0x1p-3",        LP::Float) );
	REQUIRE(testWgsl("0x1.8p3",       LP::Float) );
	REQUIRE(testWgsl("0x1.8p3h",      LP::Float) );
	REQUIRE(testWgsl("0x1.8p3f",      LP::Float) );
	REQUIRE(testWgsl("0xf.h", "0xf.", LP::Float) );
	REQUIRE(testWgsl("0xf.f",         LP::Float) );

	// prefix match: trailing input is not part of the literal
	REQUIRE(testWgsl("42;",    "42",    LP::Decimal) );
	REQUIRE(testWgsl("0xFFu-", "0xFFu", LP::Hexadecimal) );
	REQUIRE(testWgsl("1.x",    "1.",    LP::Float) ); // "1." is a float (trailing dot), then ".x"

	// WGSL has no sign, no 0b/0o prefixes, no '_' separators
	REQUIRE(!bx::LiteralParser("-5", LP::wgsl() ).isOk() ); // sign is a separate token
	REQUIRE(testWgsl("0b1", "0", LP::Decimal) );            // "0" then "b1"
	REQUIRE(testWgsl("5_5", "5", LP::Decimal) );            // "5" then "_5"
}
