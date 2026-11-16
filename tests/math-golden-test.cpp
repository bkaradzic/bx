/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/math.h>

#include <stdio.h>

namespace
{
	typedef float  (*F32Fn1)(float);
	typedef float  (*F32Fn2)(float, float);
	typedef float  (*F32Fn3)(float, float, float);
	typedef double (*F64Fn1)(double);
	typedef double (*F64Fn2)(double, double);
	typedef double (*F64Fn3)(double, double, double);

	constexpr uint32_t kNumGoldenInputs = 1<<16;
	constexpr uint32_t kNumConstInputs  = 128;
	constexpr bool     kFma             = bx::kFmaSupported;
	constexpr bool     kMsvcX87         = BX_COMPILER_MSVC && BX_CPU_X86 && BX_ARCH_32BIT;

	constexpr uint32_t kEdgeF32[] =
	{
		0x00000000, 0x80000000, 0x3f800000, 0xbf800000, 0x3f000000, 0xbf000000, 0x3fc00000, 0xbfc00000,
		0x40200000, 0xc0200000, 0x7f800000, 0xff800000, 0x7fc00000, 0x00800000, 0x80800000, 0x00000001,
		0x80000001, 0x007fffff, 0x807fffff, 0x7f7fffff, 0xff7fffff, 0x4b000000, 0xcb000000, 0x4b000001,
		0x4affffff, 0xcaffffff, 0x4b800000, 0x4f000000, 0xcf000000, 0x3effffff, 0xbeffffff, 0x3f7fffff,
		0x40490fdb, 0xc0490fdb, 0x3fc90fdb, 0xbfc90fdb, 0x40c90fdb, 0x33d6bf95, 0x501502f9, 0x42b17218,
		0xc2aeac50, 0x3e800000, 0x40400000, 0x42c80000, 0x358637bd, 0x7f000000, 0x3f8ccccd, 0x3f733333,
	};

	constexpr uint64_t kEdgeF64[] =
	{
		UINT64_C(0x0000000000000000), UINT64_C(0x8000000000000000), UINT64_C(0x3ff0000000000000), UINT64_C(0xbff0000000000000),
		UINT64_C(0x3fe0000000000000), UINT64_C(0xbfe0000000000000), UINT64_C(0x3ff8000000000000), UINT64_C(0xbff8000000000000),
		UINT64_C(0x4004000000000000), UINT64_C(0xc004000000000000), UINT64_C(0x7ff0000000000000), UINT64_C(0xfff0000000000000),
		UINT64_C(0x7ff8000000000000), UINT64_C(0x0010000000000000), UINT64_C(0x0000000000000001), UINT64_C(0x000fffffffffffff),
		UINT64_C(0x7fefffffffffffff), UINT64_C(0x4330000000000000), UINT64_C(0xc330000000000000), UINT64_C(0x4330000000000001),
		UINT64_C(0x432fffffffffffff), UINT64_C(0x43e0000000000000), UINT64_C(0x3fdfffffffffffff), UINT64_C(0x400921fb54442d18),
		UINT64_C(0x3ff921fb54442d18), UINT64_C(0x401921fb54442d18), UINT64_C(0x40862e42fefa39ef), UINT64_C(0x3fd0000000000000),
	};

	constexpr uint32_t mix32(uint32_t _a)
	{
		_a ^= _a >> 16;
		_a *= 0x7feb352d;
		_a ^= _a >> 15;
		_a *= 0x846ca68b;
		_a ^= _a >> 16;
		return _a;
	}

	constexpr uint64_t mix64(uint64_t _a)
	{
		_a ^= _a >> 30;
		_a *= UINT64_C(0xbf58476d1ce4e5b9);
		_a ^= _a >> 27;
		_a *= UINT64_C(0x94d049bb133111eb);
		_a ^= _a >> 31;
		return _a;
	}

	constexpr float inputF32(uint32_t _idx)
	{
		if (_idx < BX_COUNTOF(kEdgeF32) )
		{
			return bx::bitsToFloat(kEdgeF32[_idx]);
		}

		const uint32_t bits = mix32(_idx);

		if (0 != (_idx & 1) )
		{
			return bx::bitsToFloat(bits);
		}

		return float(bits >> 8) * (128.0f / 16777216.0f) - 64.0f;
	}

