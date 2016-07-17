/*
 * Copyright 2010-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_FLOAT4_T_H_HEADER_GUARD
#define BX_FLOAT4_T_H_HEADER_GUARD

#include "bx.h"

#define BX_FLOAT4_FORCE_INLINE BX_FORCE_INLINE
#define BX_FLOAT4_INLINE inline

#if defined(__SSE2__) || (BX_COMPILER_MSVC && (BX_ARCH_64BIT || _M_IX86_FP >= 2) )
#	include <emmintrin.h> // __m128i
#	if defined(__SSE4_1__)
#		include <smmintrin.h>
#	endif // defined(__SSE4_1__)
#	include <xmmintrin.h> // __m128

namespace bx
{
	typedef __m128 float4_sse_t;

} // namespace bx

#elif defined(__ARM_NEON__) && !BX_COMPILER_CLANG
#	include <arm_neon.h>

namespace bx
{
	typedef float32x4_t float4_neon_t;

} // namespace bx

#elif BX_COMPILER_CLANG \
		&& !BX_PLATFORM_EMSCRIPTEN \
		&& !BX_PLATFORM_IOS \
		&& BX_CLANG_HAS_EXTENSION(attribute_ext_vector_type)
#	include <math.h>

namespace bx
{
	typedef union float4_langext_t
	{
		float    __attribute__((vector_size(16))) vf;
		int32_t  __attribute__((vector_size(16))) vi;
		uint32_t __attribute__((vector_size(16))) vu;
		float    fxyzw[4];
		int32_t  ixyzw[4];
		uint32_t uxyzw[4];

	} float4_langext_t;
} // namespace bx
#endif //

namespace bx
{
	typedef union float4_ref_t
	{
		float    fxyzw[4];
		int32_t  ixyzw[4];
		uint32_t uxyzw[4];

	} float4_ref_t;
} // namespace bx

namespace bx
{
#define ELEMx 0
#define ELEMy 1
#define ELEMz 2
#define ELEMw 3
#define IMPLEMENT_SWIZZLE(_x, _y, _z, _w) \
			template<typename Ty> \
			BX_FLOAT4_FORCE_INLINE Ty float4_swiz_##_x##_y##_z##_w(Ty _a);
#include "float4_swizzle.inl"

#undef IMPLEMENT_SWIZZLE
#undef ELEMw
#undef ELEMz
#undef ELEMy
#undef ELEMx

#define IMPLEMENT_TEST(_xyzw) \
			template<typename Ty> \
			BX_FLOAT4_FORCE_INLINE bool float4_test_any_##_xyzw(Ty _test); \
			\
			template<typename Ty> \
			BX_FLOAT4_FORCE_INLINE bool float4_test_all_##_xyzw(Ty _test)

IMPLEMENT_TEST(x   );
IMPLEMENT_TEST(y   );
IMPLEMENT_TEST(xy  );
IMPLEMENT_TEST(z   );
IMPLEMENT_TEST(xz  );
IMPLEMENT_TEST(yz  );
IMPLEMENT_TEST(xyz );
IMPLEMENT_TEST(w   );
IMPLEMENT_TEST(xw  );
IMPLEMENT_TEST(yw  );
IMPLEMENT_TEST(xyw );
IMPLEMENT_TEST(zw  );
IMPLEMENT_TEST(xzw );
IMPLEMENT_TEST(yzw );
IMPLEMENT_TEST(xyzw);
#undef IMPLEMENT_TEST

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_shuf_xyAB(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_shuf_ABxy(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_shuf_CDzw(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_shuf_zwCD(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_shuf_xAyB(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_shuf_yBxA(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_shuf_zCwD(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_shuf_CzDw(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE float float4_x(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE float float4_y(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE float float4_z(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE float float4_w(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_ld(const void* _ptr);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE void float4_st(void* _ptr, Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE void float4_stx(void* _ptr, Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE void float4_stream(void* _ptr, Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_ld(float _x, float _y, float _z, float _w);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_ild(uint32_t _x, uint32_t _y, uint32_t _z, uint32_t _w);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_splat(const void* _ptr);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_splat(float _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_isplat(uint32_t _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_zero();

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_itof(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_ftoi(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_round(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_add(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_sub(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_mul(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_div(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_rcp_est(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_sqrt(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_rsqrt_est(Ty _a);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_dot3(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_dot(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_cmpeq(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_cmplt(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_cmple(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_cmpgt(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_cmpge(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_min(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_max(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_and(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_andc(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_or(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_xor(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_sll(Ty _a, int _count);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_srl(Ty _a, int _count);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_sra(Ty _a, int _count);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_icmpeq(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_icmplt(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_icmpgt(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_imin(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_imax(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_iadd(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_isub(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_shuf_xAzC(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_shuf_yBwD(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_rcp(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_orx(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_orc(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_neg(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_madd(Ty _a, Ty _b, Ty _c);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_nmsub(Ty _a, Ty _b, Ty _c);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_div_nr(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_selb(Ty _mask, Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_sels(Ty _test, Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_not(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_abs(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_clamp(Ty _a, Ty _min, Ty _max);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_lerp(Ty _a, Ty _b, Ty _s);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_rsqrt(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_rsqrt_nr(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_rsqrt_carmack(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_sqrt_nr(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_log2(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_exp2(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_pow(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_cross3(Ty _a, Ty _b);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_normalize3(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_ceil(Ty _a);

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_floor(Ty _a);

} // namespace bx

#if defined(__SSE2__) || (BX_COMPILER_MSVC && (BX_ARCH_64BIT || _M_IX86_FP >= 2) )
#	include "float4_sse.h"
#elif defined(__ARM_NEON__) && !BX_COMPILER_CLANG
#	include "float4_neon.h"
#elif BX_COMPILER_CLANG \
		&& !BX_PLATFORM_EMSCRIPTEN \
		&& !BX_PLATFORM_IOS \
		&& BX_CLANG_HAS_EXTENSION(attribute_ext_vector_type)
#	include "float4_langext.h"
#else
#	ifndef BX_FLOAT4_WARN_REFERENCE_IMPL
#		define BX_FLOAT4_WARN_REFERENCE_IMPL 0
#	endif // BX_FLOAT4_WARN_REFERENCE_IMPL

#	if BX_FLOAT4_WARN_REFERENCE_IMPL
#		pragma message("************************************\nUsing SIMD reference implementation!\n************************************")
#	endif // BX_FLOAT4_WARN_REFERENCE_IMPL

namespace bx
{
	typedef float4_ref_t float4_t;
}
#endif //

#include "float4_ref.h"

namespace bx
{
	BX_FLOAT4_FORCE_INLINE float4_t float4_zero()
	{
		return float4_zero<float4_t>();
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_ld(const void* _ptr)
	{
		return float4_ld<float4_t>(_ptr);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_ld(float _x, float _y, float _z, float _w)
	{
		return float4_ld<float4_t>(_x, _y, _z, _w);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_ild(uint32_t _x, uint32_t _y, uint32_t _z, uint32_t _w)
	{
		return float4_ild<float4_t>(_x, _y, _z, _w);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_splat(const void* _ptr)
	{
		return float4_splat<float4_t>(_ptr);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_splat(float _a)
	{
		return float4_splat<float4_t>(_a);
	}

	BX_FLOAT4_FORCE_INLINE float4_t float4_isplat(uint32_t _a)
	{
		return float4_isplat<float4_t>(_a);
	}
}

#endif // BX_FLOAT4_T_H_HEADER_GUARD
