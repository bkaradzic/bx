/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_CONSTANTS_H_HEADER_GUARD
#define BX_CONSTANTS_H_HEADER_GUARD

namespace bx
{
	/// Used to return successful execution of a program code.
	constexpr int32_t  kExitSuccess    = 0;

	/// Used to return unsuccessful execution of a program code.
	constexpr int32_t  kExitFailure    = 1;

	/// The ratio of a circle's circumference to its diameter,
	constexpr float    kPi             = 3.1415926535897932384626433832795f;

	/// The ratio of a circle's circumference to its radius. Pi multiplied by 2, or Tau. pi*2
	constexpr float    kPi2            = 6.2831853071795864769252867665590f;

	/// The reciprocal of kPi. 1/kPi
	constexpr float    kInvPi          = 1.0f/kPi;

	/// The reciprocal of kPi2. 1/kPi2
	constexpr float    kInvPi2         = 1.0f/kPi2;

	/// Pi divided by two. pi/2
	constexpr float    kPiHalf         = 1.5707963267948966192313216916398f;

	/// Pi divided by four. pi/4
	constexpr float    kPiQuarter      = 0.7853981633974483096156608458199f;

	/// The square root of two. sqrt(2)
	constexpr float    kSqrt2          = 1.4142135623730950488016887242097f;

	/// ln(10)
	constexpr float    kLogNat10       = 2.3025850929940456840179914546844f;

	/// The logarithm of the e to base 2. ln(kE) / ln(2)
	constexpr float    kInvLogNat2     = 1.4426950408889634073599246810019f;

	/// The natural logarithm of the 2. ln(2)
	constexpr float    kLogNat2        = 0.6931471805599453094172321214582f;

	/// The base of natural logarithms. e(1)
	constexpr float    kE              = 2.7182818284590452353602874713527f;

	///
	constexpr float    kNearZero       = 1.0f/float(1 << 28);

	///
	constexpr uint8_t  kHalfSignNumBits      = 1;
	constexpr uint8_t  kHalfSignBitShift     = 15;
	constexpr uint16_t kHalfSignMask         = UINT16_C(0x8000);
	constexpr uint8_t  kHalfExponentNumBits  = 5;
	constexpr uint8_t  kHalfExponentBitShift = 10;
	constexpr uint16_t kHalfExponentMask     = UINT16_C(0x7c00);
	constexpr uint32_t kHalfExponentBias     = 15;
	constexpr uint8_t  kHalfMantissaNumBits  = 10;
	constexpr uint8_t  kHalfMantissaBitShift = 0;
	constexpr uint16_t kHalfMantissaMask     = UINT16_C(0x03ff);

	///
	constexpr uint8_t  kFloatSignNumBits      = 1;
	constexpr uint8_t  kFloatSignBitShift     = 31;
	constexpr uint32_t kFloatSignMask         = UINT32_C(0x80000000);
	constexpr uint8_t  kFloatExponentNumBits  = 8;
	constexpr uint8_t  kFloatExponentBitShift = 23;
	constexpr uint32_t kFloatExponentMask     = UINT32_C(0x7f800000);
	constexpr uint32_t kFloatExponentBias     = 127;
	constexpr uint8_t  kFloatMantissaNumBits  = 23;
	constexpr uint8_t  kFloatMantissaBitShift = 0;
	constexpr uint32_t kFloatMantissaMask     = UINT32_C(0x007fffff);

	/// Smallest normalized positive floating-point number.
	constexpr float    kFloatSmallest  = 1.175494351e-38f;

	/// Maximum representable floating-point number.
	constexpr float    kFloatLargest   = 3.402823466e+38f;

	/// Floating-point infinity.
//	constexpr float    kFloatInfinity;

	/// The reciprocal of ln(2). 1/ln(2)
	constexpr float    kFloatInvLn2       = kInvLogNat2;

	/// The reciprocal of ln(10). 1/ln(10)
	constexpr float    kFloatInvLn10      = 1.0f/kLogNat10;

	/// Leading part of ln(2), with the trailing mantissa bits cleared.
	constexpr float    kFloatLn2Hi        = 6.9314575195e-01f;

	/// What is left of ln(2) after kFloatLn2Hi.
	constexpr float    kFloatLn2Lo        = 1.4286067653e-06f;

	/// Largest argument exp can answer with a finite number.
	constexpr float    kFloatExpOverflow  =  88.7228394f;

	/// Smallest argument exp can answer with a non-zero number.
	constexpr float    kFloatExpUnderflow = -103.972084f;

	/// Exponent of the square root of the scale expHalf splits its result by.
	constexpr int32_t  kFloatExpHalfShift = 235;