	constexpr double inputF64(uint32_t _idx)
	{
		if (_idx < BX_COUNTOF(kEdgeF64) )
		{
			return bx::bitsToDouble(kEdgeF64[_idx]);
		}

		const uint64_t bits = mix64(_idx);

		if (0 != (_idx & 1) )
		{
			return bx::bitsToDouble(bits);
		}

		return double(bits >> 11) * (128.0 / 9007199254740992.0) - 64.0;
	}

	constexpr uint64_t hashBits(uint64_t _hash, uint64_t _bits, uint32_t _numBytes)
	{
		for (uint32_t ii = 0; ii < _numBytes; ++ii)
		{
			_hash ^= (_bits >> (ii*8) ) & 0xff;
			_hash *= UINT64_C(0x100000001b3);
		}

		return _hash;
	}

	constexpr uint64_t kHashSeed = UINT64_C(0xcbf29ce484222325);

	constexpr uint64_t hashResult(uint64_t _hash, float _result)
	{
		const uint32_t bits = bx::isNan(_result) ? 0x7fc00000 : bx::floatToBits(_result);
		return hashBits(_hash, bits, 4);
	}

	constexpr uint64_t hashResult(uint64_t _hash, double _result)
	{
		const uint64_t bits = bx::isNan(_result) ? UINT64_C(0x7ff8000000000000) : bx::doubleToBits(_result);
		return hashBits(_hash, bits, 8);
	}

	constexpr uint32_t other(uint32_t _idx, uint32_t _salt)
	{
		return mix32(_idx*0x9e3779b9u + _salt) % kNumGoldenInputs;
	}

	struct WideInput
	{
		constexpr float  f32(uint32_t _idx) const { return inputF32(_idx); }
		constexpr double f64(uint32_t _idx) const { return inputF64(_idx); }
	};

	struct RangeInput
	{
		double lo;
		double hi;

		constexpr float  f32(uint32_t _idx) const { return float(lo + (hi - lo)*double(mix32(_idx) >> 8)*(1.0/16777216.0) ); }
		constexpr double f64(uint32_t _idx) const { return lo + (hi - lo)*double(mix64(_idx) >> 11)*(1.0/9007199254740992.0); }
	};

	template<typename InputT>
	constexpr uint64_t hashResults(F32Fn1 _fn, uint32_t _num, const InputT& _in)
	{
		uint64_t hash = kHashSeed;

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			hash = hashResult(hash, _fn(_in.f32(ii)) );
		}

