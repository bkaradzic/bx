/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "test.h"
#include <bx/string.h>

TEST_CASE("vsnprintf NULL buffer", "No output buffer provided.")
{
	REQUIRE(4 == bx::snprintf(NULL, 0, "test") );

	REQUIRE(1 == bx::snprintf(NULL, 0, "%d", 1) );
}

TEST_CASE("vsnprintf truncated", "Truncated output buffer.")
{
	char buffer[7];

	REQUIRE(10 == bx::snprintf(buffer, BX_COUNTOF(buffer), "Ten chars!") );
	REQUIRE(0  == bx::strncmp(buffer, "Ten ch") );
}

static bool test(const char* _expected, const char* _format, ...)
{
	int32_t max = (int32_t)bx::strnlen(_expected) + 1;
	char* temp = (char*)alloca(max);

	va_list argList;
	va_start(argList, _format);
	int32_t len = bx::vsnprintf(temp, max, _format, argList);
	va_end(argList);

	bool result = true
		&& len == max-1
		&& 0   == bx::strncmp(_expected, temp)
		;

	if (!result)
	{
		printf("result (%d) %s, expected (%d) %s\n", len, temp, max-1, _expected);
	}

	return result;
}

TEST_CASE("vsnprintf f", "")
{
	REQUIRE(test("1.337",    "%0.3f", 1.337) );
	REQUIRE(test("  13.370", "%8.3f", 13.37) );
	REQUIRE(test("  13.370", "%*.*f", 8, 3, 13.37) );
	REQUIRE(test("13.370  ", "%-8.3f", 13.37) );
	REQUIRE(test("13.370  ", "%*.*f", -8, 3, 13.37) );
}

TEST_CASE("vsnprintf d/i/o/u/x", "")
{
	REQUIRE(test("1337", "%d", 1337) );
	REQUIRE(test("1337                ", "%-20d",  1337) );
	REQUIRE(test("-1337               ", "%-20d", -1337) );

	REQUIRE(test("1337", "%i", 1337) );
	REQUIRE(test("1337                ", "%-20i",  1337) );
	REQUIRE(test("-1337               ", "%-20i", -1337) );

	REQUIRE(test("1337", "%o", 01337) );
	REQUIRE(test("2471", "%o", 1337) );
	REQUIRE(test("1337                ", "%-20o",  01337) );
	REQUIRE(test("37777776441         ", "%-20o", -01337) );

	REQUIRE(test("1337", "%u", 1337) );
	REQUIRE(test("1337                ", "%-20u",  1337) );
	REQUIRE(test("4294965959          ", "%-20u", -1337) );

	REQUIRE(test("1337", "%x", 0x1337) );
	REQUIRE(test("1234abcd            ", "%-20x",  0x1234abcd) );
	REQUIRE(test("1234ABCD            ", "%-20X",  0x1234abcd) );
	REQUIRE(test("edcb5433            ", "%-20x", -0x1234abcd) );
	REQUIRE(test("EDCB5433            ", "%-20X", -0x1234abcd) );
	REQUIRE(test("0000000000001234abcd", "%020x",  0x1234abcd) );
	REQUIRE(test("0000000000001234ABCD", "%020X",  0x1234abcd) );
	REQUIRE(test("000000000000edcb5433", "%020x", -0x1234abcd) );
	REQUIRE(test("000000000000EDCB5433", "%020X", -0x1234abcd) );
}

TEST_CASE("vsnprintf p", "")
{
	REQUIRE(test("0xbadc0de", "%p", (void*)0xbadc0de) );
	REQUIRE(test("0xbadc0de           ", "%-20p", (void*)0xbadc0de) );
}

TEST_CASE("vsnprintf", "")
{
	REQUIRE(test("x", "%c", 'x') );
	REQUIRE(test("x                   ", "%-20c", 'x') );

	REQUIRE(test("hello               ", "%-20s", "hello") );
	REQUIRE(test("hello, world!", "%s, %s!", "hello", "world") );
}
