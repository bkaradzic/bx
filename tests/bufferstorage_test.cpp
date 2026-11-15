/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "test.h"

#include <bx/bufferstorage.h>

struct InitNoneProbe
{
	InitNoneProbe()
		: value(1)
	{
	}

	InitNoneProbe(bx::InitNoneTag)
		: value(2)
	{
	}

	int32_t value;
};

struct InitNoneAggregate
{
	InitNoneProbe probe;
	int32_t       other = 3;
};

TEST_CASE("StorageTraits constructDefault default constructs aggregates", "[container]")
{
	constexpr uint32_t kCount = 4;

	alignas(InitNoneAggregate) uint8_t aggregateMem[sizeof(InitNoneAggregate)*kCount];
	InitNoneAggregate* aggregates = reinterpret_cast<InitNoneAggregate*>(aggregateMem);

	bx::StorageTraits::constructDefault(aggregates, kCount);

	for (uint32_t ii = 0; ii < kCount; ++ii)
	{
		REQUIRE(1 == aggregates[ii].probe.value);
		REQUIRE(3 == aggregates[ii].other);
	}

	alignas(InitNoneProbe) uint8_t probeMem[sizeof(InitNoneProbe)*kCount];
	InitNoneProbe* probes = reinterpret_cast<InitNoneProbe*>(probeMem);

	bx::StorageTraits::constructDefault(probes, kCount);

	for (uint32_t ii = 0; ii < kCount; ++ii)
	{
		REQUIRE(2 == probes[ii].value);
	}
}