		return hash;
	}

	template<typename InputT>
	constexpr uint64_t hashResults(F32Fn2 _fn, uint32_t _num, const InputT& _in)
	{
		uint64_t hash = kHashSeed;

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			hash = hashResult(hash, _fn(_in.f32(ii), _in.f32(other(ii, 1) )) );
		}

		return hash;
	}

	template<typename InputT>
	constexpr uint64_t hashResults(F32Fn3 _fn, uint32_t _num, const InputT& _in)
	{
		uint64_t hash = kHashSeed;

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			hash = hashResult(hash, _fn(_in.f32(ii), _in.f32(other(ii, 1) ), _in.f32(other(ii, 2) )) );
		}

		return hash;
	}

	template<typename InputT>
	constexpr uint64_t hashResults(F64Fn1 _fn, uint32_t _num, const InputT& _in)
	{
		uint64_t hash = kHashSeed;

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			hash = hashResult(hash, _fn(_in.f64(ii)) );
		}

		return hash;
	}

	template<typename InputT>
	constexpr uint64_t hashResults(F64Fn2 _fn, uint32_t _num, const InputT& _in)
	{
		uint64_t hash = kHashSeed;

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			hash = hashResult(hash, _fn(_in.f64(ii), _in.f64(other(ii, 1) )) );
		}

		return hash;
	}

	template<typename InputT>
	constexpr uint64_t hashResults(F64Fn3 _fn, uint32_t _num, const InputT& _in)
	{
		uint64_t hash = kHashSeed;

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			hash = hashResult(hash, _fn(_in.f64(ii), _in.f64(other(ii, 1) ), _in.f64(other(ii, 2) )) );
		}

		return hash;
	}

	constexpr uint64_t orderedDistance(uint64_t _a, uint64_t _b, uint64_t _signMask)
	{
		const uint64_t magA = _a & ~_signMask;
		const uint64_t magB = _b & ~_signMask;

		if ( (_a & _signMask) == (_b & _signMask) )
		{
			return magA > magB ? magA - magB : magB - magA;
		}

		return magA + magB;
	}

	constexpr uint64_t ulpDistance(float _a, float _b)
	{
		if (bx::isNan(_a)
		||  bx::isNan(_b) )
		{
			return bx::isNan(_a) == bx::isNan(_b) ? 0 : UINT64_MAX;
		}

		return orderedDistance(bx::floatToBits(_a), bx::floatToBits(_b), bx::kFloatSignMask);
	}

	constexpr uint64_t ulpDistance(double _a, double _b)
	{
		if (bx::isNan(_a)
		||  bx::isNan(_b) )
		{
			return bx::isNan(_a) == bx::isNan(_b) ? 0 : UINT64_MAX;
		}

		return orderedDistance(bx::doubleToBits(_a), bx::doubleToBits(_b), bx::kDoubleSignMask);
	}

	uint64_t maxUlpDistance(F32Fn1 _fast, F32Fn1 _exact)
	{
		uint64_t result = 0;

		for (uint32_t ii = 0; ii < kNumGoldenInputs; ++ii)
		{
			const float aa = inputF32(ii);

			result = bx::max(result, ulpDistance(_fast(aa), _exact(aa) ) );
		}

		return result;
	}

	uint64_t maxUlpDistance(F32Fn2 _fast, F32Fn2 _exact)
	{
		uint64_t result = 0;

		for (uint32_t ii = 0; ii < kNumGoldenInputs; ++ii)
		{
			const float aa = inputF32(ii);
			const float bb = inputF32(other(ii, 1) );

			result = bx::max(result, ulpDistance(_fast(aa, bb), _exact(aa, bb) ) );
		}

		return result;
	}

	uint64_t maxUlpDistance(F64Fn1 _fast, F64Fn1 _exact)
	{
		uint64_t result = 0;

		for (uint32_t ii = 0; ii < kNumGoldenInputs; ++ii)
		{
			const double aa = inputF64(ii);

			result = bx::max(result, ulpDistance(_fast(aa), _exact(aa) ) );
		}

		return result;
	}

	uint64_t maxUlpDistance(F64Fn2 _fast, F64Fn2 _exact)
	{
		uint64_t result = 0;

		for (uint32_t ii = 0; ii < kNumGoldenInputs; ++ii)
		{
			const double aa = inputF64(ii);
			const double bb = inputF64(other(ii, 1) );

			result = bx::max(result, ulpDistance(_fast(aa, bb), _exact(aa, bb) ) );
		}

		return result;
	}

	constexpr float ldexpF32(float _a, float _b)
	{
		return bx::ldexp(_a, int32_t(bx::floatToBits(_b) % 64) - 32);
	}

	constexpr double ldexpF64(double _a, double _b)
	{
		return bx::ldexp(_a, int32_t(bx::doubleToBits(_b) % 512) - 256);
	}

	struct GoldenF32Fn1 { const char* name; F32Fn1 fn; uint64_t hash; bool sameAtCompileTime = true; double lo = 1.0/32.0; double hi = 1.0; };
	struct GoldenF32Fn2 { const char* name; F32Fn2 fn; uint64_t hash; bool sameAtCompileTime = true; double lo = 1.0/32.0; double hi = 1.0; };
	struct GoldenF32Fn3 { const char* name; F32Fn3 fn; uint64_t hash; bool sameAtCompileTime = true; double lo = 1.0/32.0; double hi = 1.0; };
	struct GoldenF64Fn1 { const char* name; F64Fn1 fn; uint64_t hash; bool sameAtCompileTime = true; double lo = 1.0/32.0; double hi = 1.0; };
	struct GoldenF64Fn2 { const char* name; F64Fn2 fn; uint64_t hash; bool sameAtCompileTime = true; double lo = 1.0/32.0; double hi = 1.0; };
	struct GoldenF64Fn3 { const char* name; F64Fn3 fn; uint64_t hash; bool sameAtCompileTime = true; double lo = 1.0/32.0; double hi = 1.0; };

	constexpr GoldenF32Fn1 kGoldenF32Fn1[] =
	{
		{ "abs",        bx::abs,        UINT64_C(0xd8d99d4a4d705425) },
		{ "acosExact",  bx::acosExact,  UINT64_C(0x268ec00da598a212) },
		{ "acoshExact", bx::acoshExact, UINT64_C(0x3bfad3825b9e7f5b), true, 1.0, 8.0 },
		{ "asinExact",  bx::asinExact,  UINT64_C(0x1ad744f70073dbac) },
		{ "asinhExact", bx::asinhExact, UINT64_C(0x998dd5ec43ba03ed) },
		{ "atanExact",  bx::atanExact,  UINT64_C(0xd4765f882f61b081) },
		{ "atanhExact", bx::atanhExact, UINT64_C(0x7c571b9bc37a8725) },
		{ "ceil",       bx::ceil,       UINT64_C(0x8601f4fc787f2b2d) },
		{ "cosExact",   bx::cosExact,   UINT64_C(0x200ed89141e486c9) },
		{ "coshExact",  bx::coshExact,  UINT64_C(0x622653145e76eeea) },
		{ "expExact",   bx::expExact,   UINT64_C(0x70799c6964c47805) },
		{ "exp2Exact",  bx::exp2Exact,  UINT64_C(0x21407ef41dc12ec6) },
		{ "floor",      bx::floor,      UINT64_C(0x2ef255f3d7615783) },
		{ "fract",      bx::fract,      UINT64_C(0x239cb0614df58193) },
		{ "logExact",   bx::logExact,   UINT64_C(0xa22a160d28a365ca) },
		{ "log10Exact", bx::log10Exact, UINT64_C(0xa2bb60aa99b4f324) },
		{ "log1pExact", bx::log1pExact, UINT64_C(0x36e6e732f2568ad7) },
		{ "log2Exact",  bx::log2Exact,  UINT64_C(0x27ef1b1833054b42) },
		{ "rcp",        bx::rcp,        UINT64_C(0x5aab5ec5f6a39914) },
		{ "round",      bx::round,      UINT64_C(0x28ed4f7a24b9ff23) },
		{ "rsqrt",      bx::rsqrt,      UINT64_C(0x60a2508edcfea66f) },
		{ "sign",       bx::sign,       UINT64_C(0x2a71b707029acb45) },
		{ "sinExact",   bx::sinExact,   UINT64_C(0xe1bcaffbf1e22247) },
		{ "sinhExact",  bx::sinhExact,  UINT64_C(0x52c84457c625491e) },
		{ "sqrt",       bx::sqrt,       UINT64_C(0xa7ea2e575a4659ee) },
		{ "square",     bx::square,     UINT64_C(0x9e4997a866441cbd) },
		{ "tanExact",   bx::tanExact,   UINT64_C(0x411142447aab9d5e) },
		{ "tanhExact",  bx::tanhExact,  UINT64_C(0x4443e614ff58ff03) },
		{ "trunc",      bx::trunc,      UINT64_C(0x8a952d0b6e1df591) },
	};

	constexpr GoldenF32Fn2 kGoldenF32Fn2[] =
	{
		{ "atan2Exact", bx::atan2Exact, UINT64_C(0x42f7d34a0f87c322) },
		{ "copySign",   bx::copySign,   UINT64_C(0x671cd3730f601925) },
		{ "fmod",       bx::fmod,       kFma ? UINT64_C(0xfee51a4cef6e28ea) : UINT64_C(0x34fe13c1da74191b) },
		{ "fmodExact",  bx::fmodExact,  UINT64_C(0xfee51a4cef6e28ea) },
		{ "ldexp",      ldexpF32,       UINT64_C(0x72ee0f3dedbe4b6f) },
		{ "mod",        bx::mod,        kFma ? UINT64_C(0xa370ff6b4ca56966) : UINT64_C(0x4ba0f87880524287) },
		{ "modExact",   bx::modExact,   UINT64_C(0xa370ff6b4ca56966) },
		{ "powExact",   bx::powExact,   UINT64_C(0xb559d082cc25a244) },
		{ "step",       bx::step,       UINT64_C(0x7719a06569d6d718) },
	};

	constexpr GoldenF32Fn3 kGoldenF32Fn3[] =
	{
		{ "lerp",      bx::lerp,      kFma ? UINT64_C(0xb00208993933430d) : UINT64_C(0x1d7a365be9a8662f) },
		{ "lerpExact", bx::lerpExact, UINT64_C(0xb00208993933430d) },
		{ "mad",       bx::mad,       kFma ? UINT64_C(0xcd81adc3dca7aeca) : UINT64_C(0xe1387bf7d435967a) },
		{ "madExact",  bx::madExact,  UINT64_C(0xcd81adc3dca7aeca) },
		{ "nms",       bx::nms,       kFma ? UINT64_C(0x02c17982bd8eeca0) : UINT64_C(0x20c5eb43fc11c5cd) },
		{ "nmsExact",  bx::nmsExact,  UINT64_C(0x02c17982bd8eeca0) },
	};

	constexpr GoldenF64Fn1 kGoldenF64Fn1[] =
	{
		{ "abs",        bx::abs,        UINT64_C(0x124be518394a658e) },
		{ "acosExact",  bx::acosExact,  UINT64_C(0x1a568864eb56d7b5) },
		{ "acoshExact", bx::acoshExact, UINT64_C(0x4bc899121911b3f8), true, 1.0, 8.0 },
		{ "asinExact",  bx::asinExact,  UINT64_C(0x274b7ee4ebe63de7) },
		{ "asinhExact", bx::asinhExact, UINT64_C(0xd7657c4bff6804b8) },
		{ "atanExact",  bx::atanExact,  UINT64_C(0xea1f145ef558422a) },
		{ "atanhExact", bx::atanhExact, UINT64_C(0x7ee62f620ba0b054) },
		{ "ceil",       bx::ceil,       UINT64_C(0x52360e1290ce7827) },
		{ "cosExact",   bx::cosExact,   UINT64_C(0x6fafe682201315f1) },
		{ "coshExact",  bx::coshExact,  UINT64_C(0xe7a2b7f307b0683a) },
		{ "expExact",   bx::expExact,   UINT64_C(0x6f4f5310b8c9b50a) },
		{ "exp2Exact",  bx::exp2Exact,  UINT64_C(0x534ff614d98eb392) },
		{ "floor",      bx::floor,      UINT64_C(0xae4e5643852439c8) },
		{ "fract",      bx::fract,      UINT64_C(0xde5d46c2e5585f04) },
		{ "logExact",   bx::logExact,   UINT64_C(0xa7b890f9917140d1) },
		{ "log10Exact", bx::log10Exact, UINT64_C(0x71f1ec8311074972) },
		{ "log1pExact", bx::log1pExact, UINT64_C(0x8cbdbab07831572d) },
		{ "log2Exact",  bx::log2Exact,  UINT64_C(0xbfd43d89e66610b2) },
		{ "round",      bx::round,      UINT64_C(0x203a4b41e0e28aea) },
		{ "rsqrt",      bx::rsqrt,      UINT64_C(0x07498b7a2e8c2aec) },
		{ "sign",       bx::sign,       UINT64_C(0xd7e74b3038064c58) },
		{ "sinExact",   bx::sinExact,   UINT64_C(0x7b628a30d35ff0f2) },
		{ "sinhExact",  bx::sinhExact,  UINT64_C(0x54a3f65ad7958969) },
		{ "sqrt",       bx::sqrt,       UINT64_C(0xef59f892df1713ea) },
		{ "square",     bx::square,     UINT64_C(0xd385961b09a08641) },
		{ "tanExact",   bx::tanExact,   UINT64_C(0x826bdb350ad9f43b) },
		{ "tanhExact",  bx::tanhExact,  UINT64_C(0x6a9eaea0c432c083) },
		{ "trunc",      bx::trunc,      UINT64_C(0x316b02732466b87e) },
	};

	constexpr GoldenF64Fn2 kGoldenF64Fn2[] =
	{
		{ "atan2Exact", bx::atan2Exact, UINT64_C(0xb78fa46f4af46050) },
		{ "copySign",   bx::copySign,   UINT64_C(0x6dca3d815d5a070e) },
		{ "fmodExact",  bx::fmodExact,  UINT64_C(0xde3f46b93d6e9d8a) },
		{ "ldexp",      ldexpF64,       UINT64_C(0x8a87472c86119f2e) },
		{ "modExact",   bx::modExact,   UINT64_C(0x1b99e5cfaffc4e5e) },
		{ "powExact",   bx::powExact,   UINT64_C(0x12d1d36cce65da78) },
	};

	constexpr GoldenF64Fn3 kGoldenF64Fn3[] =
	{
		{ "madExact", bx::madExact, UINT64_C(0xf06e9d0633c9ccef) },
		{ "nmsExact", bx::nmsExact, UINT64_C(0xe26e9ff2ceb7c32a) },
	};

	template<typename GoldenT, uint32_t NumT>
	uint32_t checkGolden(const char* _type, const GoldenT (&_golden)[NumT])
	{
		uint32_t numFailed = 0;

		for (uint32_t ii = 0; ii < NumT; ++ii)
		{
			const uint64_t hash = hashResults(_golden[ii].fn, kNumGoldenInputs, WideInput{});

			if (hash != _golden[ii].hash)
			{
				printf("%s %s: hash 0x%016llx, expected 0x%016llx\n"
					, _type
					, _golden[ii].name
					, (unsigned long long)hash
					, (unsigned long long)_golden[ii].hash
					);
				++numFailed;
			}
		}

		return numFailed;
	}

	struct FastF32Fn1 { const char* name; F32Fn1 fast; F32Fn1 exact; uint64_t maxUlp; };
	struct FastF32Fn2 { const char* name; F32Fn2 fast; F32Fn2 exact; uint64_t maxUlp; };
	struct FastF64Fn1 { const char* name; F64Fn1 fast; F64Fn1 exact; uint64_t maxUlp; };
	struct FastF64Fn2 { const char* name; F64Fn2 fast; F64Fn2 exact; uint64_t maxUlp; };

	constexpr float acosExactClamped(float _a)
	{
		return bx::isNan(_a) ? _a : bx::acosExact(bx::clamp(_a, -1.0f, 1.0f) );
	}

	constexpr float asinExactClamped(float _a)
	{
		return bx::isNan(_a) ? _a : bx::asinExact(bx::clamp(_a, -1.0f, 1.0f) );
	}

	constexpr double acosExactClamped(double _a)
	{
		return bx::isNan(_a) ? _a : bx::acosExact(bx::clamp(_a, -1.0, 1.0) );
	}

	constexpr double asinExactClamped(double _a)
	{
		return bx::isNan(_a) ? _a : bx::asinExact(bx::clamp(_a, -1.0, 1.0) );
	}

	constexpr FastF32Fn1 kFastF32Fn1[] =
	{
		{ "acos",  bx::acos,  acosExactClamped, 4 },
		{ "acosh", bx::acosh, bx::acoshExact,   4 },
		{ "asin",  bx::asin,  asinExactClamped, 4 },
		{ "asinh", bx::asinh, bx::asinhExact,   4 },
		{ "atan",  bx::atan,  bx::atanExact,    4 },
		{ "atanh", bx::atanh, bx::atanhExact,   4 },
		{ "cos",   bx::cos,   bx::cosExact,     4 },
		{ "cosh",  bx::cosh,  bx::coshExact,    4 },
		{ "exp",   bx::exp,   bx::expExact,     4 },
		{ "exp2",  bx::exp2,  bx::exp2Exact,    4 },
		{ "log",   bx::log,   bx::logExact,     4 },
		{ "log10", bx::log10, bx::log10Exact,   4 },
		{ "log1p", bx::log1p, bx::log1pExact,   4 },
		{ "log2",  bx::log2,  bx::log2Exact,    4 },
		{ "sin",   bx::sin,   bx::sinExact,     4 },
		{ "sinh",  bx::sinh,  bx::sinhExact,    4 },
		{ "tan",   bx::tan,   bx::tanExact,     4 },
		{ "tanh",  bx::tanh,  bx::tanhExact,    8 },
	};

	constexpr FastF32Fn2 kFastF32Fn2[] =
	{
		{ "atan2", bx::atan2, bx::atan2Exact, 4 },
		{ "pow",   bx::pow,   bx::powExact,   128 },
	};

	constexpr FastF64Fn1 kFastF64Fn1[] =
	{
		{ "acos",  bx::acos,  acosExactClamped, 4 },
		{ "acosh", bx::acosh, bx::acoshExact,   4 },
		{ "asin",  bx::asin,  asinExactClamped, 4 },
		{ "asinh", bx::asinh, bx::asinhExact,   4 },
		{ "atan",  bx::atan,  bx::atanExact,    4 },
		{ "atanh", bx::atanh, bx::atanhExact,   4 },
		{ "cos",   bx::cos,   bx::cosExact,     4 },
		{ "cosh",  bx::cosh,  bx::coshExact,    4 },
		{ "exp",   bx::exp,   bx::expExact,     4 },
		{ "exp2",  bx::exp2,  bx::exp2Exact,    4 },
		{ "log",   bx::log,   bx::logExact,     4 },
		{ "log10", bx::log10, bx::log10Exact,   4 },
		{ "log1p", bx::log1p, bx::log1pExact,   4 },
		{ "log2",  bx::log2,  bx::log2Exact,    4 },
		{ "sin",   bx::sin,   bx::sinExact,     4 },
		{ "sinh",  bx::sinh,  bx::sinhExact,    4 },
		{ "tan",   bx::tan,   bx::tanExact,     4 },
		{ "tanh",  bx::tanh,  bx::tanhExact,    8 },
	};

	constexpr FastF64Fn2 kFastF64Fn2[] =
	{
		{ "atan2", bx::atan2, bx::atan2Exact, 4 },
		{ "pow",   bx::pow,   bx::powExact,   128 },
	};

	template<typename FastT, uint32_t NumT>
	uint32_t checkFast(const char* _type, const FastT (&_fast)[NumT])
	{
		uint32_t numFailed = 0;

		for (uint32_t ii = 0; ii < NumT; ++ii)
		{
			const uint64_t ulp = maxUlpDistance(_fast[ii].fast, _fast[ii].exact);

			if (ulp > _fast[ii].maxUlp)
			{
				printf("%s %s: %llu ulp from exact, expected at most %llu\n"
					, _type
					, _fast[ii].name
					, (unsigned long long)ulp
					, (unsigned long long)_fast[ii].maxUlp
					);
				++numFailed;
			}
		}

		return numFailed;
	}

	volatile uint32_t s_numConstInputs = kNumConstInputs;

	template<const auto& GoldenT, uint32_t IdxT = 0>
	uint32_t checkConst(const char* _type)
	{
		if constexpr (IdxT < BX_COUNTOF(GoldenT) )
		{
			constexpr RangeInput kRange = { GoldenT[IdxT].lo, GoldenT[IdxT].hi };
			constexpr uint64_t   kConst = hashResults(GoldenT[IdxT].fn, kNumConstInputs, kRange);
			const uint64_t       hash   = hashResults(GoldenT[IdxT].fn, s_numConstInputs, kRange);

			const bool same   = kConst == hash;
			const bool failed = same != GoldenT[IdxT].sameAtCompileTime;

			if (failed)
			{
				printf("%s %s: run time hash 0x%016llx, compile time 0x%016llx, expected %s\n"
					, _type
					, GoldenT[IdxT].name
					, (unsigned long long)hash
					, (unsigned long long)kConst
					, GoldenT[IdxT].sameAtCompileTime ? "same" : "different"
					);
			}

			return uint32_t(failed) + checkConst<GoldenT, IdxT + 1>(_type);
		}
		else
		{
			return 0;
		}
	}

} // namespace

