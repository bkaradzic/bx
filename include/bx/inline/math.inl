/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

// FPU math lib

#ifndef BX_MATH_H_HEADER_GUARD
#	error "Must be included from bx/math.h!"
#endif // BX_MATH_H_HEADER_GUARD

#include <bx/simd_t.h>

#if BX_COMPILER_MSVC
extern "C" unsigned char _BitScanReverse(unsigned long* _Index, unsigned long _Mask);
#	pragma intrinsic(_BitScanReverse)

extern "C" unsigned char _BitScanForward(unsigned long* _Index, unsigned long _Mask);
#	pragma intrinsic(_BitScanForward)

#	if BX_ARCH_64BIT
extern "C" unsigned char _BitScanReverse64(unsigned long* _Index, unsigned __int64 _Mask);
#		pragma intrinsic(_BitScanReverse64)

extern "C" unsigned char _BitScanForward64(unsigned long* _Index, unsigned __int64 _Mask);
#		pragma intrinsic(_BitScanForward64)
#	endif // BX_ARCH_64BIT
#endif // BX_COMPILER_MSVC

namespace bx
{
	inline BX_CONSTEXPR_FUNC float toRad(float _deg)
	{
		return _deg * kPi / 180.0f;
	}

	inline BX_CONSTEXPR_FUNC float toDeg(float _rad)
	{
		return _rad * 180.0f / kPi;
	}

	inline BX_CONSTEXPR_FUNC uint32_t floatToBits(float _a)
	{
		return bitCast<uint32_t>(_a);
	}

