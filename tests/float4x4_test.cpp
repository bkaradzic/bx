/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/float4x4_t.h>
#include <bx/math.h>
#include <bx/string.h>

TEST_CASE("float4x4_inverse", "[float4x4]")
{
	const float src[] =
	{
		1.20629f,     0.0f,      0.0f,  0.0f,
		    0.0f, 2.14451f,      0.0f,  0.0f,
		    0.0f,     0.0f, -1.00001f, -1.0f,
		    0.0f,     0.0f, -2.00001f,  0.0f,
	};
	static_assert(BX_COUNTOF(src) == 16);

	bx::float4x4_t mtx;
	mtx.col[0] = bx::simd128_ld(src[ 0], src[ 1], src[ 2], src[ 3]);
	mtx.col[1] = bx::simd128_ld(src[ 4], src[ 5], src[ 6], src[ 7]);
	mtx.col[2] = bx::simd128_ld(src[ 8], src[ 9], src[10], src[11]);
	mtx.col[3] = bx::simd128_ld(src[12], src[13], src[14], src[15]);

	bx::float4x4_t inv;
	bx::float4x4_inverse(&inv, &mtx);

	float ref[16];
	bx::mtxInverse(ref, src);

	BX_ALIGN_DECL_16(float) result[16];
	bx::simd128_st(&result[ 0], inv.col[0]);
	bx::simd128_st(&result[ 4], inv.col[1]);
	bx::simd128_st(&result[ 8], inv.col[2]);
	bx::simd128_st(&result[12], inv.col[3]);

	for (uint32_t ii = 0; ii < 16; ++ii)
	{
		const float diff = bx::abs(result[ii] - ref[ii]);
		INFO("ii=" << ii << " result=" << result[ii] << " ref=" << ref[ii] << " diff=" << diff);
		CHECK(bx::isEqual(result[ii], ref[ii], 0.001f) );
	}
}
