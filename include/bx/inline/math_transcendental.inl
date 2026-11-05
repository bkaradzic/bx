/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_MATH_H_HEADER_GUARD
#	error "Must be included from bx/inline/math.inl!"
#endif // BX_MATH_H_HEADER_GUARD

	BX_CONSTEXPR_FUNC float  sin(float _a);
	BX_CONSTEXPR_FUNC float  cos(float _a);
	BX_CONSTEXPR_FUNC float  tan(float _a);
	BX_CONSTEXPR_FUNC float  asin(float _a);
	BX_CONSTEXPR_FUNC float  acos(float _a);
	BX_CONSTEXPR_FUNC float  atan(float _a);
	BX_CONSTEXPR_FUNC float  sinh(float _a);
	BX_CONSTEXPR_FUNC float  cosh(float _a);
	BX_CONSTEXPR_FUNC float  tanh(float _a);
	BX_CONSTEXPR_FUNC float  asinh(float _a);
	BX_CONSTEXPR_FUNC float  acosh(float _a);
	BX_CONSTEXPR_FUNC float  atanh(float _a);
	BX_CONSTEXPR_FUNC float  exp(float _a);
	BX_CONSTEXPR_FUNC float  exp2(float _a);
	BX_CONSTEXPR_FUNC float  log(float _a);
	BX_CONSTEXPR_FUNC float  log2(float _a);
	BX_CONSTEXPR_FUNC float  log10(float _a);
	BX_CONSTEXPR_FUNC float  log1p(float _a);
	BX_CONSTEXPR_FUNC float  atan2(float _y, float _x);
	BX_CONSTEXPR_FUNC float  pow(float _a, float _b);

	BX_CONSTEXPR_FUNC double sin(double _a);
	BX_CONSTEXPR_FUNC double cos(double _a);
	BX_CONSTEXPR_FUNC double tan(double _a);
	BX_CONSTEXPR_FUNC double asin(double _a);
	BX_CONSTEXPR_FUNC double acos(double _a);
	BX_CONSTEXPR_FUNC double atan(double _a);
	BX_CONSTEXPR_FUNC double sinh(double _a);
	BX_CONSTEXPR_FUNC double cosh(double _a);
	BX_CONSTEXPR_FUNC double tanh(double _a);
	BX_CONSTEXPR_FUNC double asinh(double _a);
	BX_CONSTEXPR_FUNC double acosh(double _a);
	BX_CONSTEXPR_FUNC double atanh(double _a);
	BX_CONSTEXPR_FUNC double exp(double _a);
	BX_CONSTEXPR_FUNC double exp2(double _a);
	BX_CONSTEXPR_FUNC double log(double _a);
	BX_CONSTEXPR_FUNC double log2(double _a);
	BX_CONSTEXPR_FUNC double log10(double _a);
	BX_CONSTEXPR_FUNC double log1p(double _a);
	BX_CONSTEXPR_FUNC double atan2(double _y, double _x);
	BX_CONSTEXPR_FUNC double pow(double _a, double _b);

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty exp2T(Ty _a)
	{
		return pow(Ty(2.0), _a);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty expHalfT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		constexpr Bits kBits  = Bits(Traits::kExponentBias + Traits::kExpHalfShift/2) << Traits::kExponentBitShift;
		constexpr Ty   kScale = Traits::fromBits(kBits);

		const Ty reduced = _a - Traits::kExpHalfLn2;
		const Ty ee      = exp(reduced);
		const Ty once    = ee * kScale;
		const Ty result  = once * kScale;

		return result;
	}

	inline BX_CONSTEXPR_FUNC float expHalf(float _a)
	{
		return expHalfT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double expHalf(double _a)
	{
		return expHalfT<double>(_a);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty sinhT(Ty _a)
	{
		using Traits = FloatT<Ty>;

		constexpr Ty kSinhC3 = Ty(1.0)/Ty(6.0);
		constexpr Ty kSinhC5 = Ty(1.0)/Ty(120.0);
		constexpr Ty kSinhC7 = Ty(1.0)/Ty(5040.0);

		const Ty absA = abs(_a);

		Ty magnitude = Ty(0.0);

		if (absA < Traits::kHypSmall)
		{
			const Ty zz   = square(absA);
			const Ty inn0 = mad(zz, kSinhC7, kSinhC5);
			const Ty inn1 = mad(zz, inn0,    kSinhC3);
			const Ty poly = mad(zz, inn1,    Ty(1.0) );

			magnitude = absA * poly;
		}
		else if (absA < Traits::kHypBig)
		{
			const Ty ep   = exp(absA);
			const Ty en   = exp(-absA);
			const Ty diff = ep - en;

			magnitude = Ty(0.5) * diff;
		}
		else if (absA < Traits::kHypNoFit)
		{
			const Ty ep = exp(absA);

			magnitude = Ty(0.5) * ep;
		}
		else
		{
			magnitude = expHalfT<Ty>(absA);
		}

		return signBit(_a) ? -magnitude : magnitude;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty coshT(Ty _a)
	{
		using Traits = FloatT<Ty>;

		const Ty absA = abs(_a);

		if (absA < Traits::kHypBig)
		{
			const Ty ep  = exp(absA);
			const Ty en  = exp(-absA);
			const Ty sum = ep + en;

			return Ty(0.5) * sum;
		}

		if (absA < Traits::kHypNoFit)
		{
			const Ty ep = exp(absA);

			return Ty(0.5) * ep;
		}

		return expHalfT<Ty>(absA);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty tanhT(Ty _a)
	{
		using Traits = FloatT<Ty>;

		if (isNan(_a) )
		{
			return _a;
		}

		const Ty absA = abs(_a);

		Ty magnitude = Ty(1.0);

		if (absA < Traits::kHypSmall)
		{
			const Ty zz = square(absA);

			Ty acc = Traits::kTanhPoly[0];

			for (uint32_t ii = 1, num = uint32_t(BX_COUNTOF(Traits::kTanhPoly) ); ii < num; ++ii)
			{
				acc = mad(zz, acc, Traits::kTanhPoly[ii]);
			}

			const Ty poly = mad(zz, acc, Ty(1.0) );

			magnitude = absA * poly;
		}
		else if (absA < Traits::kTanhLarge)
		{
			const Ty ee  = exp(Ty(2.0)*absA);
			const Ty num = ee - Ty(1.0);
			const Ty den = ee + Ty(1.0);

			magnitude = num / den;
		}

		return signBit(_a) ? -magnitude : magnitude;
	}

	inline BX_CONSTEXPR_FUNC float sinh(float _a)
	{
		return sinhT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double sinh(double _a)
	{
		return sinhT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float cosh(float _a)
	{
		return coshT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double cosh(double _a)
	{
		return coshT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float tanh(float _a)
	{
		return tanhT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double tanh(double _a)
	{
		return tanhT<double>(_a);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty logT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		if (isNan(_a) )
		{
			return _a;
		}

		if (_a < Ty(0.0) )
		{
			constexpr Bits kNanBits = Traits::kSignMask | Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
		}

		if (Ty(0.0) == _a)
		{
			return -Traits::kInfinity;
		}

		if (isInfinite(_a) )
		{
			return _a;
		}

		constexpr Ty kSqrt2Half = Ty(7.07106781186547524401e-01);
		constexpr Ty kLogC0     = Ty(6.666666666666735130e-01);
		constexpr Ty kLogC1     = Ty(3.999999999940941908e-01);
		constexpr Ty kLogC2     = Ty(2.857142874366239149e-01);
		constexpr Ty kLogC3     = Ty(2.222219843214978396e-01);
		constexpr Ty kLogC4     = Ty(1.818357216161805012e-01);
		constexpr Ty kLogC5     = Ty(1.531383769920937332e-01);
		constexpr Ty kLogC6     = Ty(1.479819860511658591e-01);

		int32_t exp = 0;
		Ty      ff  = frexp(_a, &exp);

		if (ff < kSqrt2Half)
		{
			ff += ff;
			--exp;
		}

		ff -= Ty(1.0);

		const Ty kk   = Ty(exp);
		const Ty hi   = kk * Traits::kLn2Hi;
		const Ty lo   = kk * Traits::kLn2Lo;
		const Ty ss   = ff / (Ty(2.0) + ff);
		const Ty s2   = square(ss);
		const Ty s4   = square(s2);

		const Ty tmp0 = mad(kLogC6, s4, kLogC4);
		const Ty tmp1 = mad(tmp0,   s4, kLogC2);
		const Ty tmp2 = mad(tmp1,   s4, kLogC0);
		const Ty t1   = s2*tmp2;

		const Ty tmp3 = mad(kLogC5, s4, kLogC3);
		const Ty tmp4 = mad(tmp3,   s4, kLogC1);
		const Ty t2   = s4*tmp4;

		const Ty t12    = t1 + t2;
		const Ty fsq    = square(ff);
		const Ty hfsq   = Ty(0.5)*fsq;
		const Ty result = hi - ( (hfsq - (ss*(hfsq+t12) + lo) ) - ff);

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty log2T(Ty _a)
	{
		using Traits = FloatT<Ty>;

		const Ty ln     = log(_a);
		const Ty result = ln * Traits::kInvLn2;

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty log10T(Ty _a)
	{
		using Traits = FloatT<Ty>;

		const Ty ln     = log(_a);
		const Ty result = ln * Traits::kInvLn10;

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty log1pT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		if (Ty(0.0) == _a)
		{
			return _a;
		}

		if (Ty(-1.0) > _a)
		{
			constexpr Bits kNanBits = Traits::kSignMask | Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
		}

		if (Ty(-1.0) == _a)
		{
			return -Traits::kInfinity;
		}

		const Ty yy     = Ty(1.0) + _a;
		const Ty zz     = yy - Ty(1.0);
		const Ty ln     = log(yy);
		const Ty err    = (zz - _a) / yy;
		const Ty result = ln - err;

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty expT(Ty _a)
	{
		using Traits = FloatT<Ty>;

		if (isNan(_a) )
		{
			return _a;
		}

		if (_a > Traits::kExpOverflow)
		{
			return Traits::kInfinity;
		}

		if (_a < Traits::kExpUnderflow)
		{
			return Ty(0.0);
		}

		const Ty absA = abs(_a);

		if (absA <= Ty(kNearZero) )
		{
			return _a + Ty(1.0);
		}

		constexpr Ty kExpC0 = Ty( 1.66666666666666019037e-01);
		constexpr Ty kExpC1 = Ty(-2.77777777770155933842e-03);
		constexpr Ty kExpC2 = Ty( 6.61375632143793436117e-05);
		constexpr Ty kExpC3 = Ty(-1.65339022054652515390e-06);
		constexpr Ty kExpC4 = Ty( 4.13813679705723846039e-08);

		const Ty scaled = _a * Traits::kInvLn2;
		const Ty kk     = round(scaled);
		const Ty hi     = _a - kk*Traits::kLn2Hi;
		const Ty lo     =      kk*Traits::kLn2Lo;
		const Ty hml    = hi - lo;
		const Ty hmlsq  = square(hml);
		const Ty tmp0   = mad(kExpC4, hmlsq, kExpC3);
		const Ty tmp1   = mad(tmp0,   hmlsq, kExpC2);
		const Ty tmp2   = mad(tmp1,   hmlsq, kExpC1);
		const Ty tmp3   = mad(tmp2,   hmlsq, kExpC0);
		const Ty tmp4   = hml - hmlsq * tmp3;
		const Ty tmp5   = hml*tmp4/(Ty(2.0) - tmp4);
		const Ty tmp6   = Ty(1.0) - ( (lo - tmp5) - hi);
		const Ty result = ldexpT<Ty>(tmp6, int32_t(kk) );

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty powT(Ty _a, Ty _b)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

		if (Ty(0.0) == _b)
		{
			return Ty(1.0);
		}

		if (isNan(_a)
		||  isNan(_b) )
		{
			return Traits::fromBits(kNanBits);
		}

		const Ty   magnitude = abs(_a);
		const Ty   halfB     = Ty(0.5) * _b;
		const Ty   truncB    = trunc(_b);
		const Ty   truncHalf = trunc(halfB);
		const bool integral  = _b == truncB;
		const bool odd       = halfB != truncHalf;

		if (_a < Ty(0.0)
		&& !integral)
		{
			return Traits::fromBits(kNanBits);
		}

		if (Ty(0.0) == magnitude)
		{
			return _b < Ty(0.0) ? Traits::kInfinity : Ty(0.0);
		}

		constexpr Ty kMaxIntegral = Ty(2147483648.0);

		const Ty absB = abs(_b);

		Ty mag = Ty(0.0);

		if (integral
		&&  absB < kMaxIntegral)
		{
			Ty      acc   = Ty(1.0);
			Ty      base  = magnitude;
			int32_t count = int32_t(absB);

			while (count > 0)
			{
				if (0 != (count & 1) )
				{
					acc = acc * base;
				}

				base    = base * base;
				count >>= 1;
			}

			mag = _b < Ty(0.0) ? Ty(1.0) / acc : acc;
		}
		else
		{
			const Ty ln     = log(magnitude);
			const Ty scaled = _b * ln;

			mag = exp(scaled);
		}

		if (_a >= Ty(0.0) )
		{
			return mag;
		}

		return odd ? -mag : mag;
	}

	inline BX_CONSTEXPR_FUNC float log(float _a)
	{
		return logT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double log(double _a)
	{
		return logT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float log2(float _a)
	{
		return log2T<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double log2(double _a)
	{
		return log2T<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float exp(float _a)
	{
		return expT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double exp(double _a)
	{
		return expT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float pow(float _a, float _b)
	{
		return powT<float>(_a, _b);
	}

	inline BX_CONSTEXPR_FUNC double pow(double _a, double _b)
	{
		return powT<double>(_a, _b);
	}

	inline BX_CONSTEXPR_FUNC float exp2(float _a)
	{
		return exp2T<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double exp2(double _a)
	{
		return exp2T<double>(_a);
	}

	inline constexpr double reducePiHalfLarge(double _absA, int32_t* _outQuadrant)
	{
		constexpr double kPiHalfHi = 1.57079632679489655800e+00;
		constexpr double kPiHalfLo = 6.12323399573676603587e-17;

		int32_t exp = 0;

		const double  frac  = frexp(_absA, &exp);
		const double  mantF = ldexp(frac, 53);
		const uint64_t mant = uint64_t(mantF);
		const int32_t  scale = exp - 53;

		const uint64_t intBit1 = twoOverPiBit(scale - 1);
		const uint64_t intBit0 = twoOverPiBit(scale);
		const uint64_t intMod4 = 2*intBit1 + intBit0;

		const uint64_t v0 = twoOverPiWindow(scale + 1, 0);
		const uint64_t v1 = twoOverPiWindow(scale + 1, 1);
		const uint64_t v2 = twoOverPiWindow(scale + 1, 2);

		const uint64_t h0 = mulHi64(mant, v0);
		const uint64_t l0 = mant * v0;
		const uint64_t h1 = mulHi64(mant, v1);
		const uint64_t l1 = mant * v1;
		const uint64_t h2 = mulHi64(mant, v2);

		const uint64_t s2    = l1 + h2;
		const uint64_t c2    = (s2 < l1) ? 1 : 0;
		const uint64_t t1    = l0 + h1;
		const uint64_t k1    = (t1 < l0) ? 1 : 0;
		const uint64_t s1    = t1 + c2;
		const uint64_t k2    = (s1 < t1) ? 1 : 0;
		const uint64_t carry = h0 + k1 + k2;

		const uint64_t quad = mant*intMod4 + carry;

		constexpr uint64_t kLowMask = UINT64_C(0x7ff);

		const double hiPart = double(s1 >> 11) * 0x1p-53;
		const double loPart = double(s1 & kLowMask) * 0x1p-64 + double(s2 >> 11) * 0x1p-117;

		double fracHi   = hiPart;
		int32_t quadrant = int32_t(quad & 3);

		if (0.5 <= fracHi + loPart)
		{
			fracHi   -= 1.0;
			quadrant += 1;
		}

		const double term0 = fracHi * kPiHalfHi;
		const double term1 = loPart * kPiHalfHi;
		const double term2 = fracHi * kPiHalfLo;
		const double result = term0 + (term1 + term2);

		*_outQuadrant = quadrant & 3;

		return result;
	}

	inline constexpr double reducePiHalf(double _a, int32_t* _outQuadrant)
	{
		constexpr double kInvPiHalf = 6.36619772367581382433e-01;
		constexpr double kPiHalf1   = 1.57079632673412561417e+00;
		constexpr double kPiHalf2   = 6.07710050630396597660e-11;
		constexpr double kPiHalf3   = 2.02226624871116645580e-21;
		constexpr double kPiHalf3t  = 8.47842766036889956997e-32;
		constexpr double kLimit     = 1647099.0;

		const double absA = abs(_a);

		if (kLimit <= absA)
		{
			int32_t quadrant = 0;

			const double reduced = reducePiHalfLarge(absA, &quadrant);

			if (signBit(_a) )
			{
				*_outQuadrant = (-quadrant) & 3;

				return -reduced;
			}

			*_outQuadrant = quadrant;

			return reduced;
		}

		const double value = _a;

		const double scaled   = value * kInvPiHalf;
		const double quadrant = round(scaled);

		const double r0 = value - quadrant*kPiHalf1;

		const double w2 = quadrant*kPiHalf2;
		const double r2 = r0 - w2;
		const double e2 = (r0 - r2) - w2;

		const double w3 = quadrant*kPiHalf3;
		const double r3 = r2 - w3;
		const double e3 = (r2 - r3) - w3;

		const double tail = quadrant*kPiHalf3t;
		const double acc  = e2 + e3;
		const double cc   = tail - acc;

		const double result = r3 - cc;

		*_outQuadrant = int32_t(quadrant) & 3;

		return result;
	}

	inline BX_CONSTEXPR_FUNC double sinPiQuarter(double _a)
	{
		constexpr double kSinC0 = -1.66666666666666324348e-01;
		constexpr double kSinC1 =  8.33333333332248946124e-03;
		constexpr double kSinC2 = -1.98412698298579493134e-04;
		constexpr double kSinC3 =  2.75573137070700676789e-06;
		constexpr double kSinC4 = -2.50507602534068634195e-08;
		constexpr double kSinC5 =  1.58969099521155010221e-10;

		const double zz   = square(_a);
		const double vv   = zz * _a;
		const double zz2  = square(zz);
		const double zz4  = square(zz2);
		const double p01  = mad(kSinC1, zz, kSinC0);
		const double p23  = mad(kSinC3, zz, kSinC2);
		const double p45  = mad(kSinC5, zz, kSinC4);
		const double plo  = mad(p23, zz2, p01);
		const double poly = mad(p45, zz4, plo);

		const double result = mad(vv, poly, _a);

		return result;
	}

	inline BX_CONSTEXPR_FUNC double cosPiQuarter(double _a)
	{
		constexpr double kCosC0 =  4.16666666666666019037e-02;
		constexpr double kCosC1 = -1.38888888888741095749e-03;
		constexpr double kCosC2 =  2.48015872894767294178e-05;
		constexpr double kCosC3 = -2.75573143513906633035e-07;
		constexpr double kCosC4 =  2.08757232129817482790e-09;
		constexpr double kCosC5 = -1.13596475577881948265e-11;

		const double zz   = square(_a);
		const double zz2  = square(zz);
		const double zz4  = square(zz2);
		const double q01  = mad(kCosC1, zz, kCosC0);
		const double q23  = mad(kCosC3, zz, kCosC2);
		const double q45  = mad(kCosC5, zz, kCosC4);
		const double qlo  = mad(q23, zz2, q01);
		const double qq   = mad(q45, zz4, qlo);
		const double zq   = zz * qq;
		const double poly = zz * zq;

		const double halfZz = 0.5 * zz;
		const double ww     = 1.0 - halfZz;
		const double err    = (1.0 - ww) - halfZz;
		const double corr   = err + poly;

		const double result = ww + corr;

		return result;
	}

	inline BX_CONSTEXPR_FUNC double sinQuadrant(double _a, int32_t _quadrant)
	{
		const double   sinA   = sinPiQuarter(_a);
		const double   cosA   = cosPiQuarter(_a);
		const simd64_t sinV   = simd64_splat(sinA);
		const simd64_t cosV   = simd64_splat(cosA);
		const simd64_t mask   = simd64_splat(UINT64_C(0) - uint64_t(_quadrant & 1) );
		const simd64_t sign   = simd64_splat(uint64_t(_quadrant & 2) << 62);
		const simd64_t picked = simd64_selb(mask, cosV, sinV);
		const simd64_t result = simd64_xor(picked, sign);

		return bitCast<double>(result);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty sinT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		if (Ty(0.0) == _a)
		{
			return _a;
		}

		if (isNan(_a)
		||  isInfinite(_a) )
		{
			constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
		}

		int32_t quadrant = 0;

		const double rr     = reducePiHalf(double(_a), &quadrant);
		const double result = sinQuadrant(rr, quadrant);

		return Ty(result);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty cosT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		if (isNan(_a)
		||  isInfinite(_a) )
		{
			constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
		}

		int32_t quadrant = 0;

		const double rr     = reducePiHalf(double(_a), &quadrant);
		const double result = sinQuadrant(rr, quadrant + 1);

		return Ty(result);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty tanT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		if (Ty(0.0) == _a)
		{
			return _a;
		}

		if (isNan(_a)
		||  isInfinite(_a) )
		{
			constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
		}

		int32_t quadrant = 0;

		const double rr     = reducePiHalf(double(_a), &quadrant);
		const double sinR   = sinQuadrant(rr, quadrant);
		const double cosR   = sinQuadrant(rr, quadrant + 1);
		const Ty     sinA   = Ty(sinR);
		const Ty     cosA   = Ty(cosR);
		const Ty     result = sinA / cosA;

		return result;
	}

	inline BX_CONSTEXPR_FUNC float sin(float _a)
	{
		return sinT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double sin(double _a)
	{
		return sinT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float cos(float _a)
	{
		return cosT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double cos(double _a)
	{
		return cosT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float tan(float _a)
	{
		return tanT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double tan(double _a)
	{
		return tanT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC double asinRational(double _t)
	{
		constexpr double kPs0 =  1.66666666666666657415e-01;
		constexpr double kPs1 = -3.25565818622400915405e-01;
		constexpr double kPs2 =  2.01212532134862925881e-01;
		constexpr double kPs3 = -4.00555345006794114027e-02;
		constexpr double kPs4 =  7.91534994289814532176e-04;
		constexpr double kPs5 =  3.47933107596021167570e-05;
		constexpr double kQs1 = -2.40339491173441421878e+00;
		constexpr double kQs2 =  2.02094576023350569471e+00;
		constexpr double kQs3 = -6.88283971605453293030e-01;
		constexpr double kQs4 =  7.70381505559019352791e-02;

		const double p0 = mad(kPs5, _t, kPs4);
		const double p1 = mad(p0,   _t, kPs3);
		const double p2 = mad(p1,   _t, kPs2);
		const double p3 = mad(p2,   _t, kPs1);
		const double p4 = mad(p3,   _t, kPs0);
		const double pp = _t * p4;

		const double q0 = mad(kQs4, _t, kQs3);
		const double q1 = mad(q0,   _t, kQs2);
		const double q2 = mad(q1,   _t, kQs1);
		const double qq = mad(q2,   _t, 1.0);

		const double result = pp / qq;

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty asinT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		constexpr double kPiHalfHi  = 1.57079632679489655800e+00;
		constexpr double kPiHalfLo  = 6.12323399573676603587e-17;
		constexpr double kPiQuartHi = 7.85398163397448278999e-01;
		constexpr double kTiny      = 7.45058059692383e-09;
		constexpr double kNearOne   = 0.975;

		if (isNan(_a) )
		{
			return _a;
		}

		const double aa   = double(_a);
		const double absA = abs(aa);

		if (absA > 1.0)
		{
			if constexpr (kClampInverseTrigDomain)
			{
				constexpr double kPiHalfSum = kPiHalfHi + kPiHalfLo;

				return aa > 0.0 ? Ty(kPiHalfSum) : Ty(-kPiHalfSum);
			}
			else
			{
				constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

				return Traits::fromBits(kNanBits);
			}
		}

		if (1.0 == absA)
		{
			const double hi = aa * kPiHalfHi;
			const double lo = aa * kPiHalfLo;

			return Ty(hi + lo);
		}

		if (absA < 0.5)
		{
			if (absA < kTiny)
			{
				return _a;
			}

			const double tt = square(aa);
			const double ww = asinRational(tt);

			return Ty(mad(aa, ww, aa) );
		}

		const double ww = 1.0 - absA;
		const double tt = 0.5 * ww;
		const double rr = asinRational(tt);
		const double ss = sqrt(tt);

		double result = 0.0;

		if (absA >= kNearOne)
		{
			const double sw   = mad(ss, rr, ss);
			const double tmp0 = 2.0*sw - kPiHalfLo;

			result = kPiHalfHi - tmp0;
		}
		else
		{
			const double df = clearLowWord(ss);
			const double cc = (tt - df*df) / (ss + df);
			const double pp = 2.0*ss*rr - (kPiHalfLo - 2.0*cc);
			const double qq = kPiQuartHi - 2.0*df;

			result = kPiQuartHi - (pp - qq);
		}

		return aa > 0.0 ? Ty(result) : Ty(-result);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty acosT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		constexpr double kPiHalfHi = 1.57079632679489655800e+00;
		constexpr double kPiHalfLo = 6.12323399573676603587e-17;
		constexpr double kPiFull   = 3.14159265358979311600e+00;
		constexpr double kTiny     = 6.93889390390722838e-18;

		if (isNan(_a) )
		{
			return _a;
		}

		const double aa   = double(_a);
		const double absA = abs(aa);

		if (absA > 1.0)
		{
			if constexpr (kClampInverseTrigDomain)
			{
				return aa > 0.0 ? Ty(0.0) : Ty(kPiFull);
			}
			else
			{
				constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

				return Traits::fromBits(kNanBits);
			}
		}

		if (1.0 == absA)
		{
			return aa > 0.0 ? Ty(0.0) : Ty(kPiFull);
		}

		if (absA < 0.5)
		{
			if (absA <= kTiny)
			{
				return Ty(kPiHalfHi + kPiHalfLo);
			}

			const double zz = square(aa);
			const double rr = asinRational(zz);
			const double xr = aa * rr;

			return Ty(kPiHalfHi - (aa - (kPiHalfLo - xr) ) );
		}

		if (aa < 0.0)
		{
			const double zz = 0.5 * (1.0 + aa);
			const double rr = asinRational(zz);
			const double ss = sqrt(zz);
			const double ww = rr*ss - kPiHalfLo;
			const double sw = ss + ww;

			return Ty(kPiFull - 2.0*sw);
		}

		const double zz = 0.5 * (1.0 - aa);
		const double ss = sqrt(zz);
		const double df = clearLowWord(ss);
		const double cc = (zz - df*df) / (ss + df);
		const double rr = asinRational(zz);
		const double ww = mad(rr, ss, cc);
		const double dw = df + ww;

		return Ty(2.0 * dw);
	}

	inline BX_CONSTEXPR_FUNC float asin(float _a)
	{
		return asinT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double asin(double _a)
	{
		return asinT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float acos(float _a)
	{
		return acosT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double acos(double _a)
	{
		return acosT<double>(_a);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty atanT(Ty _a)
	{
		constexpr double kAtanHi[] =
		{
			4.63647609000806093515e-01,
			7.85398163397448278999e-01,
			9.82793723247329054082e-01,
			1.57079632679489655800e+00,
		};
		constexpr double kAtanLo[] =
		{
			2.26987774529616870924e-17,
			3.06161699786838301793e-17,
			1.39033110312309984516e-17,
			6.12323399573676603587e-17,
		};

		constexpr double kAt0  =  3.33333333333329318027e-01;
		constexpr double kAt1  = -1.99999999998764832476e-01;
		constexpr double kAt2  =  1.42857142725034663711e-01;
		constexpr double kAt3  = -1.11111104054623557880e-01;
		constexpr double kAt4  =  9.09088713343650656196e-02;
		constexpr double kAt5  = -7.69187620504482999495e-02;
		constexpr double kAt6  =  6.66107313738753120669e-02;
		constexpr double kAt7  = -5.83357013379057348645e-02;
		constexpr double kAt8  =  4.97687799461593236017e-02;
		constexpr double kAt9  = -3.65315727442169155270e-02;
		constexpr double kAt10 =  1.62858201153657823623e-02;

		constexpr double kHuge = 7.37869762948382e+19;
		constexpr double kTiny = 1.86264514923096e-09;

		if (isNan(_a) )
		{
			return _a;
		}

		const double aa   = double(_a);
		const double absA = abs(aa);

		if (absA > kHuge)
		{
			const double big = kAtanHi[3] + kAtanLo[3];

			return aa > 0.0 ? Ty(big) : Ty(-big);
		}

		int32_t id = -1;
		double  xx = aa;

		if (absA >= 0.4375)
		{
			if (absA < 1.1875)
			{
				if (absA < 0.6875)
				{
					id = 0;
					xx = (2.0*absA - 1.0) / (2.0 + absA);
				}
				else
				{
					id = 1;
					xx = (absA - 1.0) / (absA + 1.0);
				}
			}
			else
			{
				if (absA < 2.4375)
				{
					id = 2;
					xx = (absA - 1.5) / (1.0 + 1.5*absA);
				}
				else
				{
					id = 3;
					xx = -1.0 / absA;
				}
			}
		}
		else if (absA < kTiny)
		{
			return _a;
		}

		const double zz = square(xx);
		const double ww = square(zz);

		const double o0 = mad(kAt10, ww, kAt8);
		const double o1 = mad(o0,    ww, kAt6);
		const double o2 = mad(o1,    ww, kAt4);
		const double o3 = mad(o2,    ww, kAt2);
		const double o4 = mad(o3,    ww, kAt0);
		const double s1 = zz * o4;

		const double e0 = mad(kAt9, ww, kAt7);
		const double e1 = mad(e0,   ww, kAt5);
		const double e2 = mad(e1,   ww, kAt3);
		const double e3 = mad(e2,   ww, kAt1);
		const double s2 = ww * e3;

		const double sum = s1 + s2;

		if (id < 0)
		{
			return Ty(xx - xx*sum);
		}

		const double hi = kAtanHi[id];
		const double lo = kAtanLo[id];

		const double corr   = (xx*sum - lo) - xx;
		const double result = hi - corr;

		return aa < 0.0 ? Ty(-result) : Ty(result);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty atan2T(Ty _y, Ty _x)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		constexpr double kPiFull    = 3.14159265358979311600e+00;
		constexpr double kPiLo      = 1.22464679914735317722e-16;
		constexpr double kHalfPi    = 1.57079632679489655800e+00;
		constexpr double kQuarterPi = 7.85398163397448278999e-01;

		if (isNan(_x)
		||  isNan(_y) )
		{
			constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
		}

		const bool signY = signBit(_y);
		const bool signX = signBit(_x);

		if (Ty(0.0) == _y)
		{
			if (!signX)
			{
				return signY ? Ty(-0.0) : Ty(0.0);
			}

			return signY ? Ty(-kPiFull) : Ty(kPiFull);
		}

		if (Ty(0.0) == _x)
		{
			return signY ? Ty(-kHalfPi) : Ty(kHalfPi);
		}

		if (isInfinite(_x) )
		{
			if (isInfinite(_y) )
			{
				const double quarter = signX ? 3.0*kQuarterPi : kQuarterPi;

				return signY ? Ty(-quarter) : Ty(quarter);
			}

			const double straight = signX ? kPiFull : 0.0;

			return signY ? Ty(-straight) : Ty(straight);
		}

		if (isInfinite(_y) )
		{
			return signY ? Ty(-kHalfPi) : Ty(kHalfPi);
		}

		const double ratio = double(_y) / double(_x);
		const double zz    = atanT<double>(abs(ratio) );

		if (!signX)
		{
			return signY ? Ty(-zz) : Ty(zz);
		}

		const double folded = kPiFull - (zz - kPiLo);

		return signY ? Ty(-folded) : Ty(folded);
	}

	inline BX_CONSTEXPR_FUNC float atan(float _a)
	{
		return atanT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double atan(double _a)
	{
		return atanT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float atan2(float _y, float _x)
	{
		return atan2T<float>(_y, _x);
	}

	inline BX_CONSTEXPR_FUNC double atan2(double _y, double _x)
	{
		return atan2T<double>(_y, _x);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty asinhT(Ty _a)
	{
		using Traits = FloatT<Ty>;

		constexpr Ty kLn2 = Ty(6.93147180559945286227e-01);

		if (isNan(_a)
		||  isInfinite(_a) )
		{
			return _a;
		}

		const Ty absA = abs(_a);

		if (absA < Traits::kArcHypTiny)
		{
			return _a;
		}

		Ty magnitude = Ty(0.0);

		if (absA > Traits::kArcHypHuge)
		{
			const Ty ln = log(absA);

			magnitude = ln + kLn2;
		}
		else if (absA > Ty(2.0) )
		{
			const Ty tt   = mad(absA, absA, Ty(1.0) );
			const Ty root = sqrt(tt);
			const Ty tail = Ty(1.0) / (root + absA);
			const Ty arg  = Ty(2.0)*absA + tail;

			magnitude = log(arg);
		}
		else
		{
			const Ty tt   = square(absA);
			const Ty root = sqrt(Ty(1.0) + tt);
			const Ty tail = tt / (Ty(1.0) + root);

			magnitude = log1p(absA + tail);
		}

		return signBit(_a) ? -magnitude : magnitude;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty acoshT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		constexpr Ty kLn2 = Ty(6.93147180559945286227e-01);

		if (isNan(_a) )
		{
			return _a;
		}

		if (Ty(1.0) > _a)
		{
			constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
		}

		if (Ty(1.0) == _a)
		{
			return Ty(0.0);
		}

		if (isInfinite(_a) )
		{
			return _a;
		}

		if (_a > Traits::kArcHypHuge)
		{
			const Ty ln = log(_a);

			return ln + kLn2;
		}

		if (_a > Ty(2.0) )
		{
			const Ty tt   = square(_a);
			const Ty root = sqrt(tt - Ty(1.0) );
			const Ty tail = Ty(1.0) / (_a + root);
			const Ty arg  = Ty(2.0)*_a - tail;

			return log(arg);
		}

		const Ty tt   = _a - Ty(1.0);
		const Ty root = sqrt(Ty(2.0)*tt + square(tt) );

		return log1p(tt + root);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty atanhT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		if (isNan(_a) )
		{
			return _a;
		}

		const Ty absA = abs(_a);

		if (absA > Ty(1.0) )
		{
			constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
		}

		if (Ty(1.0) == absA)
		{
			return signBit(_a) ? -Traits::kInfinity : Traits::kInfinity;
		}

		if (absA < Traits::kArcHypTiny)
		{
			return _a;
		}

		Ty magnitude = Ty(0.0);

		if (absA < Ty(0.5) )
		{
			const Ty twice = absA + absA;
			const Ty tail  = twice*absA / (Ty(1.0) - absA);
			const Ty half  = log1p(twice + tail);

			magnitude = Ty(0.5) * half;
		}
		else
		{
			const Ty twice = absA + absA;
			const Ty ratio = twice / (Ty(1.0) - absA);

			magnitude = Ty(0.5) * log1p(ratio);
		}

		return signBit(_a) ? -magnitude : magnitude;
	}

	inline BX_CONSTEXPR_FUNC float log10(float _a)
	{
		return log10T<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double log10(double _a)
	{
		return log10T<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float log1p(float _a)
	{
		return log1pT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double log1p(double _a)
	{
		return log1pT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float asinh(float _a)
	{
		return asinhT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double asinh(double _a)
	{
		return asinhT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float acosh(float _a)
	{
		return acoshT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double acosh(double _a)
	{
		return acoshT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float atanh(float _a)
	{
		return atanhT<float>(_a);
	}
	inline BX_CONSTEXPR_FUNC double atanh(double _a)
	{
		return atanhT<double>(_a);
	}

