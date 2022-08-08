/*
 * Copyright 2010-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/sort.h>
#include <bx/string.h>
#include <bx/rng.h>

TEST_CASE("quickSort", "")
{
	const char* str[] =
	{
		"jabuka",
		"kruska",
		"malina",
		"jagoda",
	};

	auto strCmpFn = [](const void* _lhs, const void* _rhs)
	{
		const char* lhs = *(const char**)_lhs;
		const char* rhs = *(const char**)_rhs;
		return bx::strCmp(lhs, rhs);
	};

	REQUIRE(!bx::isSorted(str, BX_COUNTOF(str), sizeof(str[0]), strCmpFn) );

	bx::quickSort(str, BX_COUNTOF(str), sizeof(str[0]), strCmpFn);

	REQUIRE(0 == bx::strCmp(str[0], "jabuka") );
	REQUIRE(0 == bx::strCmp(str[1], "jagoda") );
	REQUIRE(0 == bx::strCmp(str[2], "kruska") );
	REQUIRE(0 == bx::strCmp(str[3], "malina") );

	REQUIRE(bx::isSorted(str, BX_COUNTOF(str), sizeof(str[0]), strCmpFn) );

	auto bsearchStrCmpFn = [](const void* _lhs, const void* _rhs)
	{
		const char* lhs = (const char*)_lhs;
		const char* rhs = *(const char**)_rhs;
		return bx::strCmp(lhs, rhs);
	};

	REQUIRE(-1 == bx::binarySearch("sljiva", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 0 == bx::binarySearch("jabuka", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 1 == bx::binarySearch("jagoda", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 2 == bx::binarySearch("kruska", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 3 == bx::binarySearch("malina", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE(-1 == bx::binarySearch("kupina", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );

	auto byteCmpFn = [](const void* _lhs, const void* _rhs)
	{
		int8_t lhs = *(const int8_t*)_lhs;
		int8_t rhs = *(const int8_t*)_rhs;
		return lhs - rhs;
	};

	int8_t byte[128];
	bx::RngMwc rng;
	for (uint32_t ii = 0; ii < BX_COUNTOF(byte); ++ii)
	{
		byte[ii] = rng.gen()&0xff;
	}

	REQUIRE(!bx::isSorted(byte, BX_COUNTOF(byte), sizeof(byte[0]), byteCmpFn) );

	bx::quickSort(byte, BX_COUNTOF(byte), sizeof(byte[0]), byteCmpFn);

	for (uint32_t ii = 1; ii < BX_COUNTOF(byte); ++ii)
	{
		REQUIRE(byte[ii-1] <= byte[ii]);
	}

	REQUIRE(bx::isSorted(byte, BX_COUNTOF(byte), sizeof(byte[0]), byteCmpFn) );
}
