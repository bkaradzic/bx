/*
 * Copyright 2010-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_FLOAT4_NI_H_HEADER_GUARD
#define BX_FLOAT4_NI_H_HEADER_GUARD

namespace bx
{
	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_shuf_xAzC_ni(Ty _a, Ty _b)
	{
		const Ty xAyB   = float4_shuf_xAyB(_a, _b);
		const Ty zCwD   = float4_shuf_zCwD(_a, _b);
		const Ty result = float4_shuf_xyAB(xAyB, zCwD);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_shuf_yBwD_ni(Ty _a, Ty _b)
	{
		const Ty xAyB   = float4_shuf_xAyB(_a, _b);
		const Ty zCwD   = float4_shuf_zCwD(_a, _b);
		const Ty result = float4_shuf_zwCD(xAyB, zCwD);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_madd_ni(Ty _a, Ty _b, Ty _c)
	{
		const Ty mul    = float4_mul(_a, _b);
		const Ty result = float4_add(mul, _c);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_nmsub_ni(Ty _a, Ty _b, Ty _c)
	{
		const Ty mul    = float4_mul(_a, _b);
		const Ty result = float4_sub(_c, mul);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_div_nr_ni(Ty _a, Ty _b)
	{
		const Ty oneish  = float4_isplat<Ty>(0x3f800001);
		const Ty est     = float4_rcp_est(_b);
		const Ty iter0   = float4_mul(_a, est);
		const Ty tmp1    = float4_nmsub(_b, est, oneish);
		const Ty result  = float4_madd(tmp1, iter0, iter0);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_rcp_ni(Ty _a)
	{
		const Ty one    = float4_splat<Ty>(1.0f);
		const Ty result = float4_div(one, _a);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_orx_ni(Ty _a)
	{
		const Ty zwxy   = float4_swiz_zwxy(_a);
		const Ty tmp0   = float4_or(_a, zwxy);
		const Ty tmp1   = float4_swiz_yyyy(_a);
		const Ty tmp2   = float4_or(tmp0, tmp1);
		const Ty mf000  = float4_ild<Ty>(UINT32_MAX, 0, 0, 0);
		const Ty result = float4_and(tmp2, mf000);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_orc_ni(Ty _a, Ty _b)
	{
		const Ty aorb   = float4_or(_a, _b);
		const Ty mffff  = float4_isplat<Ty>(UINT32_MAX);
		const Ty result = float4_xor(aorb, mffff);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_neg_ni(Ty _a)
	{
		const Ty zero   = float4_zero<Ty>();
		const Ty result = float4_sub(zero, _a);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_selb_ni(Ty _mask, Ty _a, Ty _b)
	{
		const Ty sel_a  = float4_and(_a, _mask);
		const Ty sel_b  = float4_andc(_b, _mask);
		const Ty result = float4_or(sel_a, sel_b);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_sels_ni(Ty _test, Ty _a, Ty _b)
	{
		const Ty mask   = float4_sra(_test, 31);
		const Ty result = float4_selb(mask, _a, _b);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_not_ni(Ty _a)
	{
		const Ty mffff  = float4_isplat<Ty>(UINT32_MAX);
		const Ty result = float4_xor(_a, mffff);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_min_ni(Ty _a, Ty _b)
	{
		const Ty mask   = float4_cmplt(_a, _b);
		const Ty result = float4_selb(mask, _a, _b);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_max_ni(Ty _a, Ty _b)
	{
		const Ty mask   = float4_cmpgt(_a, _b);
		const Ty result = float4_selb(mask, _a, _b);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_abs_ni(Ty _a)
	{
		const Ty a_neg  = float4_neg(_a);
		const Ty result = float4_max(a_neg, _a);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_imin_ni(Ty _a, Ty _b)
	{
		const Ty mask   = float4_icmplt(_a, _b);
		const Ty result = float4_selb(mask, _a, _b);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_imax_ni(Ty _a, Ty _b)
	{
		const Ty mask   = float4_icmpgt(_a, _b);
		const Ty result = float4_selb(mask, _a, _b);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_clamp_ni(Ty _a, Ty _min, Ty _max)
	{
		const Ty tmp    = float4_min(_a, _max);
		const Ty result = float4_max(tmp, _min);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_lerp_ni(Ty _a, Ty _b, Ty _s)
	{
		const Ty ba     = float4_sub(_b, _a);
		const Ty result = float4_madd(_s, ba, _a);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_sqrt_nr_ni(Ty _a)
	{
		const Ty half   = float4_splat<Ty>(0.5f);
		const Ty one    = float4_splat<Ty>(1.0f);
		const Ty tmp0   = float4_rsqrt_est(_a);
		const Ty tmp1   = float4_mul(tmp0, _a);
		const Ty tmp2   = float4_mul(tmp1, half);
		const Ty tmp3   = float4_nmsub(tmp0, tmp1, one);
		const Ty result = float4_madd(tmp3, tmp2, tmp1);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_sqrt_nr1_ni(Ty _a)
	{
		const Ty half = float4_splat<Ty>(0.5f);

		Ty result = _a;
		for (uint32_t ii = 0; ii < 11; ++ii)
		{
			const Ty tmp1 = float4_div(_a, result);
			const Ty tmp2 = float4_add(tmp1, result);
			result              = float4_mul(tmp2, half);
		}

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_rsqrt_ni(Ty _a)
	{
		const Ty one    = float4_splat<Ty>(1.0f);
		const Ty sqrt   = float4_sqrt(_a);
		const Ty result = float4_div(one, sqrt);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_rsqrt_nr_ni(Ty _a)
	{
		const Ty rsqrt           = float4_rsqrt_est(_a);
		const Ty iter0           = float4_mul(_a, rsqrt);
		const Ty iter1           = float4_mul(iter0, rsqrt);
		const Ty half            = float4_splat<Ty>(0.5f);
		const Ty half_rsqrt      = float4_mul(half, rsqrt);
		const Ty three           = float4_splat<Ty>(3.0f);
		const Ty three_sub_iter1 = float4_sub(three, iter1);
		const Ty result          = float4_mul(half_rsqrt, three_sub_iter1);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_rsqrt_carmack_ni(Ty _a)
	{
		const Ty half    = float4_splat<Ty>(0.5f);
		const Ty ah      = float4_mul(half, _a);
		const Ty ashift  = float4_sra(_a, 1);
		const Ty magic   = float4_isplat<Ty>(0x5f3759df);
		const Ty msuba   = float4_isub(magic, ashift);
		const Ty msubasq = float4_mul(msuba, msuba);
		const Ty tmp0    = float4_splat<Ty>(1.5f);
		const Ty tmp1    = float4_mul(ah, msubasq);
		const Ty tmp2    = float4_sub(tmp0, tmp1);
		const Ty result  = float4_mul(msuba, tmp2);

		return result;
	}

	namespace float4_logexp_detail
	{
		template<typename Ty>
		BX_FLOAT4_INLINE Ty float4_poly1(Ty _a, float _b, float _c)
		{
			const Ty bbbb   = float4_splat<Ty>(_b);
			const Ty cccc   = float4_splat<Ty>(_c);
			const Ty result = float4_madd(cccc, _a, bbbb);

			return result;
		}

		template<typename Ty>
		BX_FLOAT4_INLINE Ty float4_poly2(Ty _a, float _b, float _c, float _d)
		{
			const Ty bbbb   = float4_splat<Ty>(_b);
			const Ty poly   = float4_poly1(_a, _c, _d);
			const Ty result = float4_madd(poly, _a, bbbb);

			return result;
		}

		template<typename Ty>
		BX_FLOAT4_INLINE Ty float4_poly3(Ty _a, float _b, float _c, float _d, float _e)
		{
			const Ty bbbb   = float4_splat<Ty>(_b);
			const Ty poly   = float4_poly2(_a, _c, _d, _e);
			const Ty result = float4_madd(poly, _a, bbbb);

			return result;
		}

		template<typename Ty>
		BX_FLOAT4_INLINE Ty float4_poly4(Ty _a, float _b, float _c, float _d, float _e, float _f)
		{
			const Ty bbbb   = float4_splat<Ty>(_b);
			const Ty poly   = float4_poly3(_a, _c, _d, _e, _f);
			const Ty result = float4_madd(poly, _a, bbbb);

			return result;
		}

		template<typename Ty>
		BX_FLOAT4_INLINE Ty float4_poly5(Ty _a, float _b, float _c, float _d, float _e, float _f, float _g)
		{
			const Ty bbbb   = float4_splat<Ty>(_b);
			const Ty poly   = float4_poly4(_a, _c, _d, _e, _f, _g);
			const Ty result = float4_madd(poly, _a, bbbb);

			return result;
		}

		template<typename Ty>
		BX_FLOAT4_INLINE Ty float4_logpoly(Ty _a)
		{
#if 1
			const Ty result = float4_poly5(_a
				, 3.11578814719469302614f, -3.32419399085241980044f
				, 2.59883907202499966007f, -1.23152682416275988241f
				, 0.318212422185251071475f, -0.0344359067839062357313f
				);
#elif 0
			const Ty result = float4_poly4(_a
				, 2.8882704548164776201f, -2.52074962577807006663f
				, 1.48116647521213171641f, -0.465725644288844778798f
				, 0.0596515482674574969533f
				);
#elif 0
			const Ty result = float4_poly3(_a
				, 2.61761038894603480148f, -1.75647175389045657003f
				, 0.688243882994381274313f, -0.107254423828329604454f
				);
#else
			const Ty result = float4_poly2(_a
				, 2.28330284476918490682f, -1.04913055217340124191f
				, 0.204446009836232697516f
				);
#endif

			return result;
		}

		template<typename Ty>
		BX_FLOAT4_INLINE Ty float4_exppoly(Ty _a)
		{
#if 1
			const Ty result = float4_poly5(_a
				, 9.9999994e-1f, 6.9315308e-1f
				, 2.4015361e-1f, 5.5826318e-2f
				, 8.9893397e-3f, 1.8775767e-3f
				);
#elif 0
			const Ty result = float4_poly4(_a
				, 1.0000026f, 6.9300383e-1f
				, 2.4144275e-1f, 5.2011464e-2f
				, 1.3534167e-2f
				);
#elif 0
			const Ty result = float4_poly3(_a
				, 9.9992520e-1f, 6.9583356e-1f
				, 2.2606716e-1f, 7.8024521e-2f
				);
#else
			const Ty result = float4_poly2(_a
				, 1.0017247f, 6.5763628e-1f
				, 3.3718944e-1f
				);
#endif // 0

			return result;
		}
	} // namespace float4_internal

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_log2_ni(Ty _a)
	{
		const Ty expmask  = float4_isplat<Ty>(0x7f800000);
		const Ty mantmask = float4_isplat<Ty>(0x007fffff);
		const Ty one      = float4_splat<Ty>(1.0f);

		const Ty c127     = float4_isplat<Ty>(127);
		const Ty aexp     = float4_and(_a, expmask);
		const Ty aexpsr   = float4_srl(aexp, 23);
		const Ty tmp0     = float4_isub(aexpsr, c127);
		const Ty exp      = float4_itof(tmp0);

		const Ty amask    = float4_and(_a, mantmask);
		const Ty mant     = float4_or(amask, one);

		const Ty poly     = float4_logexp_detail::float4_logpoly(mant);

		const Ty mandiff  = float4_sub(mant, one);
		const Ty result   = float4_madd(poly, mandiff, exp);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_exp2_ni(Ty _a)
	{
		const Ty min      = float4_splat<Ty>( 129.0f);
		const Ty max      = float4_splat<Ty>(-126.99999f);
		const Ty tmp0     = float4_min(_a, min);
		const Ty aaaa     = float4_max(tmp0, max);

		const Ty half     = float4_splat<Ty>(0.5f);
		const Ty tmp2     = float4_sub(aaaa, half);
		const Ty ipart    = float4_ftoi(tmp2);
		const Ty iround   = float4_itof(ipart);
		const Ty fpart    = float4_sub(aaaa, iround);

		const Ty c127     = float4_isplat<Ty>(127);
		const Ty tmp5     = float4_iadd(ipart, c127);
		const Ty expipart = float4_sll(tmp5, 23);

		const Ty expfpart = float4_logexp_detail::float4_exppoly(fpart);

		const Ty result   = float4_mul(expipart, expfpart);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_pow_ni(Ty _a, Ty _b)
	{
		const Ty alog2  = float4_log2(_a);
		const Ty alog2b = float4_mul(alog2, _b);
		const Ty result = float4_exp2(alog2b);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_dot3_ni(Ty _a, Ty _b)
	{
		const Ty xyzw   = float4_mul(_a, _b);
		const Ty xxxx   = float4_swiz_xxxx(xyzw);
		const Ty yyyy   = float4_swiz_yyyy(xyzw);
		const Ty zzzz   = float4_swiz_zzzz(xyzw);
		const Ty tmp1   = float4_add(xxxx, yyyy);
		const Ty result = float4_add(zzzz, tmp1);
		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_cross3_ni(Ty _a, Ty _b)
	{
		// a.yzx * b.zxy - a.zxy * b.yzx == (a * b.yzx - a.yzx * b).yzx
#if 0
		const Ty a_yzxw = float4_swiz_yzxw(_a);
		const Ty a_zxyw = float4_swiz_zxyw(_a);
		const Ty b_zxyw = float4_swiz_zxyw(_b);
		const Ty b_yzxw = float4_swiz_yzxw(_b);
		const Ty tmp    = float4_mul(a_yzxw, b_zxyw);
		const Ty result = float4_nmsub(a_zxyw, b_yzxw, tmp);
#else
		const Ty a_yzxw = float4_swiz_yzxw(_a);
		const Ty b_yzxw = float4_swiz_yzxw(_b);
		const Ty tmp0   = float4_mul(_a, b_yzxw);
		const Ty tmp1   = float4_nmsub(a_yzxw, _b, tmp0);
		const Ty result = float4_swiz_yzxw(tmp1);
#endif

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_normalize3_ni(Ty _a)
	{
		const Ty dot3    = float4_dot3(_a, _a);
		const Ty invSqrt = float4_rsqrt(dot3);
		const Ty result  = float4_mul(_a, invSqrt);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_dot_ni(Ty _a, Ty _b)
	{
		const Ty xyzw   = float4_mul(_a, _b);
		const Ty yzwx   = float4_swiz_yzwx(xyzw);
		const Ty tmp0   = float4_add(xyzw, yzwx);
		const Ty zwxy   = float4_swiz_zwxy(tmp0);
		const Ty result = float4_add(tmp0, zwxy);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_ceil_ni(Ty _a)
	{
		const Ty tmp0   = float4_ftoi(_a);
		const Ty tmp1   = float4_itof(tmp0);
		const Ty mask   = float4_cmplt(tmp1, _a);
		const Ty one    = float4_splat<Ty>(1.0f);
		const Ty tmp2   = float4_and(one, mask);
		const Ty result = float4_add(tmp1, tmp2);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE Ty float4_floor_ni(Ty _a)
	{
		const Ty tmp0   = float4_ftoi(_a);
		const Ty tmp1   = float4_itof(tmp0);
		const Ty mask   = float4_cmpgt(tmp1, _a);
		const Ty one    = float4_splat<Ty>(1.0f);
		const Ty tmp2   = float4_and(one, mask);
		const Ty result = float4_sub(tmp1, tmp2);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_FORCE_INLINE Ty float4_round_ni(Ty _a)
	{
		const Ty tmp    = float4_ftoi(_a);
		const Ty result = float4_itof(tmp);

		return result;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE bool float4_test_any_ni(Ty _a)
	{
		const Ty mask   = float4_sra(_a, 31);
		const Ty zwxy   = float4_swiz_zwxy(mask);
		const Ty tmp0   = float4_or(mask, zwxy);
		const Ty tmp1   = float4_swiz_yyyy(tmp0);
		const Ty tmp2   = float4_or(tmp0, tmp1);
		int res;
		float4_stx(&res, tmp2);
		return 0 != res;
	}

	template<typename Ty>
	BX_FLOAT4_INLINE bool float4_test_all_ni(Ty _a)
	{
		const Ty bits   = float4_sra(_a, 31);
		const Ty m1248  = float4_ild<Ty>(1, 2, 4, 8);
		const Ty mask   = float4_and(bits, m1248);
		const Ty zwxy   = float4_swiz_zwxy(mask);
		const Ty tmp0   = float4_or(mask, zwxy);
		const Ty tmp1   = float4_swiz_yyyy(tmp0);
		const Ty tmp2   = float4_or(tmp0, tmp1);
		int res;
		float4_stx(&res, tmp2);
		return 0xf == res;
	}

} // namespace bx

#endif // BX_FLOAT4_NI_H_HEADER_GUARD
