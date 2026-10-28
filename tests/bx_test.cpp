/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/bx.h>

TEST_CASE("StrideAlign", "[bx]")
{
	REQUIRE(0  == bx::strideAlign(0, 12) );
	for (uint32_t ii = 0; ii < 12; ++ii)
	{
		REQUIRE(12 == bx::strideAlign(ii+1, 12) );
	}

	REQUIRE(0  == bx::strideAlign<16>(0, 12) );
	for (uint32_t ii = 0; ii < 12; ++ii)
	{
		REQUIRE(48 == bx::strideAlign<16>(ii+1, 12) );
	}

	uint32_t offset = 11;
	offset = bx::strideAlign(offset, 32);
	REQUIRE(offset == 32);

	offset = bx::strideAlign(offset, 24);
	REQUIRE(offset == 48);
}

TEST_CASE("gcd", "[bx]")
{
	REQUIRE(1 == bx::gcd(13u, 89u) );
	REQUIRE(3 == bx::gcd( 3u,  9u) );
	REQUIRE(8 == bx::gcd( 8u, 64u) );
	REQUIRE(9 == bx::gcd(18u, 81u) );
}

TEST_CASE("isInRange", "[bx]")
{
	REQUIRE( bx::isInRange(5, 0, 10) );
	REQUIRE( bx::isInRange(0, 0, 10) );
	REQUIRE( bx::isInRange(10, 0, 10) );
	REQUIRE(!bx::isInRange(-1, 0, 10) );
	REQUIRE(!bx::isInRange(11, 0, 10) );
	REQUIRE( bx::isInRange(uint8_t(255), 0, 255) );
	REQUIRE(!bx::isInRange(uint8_t(0), 1, 255) );
	REQUIRE( bx::isInRange('m', 'a', 'z') );
	REQUIRE(!bx::isInRange('M', 'a', 'z') );
	REQUIRE( bx::isInRange(0.5f, 0.0f, 1.0f) );
	REQUIRE(!bx::isInRange(1.5f, 0.0f, 1.0f) );

	STATIC_REQUIRE(bx::isInRange(3, 1, 3) );
}

TEST_CASE("lcm", "[bx]")
{
	REQUIRE(1157 == bx::lcm(13u, 89u) );
	REQUIRE(   9 == bx::lcm( 3u,  9u) );
	REQUIRE(  48 == bx::lcm( 6u, 16u) );
	REQUIRE(  80 == bx::lcm(16u, 20u) );
}

TEST_CASE("align", "[bx]")
{
	REQUIRE( bx::isAligned(0,  8) );
	REQUIRE(!bx::isAligned(7,  8) );
	REQUIRE( bx::isAligned(64, 8) );
	REQUIRE(!bx::isAligned(63, 8) );

	for (int32_t ii = 0; ii < 1024; ++ii)
	{
		REQUIRE(bx::isAligned(ii, 0) );
		REQUIRE(ii == bx::alignUp(ii, 0) );
		REQUIRE(ii == bx::alignDown(ii, 0) );
	}

	REQUIRE(  0 == bx::alignUp(  0, 16) );
	REQUIRE( 16 == bx::alignUp(  1, 16) );
	REQUIRE( 16 == bx::alignUp( 15, 16) );
	REQUIRE( 16 == bx::alignUp( 16, 16) );
	REQUIRE(256 == bx::alignUp(255, 16) );
	REQUIRE(  0 == bx::alignUp(-1,  16) );
	REQUIRE(-16 == bx::alignUp(-31, 16) );

	REQUIRE(  0 == bx::alignUp(  0, 256) );
	REQUIRE(256 == bx::alignUp(  1, 256) );
	REQUIRE(256 == bx::alignUp( 15, 256) );
	REQUIRE(256 == bx::alignUp(255, 256) );
	REQUIRE(256 == bx::alignUp(256, 256) );
	REQUIRE(256 == bx::alignUp(256, 256) );
	REQUIRE(512 == bx::alignUp(511, 256) );

	REQUIRE(  0 == bx::alignDown(  0, 16) );
	REQUIRE(  0 == bx::alignDown(  1, 16) );
	REQUIRE(  0 == bx::alignDown( 15, 16) );
	REQUIRE( 16 == bx::alignDown( 16, 16) );
	REQUIRE(240 == bx::alignDown(255, 16) );
	REQUIRE(-16 == bx::alignDown(-1,  16) );
	REQUIRE(-32 == bx::alignDown(-31, 16) );
}

TEST_CASE("loadUnaligned, storeUnaligned", "[bx]")
{
	uint8_t bytes[24];

	for (uint32_t ii = 0; ii < BX_COUNTOF(bytes); ++ii)
	{
		bytes[ii] = uint8_t(ii + 1);
	}

	for (uint32_t ii = 0; ii < 8; ++ii)
	{
		REQUIRE(bx::loadUnalignedRef<uint32_t>(&bytes[ii]) == bx::loadUnaligned<uint32_t>(&bytes[ii]) );
		REQUIRE(bx::loadUnalignedRef<uint64_t>(&bytes[ii]) == bx::loadUnaligned<uint64_t>(&bytes[ii]) );
	}

	for (uint32_t ii = 0; ii < 8; ++ii)
	{
		bx::memSet(bytes, 0, sizeof(bytes) );
		bx::storeUnaligned<uint64_t>(&bytes[ii], UINT64_C(0x0807060504030201) );

		REQUIRE(UINT64_C(0x0807060504030201) == bx::loadUnalignedRef<uint64_t>(&bytes[ii]) );
		REQUIRE(0 == bx::loadUnalignedRef<uint64_t>(&bytes[ii+8]) );
		REQUIRE(0 == bytes[ii+16]);
	}
}
