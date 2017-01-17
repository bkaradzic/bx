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
	REQUIRE(0  == strcmp(buffer, "Ten ch") );
}
