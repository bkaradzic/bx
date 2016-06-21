/*
 * Copyright 2010-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_FLOAT4_NEON_H_HEADER_GUARD
#define BX_FLOAT4_NEON_H_HEADER_GUARD

#include <arm_neon.h>

namespace bx
{
	typedef float32x4_t float4_t;


#define ELEMx 0
#define ELEMy 1
#define ELEMz 2
#define ELEMw 3
#define IMPLEMENT_SWIZZLE(_x, _y, _z, _w) \
			BX_FLOAT4_FORCE_INLINE float4_t float4_swiz_##_x##_y##_z##_w(float4_t _a) \
			{ \
				return __builtin_shuffle(_a, (uint32x4_t){ ELEM##_x, ELEM##_y, ELEM##_z, ELEM##_w }); \
			}

#include "float4_swizzle.inl"

#undef IMPLEMENT_SWIZZLE
#undef ELEMw
#undef ELEMz
#undef ELEMy
#undef ELEMx

#define IMPLEMENT_TEST(_xyzw, _swizzle) \
			BX_FLOAT4_FORCE_INLINE bool float4_test_any_##_xyzw(float4_t _test); \
			BX_FLOAT4_FORCE_INLINE bool float4_test_all_##_xyzw(float4_t _test);

IMPLEMENT_TEST(x    , xxxx);
IMPLEMENT_TEST(y    , yyyy);
IMPLEMENT_TEST(xy   , xyyy);
IMPLEMENT_TEST(z    , zzzz);
IMPLEMENT_TEST(xz   , xzzz);
IMPLEMENT_TEST(yz   , yzzz);
IMPLEMENT_TEST(xyz  , xyzz);
IMPLEMENT_TEST(w    , wwww);
IMPLEMENT_TEST(xw   , xwww);
IMPLEMENT_TEST(yw   , ywww);
IMPLEMENT_TEST(xyw  , xyww);
IMPLEMENT_TEST(zw   , zwww);
IMPLEMENT_TEST(xzw  , xzww);
IMPLEMENT_TEST(yzw  , yzww);
IMPLEMENT_TEST(xyzw , xyzw);

#undef IMPLEMENT_TEST

	BX_FLOAT4_FORCE_INLINE float4_t float4_shuf_xyAB(float4_t _a, float4_t _b)
	{
		return __builtin_shuffle(_a, _b, (uint32x4_t){ 0, 1, 4, 5 });
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_shuf_ABxy(float4_t _a, float4_t _b)
	{
		return __builtin_shuffle(_a, _b, (uint32x4_t){ 4, 5, 0, 1 });
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_shuf_CDzw(float4_t _a, float4_t _b)
	{
		return __builtin_shuffle(_a, _b, (uint32x4_t){ 6, 7, 2, 3 });
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_shuf_zwCD(float4_t _a, float4_t _b)
	{
		return __builtin_shuffle(_a, _b, (uint32x4_t){ 2, 3, 6, 7 });
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_shuf_xAyB(float4_t _a, float4_t _b)
	{
		return __builtin_shuffle(_a, _b, (uint32x4_t){ 0, 4, 1, 5 });
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_shuf_yBxA(float4_t _a, float4_t _b)
	{
		return __builtin_shuffle(_a, _b, (uint32x4_t){ 1, 5, 0, 4 });
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_shuf_zCwD(float4_t _a, float4_t _b)
	{
		return __builtin_shuffle(_a, _b, (uint32x4_t){ 2, 6, 3, 7 });
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_shuf_CzDw(float4_t _a, float4_t _b)
	{
		return __builtin_shuffle(_a, _b, (uint32x4_t){ 6, 2, 7, 3 });
	}

	BX_FLOAT4_FORCE_INLINE float float4_x(float4_t _a)
	{
		return vgetq_lane_f32(_a, 0);
	}

	BX_FLOAT4_FORCE_INLINE float float4_y(float4_t _a)
	{
		return vgetq_lane_f32(_a, 1);
	}

	BX_FLOAT4_FORCE_INLINE float float4_z(float4_t _a)
	{
		return vgetq_lane_f32(_a, 2);
	}

	BX_FLOAT4_FORCE_INLINE float float4_w(float4_t _a)
	{
		return vgetq_lane_f32(_a, 3);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_ld(const void* _ptr)
	{
		return vld1q_f32( (const float32_t*)_ptr);
	}

	BX_FLOAT4_FORCE_INLINE void float4_st(void* _ptr, float4_t _a)
	{
		vst1q_f32( (float32_t*)_ptr, _a);
	}

	BX_FLOAT4_FORCE_INLINE void float4_stx(void* _ptr, float4_t _a)
	{
		vst1q_lane_f32( (float32_t*)_ptr, _a, 0);
	}

	BX_FLOAT4_FORCE_INLINE void float4_stream(void* _ptr, float4_t _a)
	{
		vst1q_f32( (float32_t*)_ptr, _a);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_ld(float _x, float _y, float _z, float _w)
	{
		const float32_t val[4] = {_x, _y, _z, _w};
		return float4_ld(val);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_ild(uint32_t _x, uint32_t _y, uint32_t _z, uint32_t _w)
	{
		const uint32_t   val[4] = {_x, _y, _z, _w};
		const uint32x4_t tmp    = vld1q_u32(val);
		const float4_t   result = vreinterpretq_f32_u32(tmp);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_splat(const void* _ptr)
	{
		const float4_t    tmp0   = vld1q_f32( (const float32_t*)_ptr);
		const float32x2_t tmp1   = vget_low_f32(tmp0);
		const float4_t    result = vdupq_lane_f32(tmp1, 0);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_splat(float _a)
	{
		return vdupq_n_f32(_a);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_isplat(uint32_t _a)
	{
		const int32x4_t tmp    = vdupq_n_s32(_a);
		const float4_t  result = vreinterpretq_f32_s32(tmp);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_zero()
	{
		return float4_isplat(0);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_itof(float4_t _a)
	{
		const int32x4_t itof   = vreinterpretq_s32_f32(_a);
		const float4_t  result = vcvtq_f32_s32(itof);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_ftoi(float4_t _a)
	{
		const int32x4_t ftoi  = vcvtq_s32_f32(_a);
		const float4_t result = vreinterpretq_f32_s32(ftoi);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_add(float4_t _a, float4_t _b)
	{
		return vaddq_f32(_a, _b);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_sub(float4_t _a, float4_t _b)
	{
		return vsubq_f32(_a, _b);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_mul(float4_t _a, float4_t _b)
	{
		return vmulq_f32(_a, _b);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_rcp_est(float4_t _a)
	{
		return vrecpeq_f32(_a);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_rsqrt_est(float4_t _a)
	{
		return vrsqrteq_f32(_a);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_cmpeq(float4_t _a, float4_t _b)
	{
		const uint32x4_t tmp    = vceqq_f32(_a, _b);
		const float4_t   result = vreinterpretq_f32_u32(tmp);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_cmplt(float4_t _a, float4_t _b)
	{
		const uint32x4_t tmp    = vcltq_f32(_a, _b);
		const float4_t   result = vreinterpretq_f32_u32(tmp);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_cmple(float4_t _a, float4_t _b)
	{
		const uint32x4_t tmp    = vcleq_f32(_a, _b);
		const float4_t   result = vreinterpretq_f32_u32(tmp);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_cmpgt(float4_t _a, float4_t _b)
	{
		const uint32x4_t tmp    = vcgtq_f32(_a, _b);
		const float4_t   result = vreinterpretq_f32_u32(tmp);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_cmpge(float4_t _a, float4_t _b)
	{
		const uint32x4_t tmp    = vcgeq_f32(_a, _b);
		const float4_t   result = vreinterpretq_f32_u32(tmp);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_min(float4_t _a, float4_t _b)
	{
		return vminq_f32(_a, _b);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_max(float4_t _a, float4_t _b)
	{
		return vmaxq_f32(_a, _b);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_and(float4_t _a, float4_t _b)
	{
		const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t tmp1   = vreinterpretq_s32_f32(_b);
		const int32x4_t tmp2   = vandq_s32(tmp0, tmp1);
		const float4_t  result = vreinterpretq_f32_s32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_andc(float4_t _a, float4_t _b)
	{
		const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t tmp1   = vreinterpretq_s32_f32(_b);
		const int32x4_t tmp2   = vbicq_s32(tmp0, tmp1);
		const float4_t  result = vreinterpretq_f32_s32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_or(float4_t _a, float4_t _b)
	{
		const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t tmp1   = vreinterpretq_s32_f32(_b);
		const int32x4_t tmp2   = vorrq_s32(tmp0, tmp1);
		const float4_t  result = vreinterpretq_f32_s32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_xor(float4_t _a, float4_t _b)
	{
		const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t tmp1   = vreinterpretq_s32_f32(_b);
		const int32x4_t tmp2   = veorq_s32(tmp0, tmp1);
		const float4_t  result = vreinterpretq_f32_s32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_sll(float4_t _a, int _count)
	{
		if (__builtin_constant_p(_count) )
		{
			const uint32x4_t tmp0   = vreinterpretq_u32_f32(_a);
			const uint32x4_t tmp1   = vshlq_n_u32(tmp0, _count);
			const float4_t   result = vreinterpretq_f32_u32(tmp1);

			return result;
		}

		const uint32x4_t tmp0   = vreinterpretq_u32_f32(_a);
		const int32x4_t  shift  = vdupq_n_s32(_count);
		const uint32x4_t tmp1   = vshlq_u32(tmp0, shift);
		const float4_t   result = vreinterpretq_f32_u32(tmp1);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_srl(float4_t _a, int _count)
	{
		if (__builtin_constant_p(_count) )
		{
			const uint32x4_t tmp0   = vreinterpretq_u32_f32(_a);
			const uint32x4_t tmp1   = vshrq_n_u32(tmp0, _count);
			const float4_t   result = vreinterpretq_f32_u32(tmp1);

			return result;
		}

		const uint32x4_t tmp0   = vreinterpretq_u32_f32(_a);
		const int32x4_t  shift  = vdupq_n_s32(-_count);
		const uint32x4_t tmp1   = vshlq_u32(tmp0, shift);
		const float4_t   result = vreinterpretq_f32_u32(tmp1);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_sra(float4_t _a, int _count)
	{
		if (__builtin_constant_p(_count) )
		{
			const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
			const int32x4_t tmp1   = vshrq_n_s32(tmp0, _count);
			const float4_t  result = vreinterpretq_f32_s32(tmp1);

			return result;
		}

		const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t shift  = vdupq_n_s32(-_count);
		const int32x4_t tmp1   = vshlq_s32(tmp0, shift);
		const float4_t  result = vreinterpretq_f32_s32(tmp1);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_madd(float4_t _a, float4_t _b, float4_t _c)
	{
		return vmlaq_f32(_c, _a, _b);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_nmsub(float4_t _a, float4_t _b, float4_t _c)
	{
		return vmlsq_f32(_c, _a, _b);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_icmpeq(float4_t _a, float4_t _b)
	{
		const int32x4_t  tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t  tmp1   = vreinterpretq_s32_f32(_b);
		const uint32x4_t tmp2   = vceqq_s32(tmp0, tmp1);
		const float4_t   result = vreinterpretq_f32_u32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_icmplt(float4_t _a, float4_t _b)
	{
		const int32x4_t  tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t  tmp1   = vreinterpretq_s32_f32(_b);
		const uint32x4_t tmp2   = vcltq_s32(tmp0, tmp1);
		const float4_t   result = vreinterpretq_f32_u32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_icmpgt(float4_t _a, float4_t _b)
	{
		const int32x4_t  tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t  tmp1   = vreinterpretq_s32_f32(_b);
		const uint32x4_t tmp2   = vcgtq_s32(tmp0, tmp1);
		const float4_t   result = vreinterpretq_f32_u32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_imin(float4_t _a, float4_t _b)
	{
		const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t tmp1   = vreinterpretq_s32_f32(_b);
		const int32x4_t tmp2   = vminq_s32(tmp0, tmp1);
		const float4_t  result = vreinterpretq_f32_s32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_imax(float4_t _a, float4_t _b)
	{
		const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t tmp1   = vreinterpretq_s32_f32(_b);
		const int32x4_t tmp2   = vmaxq_s32(tmp0, tmp1);
		const float4_t  result = vreinterpretq_f32_s32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_iadd(float4_t _a, float4_t _b)
	{
		const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t tmp1   = vreinterpretq_s32_f32(_b);
		const int32x4_t tmp2   = vaddq_s32(tmp0, tmp1);
		const float4_t  result = vreinterpretq_f32_s32(tmp2);

		return result;
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_isub(float4_t _a, float4_t _b)
	{
		const int32x4_t tmp0   = vreinterpretq_s32_f32(_a);
		const int32x4_t tmp1   = vreinterpretq_s32_f32(_b);
		const int32x4_t tmp2   = vsubq_s32(tmp0, tmp1);
		const float4_t  result = vreinterpretq_f32_s32(tmp2);

		return result;
	}

} // namespace bx

#define float4_shuf_xAzC     float4_shuf_xAzC_ni
#define float4_shuf_yBwD     float4_shuf_yBwD_ni
#define float4_rcp           float4_rcp_ni
#define float4_orx           float4_orx_ni
#define float4_orc           float4_orc_ni
#define float4_neg           float4_neg_ni
#define float4_madd          float4_madd_ni
#define float4_nmsub         float4_nmsub_ni
#define float4_div_nr        float4_div_nr_ni
#define float4_div           float4_div_nr_ni
#define float4_selb          float4_selb_ni
#define float4_sels          float4_sels_ni
#define float4_not           float4_not_ni
#define float4_abs           float4_abs_ni
#define float4_clamp         float4_clamp_ni
#define float4_lerp          float4_lerp_ni
#define float4_rsqrt         float4_rsqrt_ni
#define float4_rsqrt_nr      float4_rsqrt_nr_ni
#define float4_rsqrt_carmack float4_rsqrt_carmack_ni
#define float4_sqrt_nr       float4_sqrt_nr_ni
#define float4_sqrt          float4_sqrt_nr_ni
#define float4_log2          float4_log2_ni
#define float4_exp2          float4_exp2_ni
#define float4_pow           float4_pow_ni
#define float4_cross3        float4_cross3_ni
#define float4_normalize3    float4_normalize3_ni
#define float4_dot3          float4_dot3_ni
#define float4_dot           float4_dot_ni
#define float4_ceil          float4_ceil_ni
#define float4_floor         float4_floor_ni

#include "float4_ni.h"

namespace bx
{
#define IMPLEMENT_TEST(_xyzw, _swizzle) \
			BX_FLOAT4_FORCE_INLINE bool float4_test_any_##_xyzw(float4_t _test) \
			{ \
				const float4_t tmp0 = float4_swiz_##_swizzle(_test); \
				return float4_test_any_ni(tmp0); \
			} \
			\
			BX_FLOAT4_FORCE_INLINE bool float4_test_all_##_xyzw(float4_t _test) \
			{ \
				const float4_t tmp0 = float4_swiz_##_swizzle(_test); \
				return float4_test_all_ni(tmp0); \
			}

IMPLEMENT_TEST(x    , xxxx);
IMPLEMENT_TEST(y    , yyyy);
IMPLEMENT_TEST(xy   , xyyy);
IMPLEMENT_TEST(z    , zzzz);
IMPLEMENT_TEST(xz   , xzzz);
IMPLEMENT_TEST(yz   , yzzz);
IMPLEMENT_TEST(xyz  , xyzz);
IMPLEMENT_TEST(w    , wwww);
IMPLEMENT_TEST(xw   , xwww);
IMPLEMENT_TEST(yw   , ywww);
IMPLEMENT_TEST(xyw  , xyww);
IMPLEMENT_TEST(zw   , zwww);
IMPLEMENT_TEST(xzw  , xzww);
IMPLEMENT_TEST(yzw  , yzww);

	BX_FLOAT4_FORCE_INLINE bool float4_test_any_xyzw(float4_t _test)
	{
		return float4_test_any_ni(_test);
	}

	BX_FLOAT4_FORCE_INLINE bool float4_test_all_xyzw(float4_t _test)
	{
		return float4_test_all_ni(_test);
	}

#undef IMPLEMENT_TEST
} // namespace bx

#endif // BX_FLOAT4_NEON_H_HEADER_GUARD
