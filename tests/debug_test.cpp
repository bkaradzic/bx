/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/debug.h>

static BX_NO_INLINE uintptr_t getCalleeStackAddress()
{
	return BX_STACK_FRAME_ADDRESS();
}

TEST_CASE("BX_STACK_FRAME_ADDRESS", "[debug]")
{
	const uintptr_t local = uintptr_t(&local);
	const uintptr_t addr  = BX_STACK_FRAME_ADDRESS();

	// Stack grows down on all supported platforms, callee frame is below caller's.
	REQUIRE(getCalleeStackAddress() < addr);

	// AddressSanitizer keeps locals on its fake stack.
	if (!BX_SANITIZER_ADDRESS)
	{
		REQUIRE( (addr > local ? addr - local : local - addr) < 64<<10);
	}
}

TEST_CASE("getCallStackFast", "[debug]")
{
	uintptr_t stack[32];
	const uint32_t num = bx::getCallStackFast(0, BX_COUNTOF(stack), stack);

	// Linux walks frame pointers, and compilers there omit them when optimizing.
	if (!BX_ENABLED(BX_PLATFORM_LINUX) )
	{
		REQUIRE(0 < num);
	}
}