	/// Argument reduction expHalf pairs with kFloatExpHalfShift. ln(2)*kFloatExpHalfShift/2
	constexpr float    kFloatExpHalfLn2   = 1.62889587431602213e+02f;

	/// Below this the hyperbolic functions take their series instead of exp.
	constexpr float    kFloatHypSmall     = 0.0625f;

	/// Above this exp(-_a) no longer contributes to sinh and cosh.
	constexpr float    kFloatHypBig       = 9.0f;

	/// Above this half of exp(_a) overflows, and expHalf takes over.
	constexpr float    kFloatHypNoFit     = 88.0f;

	/// Above this tanh is 1.
	constexpr float    kFloatTanhLarge    = 10.0f;

	/// Below this the arc hyperbolic functions answer with the argument.
	constexpr float    kFloatArcHypTiny   = 0.000244140625f;

	/// Above this asinh and acosh are log(_a) + ln(2).
	constexpr float    kFloatArcHypHuge   = 4096.0f;

	/// Seed for the reciprocal square root iteration.
	constexpr uint32_t kFloatRsqrtSeed    = UINT32_C(0x5f3759df);

	///
	constexpr uint8_t  kDoubleSignNumBits     = 1;
	constexpr uint8_t  kDoubleSignBitShift    = 63;
	constexpr uint64_t kDoubleSignMask        = UINT64_C(0x8000000000000000);
	constexpr uint8_t  kDoubleExponentNumBits = 11;
	constexpr uint8_t  kDoubleExponentShift   = 52;
	constexpr uint64_t kDoubleExponentMask    = UINT64_C(0x7ff0000000000000);
	constexpr uint32_t kDoubleExponentBias    = 1023;
	constexpr uint8_t  kDoubleMantissaNumBits = 52;
	constexpr uint8_t  kDoubleMantissaShift   = 0;
	constexpr uint64_t kDoubleMantissaMask    = UINT64_C(0x000fffffffffffff);

	/// Smallest normalized positive double-precision floating-point number.
	constexpr double   kDoubleSmallest = 2.2250738585072014e-308;

	/// Largest representable double-precision floating-point number.
	constexpr double   kDoubleLargest  = 1.7976931348623158e+308;

	// Double-precision floating-point infinity.
//	constexpr double   kDoubleInfinity;

	/// The reciprocal of ln(2). 1/ln(2)
	constexpr double   kDoubleInvLn2       = 1.44269504088896338700;

	/// The reciprocal of ln(10). 1/ln(10)
	constexpr double   kDoubleInvLn10      = 4.34294481903251827651e-01;

	/// Leading part of ln(2), with the trailing mantissa bits cleared.
	constexpr double   kDoubleLn2Hi        = 6.93147180369123816490e-01;

	/// What is left of ln(2) after kDoubleLn2Hi.
	constexpr double   kDoubleLn2Lo        = 1.90821492927058770002e-10;

	/// Largest argument exp can answer with a finite number.
	constexpr double   kDoubleExpOverflow  =  7.09782712893383973096e+02;

	/// Smallest argument exp can answer with a non-zero number.
	constexpr double   kDoubleExpUnderflow = -7.45133219101941108420e+02;

	/// Exponent of the square root of the scale expHalf splits its result by.
	constexpr int32_t  kDoubleExpHalfShift = 2043;

	/// Argument reduction expHalf pairs with kDoubleExpHalfShift. ln(2)*kDoubleExpHalfShift/2
	constexpr double   kDoubleExpHalfLn2   = 1.41609968988396828e+03;

	/// Below this the hyperbolic functions take their series instead of exp.
	constexpr double   kDoubleHypSmall     = 0.03125;

	/// Above this exp(-_a) no longer contributes to sinh and cosh.
	constexpr double   kDoubleHypBig       = 22.0;

	/// Above this half of exp(_a) overflows, and expHalf takes over.
	constexpr double   kDoubleHypNoFit     = 709.0;

	/// Above this tanh is 1.
	constexpr double   kDoubleTanhLarge    = 20.0;

	/// Below this the arc hyperbolic functions answer with the argument.
	constexpr double   kDoubleArcHypTiny   = 3.72529029846191406250e-09;

	/// Above this asinh and acosh are log(_a) + ln(2).
	constexpr double   kDoubleArcHypHuge   = 2.68435456e+08;

	/// Seed for the reciprocal square root iteration.
	constexpr uint64_t kDoubleRsqrtSeed    = UINT64_C(0x5fe6eb50c7b537a9);

} // namespace bx

#endif // BX_CONSTANTS_H_HEADER_GUARD
