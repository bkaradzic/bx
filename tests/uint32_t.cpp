/*
 * Copyright 2010-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "test.h"
#include <bx/uint32_t.h>

TEST(StrideAlign)
{
	CHECK(0  == bx::strideAlign(0, 12) );
	for (uint32_t ii = 0; ii < 12; ++ii)
	{
		CHECK(12 == bx::strideAlign(ii+1, 12) );
	}

	CHECK(0  == bx::strideAlign16(0, 12) );
	for (uint32_t ii = 0; ii < 12; ++ii)
	{
		CHECK(48 == bx::strideAlign16(ii+1, 12) );
	}

	CHECK(0  == bx::uint32_cnttz(UINT32_C(1) ) );
	CHECK(0  == bx::uint32_cnttz_ref(UINT32_C(1) ) );

	CHECK(31 == bx::uint32_cntlz(UINT32_C(1) ) );
	CHECK(31 == bx::uint32_cntlz_ref(UINT32_C(1) ) );

	CHECK(0  == bx::uint64_cnttz(UINT64_C(1) ) );
	CHECK(0  == bx::uint64_cnttz_ref(UINT64_C(1) ) );

	CHECK(63 == bx::uint64_cntlz(UINT64_C(1) ) );
	CHECK(63 == bx::uint64_cntlz_ref(UINT64_C(1) ) );
}