	inline BX_CONSTEXPR_FUNC float bitsToFloat(uint32_t _a)
	{
		return bitCast<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC uint64_t doubleToBits(double _a)
	{
		return bitCast<uint64_t>(_a);
	}

	inline BX_CONSTEXPR_FUNC double bitsToDouble(uint64_t _a)
	{
		return bitCast<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC uint32_t floatFlip(uint32_t _value)
	{
		// Reference(s):
		// - http://archive.fo/2012.12.08-212402/http://stereopsis.com/radix.html
		//
		const simd32_t signMask = simd32_splat(kFloatSignMask);
		const simd32_t value    = simd32_splat(_value);
		const simd32_t tmp0     = simd32_x32_sra(value, 31);
		const simd32_t mask     = simd32_or(tmp0, signMask);
		const simd32_t result   = simd32_xor(value, mask);

		return result.u32;
	}

	inline BX_CONSTEXPR_FUNC bool isNan(float _f)
	{
		const uint32_t tmp = floatToBits(_f) & INT32_MAX;
		return tmp > kFloatExponentMask;
	}

	inline BX_CONSTEXPR_FUNC bool isNan(double _f)
	{
		const uint64_t tmp = doubleToBits(_f) & INT64_MAX;
		return tmp > kDoubleExponentMask;
	}

	inline BX_CONSTEXPR_FUNC bool isFinite(float _f)
	{
		const uint32_t tmp = floatToBits(_f) & INT32_MAX;
		return tmp < kFloatExponentMask;
	}

	inline BX_CONSTEXPR_FUNC bool isFinite(double _f)
	{
		const uint64_t tmp = doubleToBits(_f) & INT64_MAX;
		return tmp < kDoubleExponentMask;
	}

	inline BX_CONSTEXPR_FUNC bool isInfinite(float _f)
	{
		const uint32_t tmp = floatToBits(_f) & INT32_MAX;
		return tmp == kFloatExponentMask;
	}

	inline BX_CONSTEXPR_FUNC bool isInfinite(double _f)
	{
		const uint64_t tmp = doubleToBits(_f) & INT64_MAX;
		return tmp == kDoubleExponentMask;
	}

	/// True for an argument whose magnitude has no fractional part left.
	template<typename Ty>
	inline BX_CONSTEXPR_FUNC bool isIntegralMagnitude(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		constexpr Bits kIntegralExp = Bits(Traits::kExponentBias + Traits::kMantissaNumBits);
		constexpr Bits kIntegral    = kIntegralExp << Traits::kExponentBitShift;

		const Bits bits      = Traits::toBits(_a);
		const Bits magnitude = bits & ~Traits::kSignMask;

		return kIntegral <= magnitude;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty truncT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;
		using Int    = typename Traits::Int;

		if (isIntegralMagnitude(_a) )
		{
			return _a;
		}

		const Bits sign     = Traits::toBits(_a) & Traits::kSignMask;
		const Int  integral = Int(_a);
		const Ty   tr       = Ty(integral);
		const Bits trBits   = Traits::toBits(tr) | sign;
		const Ty   result   = Traits::fromBits(trBits);

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty floorT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;
		using Int    = typename Traits::Int;

		if (isIntegralMagnitude(_a) )
		{
			return _a;
		}

		const Bits sign     = Traits::toBits(_a) & Traits::kSignMask;
		const Int  integral = Int(_a);
		const Ty   tr       = Ty(integral);
		const Ty   fl       = tr > _a ? tr - Ty(1.0) : tr;
		const Bits flBits   = Traits::toBits(fl) | sign;
		const Ty   result   = Traits::fromBits(flBits);

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty ceilT(Ty _a)
	{
		const Ty na     = -_a;
		const Ty fl     = floorT<Ty>(na);
		const Ty result = -fl;

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty roundT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		if (isIntegralMagnitude(_a) )
		{
			return _a;
		}

		constexpr Ty kMagic = Ty(Bits(1) << Traits::kMantissaNumBits);

		const Bits bits   = Traits::toBits(_a);
		const Bits sign   = bits & Traits::kSignMask;
		const Ty   absA   = Traits::fromBits(bits & ~Traits::kSignMask);
		const Ty   raised = absA + kMagic;
		const Ty   rd     = raised - kMagic;
		const Bits rdBits = Traits::toBits(rd) | sign;
		const Ty   result = Traits::fromBits(rdBits);

		return result;
	}

	inline BX_CONSTEXPR_FUNC float truncRef(float _a)
	{
		return truncT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double truncRef(double _a)
	{
		return truncT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float floorRef(float _a)
	{
		return floorT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double floorRef(double _a)
	{
		return floorT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float ceilRef(float _a)
	{
		return ceilT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double ceilRef(double _a)
	{
		return ceilT<double>(_a);
	}

#if BX_SIMD_SUPPORTED
	inline BX_CONST_FUNC float truncSimd(float _a)
	{
		const simd128_t aa     = simd_splat<simd128_t>(_a);
		const simd128_t result = simd_f32_trunc<simd128_t>(aa);

		float out = 0.0f;
		simd_x32_st1<simd128_t>(&out, result);

		return out;
	}

	inline BX_CONST_FUNC float floorSimd(float _a)
	{
		const simd128_t aa     = simd_splat<simd128_t>(_a);
		const simd128_t result = simd_f32_floor<simd128_t>(aa);

		float out = 0.0f;
		simd_x32_st1<simd128_t>(&out, result);

		return out;
	}

	inline BX_CONST_FUNC double floorSimd(double _a)
	{
		const simd128_t aa     = simd_splat<simd128_t>(_a);
		const simd128_t result = simd_f64_floor<simd128_t>(aa);

		alignas(16) double out[2] = { 0.0, 0.0 };
		simd_st<simd128_t>(out, result);

		return out[0];
	}

	inline BX_CONST_FUNC float ceilSimd(float _a)
	{
		const simd128_t aa     = simd_splat<simd128_t>(_a);
		const simd128_t result = simd_f32_ceil<simd128_t>(aa);

		float out = 0.0f;
		simd_x32_st1<simd128_t>(&out, result);

		return out;
	}

	inline BX_CONST_FUNC double ceilSimd(double _a)
	{
		const simd128_t aa     = simd_splat<simd128_t>(_a);
		const simd128_t result = simd_f64_ceil<simd128_t>(aa);

		alignas(16) double out[2] = { 0.0, 0.0 };
		simd_st<simd128_t>(out, result);

		return out[0];
	}
#endif // BX_SIMD_SUPPORTED

	inline BX_CONSTEXPR_FUNC float roundRef(float _a)
	{
		return roundT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double roundRef(double _a)
	{
		return roundT<double>(_a);
	}

#if BX_SIMD_SUPPORTED
	inline BX_CONST_FUNC float roundSimd(float _a)
	{
		const simd128_t aa     = simd_splat<simd128_t>(_a);
		const simd128_t result = simd_f32_round<simd128_t>(aa);

		float out = 0.0f;
		simd_x32_st1<simd128_t>(&out, result);

		return out;
	}

	inline BX_CONST_FUNC double roundSimd(double _a)
	{
		const simd128_t aa     = simd_splat<simd128_t>(_a);
		const simd128_t result = simd_f64_round<simd128_t>(aa);

		alignas(16) double out[2] = { 0.0, 0.0 };
		simd_st<simd128_t>(out, result);

		return out[0];
	}
#endif // BX_SIMD_SUPPORTED

	inline BX_CONSTEXPR_FUNC float lerp(float _a, float _b, float _t)
	{
		// Reference(s):
		// - Linear interpolation past, present and future
		//   https://web.archive.org/web/20200404165201/https://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/
		//
		return mad(_t, _b, nms(_t, _a, _a) );
	}

	inline BX_CONSTEXPR_FUNC float invLerp(float _a, float _b, float _value)
	{
		return (_value - _a) / (_b - _a);
	}

	inline BX_CONSTEXPR_FUNC bool signBit(float _a)
	{
		const uint32_t bits = floatToBits(_a);
		return 0 != (bits & kFloatSignMask);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty fractT(Ty _a)
	{
		const Ty tr     = trunc(_a);
		const Ty result = _a - tr;

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty fmodT(Ty _a, Ty _b)
	{
		const Ty quotient = _a / _b;
		const Ty whole    = trunc(quotient);
		const Ty result   = nms(_b, whole, _a);

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty modT(Ty _a, Ty _b)
	{
		const Ty quotient = _a / _b;
		const Ty whole    = floor(quotient);
		const Ty result   = nms(_b, whole, _a);

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty exp2T(Ty _a)
	{
		return pow(Ty(2.0), _a);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty tanT(Ty _a)
	{
		const Ty tmp0   = sin(_a);
		const Ty tmp1   = cos(_a);
		const Ty result = tmp0 / tmp1;

		return result;
	}

	inline BX_CONSTEXPR_FUNC float add(float _a, float _b)
	{
		return _a + _b;
	}

	inline BX_CONSTEXPR_FUNC float sub(float _a, float _b)
	{
		return _a - _b;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty satAdd(Ty _a, Ty _b)
	{
		static_assert(isInteger<Ty>(), "Type Ty must be an integer type.");

		using UTy = MakeUnsignedType<Ty>;

		const UTy ua  = UTy(_a);
		const UTy ub  = UTy(_b);
		const UTy sum = UTy(ua + ub);

		if constexpr (isSigned<Ty>() )
		{
			const UTy signBit  = UTy(UTy(1) << (sizeof(Ty)*8 - 1) );
			const UTy overflow = UTy(~(ua ^ ub) & (ua ^ sum) & signBit);
			const Ty  satVal   = (ua & signBit) ? LimitsT<Ty>::min : LimitsT<Ty>::max;
			return 0 != overflow ? satVal : Ty(sum);
		}

		return sum < ua ? LimitsT<Ty>::max : Ty(sum);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty satSub(Ty _a, Ty _b)
	{
		static_assert(isInteger<Ty>(), "Type Ty must be an integer type.");

		using UTy = MakeUnsignedType<Ty>;

		const UTy ua   = UTy(_a);
		const UTy ub   = UTy(_b);
		const UTy diff = UTy(ua - ub);

		if constexpr (isSigned<Ty>() )
		{
			const UTy signBit  = UTy(UTy(1) << (sizeof(Ty)*8 - 1) );
			const UTy overflow = UTy( (ua ^ ub) & (ua ^ diff) & signBit);
			const Ty  satVal   = (ua & signBit) ? LimitsT<Ty>::min : LimitsT<Ty>::max;

			return 0 != overflow ? satVal : Ty(diff);
		}

		return ua > ub ? Ty(diff) : Ty(0);
	}

	inline BX_CONSTEXPR_FUNC float mul(float _a, float _b)
	{
		return _a * _b;
	}

	inline BX_CONSTEXPR_FUNC float rcp(float _a)
	{
		return 1.0f / _a;
	}

	inline BX_CONSTEXPR_FUNC float rcpSafe(float _a)
	{
		return rcp(copySign(max(kFloatSmallest, abs(_a) ), _a) );
	}

	inline BX_CONSTEXPR_FUNC float div(float _a, float _b)
	{
		return mul(_a, rcp(_b) );
	}

	inline BX_CONSTEXPR_FUNC float divSafe(float _a, float _b)
	{
		return mul(_a, rcpSafe(_b) );
	}

BX_FP_PRECISE_BEGIN()

	inline void sinCosApprox(float& _outSinApprox, float& _outCos, float _a)
	{
		const float aa     = _a - floor(_a*kInvPi2)*kPi2;
		const float absA   = abs(aa);
		const float cosA   = cos(absA);
		const float cosASq = square(cosA);
		const float tmp0   = max(0.0f, 1.0f - cosASq);
		const float tmp1   = sqrt(tmp0);
		const float tmp2   = aa > 0.0f && aa < kPi ? 1.0f : -1.0f;
		const float sinA   = mul(tmp1, tmp2);

		_outSinApprox = sinA;
		_outCos = cosA;
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
	inline BX_CONSTEXPR_FUNC Ty ldexpT(Ty _a, int32_t _b)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		constexpr int32_t kMinStep = 1 - Traits::kExponentBias;
		constexpr int32_t kMaxStep =     Traits::kExponentBias;
		constexpr int32_t kLimit   = 2*Traits::kExponentBias + Traits::kMantissaNumBits + 2;

		const int32_t total = clamp(_b, -kLimit, kLimit);

		if (Ty(0.0) == _a
		||  isNan(_a)
		||  isInfinite(_a) )
		{
			return _a;
		}

		int32_t expA = 0;
		frexp(_a, &expA);

		if (expA + total > Traits::kExponentBias + 1)
		{
			return signBit(_a) ? -Traits::kInfinity : Traits::kInfinity;
		}

		const int32_t step0 = clamp(total, kMinStep, kMaxStep);
		const int32_t rest0 = total - step0;
		const int32_t step1 = clamp(rest0, kMinStep, kMaxStep);
		const int32_t rest1 = rest0 - step1;
		const int32_t step2 = clamp(rest1, kMinStep, kMaxStep);

		const Bits exp0 = Bits(step0 + Traits::kExponentBias);
		const Bits exp1 = Bits(step1 + Traits::kExponentBias);
		const Bits exp2 = Bits(step2 + Traits::kExponentBias);

		const Bits bits0 = exp0 << Traits::kExponentBitShift;
		const Bits bits1 = exp1 << Traits::kExponentBitShift;
		const Bits bits2 = exp2 << Traits::kExponentBitShift;

		const Ty scale0 = Traits::fromBits(bits0);
		const Ty scale1 = Traits::fromBits(bits1);
		const Ty scale2 = Traits::fromBits(bits2);

		const Ty tmp0   = _a   * scale0;
		const Ty tmp1   = tmp0 * scale1;
		const Ty result = tmp1 * scale2;

		return result;
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

	inline BX_CONSTEXPR_FUNC float mad(float _a, float _b, float _c)
	{
		const simd32_t aa     = simd32_ld(_a);
		const simd32_t bb     = simd32_ld(_b);
		const simd32_t cc     = simd32_ld(_c);
		const simd32_t result = simd32_f32_madd(aa, bb, cc);

		return bitCast<float>(result);
	}

	inline BX_CONSTEXPR_FUNC double mad(double _a, double _b, double _c)
	{
		return _a*_b + _c;
	}

	inline BX_CONSTEXPR_FUNC float nms(float _a, float _b, float _c)
	{
		const float na     = -_a;
		const float result = mad(na, _b, _c);

		return result;
	}

	inline BX_CONSTEXPR_FUNC double nms(double _a, double _b, double _c)
	{
		const double na     = -_a;
		const double result = mad(na, _b, _c);

		return result;
	}

	inline BX_CONSTEXPR_FUNC float abs(float _a)
	{
		const uint32_t bits      = floatToBits(_a);
		const uint32_t magnitude = bits & ~kFloatSignMask;
		const float    result    = bitsToFloat(magnitude);

		return result;
	}

	inline BX_CONSTEXPR_FUNC double abs(double _a)
	{
		const uint64_t bits      = doubleToBits(_a);
		const uint64_t magnitude = bits & ~kDoubleSignMask;
		const double   result    = bitsToDouble(magnitude);

		return result;
	}

	template<typename Ty>
	requires (isInteger<Ty>() )
	inline constexpr Ty abs(Ty _a)
	{
		if constexpr (isSigned<Ty>() )
		{
			const Ty negated = Ty(-_a);
			const Ty result  = _a < Ty(0) ? negated : _a;

			return result;
		}
		else
		{
			return _a;
		}
	}

	inline BX_CONSTEXPR_FUNC float square(float _a)
	{
		return _a * _a;
	}

	inline BX_CONSTEXPR_FUNC double square(double _a)
	{
		return _a * _a;
	}

	inline BX_CONSTEXPR_FUNC float sign(float _a)
	{
		return float( (0.0f < _a) - (0.0f > _a) );
	}

	inline BX_CONSTEXPR_FUNC double sign(double _a)
	{
		const int32_t positive = 0.0 < _a;
		const int32_t negative = 0.0 > _a;
		const int32_t signum   = positive - negative;
		const double  result   = double(signum);

		return result;
	}

	inline BX_CONSTEXPR_FUNC bool signBit(double _a)
	{
		const uint64_t bits = doubleToBits(_a);

		return 0 != (bits & kDoubleSignMask);
	}

	inline BX_CONSTEXPR_FUNC float copySign(float _value, float _sign)
	{
#if BX_COMPILER_MSVC
		const uint32_t magnitude = floatToBits(_value) & ~kFloatSignMask;
		const uint32_t sign      = floatToBits(_sign)  &  kFloatSignMask;
		const uint32_t bits      = magnitude | sign;
		const float    result    = bitsToFloat(bits);
		return result;
#else
		return __builtin_copysign(_value, _sign);
#endif // BX_COMPILER_MSVC
	}

	inline BX_CONSTEXPR_FUNC double copySign(double _value, double _sign)
	{
		const uint64_t magnitude = doubleToBits(_value) & ~kDoubleSignMask;
		const uint64_t sign      = doubleToBits(_sign)  &  kDoubleSignMask;
		const uint64_t bits      = magnitude | sign;
		const double   result    = bitsToDouble(bits);

		return result;
	}

	inline BX_CONSTEXPR_FUNC float trunc(float _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return truncRef(_a);
		}

		return truncSimd(_a);
#else
		return truncRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC double trunc(double _a)
	{
		return truncRef(_a);
	}

	inline BX_CONSTEXPR_FUNC float floor(float _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return floorRef(_a);
		}

		return floorSimd(_a);
#else
		return floorRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC double floor(double _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return floorRef(_a);
		}

		return floorSimd(_a);
#else
		return floorRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC float ceil(float _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return ceilRef(_a);
		}

		return ceilSimd(_a);
#else
		return ceilRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC double ceil(double _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return ceilRef(_a);
		}

		return ceilSimd(_a);
#else
		return ceilRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC float round(float _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return roundRef(_a);
		}

		return roundSimd(_a);
#else
		return roundRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC double round(double _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return roundRef(_a);
		}

		return roundSimd(_a);
#else
		return roundRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC float fract(float _a)
	{
		return fractT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double fract(double _a)
	{
		return fractT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC bool isEqual(double _a, double _b, double _epsilon)
	{
		const double diff  = _a - _b;
		const double lhs   = abs(diff);
		const double absA  = abs(_a);
		const double absB  = abs(_b);
		const double scale = max(1.0, absA, absB);
		const double rhs   = _epsilon * scale;

		return lhs <= rhs;
	}

	inline BX_CONSTEXPR_FUNC float ldexp(float _a, int32_t _b)
	{
		return ldexpT<float>(_a, _b);
	}

	inline BX_CONSTEXPR_FUNC double ldexp(double _a, int32_t _b)
	{
		return ldexpT<double>(_a, _b);
	}

	inline constexpr float frexp(float _a, int32_t* _outExp)
	{
		const uint32_t bits      = floatToBits(_a);
		const uint32_t magnitude = bits & ~kFloatSignMask;

		if (0 == magnitude
		||  kFloatExponentMask <= magnitude)
		{
			*_outExp = 0;

			return _a;
		}

		constexpr int32_t  kSubnormalShift = int32_t(kFloatMantissaNumBits) + 2;
		constexpr uint32_t kSubnormalExp   = uint32_t(int32_t(kFloatExponentBias) + kSubnormalShift);
		constexpr uint32_t kSubnormalBits  = kSubnormalExp << kFloatExponentBitShift;
		constexpr uint32_t kNormalSmallest = uint32_t(1) << kFloatExponentBitShift;
		constexpr uint32_t kHalfExp        = kFloatExponentBias - 1;
		constexpr uint32_t kHalfBits       = kHalfExp << kFloatExponentBitShift;

		const bool    subnormal = magnitude < kNormalSmallest;
		const float   scale     = bitsToFloat(kSubnormalBits);
		const float   scaled    = _a * scale;
		const float   value     = subnormal ? scaled : _a;
		const int32_t bias      = subnormal ? -kSubnormalShift : 0;

		const uint32_t valueBits = floatToBits(value);
		const uint32_t expBits   = valueBits & kFloatExponentMask;
		const int32_t  raw       = int32_t(expBits >> kFloatExponentBitShift);
		const int32_t  exp       = raw - int32_t(kFloatExponentBias) + 1;
		const uint32_t kept      = valueBits & (kFloatSignMask | kFloatMantissaMask);
		const uint32_t resBits   = kept | kHalfBits;
		const float    result    = bitsToFloat(resBits);

		*_outExp = exp + bias;

		return result;
	}

	inline constexpr double frexp(double _a, int32_t* _outExp)
	{
		const uint64_t bits      = doubleToBits(_a);
		const uint64_t magnitude = bits & ~kDoubleSignMask;

		if (0 == magnitude
		||  kDoubleExponentMask <= magnitude)
		{
			*_outExp = 0;

			return _a;
		}

		constexpr int32_t  kSubnormalShift = int32_t(kDoubleMantissaNumBits) + 2;
		constexpr uint64_t kSubnormalExp   = uint64_t(int32_t(kDoubleExponentBias) + kSubnormalShift);
		constexpr uint64_t kSubnormalBits  = kSubnormalExp << kDoubleExponentShift;
		constexpr uint64_t kNormalSmallest = uint64_t(1) << kDoubleExponentShift;
		constexpr uint64_t kHalfExp        = kDoubleExponentBias - 1;
		constexpr uint64_t kHalfBits       = kHalfExp << kDoubleExponentShift;

		const bool    subnormal = magnitude < kNormalSmallest;
		const double  scale     = bitsToDouble(kSubnormalBits);
		const double  scaled    = _a * scale;
		const double  value     = subnormal ? scaled : _a;
		const int32_t bias      = subnormal ? -kSubnormalShift : 0;

		const uint64_t valueBits = doubleToBits(value);
		const uint64_t expBits   = valueBits & kDoubleExponentMask;
		const int32_t  raw       = int32_t(expBits >> kDoubleExponentShift);
		const int32_t  exp       = raw - int32_t(kDoubleExponentBias) + 1;
		const uint64_t kept      = valueBits & (kDoubleSignMask | kDoubleMantissaMask);
		const uint64_t resBits   = kept | kHalfBits;
		const double   result    = bitsToDouble(resBits);

		*_outExp = exp + bias;

		return result;
	}

	inline constexpr float modf(float _a, float* _outIntegral)
	{
		if (isInfinite(_a) )
		{
			const uint32_t sign = floatToBits(_a) & kFloatSignMask;

			*_outIntegral = _a;

			return bitsToFloat(sign);
		}

		const float integral = trunc(_a);
		const float fraction = _a - integral;

		*_outIntegral = integral;

		return fraction;
	}

	inline constexpr double modf(double _a, double* _outIntegral)
	{
		if (isInfinite(_a) )
		{
			const uint64_t sign = doubleToBits(_a) & kDoubleSignMask;

			*_outIntegral = _a;

			return bitsToDouble(sign);
		}

		const double integral = trunc(_a);
		const double fraction = _a - integral;

		*_outIntegral = integral;

		return fraction;
	}

	inline BX_CONSTEXPR_FUNC float fmod(float _a, float _b)
	{
		return fmodT<float>(_a, _b);
	}

	inline BX_CONSTEXPR_FUNC double fmod(double _a, double _b)
	{
		return fmodT<double>(_a, _b);
	}

	inline BX_CONSTEXPR_FUNC float mod(float _a, float _b)
	{
		return modT<float>(_a, _b);
	}

	inline BX_CONSTEXPR_FUNC double mod(double _a, double _b)
	{
		return modT<double>(_a, _b);
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

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty sqrtT(Ty _a)
	{
		using Traits = FloatT<Ty>;
		using Bits   = typename Traits::Bits;

		if (isNan(_a) )
		{
			return _a;
		}

		if (_a < Ty(0.0) )
		{
			constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
		}

		if (Ty(0.0) == _a)
		{
			return _a;
		}

		if (isInfinite(_a) )
		{
			return _a;
		}

		int32_t  exp  = 0;
		const Ty frac = frexp(_a, &exp);

		const bool    odd  = 0 != (exp & 1);
		const Ty      mant = odd ? frac + frac : frac;
		const int32_t rest = odd ? (exp - 1) >> 1 : exp >> 1;

		const Bits mantBits = Traits::toBits(mant);
		const Bits seedBits = Traits::kRsqrtSeed - (mantBits >> 1);

		Ty rsq = Traits::fromBits(seedBits);

		for (uint32_t ii = 0; ii < 5; ++ii)
		{
			const Ty rsqSq  = square(rsq);
			const Ty scaled = mant * rsqSq;
			const Ty halved = Ty(0.5) * scaled;
			const Ty corr   = Ty(1.5) - halved;

			rsq = rsq * corr;
		}

		const Ty approx = mant * rsq;
		const Ty quot   = mant / approx;
		const Ty sum    = approx + quot;
		const Ty root   = Ty(0.5) * sum;
		const Ty result = ldexpT<Ty>(root, rest);

		return result;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty rsqrtT(Ty _a)
	{
		using Traits = FloatT<Ty>;

		if (Ty(0.0) == _a)
		{
			return Traits::kInfinity;
		}

		const Ty root   = sqrt(_a);
		const Ty result = Ty(1.0) / root;

		return result;
	}

	inline BX_CONSTEXPR_FUNC uint64_t mulHi64(uint64_t _a, uint64_t _b)
	{
		constexpr uint64_t kLoMask = UINT64_C(0xffffffff);

		const uint64_t aLo = _a & kLoMask;
		const uint64_t aHi = _a >> 32;
		const uint64_t bLo = _b & kLoMask;
		const uint64_t bHi = _b >> 32;

		const uint64_t ll = aLo * bLo;
		const uint64_t lh = aLo * bHi;
		const uint64_t hl = aHi * bLo;
		const uint64_t hh = aHi * bHi;

		const uint64_t mid    = (ll >> 32) + (lh & kLoMask) + (hl & kLoMask);
		const uint64_t result = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);

		return result;
	}

	inline BX_CONSTEXPR_FUNC uint64_t twoOverPiWord(int32_t _index)
	{
		constexpr uint64_t kTwoOverPi[] =
		{
			UINT64_C(0xa2f9836e4e441529),
			UINT64_C(0xfc2757d1f534ddc0),
			UINT64_C(0xdb6295993c439041),
			UINT64_C(0xfe5163abdebbc561),
			UINT64_C(0xb7246e3a424dd2e0),
			UINT64_C(0x06492eea09d1921c),
			UINT64_C(0xfe1deb1cb129a73e),
			UINT64_C(0xe88235f52ebb4484),
			UINT64_C(0xe99c7026b45f7e41),
			UINT64_C(0x3991d639835339f4),
			UINT64_C(0x9c845f8bbdf9283b),
			UINT64_C(0x1ff897ffde05980f),
			UINT64_C(0xef2f118b5a0a6d1f),
			UINT64_C(0x6d367ecf27cb09b7),
			UINT64_C(0x4f463f669e5fea2d),
			UINT64_C(0x7527bac7ebe5f17b),
			UINT64_C(0x3d0739f78a5292ea),
			UINT64_C(0x6bfb5fb11f8d5d08),
			UINT64_C(0x56033046fc7b6bab),
			UINT64_C(0xf0cfbc209af4361d),
			UINT64_C(0xa9e391615ee61b08),
			UINT64_C(0x6599855f14a06840),
			UINT64_C(0x8dffd8804d732731),
			UINT64_C(0x06061556ca73a8c9),
		};
		constexpr int32_t kNumWords = 24;

		if (0 > _index
		||  kNumWords <= _index)
		{
			return 0;
		}

		return kTwoOverPi[uint32_t(_index)];
	}

	inline BX_CONSTEXPR_FUNC uint64_t twoOverPiBit(int32_t _bit)
	{
		if (1 > _bit)
		{
			return 0;
		}

		const int32_t  index = (_bit - 1) / 64;
		const int32_t  shift = 63 - ( (_bit - 1) % 64);
		const uint64_t word  = twoOverPiWord(index);

		return (word >> shift) & 1;
	}

	inline BX_CONSTEXPR_FUNC uint64_t twoOverPiWindow(int32_t _bit, int32_t _which)
	{
		const int32_t start = _bit + 64*_which;
		const int32_t index = (0 < start) ? (start - 1) / 64 : -( (1 - start) / 64 + 1);
		const int32_t shift = start - 1 - 64*index;

		const uint64_t lo = twoOverPiWord(index);
		const uint64_t hi = twoOverPiWord(index + 1);

		if (0 == shift)
		{
			return lo;
		}

		return (lo << shift) | (hi >> (64 - shift) );
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

		const double rr = reducePiHalf(double(_a), &quadrant);

		switch (quadrant)
		{
			case  0: return Ty( sinPiQuarter(rr) );
			case  1: return Ty( cosPiQuarter(rr) );
			case  2: return Ty(-sinPiQuarter(rr) );
			default: return Ty(-cosPiQuarter(rr) );
		}
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

		const double rr = reducePiHalf(double(_a), &quadrant);

		switch (quadrant)
		{
			case  0: return Ty( cosPiQuarter(rr) );
			case  1: return Ty(-sinPiQuarter(rr) );
			case  2: return Ty(-cosPiQuarter(rr) );
			default: return Ty( sinPiQuarter(rr) );
		}
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

	inline BX_CONSTEXPR_FUNC double clearLowWord(double _a)
	{
		constexpr uint64_t kHighMask = UINT64_C(0xffffffff00000000);

		const uint64_t bits = doubleToBits(_a);
		const uint64_t high = bits & kHighMask;

		return bitsToDouble(high);
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
			constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
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
			constexpr Bits kNanBits = Traits::kExponentMask | Traits::kMantissaMask;

			return Traits::fromBits(kNanBits);
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

BX_FP_PRECISE_END()

	template<>
	inline BX_CONSTEXPR_FUNC uint8_t countBits(uint32_t _val)
	{
#if BX_COMPILER_GCC || BX_COMPILER_CLANG
		return __builtin_popcount(_val);
#else
		const uint32_t tmp0   = (_val >> 1);
		const uint32_t tmp1   = (tmp0 & 0x55555555);
		const uint32_t tmp2   = (_val - tmp1);
		const uint32_t tmp3   = (tmp2 & 0xc30c30c3);
		const uint32_t tmp4   = (tmp2 >> 2);
		const uint32_t tmp5   = (tmp4 & 0xc30c30c3);
		const uint32_t tmp6   = (tmp2 >> 4);
		const uint32_t tmp7   = (tmp6 & 0xc30c30c3);
		const uint32_t tmp8   = (tmp3 + tmp5);
		const uint32_t tmp9   = (tmp7 + tmp8);
		const uint32_t tmpA   = (tmp9 >> 6);
		const uint32_t tmpB   = (tmp9 + tmpA);
		const uint32_t tmpC   = (tmpB >> 12);
		const uint32_t tmpD   = (tmpB >> 24);
		const uint32_t tmpE   = (tmpB + tmpC);
		const uint32_t tmpF   = (tmpD + tmpE);
		const uint32_t result = (tmpF & 0x3f);

		return uint8_t(result);
#endif // BX_COMPILER_*
	}

	template<>
	inline BX_CONSTEXPR_FUNC uint8_t countBits(unsigned long long _val)
	{
#if BX_COMPILER_GCC || BX_COMPILER_CLANG
		return __builtin_popcountll(_val);
#else
		const uint32_t lo = uint32_t(_val&UINT32_MAX);
		const uint32_t hi = uint32_t(_val>>32);

		return countBits<uint32_t>(lo)
			+  countBits<uint32_t>(hi)
			;
#endif // BX_COMPILER_*
	}

	template<>
	inline BX_CONSTEXPR_FUNC uint8_t countBits(unsigned long _val)
	{
		return countBits<unsigned long long>(_val);
	}

	template<> inline BX_CONSTEXPR_FUNC uint8_t countBits(uint8_t  _val) { return countBits<uint32_t>(_val); }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countBits(int8_t   _val) { return countBits<uint8_t >(_val); }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countBits(uint16_t _val) { return countBits<uint32_t>(_val); }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countBits(int16_t  _val) { return countBits<uint16_t>(_val); }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countBits(int32_t  _val) { return countBits<uint32_t>(_val); }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countBits(int64_t  _val) { return countBits<uint64_t>(_val); }

	template<>
	inline BX_CONSTEXPR_FUNC uint8_t countLeadingZeros(uint32_t _val)
	{
#if BX_COMPILER_GCC || BX_COMPILER_CLANG
		return 0 == _val ? 32 : __builtin_clz(_val);
#else
#	if BX_COMPILER_MSVC
		if (!isConstantEvaluated() )
		{
			unsigned long index;
			return 0 != _BitScanReverse(&index, (unsigned long)_val)
				? uint8_t(31 - index)
				: uint8_t(32)
				;
		}
#	endif // BX_COMPILER_MSVC
		const simd32_t val    = simd32_splat(_val);
		const simd32_t tmp0   = simd32_x32_srl(val, 1);
		const simd32_t tmp1   = simd32_or(tmp0, val);
		const simd32_t tmp2   = simd32_x32_srl(tmp1, 2);
		const simd32_t tmp3   = simd32_or(tmp2, tmp1);
		const simd32_t tmp4   = simd32_x32_srl(tmp3, 4);
		const simd32_t tmp5   = simd32_or(tmp4, tmp3);
		const simd32_t tmp6   = simd32_x32_srl(tmp5, 8);
		const simd32_t tmp7   = simd32_or(tmp6, tmp5);
		const simd32_t tmp8   = simd32_x32_srl(tmp7, 16);
		const simd32_t tmp9   = simd32_or(tmp8, tmp7);
		const simd32_t tmpA   = simd32_not(tmp9);
		const simd32_t result = simd32_x32_cntbits(tmpA);

		return uint8_t(result.u32);
#endif // BX_COMPILER_*
	}

	template<>
	inline BX_CONSTEXPR_FUNC uint8_t countLeadingZeros(unsigned long long _val)
	{
#if BX_COMPILER_GCC || BX_COMPILER_CLANG
		return 0 == _val ? 64 : __builtin_clzll(_val);
#else
#	if BX_COMPILER_MSVC && BX_ARCH_64BIT
		if (!isConstantEvaluated() )
		{
			unsigned long index;
			return 0 != _BitScanReverse64(&index, (unsigned __int64)_val)
				? uint8_t(63 - index)
				: uint8_t(64)
				;
		}
#	endif // BX_COMPILER_MSVC && BX_ARCH_64BIT
		return _val & UINT64_C(0xffffffff00000000)
			 ? countLeadingZeros<uint32_t>(uint32_t(_val>>32) )
			 : countLeadingZeros<uint32_t>(uint32_t(_val) ) + 32
			 ;
#endif // BX_COMPILER_*
	}

	template<>
	inline BX_CONSTEXPR_FUNC uint8_t countLeadingZeros(unsigned long _val)
	{
		return countLeadingZeros<unsigned long long>(_val);
	}

	template<> inline BX_CONSTEXPR_FUNC uint8_t countLeadingZeros(uint8_t  _val) { return countLeadingZeros<uint32_t>(_val)-24; }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countLeadingZeros(int8_t   _val) { return countLeadingZeros<uint8_t >(_val);    }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countLeadingZeros(uint16_t _val) { return countLeadingZeros<uint32_t>(_val)-16; }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countLeadingZeros(int16_t  _val) { return countLeadingZeros<uint16_t>(_val);    }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countLeadingZeros(int32_t  _val) { return countLeadingZeros<uint32_t>(_val);    }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countLeadingZeros(int64_t  _val) { return countLeadingZeros<uint64_t>(_val);    }

	template<>
	inline BX_CONSTEXPR_FUNC uint8_t countTrailingZeros(uint32_t _val)
	{
#if BX_COMPILER_GCC || BX_COMPILER_CLANG
		return 0 == _val ? 32 : __builtin_ctz(_val);
#else
#	if BX_COMPILER_MSVC
		if (!isConstantEvaluated() )
		{
			unsigned long index;
			return 0 != _BitScanForward(&index, (unsigned long)_val)
				? uint8_t(index)
				: uint8_t(32)
				;
		}
#	endif // BX_COMPILER_MSVC
		const simd32_t val    = simd32_splat(_val);
		const simd32_t one    = simd32_splat(1);
		const simd32_t tmp0   = simd32_not(val);
		const simd32_t tmp1   = simd32_u32_sub(val, one);
		const simd32_t tmp2   = simd32_and(tmp0, tmp1);
		const simd32_t result = simd32_x32_cntbits(tmp2);

		return uint8_t(result.u32);
#endif // BX_COMPILER_*
	}

	template<>
	inline BX_CONSTEXPR_FUNC uint8_t countTrailingZeros(unsigned long long _val)
	{
#if BX_COMPILER_GCC || BX_COMPILER_CLANG
		return 0 == _val ? 64 : __builtin_ctzll(_val);
#else
#	if BX_COMPILER_MSVC && BX_ARCH_64BIT
		if (!isConstantEvaluated() )
		{
			unsigned long index;
			return 0 != _BitScanForward64(&index, (unsigned __int64)_val)
				? uint8_t(index)
				: uint8_t(64)
				;
		}
#	endif // BX_COMPILER_MSVC && BX_ARCH_64BIT
		return _val & UINT64_C(0xffffffff)
			? countTrailingZeros<uint32_t>(uint32_t(_val) )
			: countTrailingZeros<uint32_t>(uint32_t(_val>>32) ) + 32
			;
#endif // BX_COMPILER_*
	}

	template<>
	inline BX_CONSTEXPR_FUNC uint8_t countTrailingZeros(unsigned long _val)
	{
		return countTrailingZeros<unsigned long long>(_val);
	}

	template<> inline BX_CONSTEXPR_FUNC uint8_t countTrailingZeros(uint8_t  _val) { return min<uint8_t>(8,  countTrailingZeros<uint32_t>(_val) ); }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countTrailingZeros(int8_t   _val) { return                  countTrailingZeros<uint8_t >(_val);   }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countTrailingZeros(uint16_t _val) { return min<uint8_t>(16, countTrailingZeros<uint32_t>(_val) ); }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countTrailingZeros(int16_t  _val) { return                  countTrailingZeros<uint16_t>(_val);   }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countTrailingZeros(int32_t  _val) { return                  countTrailingZeros<uint32_t>(_val);   }
	template<> inline BX_CONSTEXPR_FUNC uint8_t countTrailingZeros(int64_t  _val) { return                  countTrailingZeros<uint64_t>(_val);   }

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC uint8_t findFirstSet(Ty _val)
	{
		static_assert(isInteger<Ty>(), "Type Ty must be of integer type!");
		return Ty(0) == _val ? uint8_t(0) : countTrailingZeros<Ty>(_val) + 1;
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC uint8_t findLastSet(Ty _val)
	{
		static_assert(isInteger<Ty>(), "Type Ty must be of integer type!");
		return Ty(0) == _val ? uint8_t(0) : sizeof(Ty)*8 - countLeadingZeros<Ty>(_val);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC uint8_t ceilLog2(Ty _a)
	{
		static_assert(isInteger<Ty>(), "Type Ty must be of integer type!");
		return Ty(_a) < Ty(1) ? Ty(0) : sizeof(Ty)*8 - countLeadingZeros<Ty>(_a - 1);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC uint8_t floorLog2(Ty _a)
	{
		static_assert(isInteger<Ty>(), "Type Ty must be of integer type!");
		return Ty(_a) < Ty(1) ? Ty(0) : sizeof(Ty)*8 - 1 - countLeadingZeros<Ty>(_a);
	}

	template<typename Ty>
	inline BX_CONSTEXPR_FUNC Ty nextPow2(Ty _a)
	{
		const uint8_t log2 = ceilLog2(_a);
		BX_ASSERT(log2 < sizeof(Ty)*8
			, "Type Ty cannot represent the next power-of-two value (1<<%u is larger than %u-bit type)."
			, log2
			, sizeof(Ty)*8
			);
		return Ty(1)<<log2;
	}

	inline BX_CONSTEXPR_FUNC float rsqrtRef(float _a)
	{
		return rsqrtT<float>(_a);
	}

	inline BX_CONST_FUNC float rsqrtSimd(float _a)
	{
		if (_a < kFloatSmallest)
		{
			return kFloatInfinity;
		}

		const simd128_t aa     = simd_splat<simd128_t>(_a);
		const simd128_t rsqrta = simd_f32_rsqrt<simd128_t>(aa);

		float result = 0.0f;
		simd_x32_st1<simd128_t>(&result, rsqrta);

		return result;
	}

	inline BX_CONSTEXPR_FUNC float sqrtRef(float _a)
	{
		return sqrtT<float>(_a);
	}

	inline BX_CONSTEXPR_FUNC double sqrtRef(double _a)
	{
		return sqrtT<double>(_a);
	}

	inline BX_CONST_FUNC float sqrtSimd(float _a)
	{
		if (_a < 0.0f)
		{
			return bitsToFloat(kFloatExponentMask | kFloatMantissaMask);
		}
		else if (_a < kFloatSmallest)
		{
			return 0.0f;
		}

		const simd128_t aa   = simd_splat<simd128_t>(_a);
		const simd128_t sqrt = simd_f32_sqrt<simd128_t>(aa);

		float result = 0.0f;
		simd_x32_st1<simd128_t>(&result, sqrt);

		return result;
	}

	inline BX_CONST_FUNC double sqrtSimd(double _a)
	{
		if (_a < 0.0)
		{
			return bitsToDouble(kDoubleExponentMask | kDoubleMantissaMask);
		}

		const simd128_t aa   = simd_splat<simd128_t>(_a);
		const simd128_t sqrt = simd_f64_sqrt<simd128_t>(aa);

		alignas(16) double result[2] = { 0.0, 0.0 };
		simd_st<simd128_t>(result, sqrt);

		return result[0];
	}

	inline BX_CONSTEXPR_FUNC float rsqrt(float _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return rsqrtRef(_a);
		}

		return rsqrtSimd(_a);
#else
		return rsqrtRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC double rsqrt(double _a)
	{
		return rsqrtT<double>(_a);
	}

	inline BX_CONSTEXPR_FUNC float sqrt(float _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return sqrtRef(_a);
		}

		return sqrtSimd(_a);
#else
		return sqrtRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC double sqrt(double _a)
	{
#if BX_SIMD_SUPPORTED
		if (isConstantEvaluated() )
		{
			return sqrtRef(_a);
		}

		return sqrtSimd(_a);
#else
		return sqrtRef(_a);
#endif // BX_SIMD_SUPPORTED
	}

	inline BX_CONSTEXPR_FUNC bool isEqual(float _a, float _b, float _epsilon)
	{
		// Reference(s):
		// - Floating-point tolerances revisited
		//   https://web.archive.org/web/20181103180318/http://realtimecollisiondetection.net/blog/?p=89
		//
		const float lhs = abs(_a - _b);
		const float rhs = _epsilon * max(1.0f, abs(_a), abs(_b) );
		return lhs <= rhs;
	}

	inline BX_CONST_FUNC bool isEqual(const float* _a, const float* _b, uint32_t _num, float _epsilon)
	{
		bool result = isEqual(_a[0], _b[0], _epsilon);
		for (uint32_t ii = 1; result && ii < _num; ++ii)
		{
			result = isEqual(_a[ii], _b[ii], _epsilon);
		}
		return result;
	}

	inline BX_CONSTEXPR_FUNC bool isNearZero(float _v)
	{
		return isEqual(_v, 0.0f, 0.00001f);
	}

	inline BX_CONSTEXPR_FUNC float wrap(float _a, float _wrap)
	{
		const float tmp0   = mod(_a, _wrap);
		const float result = tmp0 < 0.0f ? _wrap + tmp0 : tmp0;
		return result;
	}

	inline BX_CONSTEXPR_FUNC float step(float _edge, float _a)
	{
		return _a < _edge ? 0.0f : 1.0f;
	}

	inline BX_CONSTEXPR_FUNC float pulse(float _a, float _start, float _end)
	{
		return step(_a, _start) - step(_a, _end);
	}

	inline BX_CONSTEXPR_FUNC float smoothStep(float _a)
	{
		return square(_a)*(3.0f - 2.0f*_a);
	}

	inline BX_CONSTEXPR_FUNC float invSmoothStep(float _a)
	{
		return 0.5f - sin(asin(1.0f - 2.0f * _a) / 3.0f);
	}

	inline BX_CONSTEXPR_FUNC float bias(float _time, float _bias)
	{
		return _time / ( ( (1.0f/_bias - 2.0f)*(1.0f - _time) ) + 1.0f);
	}

	inline BX_CONSTEXPR_FUNC float gain(float _time, float _gain)
	{
		// Reference(s):
		// - Bias And Gain Are Your Friend
		//   https://web.archive.org/web/20181126040535/https://blog.demofox.org/2012/09/24/bias-and-gain-are-your-friend/
		//   https://web.archive.org/web/20181126040558/http://demofox.org/biasgain.html
		//
		if (_time < 0.5f)
		{
			return bias(_time * 2.0f, _gain) * 0.5f;
		}

		return bias(_time * 2.0f - 1.0f, 1.0f - _gain) * 0.5f + 0.5f;
	}

	inline BX_CONSTEXPR_FUNC float angleDiff(float _a, float _b)
	{
		const float dist = wrap(_b - _a, kPi2);
		return wrap(dist*2.0f, kPi2) - dist;
	}

	inline BX_CONSTEXPR_FUNC float angleLerp(float _a, float _b, float _t)
	{
		return _a + angleDiff(_a, _b) * _t;
	}

	template<typename Ty>
	inline Ty load(const void* _ptr)
	{
		Ty result(InitNone);
		memCopy(&result, _ptr, sizeof(Ty) );
		return result;
	}

	template<typename Ty>
	inline void store(void* _ptr, const Ty& _a)
	{
		memCopy(_ptr, &_a, sizeof(Ty) );
	}

	inline Vec3::Vec3(InitNoneTag)
	{
	}

	constexpr Vec3::Vec3(InitZeroTag)
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
	{
	}

	constexpr Vec3::Vec3(InitIdentityTag)
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
	{
	}

	constexpr Vec3::Vec3(float _v)
		: x(_v)
		, y(_v)
		, z(_v)
	{
	}

	constexpr Vec3::Vec3(float _x, float _y, float _z)
		: x(_x)
		, y(_y)
		, z(_z)
	{
	}

	inline Plane::Plane(InitNoneTag)
		: normal(InitNone)
	{
	}

	constexpr Plane::Plane(InitZeroTag)
		: normal(InitZero)
		, dist(0.0f)
	{
	}

	constexpr Plane::Plane(InitIdentityTag)
		: normal(0.0f, 1.0f, 0.0f)
		, dist(0.0f)
	{
	}

	constexpr Plane::Plane(Vec3 _normal, float _dist)
		: normal(_normal)
		, dist(_dist)
	{
	}

	inline Quaternion::Quaternion(InitNoneTag)
	{
	}

	constexpr Quaternion::Quaternion(InitZeroTag)
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
		, w(0.0f)
	{
	}

	constexpr Quaternion::Quaternion(InitIdentityTag)
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
		, w(1.0f)
	{
	}

	constexpr Quaternion::Quaternion(float _x, float _y, float _z, float _w)
		: x(_x)
		, y(_y)
		, z(_z)
		, w(_w)
	{
	}

	inline BX_CONSTEXPR_FUNC Vec3 round(const Vec3& _a)
	{
		return
		{
			round(_a.x),
			round(_a.y),
			round(_a.z),
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 abs(const Vec3& _a)
	{
		return
		{
			abs(_a.x),
			abs(_a.y),
			abs(_a.z),
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 neg(const Vec3& _a)
	{
		return
		{
			-_a.x,
			-_a.y,
			-_a.z,
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 add(const Vec3& _a, const Vec3& _b)
	{
		return
		{
			_a.x + _b.x,
			_a.y + _b.y,
			_a.z + _b.z,
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 add(const Vec3& _a, float _b)
	{
		return
		{
			_a.x + _b,
			_a.y + _b,
			_a.z + _b,
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 sub(const Vec3& _a, const Vec3& _b)
	{
		return
		{
			_a.x - _b.x,
			_a.y - _b.y,
			_a.z - _b.z,
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 sub(const Vec3& _a, float _b)
	{
		return
		{
			_a.x - _b,
			_a.y - _b,
			_a.z - _b,
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 mul(const Vec3& _a, const Vec3& _b)
	{
		return
		{
			_a.x * _b.x,
			_a.y * _b.y,
			_a.z * _b.z,
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 mul(const Vec3& _a, float _b)
	{
		return
		{
			_a.x * _b,
			_a.y * _b,
			_a.z * _b,
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 div(const Vec3& _a, const Vec3& _b)
	{
		return mul(_a, rcp(_b) );
	}

	inline BX_CONSTEXPR_FUNC Vec3 divSafe(const Vec3& _a, const Vec3& _b)
	{
		return mul(_a, rcpSafe(_b) );
	}

	inline BX_CONSTEXPR_FUNC Vec3 div(const Vec3& _a, float _b)
	{
		return mul(_a, rcp(_b) );
	}

	inline BX_CONSTEXPR_FUNC Vec3 divSafe(const Vec3& _a, float _b)
	{
		return mul(_a, rcpSafe(_b) );
	}

	inline BX_CONSTEXPR_FUNC Vec3 nms(const Vec3& _a, const float _b, const Vec3& _c)
	{
		return sub(_c, mul(_a, _b) );
	}

	inline BX_CONSTEXPR_FUNC Vec3 nms(const Vec3& _a, const Vec3& _b, const Vec3& _c)
	{
		const float xx = nms(_a.x, _b.x, _c.x);
		const float yy = nms(_a.y, _b.y, _c.y);
		const float zz = nms(_a.z, _b.z, _c.z);

		return Vec3(xx, yy, zz);
	}

	inline BX_CONSTEXPR_FUNC Vec3 mad(const Vec3& _a, const float _b, const Vec3& _c)
	{
		const float xx = mad(_a.x, _b, _c.x);
		const float yy = mad(_a.y, _b, _c.y);
		const float zz = mad(_a.z, _b, _c.z);

		return Vec3(xx, yy, zz);
	}

	inline BX_CONSTEXPR_FUNC Vec3 mad(const Vec3& _a, const Vec3& _b, const Vec3& _c)
	{
		const float xx = mad(_a.x, _b.x, _c.x);
		const float yy = mad(_a.y, _b.y, _c.y);
		const float zz = mad(_a.z, _b.z, _c.z);

		return Vec3(xx, yy, zz);
	}

	inline BX_CONSTEXPR_FUNC float dot(const Vec3& _a, const Vec3& _b)
	{
		return _a.x*_b.x + _a.y*_b.y + _a.z*_b.z;
	}

	inline BX_CONSTEXPR_FUNC Vec3 cross(const Vec3& _a, const Vec3& _b)
	{
		return
		{
			_a.y*_b.z - _a.z*_b.y,
			_a.z*_b.x - _a.x*_b.z,
			_a.x*_b.y - _a.y*_b.x,
		};
	}

	inline BX_CONSTEXPR_FUNC float length(const Vec3& _a)
	{
		return sqrt(dot(_a, _a) );
	}

	inline BX_CONSTEXPR_FUNC float distanceSq(const Vec3& _a, const Vec3& _b)
	{
		const Vec3 ba = sub(_b, _a);
		return dot(ba, ba);
	}

	inline BX_CONSTEXPR_FUNC float distance(const Vec3& _a, const Vec3& _b)
	{
		return length(sub(_b, _a) );
	}

	inline BX_CONSTEXPR_FUNC Vec3 lerp(const Vec3& _a, const Vec3& _b, float _t)
	{
		return
		{
			lerp(_a.x, _b.x, _t),
			lerp(_a.y, _b.y, _t),
			lerp(_a.z, _b.z, _t),
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 lerp(const Vec3& _a, const Vec3& _b, const Vec3& _t)
	{
		return
		{
			lerp(_a.x, _b.x, _t.x),
			lerp(_a.y, _b.y, _t.y),
			lerp(_a.z, _b.z, _t.z),
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 normalize(const Vec3& _a)
	{
		const float len   = length(_a);
		const Vec3 result = divSafe(_a, len);
		return result;
	}

	inline BX_CONSTEXPR_FUNC Vec3 min(const Vec3& _a, const Vec3& _b)
	{
		return
		{
			min(_a.x, _b.x),
			min(_a.y, _b.y),
			min(_a.z, _b.z),
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 max(const Vec3& _a, const Vec3& _b)
	{
		return
		{
			max(_a.x, _b.x),
			max(_a.y, _b.y),
			max(_a.z, _b.z),
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 rcp(const Vec3& _a)
	{
		return
		{
			rcp(_a.x),
			rcp(_a.y),
			rcp(_a.z),
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 rcpSafe(const Vec3& _a)
	{
		return
		{
			rcpSafe(_a.x),
			rcpSafe(_a.y),
			rcpSafe(_a.z),
		};
	}

	inline BX_CONSTEXPR_FUNC bool isEqual(const Vec3& _a, const Vec3& _b, float _epsilon)
	{
		return isEqual(_a.x, _b.x, _epsilon)
			&& isEqual(_a.y, _b.y, _epsilon)
			&& isEqual(_a.z, _b.z, _epsilon)
			;
	}

	inline BX_CONSTEXPR_FUNC bool isNearZero(const Vec3& _v)
	{
		return isNearZero(dot(_v, _v) );
	}

	inline void calcTangentFrame(Vec3& _outT, Vec3& _outB, const Vec3& _n)
	{
		const float nx = _n.x;
		const float ny = _n.y;
		const float nz = _n.z;

		if (abs(nx) > abs(nz) )
		{
			const float invLen = rcpSafe(sqrt(nx*nx + nz*nz) );
			_outT.x = -nz * invLen;
			_outT.y =  0.0f;
			_outT.z =  nx * invLen;
		}
		else
		{
			const float invLen = rcpSafe(sqrt(ny*ny + nz*nz) );
			_outT.x =  0.0f;
			_outT.y =  nz * invLen;
			_outT.z = -ny * invLen;
		}

		_outB = cross(_n, _outT);
	}

	inline void calcTangentFrame(Vec3& _outT, Vec3& _outB, const Vec3& _n, float _angle)
	{
		calcTangentFrame(_outT, _outB, _n);

		const float sa = sin(_angle);
		const float ca = cos(_angle);

		_outT.x = -sa * _outB.x + ca * _outT.x;
		_outT.y = -sa * _outB.y + ca * _outT.y;
		_outT.z = -sa * _outB.z + ca * _outT.z;

		_outB = cross(_n, _outT);
	}

	inline BX_CONSTEXPR_FUNC Vec3 fromLatLong(float _u, float _v)
	{
		const float phi   = _u * kPi2;
		const float theta = _v * kPi;

		const float st = sin(theta);
		const float sp = sin(phi);
		const float ct = cos(theta);
		const float cp = cos(phi);

		return
		{
			-st*sp,
			 ct,
			-st*cp,
		};
	}

	inline void toLatLong(float* _outU, float* _outV, const Vec3& _dir)
	{
		const float phi   = atan2(_dir.x, _dir.z);
		const float theta = acos(_dir.y);

		*_outU = (kPi + phi)/kPi2;
		*_outV = theta*kInvPi;
	}

	inline BX_CONSTEXPR_FUNC Quaternion neg(const Quaternion& _a)
	{
		return
		{
			-_a.x,
			-_a.y,
			-_a.z,
			-_a.w,
		};
	}

	inline BX_CONSTEXPR_FUNC Quaternion conjugate(const Quaternion& _a)
	{
		return
		{
			-_a.x,
			-_a.y,
			-_a.z,
			 _a.w,
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 mulXyz(const Quaternion& _a, const Quaternion& _b)
	{
		const float ax = _a.x;
		const float ay = _a.y;
		const float az = _a.z;
		const float aw = _a.w;

		const float bx = _b.x;
		const float by = _b.y;
		const float bz = _b.z;
		const float bw = _b.w;

		return
		{
			aw * bx + ax * bw + ay * bz - az * by,
			aw * by - ax * bz + ay * bw + az * bx,
			aw * bz + ax * by - ay * bx + az * bw,
		};
	}

	inline BX_CONSTEXPR_FUNC Quaternion add(const Quaternion& _a, const Quaternion& _b)
	{
		return
		{
			_a.x + _b.x,
			_a.y + _b.y,
			_a.z + _b.z,
			_a.w + _b.w,
		};
	}

	inline BX_CONSTEXPR_FUNC Quaternion sub(const Quaternion& _a, const Quaternion& _b)
	{
		return
		{
			_a.x - _b.x,
			_a.y - _b.y,
			_a.z - _b.z,
			_a.w - _b.w,
		};
	}

	inline BX_CONSTEXPR_FUNC Quaternion mul(const Quaternion& _a, float _b)
	{
		return
		{
			_a.x * _b,
			_a.y * _b,
			_a.z * _b,
			_a.w * _b,
		};
	}

	inline BX_CONSTEXPR_FUNC Quaternion mul(const Quaternion& _a, const Quaternion& _b)
	{
		const float ax = _a.x;
		const float ay = _a.y;
		const float az = _a.z;
		const float aw = _a.w;

		const float bx = _b.x;
		const float by = _b.y;
		const float bz = _b.z;
		const float bw = _b.w;

		return
		{
			aw * bx + ax * bw + ay * bz - az * by,
			aw * by - ax * bz + ay * bw + az * bx,
			aw * bz + ax * by - ay * bx + az * bw,
			aw * bw - ax * bx - ay * by - az * bz,
		};
	}

	inline BX_CONSTEXPR_FUNC Vec3 mul(const Vec3& _v, const Quaternion& _q)
	{
		const Quaternion qv   = { _v.x, _v.y, _v.z, 0.0f };
		const Quaternion tmp0 = mul(_q, qv);
		const Vec3 result     = mulXyz(tmp0, conjugate(_q) );

		return result;
	}

	inline BX_CONSTEXPR_FUNC float dot(const Quaternion& _a, const Quaternion& _b)
	{
		return
			  _a.x * _b.x
			+ _a.y * _b.y
			+ _a.z * _b.z
			+ _a.w * _b.w
			;
	}

	inline BX_CONSTEXPR_FUNC Quaternion normalize(const Quaternion& _a)
	{
		const float norm = dot(_a, _a);
		if (0.0f < norm)
		{
			const float invNorm = rsqrt(norm);

			return mul(_a, invNorm);
		}

		return
		{
			0.0f,
			0.0f,
			0.0f,
			1.0f,
		};
	}

	inline BX_CONSTEXPR_FUNC Quaternion lerp(const Quaternion& _a, const Quaternion& _b, float _t)
	{
		const float sa    = 1.0f - _t;
		const float adotb = dot(_a, _b);
		const float sb    = sign(adotb) * _t;

		const Quaternion aa = mul(_a, sa);
		const Quaternion bb = mul(_b, sb);
		const Quaternion qq = add(aa, bb);

		return normalize(qq);
	}

	inline BX_CONST_FUNC Quaternion slerp(const Quaternion& _a, const Quaternion& _b, float _t)
	{
		float cosTheta = dot(_a, _b);

		Quaternion bb = _b;
		if (cosTheta < 0.0f)
		{
			cosTheta = -cosTheta;
			bb = neg(_b);
		}

		if (cosTheta > 0.9995f)
		{
			return lerp(_a, bb, _t);
		}

		const float theta  = acos(cosTheta);
		const float invSin = rcp(sin(theta) );
		const float sa     = sin( (1.0f - _t) * theta) * invSin;
		const float sb     = sin(_t * theta)           * invSin;

		return
		{
			_a.x * sa + bb.x * sb,
			_a.y * sa + bb.y * sb,
			_a.z * sa + bb.z * sb,
			_a.w * sa + bb.w * sb,
		};
	}

	inline BX_CONST_FUNC Quaternion fromEuler(const Vec3& _euler)
	{
		const float sx = sin(_euler.x * 0.5f);
		const float cx = cos(_euler.x * 0.5f);
		const float sy = sin(_euler.y * 0.5f);
		const float cy = cos(_euler.y * 0.5f);
		const float sz = sin(_euler.z * 0.5f);
		const float cz = cos(_euler.z * 0.5f);

		return
		{
			sx * cy * cz - cx * sy * sz,
			cx * sy * cz + sx * cy * sz,
			cx * cy * sz - sx * sy * cz,
			cx * cy * cz + sx * sy * sz,
		};
	}

	inline BX_CONST_FUNC Vec3 toEuler(const Quaternion& _a)
	{
		const float xx  = _a.x;
		const float yy  = _a.y;
		const float zz  = _a.z;
		const float ww  = _a.w;
		const float xsq = square(xx);
		const float ysq = square(yy);
		const float zsq = square(zz);

		return
		{
			atan2(2.0f * (xx * ww - yy * zz), 1.0f - 2.0f * (xsq + zsq) ),
			atan2(2.0f * (yy * ww + xx * zz), 1.0f - 2.0f * (ysq + zsq) ),
			asin( 2.0f * (xx * yy + zz * ww) ),
		};
	}

	inline BX_CONST_FUNC Vec3 toXAxis(const Quaternion& _a)
	{
		const float xx  = _a.x;
		const float yy  = _a.y;
		const float zz  = _a.z;
		const float ww  = _a.w;
		const float ysq = square(yy);
		const float zsq = square(zz);

		return
		{
			1.0f - 2.0f * ysq     - 2.0f * zsq,
			       2.0f * xx * yy + 2.0f * zz * ww,
			       2.0f * xx * zz - 2.0f * yy * ww,
		};
	}

	inline BX_CONST_FUNC Vec3 toYAxis(const Quaternion& _a)
	{
		const float xx  = _a.x;
		const float yy  = _a.y;
		const float zz  = _a.z;
		const float ww  = _a.w;
		const float xsq = square(xx);
		const float zsq = square(zz);

		return
		{
			       2.0f * xx * yy - 2.0f * zz * ww,
			1.0f - 2.0f * xsq     - 2.0f * zsq,
			       2.0f * yy * zz + 2.0f * xx * ww,
		};
	}

	inline BX_CONST_FUNC Vec3 toZAxis(const Quaternion& _a)
	{
		const float xx  = _a.x;
		const float yy  = _a.y;
		const float zz  = _a.z;
		const float ww  = _a.w;
		const float xsq = square(xx);
		const float ysq = square(yy);

		return
		{
			       2.0f * xx * zz + 2.0f * yy * ww,
			       2.0f * yy * zz - 2.0f * xx * ww,
			1.0f - 2.0f * xsq     - 2.0f * ysq,
		};
	}

	inline BX_CONST_FUNC Quaternion fromAxisAngle(const Vec3& _axis, float _angle)
	{
		const float ha = _angle * 0.5f;
		const float sa = sin(ha);

		return
		{
			_axis.x * sa,
			_axis.y * sa,
			_axis.z * sa,
			cos(ha),
		};
	}

	inline void toAxisAngle(Vec3& _outAxis, float& _outAngle, const Quaternion& _a)
	{
		const float ww = _a.w;
		const float sa = sqrt(1.0f - square(ww) );

		_outAngle = 2.0f * acos(ww);

		if (0.001f > sa)
		{
			_outAxis = { _a.x, _a.y, _a.z };
			return;
		}

		const float invSa = rcpSafe(sa);

		_outAxis = { _a.x * invSa, _a.y * invSa, _a.z * invSa };
	}

	inline BX_CONST_FUNC Quaternion rotateX(float _ax)
	{
		const float hx = _ax * 0.5f;

		return
		{
			sin(hx),
			0.0f,
			0.0f,
			cos(hx),
		};
	}

	inline BX_CONST_FUNC Quaternion rotateY(float _ay)
	{
		const float hy = _ay * 0.5f;

		return
		{
			0.0f,
			sin(hy),
			0.0f,
			cos(hy),
		};
	}

	inline BX_CONST_FUNC Quaternion rotateZ(float _az)
	{
		const float hz = _az * 0.5f;

		return
		{
			0.0f,
			0.0f,
			sin(hz),
			cos(hz),
		};
	}

	inline BX_CONSTEXPR_FUNC bool isEqual(const Quaternion& _a, const Quaternion& _b, float _epsilon)
	{
		return isEqual(_a.x, _b.x, _epsilon)
			&& isEqual(_a.y, _b.y, _epsilon)
			&& isEqual(_a.z, _b.z, _epsilon)
			&& isEqual(_a.w, _b.w, _epsilon)
			;
	}

	inline void mtxIdentity(float* _result)
	{
		memSet(_result, 0, sizeof(float)*16);
		_result[0] = _result[5] = _result[10] = _result[15] = 1.0f;
	}

	inline void mtxTranslate(float* _result, float _tx, float _ty, float _tz)
	{
		mtxIdentity(_result);
		_result[12] = _tx;
		_result[13] = _ty;
		_result[14] = _tz;
	}

	inline void mtxScale(float* _result, float _sx, float _sy, float _sz)
	{
		memSet(_result, 0, sizeof(float) * 16);
		_result[0]  = _sx;
		_result[5]  = _sy;
		_result[10] = _sz;
		_result[15] = 1.0f;
	}

	inline void mtxScale(float* _result, float _scale)
	{
		mtxScale(_result, _scale, _scale, _scale);
	}

	inline void mtxFromNormal(float* _result, const Vec3& _normal, float _scale, const Vec3& _pos)
	{
		Vec3 tangent(InitNone);
		Vec3 bitangent(InitNone);
		calcTangentFrame(tangent, bitangent, _normal);

		store(&_result[ 0], mul(bitangent, _scale) );
		store(&_result[ 4], mul(_normal,   _scale) );
		store(&_result[ 8], mul(tangent,   _scale) );

		_result[ 3] = 0.0f;
		_result[ 7] = 0.0f;
		_result[11] = 0.0f;
		_result[12] = _pos.x;
		_result[13] = _pos.y;
		_result[14] = _pos.z;
		_result[15] = 1.0f;
	}

	inline void mtxFromNormal(float* _result, const Vec3& _normal, float _scale, const Vec3& _pos, float _angle)
	{
		Vec3 tangent(InitNone);
		Vec3 bitangent(InitNone);
		calcTangentFrame(tangent, bitangent, _normal, _angle);

		store(&_result[0], mul(bitangent, _scale) );
		store(&_result[4], mul(_normal,   _scale) );
		store(&_result[8], mul(tangent,   _scale) );

		_result[ 3] = 0.0f;
		_result[ 7] = 0.0f;
		_result[11] = 0.0f;
		_result[12] = _pos.x;
		_result[13] = _pos.y;
		_result[14] = _pos.z;
		_result[15] = 1.0f;
	}

	inline void mtxFromQuaternion(float* _result, const Quaternion& _rotation)
	{
		const float qx = _rotation.x;
		const float qy = _rotation.y;
		const float qz = _rotation.z;
		const float qw = _rotation.w;

		const float x2  = qx + qx;
		const float y2  = qy + qy;
		const float z2  = qz + qz;
		const float x2x = x2 * qx;
		const float x2y = x2 * qy;
		const float x2z = x2 * qz;
		const float x2w = x2 * qw;
		const float y2y = y2 * qy;
		const float y2z = y2 * qz;
		const float y2w = y2 * qw;
		const float z2z = z2 * qz;
		const float z2w = z2 * qw;

		_result[ 0] = 1.0f - (y2y + z2z);
		_result[ 1] =         x2y - z2w;
		_result[ 2] =         x2z + y2w;
		_result[ 3] = 0.0f;

		_result[ 4] =         x2y + z2w;
		_result[ 5] = 1.0f - (x2x + z2z);
		_result[ 6] =         y2z - x2w;
		_result[ 7] = 0.0f;

		_result[ 8] =         x2z - y2w;
		_result[ 9] =         y2z + x2w;
		_result[10] = 1.0f - (x2x + y2y);
		_result[11] = 0.0f;

		_result[12] = 0.0f;
		_result[13] = 0.0f;
		_result[14] = 0.0f;
		_result[15] = 1.0f;
	}

	inline void mtxFromQuaternion(float* _result, const Quaternion& _rotation, const Vec3& _translation)
	{
		mtxFromQuaternion(_result, _rotation);
		store(&_result[12], neg(mulXyz0(_translation, _result) ) );
	}

	inline Vec3 mul(const Vec3& _vec, const float* _mat)
	{
		Vec3 result(InitNone);
		result.x = _vec.x * _mat[0] + _vec.y * _mat[4] + _vec.z * _mat[ 8] + _mat[12];
		result.y = _vec.x * _mat[1] + _vec.y * _mat[5] + _vec.z * _mat[ 9] + _mat[13];
		result.z = _vec.x * _mat[2] + _vec.y * _mat[6] + _vec.z * _mat[10] + _mat[14];
		return result;
	}

	inline Vec3 mulXyz0(const Vec3& _vec, const float* _mat)
	{
		Vec3 result(InitNone);
		result.x = _vec.x * _mat[0] + _vec.y * _mat[4] + _vec.z * _mat[ 8];
		result.y = _vec.x * _mat[1] + _vec.y * _mat[5] + _vec.z * _mat[ 9];
		result.z = _vec.x * _mat[2] + _vec.y * _mat[6] + _vec.z * _mat[10];
		return result;
	}

	inline Vec3 mulH(const Vec3& _vec, const float* _mat)
	{
		const float xx   = _vec.x * _mat[0] + _vec.y * _mat[4] + _vec.z * _mat[ 8] + _mat[12];
		const float yy   = _vec.x * _mat[1] + _vec.y * _mat[5] + _vec.z * _mat[ 9] + _mat[13];
		const float zz   = _vec.x * _mat[2] + _vec.y * _mat[6] + _vec.z * _mat[10] + _mat[14];
		const float ww   = _vec.x * _mat[3] + _vec.y * _mat[7] + _vec.z * _mat[11] + _mat[15];
		const float invW = sign(ww) / ww;

		Vec3 result =
		{
			xx * invW,
			yy * invW,
			zz * invW,
		};

		return result;
	}

	inline void vec4MulMtx(float* _result, const float* _vec, const float* _mat)
	{
		_result[0] = _vec[0] * _mat[ 0] + _vec[1] * _mat[4] + _vec[2] * _mat[ 8] + _vec[3] * _mat[12];
		_result[1] = _vec[0] * _mat[ 1] + _vec[1] * _mat[5] + _vec[2] * _mat[ 9] + _vec[3] * _mat[13];
		_result[2] = _vec[0] * _mat[ 2] + _vec[1] * _mat[6] + _vec[2] * _mat[10] + _vec[3] * _mat[14];
		_result[3] = _vec[0] * _mat[ 3] + _vec[1] * _mat[7] + _vec[2] * _mat[11] + _vec[3] * _mat[15];
	}

	inline void mtxMul(float* _result, const float* _a, const float* _b)
	{
		vec4MulMtx(&_result[ 0], &_a[ 0], _b);
		vec4MulMtx(&_result[ 4], &_a[ 4], _b);
		vec4MulMtx(&_result[ 8], &_a[ 8], _b);
		vec4MulMtx(&_result[12], &_a[12], _b);
	}

	inline void mtxTranspose(float* _result, const float* _a)
	{
		_result[ 0] = _a[ 0];
		_result[ 4] = _a[ 1];
		_result[ 8] = _a[ 2];
		_result[12] = _a[ 3];
		_result[ 1] = _a[ 4];
		_result[ 5] = _a[ 5];
		_result[ 9] = _a[ 6];
		_result[13] = _a[ 7];
		_result[ 2] = _a[ 8];
		_result[ 6] = _a[ 9];
		_result[10] = _a[10];
		_result[14] = _a[11];
		_result[ 3] = _a[12];
		_result[ 7] = _a[13];
		_result[11] = _a[14];
		_result[15] = _a[15];
	}

	inline Vec3 calcNormal(const Vec3& _va, const Vec3& _vb, const Vec3& _vc)
	{
		const Vec3 ba    = sub(_vb, _va);
		const Vec3 ca    = sub(_vc, _va);
		const Vec3 baxca = cross(ba, ca);

		return normalize(baxca);
	}

	inline void calcPlane(Plane& _outPlane, const Vec3& _va, const Vec3& _vb, const Vec3& _vc)
	{
		Vec3 normal = calcNormal(_va, _vb, _vc);
		calcPlane(_outPlane, normal, _va);
	}

	inline void calcPlane(Plane& _outPlane, const Vec3& _normal, const Vec3& _pos)
	{
		_outPlane.normal = _normal;
		_outPlane.dist   = -dot(_normal, _pos);
	}

	inline BX_CONSTEXPR_FUNC float distance(const Plane& _plane, const Vec3& _pos)
	{
		return dot(_plane.normal, _pos) + _plane.dist;
	}

	inline BX_CONSTEXPR_FUNC bool isEqual(const Plane& _a, const Plane& _b, float _epsilon)
	{
		return isEqual(_a.normal, _b.normal, _epsilon)
			&& isEqual(_a.dist,   _b.dist,   _epsilon)
			;
	}

	inline BX_CONSTEXPR_FUNC float toLinear(float _a)
	{
		const float lo     = _a / 12.92f;
		const float hi     = pow( (_a + 0.055f) / 1.055f, 2.4f);
		const float result = lerp(hi, lo, _a <= 0.04045f);
		return result;
	}

	inline BX_CONSTEXPR_FUNC float toGamma(float _a)
	{
		const float lo     = _a * 12.92f;
		const float hi     = pow(abs(_a), 1.0f/2.4f) * 1.055f - 0.055f;
		const float result = lerp(hi, lo, _a <= 0.0031308f);
		return result;
	}

	inline BX_CONST_FUNC uint16_t halfFromFloat(float _a)
	{
		const simd32_t a      = { .u32 = bitCast<uint32_t>(_a) };
		const simd32_t result = simd_f16_fromf32_ni(a);
		return uint16_t(result.u32);
	}

	inline BX_CONST_FUNC float halfToFloat(uint16_t _a)
	{
		const simd32_t a      = simd32_splat(uint32_t(_a) );
		const simd32_t result = simd_f16_tof32_ni(a);
		return bitCast<float>(result.u32);
	}

} // namespace bx
