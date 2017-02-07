/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "test.h"
#include <bx/string.h>

TEST_CASE("memSet", "")
{
	char temp[] =  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };

	REQUIRE(temp == bx::memSet(temp, 0, 0) );
	REQUIRE(temp[0] == 1);

	REQUIRE(temp == bx::memSet(temp, 0, 5) );
	REQUIRE(temp[0] == 0);
	REQUIRE(temp[1] == 0);
	REQUIRE(temp[2] == 0);
	REQUIRE(temp[3] == 0);
	REQUIRE(temp[4] == 0);
	REQUIRE(temp[5] == 6);
}

TEST_CASE("memMove", "")
{
	char str[] = { 'x', 'x', 'x', 'x', 'a', 'b', 'v', 'g', 'd' };

	REQUIRE(bx::memMove(&str[4], &str[4], 0) );
	REQUIRE(bx::memMove(&str[4], &str[4], 5) );

	REQUIRE(&str[0] == bx::memMove(str, &str[4], 5) );
	REQUIRE(      0 == bx::strncmp(str, "abvgd", 5) );

	REQUIRE(&str[4] == bx::memMove(&str[4], str, 5) );
	REQUIRE( str[4] == 'a' );

	bx::memSet(str, 'x', 4);
	REQUIRE(0 == bx::strncmp(str, "xxxxabvgd", 9) );
}