TEST_CASE("Math results are bit identical on every platform", "[math][golden]")
{
	if (kMsvcX87)
	{
		SKIP("MSVC 32-bit x86 evaluates through x87.");
	}

	uint32_t numFailed = 0;
	numFailed += checkGolden("float",  kGoldenF32Fn1);
	numFailed += checkGolden("float",  kGoldenF32Fn2);
	numFailed += checkGolden("float",  kGoldenF32Fn3);
	numFailed += checkGolden("double", kGoldenF64Fn1);
	numFailed += checkGolden("double", kGoldenF64Fn2);
	numFailed += checkGolden("double", kGoldenF64Fn3);
	REQUIRE(0 == numFailed);
}

TEST_CASE("Math results are the same at compile time and run time", "[math][golden]")
{
	if (kMsvcX87)
	{
		SKIP("MSVC 32-bit x86 evaluates through x87.");
	}

	uint32_t numFailed = 0;
	numFailed += checkConst<kGoldenF32Fn1>("float");
	numFailed += checkConst<kGoldenF32Fn2>("float");
	numFailed += checkConst<kGoldenF32Fn3>("float");
	numFailed += checkConst<kGoldenF64Fn1>("double");
	numFailed += checkConst<kGoldenF64Fn2>("double");
	numFailed += checkConst<kGoldenF64Fn3>("double");
	REQUIRE(0 == numFailed);
}

TEST_CASE("Fast math results are close to exact", "[math][golden]")
{
	uint32_t numFailed = 0;
	numFailed += checkFast("float",  kFastF32Fn1);
	numFailed += checkFast("float",  kFastF32Fn2);
	numFailed += checkFast("double", kFastF64Fn1);
	numFailed += checkFast("double", kFastF64Fn2);
	REQUIRE(0 == numFailed);
}
