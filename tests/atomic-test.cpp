/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/cpu.h>

TEST_CASE("atomic", "")
{
	uint32_t test = 1337;
	uint32_t fetched;

	fetched = bx::atomicFetchAndAdd(&test, 52u);
	REQUIRE(fetched == 1337);
	REQUIRE(test == 1389);

	fetched = bx::atomicAddAndFetch(&test, 64u);
	REQUIRE(fetched == 1453);
	REQUIRE(test == 1453);

	fetched = bx::atomicFetchAndSub(&test, 64u);
	REQUIRE(fetched == 1453);
	REQUIRE(test == 1389);

	fetched = bx::atomicSubAndFetch(&test, 52u);
	REQUIRE(fetched == 1337);
	REQUIRE(test == 1337);

	fetched = bx::atomicFetchAndAddsat(&test, 52u, 1453u);
	REQUIRE(fetched == 1337);
	REQUIRE(test == 1389);

	fetched = bx::atomicFetchAndAddsat(&test, 1000u, 1453u);
	REQUIRE(fetched == 1389);
	REQUIRE(test == 1453);

	fetched = bx::atomicFetchAndSubsat(&test, 64u, 1337u);
	REQUIRE(fetched == 1453);
	REQUIRE(test == 1389);

	fetched = bx::atomicFetchAndSubsat(&test, 1000u, 1337u);
	REQUIRE(fetched == 1389);
	REQUIRE(test == 1337);

}

TEST_CASE("atomic load, store, exchange", "")
{
	uint32_t value32 = 1337;
	REQUIRE(1337 == bx::atomicLoad(&value32) );

	bx::atomicStore(&value32, 1389u);
	REQUIRE(1389 == value32);

	bx::atomicStoreRelaxed(&value32, 1453u);
	REQUIRE(1453 == value32);

	REQUIRE(1453 == bx::atomicExchange(&value32, 42u) );
	REQUIRE(42 == value32);

	uint64_t value64 = UINT64_C(0x123456789abcdef0);
	REQUIRE(UINT64_C(0x123456789abcdef0) == bx::atomicLoad(&value64) );

	bx::atomicStore(&value64, UINT64_C(0xfedcba9876543210) );
	REQUIRE(UINT64_C(0xfedcba9876543210) == value64);

	REQUIRE(UINT64_C(0xfedcba9876543210) == bx::atomicExchange(&value64, UINT64_C(1) ) );
	REQUIRE(1 == value64);

	uintptr_t ptr = 16;
	REQUIRE(16 == bx::atomicCompareAndSwapPtr(&ptr, 8, 32) );
	REQUIRE(16 == ptr);

	REQUIRE(16 == bx::atomicCompareAndSwapPtr(&ptr, 16, 32) );
	REQUIRE(32 == ptr);

	bx::cpuRelax();
}
