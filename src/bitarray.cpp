/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include <bx/bitarray.h>
#include <bx/simd_t.h>
#include <bx/debug.h>

namespace bx
{
	namespace
	{
#if BX_SIMD_AVX2
		typedef simd256_t SimdWideT;
#else
		typedef simd128_t SimdWideT;
#endif // BX_SIMD_AVX2

		constexpr uint32_t kNumSimdWords  = sizeof(SimdWideT)/sizeof(uint64_t);
		constexpr uint32_t kNumSimdDwords = sizeof(SimdWideT)/sizeof(uint32_t);
		constexpr uint32_t kSimdDwordMask = (1u << kNumSimdDwords) - 1;

		struct Match
		{
			enum Enum
			{
				Zero,
				NonZero,
				NotFull,
			};
		};

		template<Match::Enum MatchT>
		BX_FORCE_INLINE bool isMatch(uint64_t _word)
		{
			if constexpr (Match::Zero == MatchT)
			{
				return 0 == _word;
			}
			else if constexpr (Match::NonZero == MatchT)
			{
				return 0 != _word;
			}
			else
			{
				return UINT64_MAX != _word;
			}
		}

		template<Match::Enum MatchT>
		BX_FORCE_INLINE simd32_t matchLanes(SimdWideT _a)
		{
			const SimdWideT ref       = Match::NotFull == MatchT ? simd_splat<SimdWideT>(UINT32_MAX) : simd_zero<SimdWideT>();
			const SimdWideT equal     = simd_i32_cmpeq(_a, ref);
			const simd32_t  mask      = simd32_ld(uint32_t(simd_x32_signbitsmask(equal) ) );
			const simd32_t  maskHi    = simd32_x32_srl(mask, 1);
			const simd32_t  wordLanes = simd32_splat(uint32_t(0x55555555 & kSimdDwordMask) );
			const simd32_t  both      = simd32_and(mask, maskHi);
			const simd32_t  equalBoth = simd32_and(both, wordLanes);

			if constexpr (Match::Zero == MatchT)
			{
				return equalBoth;
			}
			else
			{
				return simd32_andc(wordLanes, equalBoth);
			}
		}

		template<Match::Enum MatchT>
		BX_FORCE_INLINE bool anyMatch(SimdWideT _a0, SimdWideT _a1, SimdWideT _a2, SimdWideT _a3)
		{
			if constexpr (Match::Zero == MatchT)
			{
				const simd32_t m0  = matchLanes<MatchT>(_a0);
				const simd32_t m1  = matchLanes<MatchT>(_a1);
				const simd32_t m2  = matchLanes<MatchT>(_a2);
				const simd32_t m3  = matchLanes<MatchT>(_a3);
				const simd32_t m01 = simd32_or(m0, m1);
				const simd32_t m23 = simd32_or(m2, m3);
				const simd32_t any = simd32_or(m01, m23);
				return 0 != any.u32;
			}
			else if constexpr (Match::NonZero == MatchT)
			{
				const SimdWideT a01 = simd_or(_a0, _a1);
				const SimdWideT a23 = simd_or(_a2, _a3);
				const SimdWideT any = simd_or(a01, a23);
				return !simd_test_zero(any, any);
			}
			else
			{
				const SimdWideT a01 = simd_and(_a0, _a1);
				const SimdWideT a23 = simd_and(_a2, _a3);
				const SimdWideT all = simd_and(a01, a23);
				const SimdWideT inv = simd_not(all);
				return !simd_test_zero(inv, inv);
			}
		}

		template<Match::Enum MatchT>
		inline uint32_t findWordRef(const uint64_t* _ptr, uint32_t _idx, uint32_t _end)
		{
			for (; _idx < _end; ++_idx)
			{
				if (isMatch<MatchT>(_ptr[_idx]) )
				{
					return _idx;
				}
			}

			return _end;
		}

		template<Match::Enum MatchT>
		BX_NO_INLINE uint32_t findWordSimd(const uint64_t* _ptr, uint32_t _idx, uint32_t _end)
		{
			for (; _idx + 4*kNumSimdWords <= _end; _idx += 4*kNumSimdWords)
			{
				const SimdWideT a0 = simd_ldu<SimdWideT>(&_ptr[_idx                  ]);
				const SimdWideT a1 = simd_ldu<SimdWideT>(&_ptr[_idx +   kNumSimdWords]);
				const SimdWideT a2 = simd_ldu<SimdWideT>(&_ptr[_idx + 2*kNumSimdWords]);
				const SimdWideT a3 = simd_ldu<SimdWideT>(&_ptr[_idx + 3*kNumSimdWords]);

				if (anyMatch<MatchT>(a0, a1, a2, a3) )
				{
					break;
				}
			}

			for (; _idx + kNumSimdWords <= _end; _idx += kNumSimdWords)
			{
				const SimdWideT test  = simd_ldu<SimdWideT>(&_ptr[_idx]);
				const simd32_t  lanes = matchLanes<MatchT>(test);

				if (0 != lanes.u32)
				{
					return _idx + countTrailingZeros(lanes.u32)/2;
				}
			}

			return findWordRef<MatchT>(_ptr, _idx, _end);
		}

		template<Match::Enum MatchT>
		inline uint32_t findLastWordRef(const uint64_t* _ptr, uint32_t _begin, uint32_t _idx)
		{
			for (; _idx > _begin; --_idx)
			{
				if (isMatch<MatchT>(_ptr[_idx-1]) )
				{
					return _idx-1;
				}
			}

			return kInvalid;
		}

		template<Match::Enum MatchT>
		BX_NO_INLINE uint32_t findLastWordSimd(const uint64_t* _ptr, uint32_t _begin, uint32_t _idx)
		{
			for (; _idx >= _begin + 4*kNumSimdWords; _idx -= 4*kNumSimdWords)
			{
				const SimdWideT a0 = simd_ldu<SimdWideT>(&_ptr[_idx - 4*kNumSimdWords]);
				const SimdWideT a1 = simd_ldu<SimdWideT>(&_ptr[_idx - 3*kNumSimdWords]);
				const SimdWideT a2 = simd_ldu<SimdWideT>(&_ptr[_idx - 2*kNumSimdWords]);
				const SimdWideT a3 = simd_ldu<SimdWideT>(&_ptr[_idx -   kNumSimdWords]);

				if (anyMatch<MatchT>(a0, a1, a2, a3) )
				{
					break;
				}
			}

			for (; _idx >= _begin + kNumSimdWords; _idx -= kNumSimdWords)
			{
				const SimdWideT test  = simd_ldu<SimdWideT>(&_ptr[_idx - kNumSimdWords]);
				const simd32_t  lanes = matchLanes<MatchT>(test);

				if (0 != lanes.u32)
				{
					return _idx - kNumSimdWords + (findLastSet(lanes.u32) - 1)/2;
				}
			}

			return findLastWordRef<MatchT>(_ptr, _begin, _idx);
		}

		template<Match::Enum MatchT, bool SimdT>
		BX_FORCE_INLINE uint32_t findWord(const uint64_t* _ptr, uint32_t _idx, uint32_t _end)
		{
			if constexpr (SimdT)
			{
				if (_end - _idx >= 4*kNumSimdWords)
				{
					return findWordSimd<MatchT>(_ptr, _idx, _end);
				}
			}

			return findWordRef<MatchT>(_ptr, _idx, _end);
		}

		template<Match::Enum MatchT, bool SimdT>
		BX_FORCE_INLINE uint32_t findLastWord(const uint64_t* _ptr, uint32_t _begin, uint32_t _end)
		{
			if constexpr (SimdT)
			{
				if (_end - _begin >= 4*kNumSimdWords)
				{
					return findLastWordSimd<MatchT>(_ptr, _begin, _end);
				}
			}

			return findLastWordRef<MatchT>(_ptr, _begin, _end);
		}

		constexpr uint32_t kSimdByteMask = uint32_t( (UINT64_C(1) << sizeof(SimdWideT) ) - 1);

		template<typename Ty>
		constexpr uint64_t kElementLowBits = 1 == sizeof(Ty) ? UINT64_C(0x0101010101010101)
			: 2 == sizeof(Ty) ? UINT64_C(0x0001000100010001)
			:                   UINT64_C(0x0000000100000001)
			;

		template<typename Ty>
		constexpr uint32_t kMaskBitsPerWord = 4 == sizeof(Ty) ? 2 : 8;

		template<typename Ty>
		BX_FORCE_INLINE bool hasZeroElement(uint64_t _word)
		{
			const uint64_t lowBits  = kElementLowBits<Ty>;
			const uint64_t highBits = lowBits << (sizeof(Ty)*8 - 1);
			const uint64_t borrow   = _word - lowBits;
			return 0 != (borrow & ~_word & highBits);
		}

		template<typename Ty>
		BX_FORCE_INLINE simd32_t zeroElementMask(SimdWideT _a)
		{
			if constexpr (4 == sizeof(Ty) )
			{
				const SimdWideT zero  = simd_zero<SimdWideT>();
				const SimdWideT equal = simd_i32_cmpeq(_a, zero);
				return simd32_ld(uint32_t(simd_x32_signbitsmask(equal) ) );
			}
			else
			{
				const SimdWideT bias     = simd_splat<SimdWideT>(uint32_t(0x7f7f7f7f) );
				const SimdWideT biased   = simd_u8_satadd(_a, bias);
				const simd32_t  nonZero  = simd32_ld(uint32_t(simd_x8_signbitsmask(biased) ) );
				const simd32_t  byteMask = simd32_splat(kSimdByteMask);
				const simd32_t  zero     = simd32_andc(byteMask, nonZero);

				if constexpr (1 == sizeof(Ty) )
				{
					return zero;
				}
				else
				{
					const simd32_t zeroHi    = simd32_x32_srl(zero, 1);
					const simd32_t pairLanes = simd32_splat(uint32_t(0x55555555) );
					const simd32_t both      = simd32_and(zero, zeroHi);
					return simd32_and(both, pairLanes);
				}
			}
		}

		template<typename Ty>
		inline uint32_t findZeroElementWordRef(const uint64_t* _ptr, uint32_t _idx, uint32_t _end)
		{
			for (; _idx < _end; ++_idx)
			{
				if (hasZeroElement<Ty>(_ptr[_idx]) )
				{
					return _idx;
				}
			}

			return _end;
		}

		template<typename Ty>
		BX_NO_INLINE uint32_t findZeroElementWordSimd(const uint64_t* _ptr, uint32_t _idx, uint32_t _end)
		{
			for (; _idx + 4*kNumSimdWords <= _end; _idx += 4*kNumSimdWords)
			{
				const SimdWideT a0  = simd_ldu<SimdWideT>(&_ptr[_idx                  ]);
				const SimdWideT a1  = simd_ldu<SimdWideT>(&_ptr[_idx +   kNumSimdWords]);
				const SimdWideT a2  = simd_ldu<SimdWideT>(&_ptr[_idx + 2*kNumSimdWords]);
				const SimdWideT a3  = simd_ldu<SimdWideT>(&_ptr[_idx + 3*kNumSimdWords]);
				const simd32_t  m0  = zeroElementMask<Ty>(a0);
				const simd32_t  m1  = zeroElementMask<Ty>(a1);
				const simd32_t  m2  = zeroElementMask<Ty>(a2);
				const simd32_t  m3  = zeroElementMask<Ty>(a3);
				const simd32_t  m01 = simd32_or(m0, m1);
				const simd32_t  m23 = simd32_or(m2, m3);
				const simd32_t  any = simd32_or(m01, m23);

				if (0 != any.u32)
				{
					break;
				}
			}

			for (; _idx + kNumSimdWords <= _end; _idx += kNumSimdWords)
			{
				const SimdWideT test = simd_ldu<SimdWideT>(&_ptr[_idx]);
				const simd32_t  mask = zeroElementMask<Ty>(test);

				if (0 != mask.u32)
				{
					return _idx + countTrailingZeros(mask.u32)/kMaskBitsPerWord<Ty>;
				}
			}

			return findZeroElementWordRef<Ty>(_ptr, _idx, _end);
		}

		template<typename Ty, bool SimdT>
		BX_FORCE_INLINE uint32_t findZeroElementWord(const uint64_t* _ptr, uint32_t _idx, uint32_t _end)
		{
			if constexpr (SimdT && 1 < sizeof(Ty) )
			{
				return findZeroElementWordSimd<Ty>(_ptr, _idx, _end);
			}
			else if constexpr (SimdT)
			{
				const uint32_t end = min(_end, _idx + 4);
				const uint32_t idx = findZeroElementWordRef<Ty>(_ptr, _idx, end);

				return idx != end || end == _end
					? idx
					: findZeroElementWordSimd<Ty>(_ptr, end, _end)
					;
			}
			else
			{
				return findZeroElementWordRef<Ty>(_ptr, _idx, _end);
			}
		}

		inline uint32_t countBitsWords(const uint64_t* _ptr, uint32_t _idx, uint32_t _end)
		{
			const uint64_t* ptr = _ptr + _idx;
			const uint64_t* end = _ptr + _end;

			uint32_t count0 = 0;
			uint32_t count1 = 0;
			uint32_t count2 = 0;
			uint32_t count3 = 0;

			for (; end - ptr >= 4; ptr += 4)
			{
				count0 += countBits(ptr[0]);
				count1 += countBits(ptr[1]);
				count2 += countBits(ptr[2]);
				count3 += countBits(ptr[3]);
			}

			for (; ptr < end; ++ptr)
			{
				count0 += countBits(ptr[0]);
			}

			return count0 + count1 + count2 + count3;
		}

	} // namespace

	uint32_t bitSetFindWordRef(const uint64_t* _ptr, uint32_t _beginWord, uint32_t _endWord, uint32_t _match)
	{
		switch (_match)
		{
		case Match::Zero:    return findWordRef<Match::Zero   >(_ptr, _beginWord, _endWord);
		case Match::NonZero: return findWordRef<Match::NonZero>(_ptr, _beginWord, _endWord);
		default:             return findWordRef<Match::NotFull>(_ptr, _beginWord, _endWord);
		}
	}

	uint32_t bitSetFindWordSimd(const uint64_t* _ptr, uint32_t _beginWord, uint32_t _endWord, uint32_t _match)
	{
		switch (_match)
		{
		case Match::Zero:    return findWordSimd<Match::Zero   >(_ptr, _beginWord, _endWord);
		case Match::NonZero: return findWordSimd<Match::NonZero>(_ptr, _beginWord, _endWord);
		default:             return findWordSimd<Match::NotFull>(_ptr, _beginWord, _endWord);
		}
	}

	uint32_t bitSetFindLastWordRef(const uint64_t* _ptr, uint32_t _beginWord, uint32_t _endWord, uint32_t _match)
	{
		switch (_match)
		{
		case Match::Zero:    return findLastWordRef<Match::Zero   >(_ptr, _beginWord, _endWord);
		case Match::NonZero: return findLastWordRef<Match::NonZero>(_ptr, _beginWord, _endWord);
		default:             return findLastWordRef<Match::NotFull>(_ptr, _beginWord, _endWord);
		}
	}

	uint32_t bitSetFindLastWordSimd(const uint64_t* _ptr, uint32_t _beginWord, uint32_t _endWord, uint32_t _match)
	{
		switch (_match)
		{
		case Match::Zero:    return findLastWordSimd<Match::Zero   >(_ptr, _beginWord, _endWord);
		case Match::NonZero: return findLastWordSimd<Match::NonZero>(_ptr, _beginWord, _endWord);
		default:             return findLastWordSimd<Match::NotFull>(_ptr, _beginWord, _endWord);
		}
	}

	inline void bitSetSet(uint64_t* _ptr, uint32_t _idx, bool _value)
	{
		const simd32_t mask63   = simd32_splat(uint32_t(63) );
		const simd32_t bit      = simd32_splat(_idx);
		const simd32_t idxVec   = simd32_x32_srl(bit, 6);
		const simd32_t shiftVec = simd32_and(bit, mask63);
		const uint32_t idx      = idxVec.u32;
		const uint32_t shift    = shiftVec.u32;
		const simd64_t zero     = simd64_zero();
		const simd64_t one      = simd64_splat(uint64_t(1) );
		const simd64_t mask     = simd64_x64_sll(one, int(shift) );
		const simd64_t cmask    = simd64_not(mask);
		const simd64_t valVec   = simd64_splat(uint64_t(_value) );
		const simd64_t neg      = simd64_u64_sub(zero, valVec);
		const simd64_t orbit    = simd64_and(mask, neg);
		const simd64_t prev     = simd64_splat(_ptr[idx]);
		const simd64_t cleared  = simd64_and(prev, cmask);
		const simd64_t result   = simd64_or(cleared, orbit);

		_ptr[idx] = result.u64;
	}

	inline void bitSetSet(uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, bool _value)
	{
		if (_beginBit >= _endBit)
		{
			return;
		}

		const simd32_t mask63    = simd32_splat(uint32_t(63) );
		const simd32_t one       = simd32_splat(1u);
		const simd32_t bitB      = simd32_splat(_beginBit);
		const simd32_t bitEnd    = simd32_splat(_endBit);
		const simd32_t bitE      = simd32_u32_satsub(bitEnd, one);
		const simd32_t idxBVec   = simd32_x32_srl(bitB, 6);
		const simd32_t shiftBVec = simd32_and(bitB, mask63);
		const simd32_t idxEVec   = simd32_x32_srl(bitE, 6);
		const simd32_t shiftEVec = simd32_and(bitEnd, mask63);
		const uint32_t idxB      = idxBVec.u32;
		const uint32_t shiftB    = shiftBVec.u32;
		const uint32_t idxE      = idxEVec.u32;
		const uint32_t shiftE    = shiftEVec.u32;
		const simd64_t zero      = simd64_zero();
		const simd64_t allOnes   = simd64_splat(UINT64_MAX);
		const simd64_t maskB     = simd64_x64_sll(allOnes, int(shiftB) );
		const simd64_t maskE     = simd64_x64_srl(allOnes, int( (64-shiftE) & 63) );
		const simd64_t valVec    = simd64_splat(uint64_t(_value) );
		const simd64_t orbits    = simd64_u64_sub(zero, valVec);

		if (idxB == idxE)
		{
			const simd64_t maskBE   = simd64_and(maskB, maskE);
			const simd64_t cmaskBE  = simd64_not(maskBE);
			const simd64_t orbitsBE = simd64_and(orbits, maskBE);
			const simd64_t prev     = simd64_splat(_ptr[idxB]);
			const simd64_t cleared  = simd64_and(prev, cmaskBE);
			const simd64_t result   = simd64_or(cleared, orbitsBE);
			_ptr[idxB] = result.u64;
			return;
		}

		{
			const simd64_t cmaskB  = simd64_not(maskB);
			const simd64_t orbitsB = simd64_and(orbits, maskB);
			const simd64_t prev    = simd64_splat(_ptr[idxB]);
			const simd64_t cleared = simd64_and(prev, cmaskB);
			const simd64_t result  = simd64_or(cleared, orbitsB);
			_ptr[idxB] = result.u64;
		}

		{
			const simd64_t cmaskE  = simd64_not(maskE);
			const simd64_t orbitsE = simd64_and(orbits, maskE);
			const simd64_t prev    = simd64_splat(_ptr[idxE]);
			const simd64_t cleared = simd64_and(prev, cmaskE);
			const simd64_t result  = simd64_or(cleared, orbitsE);
			_ptr[idxE] = result.u64;
		}

		const uint32_t numWords = idxE - idxB - 1;

		if (numWords > 2)
		{
			bx::memSet(&_ptr[idxB+1], _value ? 0xff : 0, numWords*sizeof(uint64_t) );
			return;
		}

		if (numWords > 0)
		{
			_ptr[idxB+1] = orbits.u64;
		}

		if (numWords > 1)
		{
			_ptr[idxB+2] = orbits.u64;
		}
	}

	inline bool bitSetGet(const uint64_t* _ptr, uint32_t _idx)
	{
		const simd32_t mask63   = simd32_splat(uint32_t(63) );
		const simd32_t bitVec   = simd32_splat(_idx);
		const simd32_t idxVec   = simd32_x32_srl(bitVec, 6);
		const simd32_t shiftVec = simd32_and(bitVec, mask63);
		const uint32_t idx      = idxVec.u32;
		const uint32_t shift    = shiftVec.u32;
		const simd64_t one      = simd64_splat(uint64_t(1) );
		const simd64_t bit      = simd64_x64_sll(one, int(shift) );
		const simd64_t prev     = simd64_splat(_ptr[idx]);
		const simd64_t masked   = simd64_and(prev, bit);

		return 0 != masked.u64;
	}

	inline uint32_t bitSetCountBits(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit)
	{
		if (_beginBit >= _endBit)
		{
			return 0;
		}

		const simd32_t mask63    = simd32_splat(uint32_t(63) );
		const simd32_t one       = simd32_splat(1u);
		const simd32_t bitB      = simd32_splat(_beginBit);
		const simd32_t bitEnd    = simd32_splat(_endBit);
		const simd32_t bitE      = simd32_u32_satsub(bitEnd, one);
		const simd32_t idxBVec   = simd32_x32_srl(bitB, 6);
		const simd32_t shiftBVec = simd32_and(bitB, mask63);
		const simd32_t idxEVec   = simd32_x32_srl(bitE, 6);
		const simd32_t shiftEVec = simd32_and(bitEnd, mask63);
		const uint32_t idxB      = idxBVec.u32;
		const uint32_t shiftB    = shiftBVec.u32;
		const uint32_t idxE      = idxEVec.u32;
		const uint32_t shiftE    = shiftEVec.u32;
		const simd64_t allOnes   = simd64_splat(UINT64_MAX);
		const simd64_t maskB     = simd64_x64_sll(allOnes, int(shiftB) );
		const simd64_t maskE     = simd64_x64_srl(allOnes, int( (64-shiftE) & 63) );

		if (idxB == idxE)
		{
			const simd64_t mask   = simd64_and(maskB, maskE);
			const simd64_t prev   = simd64_splat(_ptr[idxB]);
			const simd64_t masked = simd64_and(prev, mask);
			return countBits(masked.u64);
		}

		const simd64_t prevB   = simd64_splat(_ptr[idxB]);
		const simd64_t maskedB = simd64_and(prevB, maskB);
		uint32_t count = countBits(maskedB.u64);

		count += countBitsWords(_ptr, idxB+1, idxE);

		const simd64_t prevE   = simd64_splat(_ptr[idxE]);
		const simd64_t maskedE = simd64_and(prevE, maskE);
		count += countBits(maskedE.u64);

		return count;
	}

	template<bool SimdT>
	inline bool bitSetTestAny(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit)
	{
		if (_beginBit >= _endBit)
		{
			return false;
		}

		const simd32_t mask63    = simd32_splat(uint32_t(63) );
		const simd32_t one       = simd32_splat(1u);
		const simd32_t bitB      = simd32_splat(_beginBit);
		const simd32_t bitEnd    = simd32_splat(_endBit);
		const simd32_t bitE      = simd32_u32_satsub(bitEnd, one);
		const simd32_t idxBVec   = simd32_x32_srl(bitB, 6);
		const simd32_t shiftBVec = simd32_and(bitB, mask63);
		const simd32_t idxEVec   = simd32_x32_srl(bitE, 6);
		const simd32_t shiftEVec = simd32_and(bitEnd, mask63);
		const uint32_t idxB      = idxBVec.u32;
		const uint32_t shiftB    = shiftBVec.u32;
		const uint32_t idxE      = idxEVec.u32;
		const uint32_t shiftE    = shiftEVec.u32;
		const simd64_t allOnes   = simd64_splat(UINT64_MAX);
		const simd64_t maskB     = simd64_x64_sll(allOnes, int(shiftB) );
		const simd64_t maskE     = simd64_x64_srl(allOnes, int( (64-shiftE) & 63) );

		if (idxB == idxE)
		{
			const simd64_t mask   = simd64_and(maskB, maskE);
			const simd64_t prev   = simd64_splat(_ptr[idxB]);
			const simd64_t masked = simd64_and(prev, mask);
			return 0 != masked.u64;
		}

		{
			const simd64_t prev   = simd64_splat(_ptr[idxB]);
			const simd64_t masked = simd64_and(prev, maskB);
			if (0 != masked.u64)
			{
				return true;
			}
		}

		if (idxE != findWord<Match::NonZero, SimdT>(_ptr, idxB+1, idxE) )
		{
			return true;
		}

		const simd64_t prev   = simd64_splat(_ptr[idxE]);
		const simd64_t masked = simd64_and(prev, maskE);
		return 0 != masked.u64;
	}

	template<bool SimdT>
	inline bool bitSetTestAll(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit)
	{
		if (_beginBit >= _endBit)
		{
			return true;
		}

		const simd32_t mask63    = simd32_splat(uint32_t(63) );
		const simd32_t one       = simd32_splat(1u);
		const simd32_t bitB      = simd32_splat(_beginBit);
		const simd32_t bitEnd    = simd32_splat(_endBit);
		const simd32_t bitE      = simd32_u32_satsub(bitEnd, one);
		const simd32_t idxBVec   = simd32_x32_srl(bitB, 6);
		const simd32_t shiftBVec = simd32_and(bitB, mask63);
		const simd32_t idxEVec   = simd32_x32_srl(bitE, 6);
		const simd32_t shiftEVec = simd32_and(bitEnd, mask63);
		const uint32_t idxB      = idxBVec.u32;
		const uint32_t shiftB    = shiftBVec.u32;
		const uint32_t idxE      = idxEVec.u32;
		const uint32_t shiftE    = shiftEVec.u32;
		const simd64_t allOnes   = simd64_splat(UINT64_MAX);
		const simd64_t maskB     = simd64_x64_sll(allOnes, int(shiftB) );
		const simd64_t maskE     = simd64_x64_srl(allOnes, int( (64-shiftE) & 63) );

		if (idxB == idxE)
		{
			const simd64_t mask   = simd64_and(maskB, maskE);
			const simd64_t prev   = simd64_splat(_ptr[idxB]);
			const simd64_t masked = simd64_and(prev, mask);
			return mask.u64 == masked.u64;
		}

		{
			const simd64_t prev   = simd64_splat(_ptr[idxB]);
			const simd64_t masked = simd64_and(prev, maskB);
			if (maskB.u64 != masked.u64)
			{
				return false;
			}
		}

		if (idxE != findWord<Match::NotFull, SimdT>(_ptr, idxB+1, idxE) )
		{
			return false;
		}

		const simd64_t prev   = simd64_splat(_ptr[idxE]);
		const simd64_t masked = simd64_and(prev, maskE);
		return maskE.u64 == masked.u64;
	}

	template<bool SimdT>
	inline bool bitSetTestNone(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit)
	{
		if (_beginBit >= _endBit)
		{
			return true;
		}

		const simd32_t mask63    = simd32_splat(uint32_t(63) );
		const simd32_t one       = simd32_splat(1u);
		const simd32_t bitB      = simd32_splat(_beginBit);
		const simd32_t bitEnd    = simd32_splat(_endBit);
		const simd32_t bitE      = simd32_u32_satsub(bitEnd, one);
		const simd32_t idxBVec   = simd32_x32_srl(bitB, 6);
		const simd32_t shiftBVec = simd32_and(bitB, mask63);
		const simd32_t idxEVec   = simd32_x32_srl(bitE, 6);
		const simd32_t shiftEVec = simd32_and(bitEnd, mask63);
		const uint32_t idxB      = idxBVec.u32;
		const uint32_t shiftB    = shiftBVec.u32;
		const uint32_t idxE      = idxEVec.u32;
		const uint32_t shiftE    = shiftEVec.u32;
		const simd64_t allOnes   = simd64_splat(UINT64_MAX);
		const simd64_t maskB     = simd64_x64_sll(allOnes, int(shiftB) );
		const simd64_t maskE     = simd64_x64_srl(allOnes, int( (64-shiftE) & 63) );

		if (idxB == idxE)
		{
			const simd64_t mask   = simd64_and(maskB, maskE);
			const simd64_t prev   = simd64_splat(_ptr[idxB]);
			const simd64_t masked = simd64_and(prev, mask);
			return 0 == masked.u64;
		}

		{
			const simd64_t prev   = simd64_splat(_ptr[idxB]);
			const simd64_t masked = simd64_and(prev, maskB);
			if (0 != masked.u64)
			{
				return false;
			}
		}

		if (idxE != findWord<Match::NonZero, SimdT>(_ptr, idxB+1, idxE) )
		{
			return false;
		}

		const simd64_t prev   = simd64_splat(_ptr[idxE]);
		const simd64_t masked = simd64_and(prev, maskE);
		return 0 == masked.u64;
	}

	inline uint64_t extractBits(uint64_t _input, uint32_t _pos, uint64_t _mask)
	{
		const uint64_t bits = _input >> _pos;
		return bits & _mask;
	}

	inline uint64_t replaceBits(uint64_t _input, uint32_t _pos, uint64_t _mask, uint64_t _bits)
	{
		const uint64_t bits = (_bits  &   _mask) << _pos;
		const uint64_t old  =  _input & ~(_mask  << _pos);
		return bits | old;
	}

	inline void bitSet_setBits(uint64_t* _ptr, uint32_t _pos, uint64_t _bits, uint32_t _numBits)
	{
		BX_ASSERT(_numBits >= 1 && _numBits <= 64
			, "_numBits (%d) must be between 1-64."
			, _numBits
			);

		const uint32_t idxB   = (_pos >> 6);
		const uint32_t shiftB = (_pos & 63);

		if (shiftB + _numBits <= 64)
		{
			const uint64_t mask = UINT64_MAX >> (64-_numBits);
			_ptr[idxB] = replaceBits(_ptr[idxB], shiftB, mask, _bits);

			return;
		}

		const uint32_t end    = (_pos + _numBits);
		const uint32_t idxE   = ((end - 1) >> 6);
		const uint32_t shiftE = (end & 63);

		const uint64_t maskB = UINT64_MAX >> shiftB;
		_ptr[idxB] = replaceBits(_ptr[idxB], shiftB, maskB, _bits);

		const uint64_t bitsE = _bits >> (64 - shiftB);
		const uint64_t maskE = UINT64_MAX >> ( (64 - shiftE) & 63);
		_ptr[idxE] = replaceBits(_ptr[idxE], 0, maskE, bitsE);
	}

	inline uint64_t bitSet_getBits(uint64_t* _ptr, uint32_t _pos, uint32_t _numBits)
	{
		BX_ASSERT(_numBits >= 1 && _numBits <= 64
			, "_numBits (%d) must be between 1-64."
			, _numBits
			);

		const uint32_t idxB   = (_pos >> 6);
		const uint32_t shiftB = (_pos & 63);

		if (shiftB + _numBits <= 64)
		{
			const uint64_t mask = UINT64_MAX >> (64-_numBits);
			return extractBits(_ptr[idxB], shiftB, mask);
		}

		const uint32_t end    = (_pos + _numBits);
		const uint32_t idxE   = ((end - 1) >> 6);
		const uint32_t shiftE = (end & 63);

		const uint64_t maskB  = UINT64_MAX >> shiftB;
		const uint64_t bitsB  = extractBits(_ptr[idxB], shiftB, maskB);

		const uint64_t maskE  = UINT64_MAX >> ( (64 - shiftE) & 63);
		const uint64_t bitsE  = extractBits(_ptr[idxE], 0, maskE);

		return (bitsE << (64-shiftB) ) | bitsB;
	}

	inline void copyBits(uint64_t* _ptr, uint32_t _dstBit, uint32_t _srcBit, uint32_t _numBits)
	{
		const uint64_t bits = bitSet_getBits(_ptr, _srcBit, _numBits);
		bitSet_setBits(_ptr, _dstBit, bits, _numBits);
	}

	inline void bitSetBlitForward(uint64_t* _ptr, uint32_t _dstBit, uint32_t _srcBit, uint32_t _numBits)
	{
		const uint32_t numPreBits = (64 - (_dstBit & 63) ) & 63;

		if (0 < numPreBits)
		{
			copyBits(_ptr, _dstBit, _srcBit, numPreBits);
		}

		const uint32_t srcBit = _srcBit + numPreBits;
		const uint32_t shift  = srcBit & 63;

		uint32_t dst     = (_dstBit + numPreBits) >> 6;
		uint32_t src     = srcBit >> 6;
		uint32_t numBits = _numBits - numPreBits;

		if (0 == shift)
		{
			for (; numBits >= 64; numBits -= 64, ++dst, ++src)
			{
				_ptr[dst] = _ptr[src];
			}
		}
		else
		{
			uint64_t lo = _ptr[src];

			for (; numBits >= 64; numBits -= 64, ++dst, ++src)
			{
				const uint64_t hi = _ptr[src+1];
				_ptr[dst] = (lo >> shift) | (hi << (64 - shift) );
				lo = hi;
			}
		}

		if (0 < numBits)
		{
			copyBits(_ptr, dst<<6, (src<<6) + shift, numBits);
		}
	}

	inline void bitSetBlitBackward(uint64_t* _ptr, uint32_t _dstBit, uint32_t _srcBit, uint32_t _numBits)
	{
		const uint32_t dstEnd      = _dstBit + _numBits;
		const uint32_t numPostBits = dstEnd & 63;

		if (0 < numPostBits)
		{
			copyBits(_ptr, dstEnd - numPostBits, _srcBit + _numBits - numPostBits, numPostBits);
		}

		const uint32_t srcEnd = _srcBit + _numBits - numPostBits;
		const uint32_t shift  = srcEnd & 63;

		uint32_t dst     = (dstEnd - numPostBits) >> 6;
		uint32_t src     = srcEnd >> 6;
		uint32_t numBits = _numBits - numPostBits;

		if (0 == shift)
		{
			for (; numBits >= 64; numBits -= 64)
			{
				_ptr[--dst] = _ptr[--src];
			}
		}
		else
		{
			uint64_t hi = _ptr[src];

			for (; numBits >= 64; numBits -= 64, --src)
			{
				const uint64_t lo = _ptr[src-1];
				_ptr[--dst] = (lo >> shift) | (hi << (64 - shift) );
				hi = lo;
			}
		}

		if (0 < numBits)
		{
			copyBits(_ptr, _dstBit, _srcBit, numBits);
		}
	}

	inline void bitSetBlit(uint64_t* _ptr, uint32_t _dstBit, uint32_t _srcBit, uint32_t _numBits)
	{
		if (_dstBit == _srcBit
		||        0 == _numBits)
		{
			return;
		}

		if (_numBits <= 64)
		{
			copyBits(_ptr, _dstBit, _srcBit, _numBits);
			return;
		}

		if ( (_dstBit&7) != (_srcBit&7) )
		{
			if (_dstBit < _srcBit)
			{
				bitSetBlitForward(_ptr, _dstBit, _srcBit, _numBits);
			}
			else
			{
				bitSetBlitBackward(_ptr, _dstBit, _srcBit, _numBits);
			}

			return;
		}

		const uint32_t numPreBits  = (8 - (_dstBit & 7) ) & 7;
		const uint32_t dstByte     = (_dstBit + numPreBits) >> 3;
		const uint32_t srcByte     = (_srcBit + numPreBits) >> 3;
		const uint32_t rest        = _numBits - numPreBits;
		const uint32_t numPostBits = rest & 7;
		const uint32_t numBytes    = rest >> 3;
		uint8_t* ptr = (uint8_t*)_ptr;

		if (_dstBit < _srcBit
		&&  0 < numPreBits)
		{
			copyBits(_ptr, _dstBit, _srcBit, numPreBits);
		}

		if (_dstBit > _srcBit
		&&  0 < numPostBits)
		{
			copyBits(_ptr, (dstByte + numBytes)<<3, (srcByte + numBytes)<<3, numPostBits);
		}

		bx::memMove(ptr + dstByte, ptr + srcByte, numBytes);

		if (_dstBit < _srcBit
		&&  0 < numPostBits)
		{
			copyBits(_ptr, (dstByte + numBytes)<<3, (srcByte + numBytes)<<3, numPostBits);
		}

		if (_dstBit > _srcBit
		&&  0 < numPreBits)
		{
			copyBits(_ptr, _dstBit, _srcBit, numPreBits);
		}
	}

	template<uint32_t NumStepsT>
	BX_FORCE_INLINE uint64_t bitSetRunStarts(uint64_t _free, uint32_t _rest)
	{
		uint64_t runs = _free;

		if constexpr (1 <= NumStepsT) { runs &= runs >>  1; }
		if constexpr (2 <= NumStepsT) { runs &= runs >>  2; }
		if constexpr (3 <= NumStepsT) { runs &= runs >>  4; }
		if constexpr (4 <= NumStepsT) { runs &= runs >>  8; }
		if constexpr (5 <= NumStepsT) { runs &= runs >> 16; }
		if constexpr (6 <= NumStepsT) { runs &= runs >> 32; }

		const uint64_t tail = runs >> _rest;

		return runs & tail;
	}

	template<bool SimdT, uint32_t NumStepsT, typename ElementT>
	inline uint32_t bitSetFindShortStepsT(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		constexpr bool kInWord = NumStepsT <= 6;
		constexpr bool kSkipToZeroElement = sizeof(ElementT) < sizeof(uint64_t);

		const uint32_t begin     = _beginBit/64;
		const uint32_t last      = (_endBit - 1)/64;
		const uint32_t endShift  = _endBit & 63;
		const uint64_t beginMask = ~(UINT64_MAX << (_beginBit & 63) );
		const uint64_t endMask   = 0 == endShift ? 0 : UINT64_MAX << endShift;
		const uint32_t rest      = kInWord ? _numBits - (1u << NumStepsT) : 0;

		uint32_t carry    = 0;
		uint32_t ii       = begin;
		uint64_t occupied = _ptr[ii] | beginMask;

		for (;;)
		{
			occupied |= ii == last ? endMask : 0;

			const uint32_t pos = ii*64;
			const uint32_t tz  = countTrailingZeros(occupied);

			if (carry + tz >= _numBits)
			{
				return pos - carry;
			}

			if constexpr (kInWord)
			{
				const uint64_t runs = bitSetRunStarts<NumStepsT>(~occupied, rest);

				if (0 != runs)
				{
					return pos + countTrailingZeros(runs);
				}
			}

			carry = 0 == occupied ? carry + 64 : countLeadingZeros(occupied);

			if (ii == last)
			{
				return kInvalid;
			}

			++ii;

			if constexpr (kSkipToZeroElement)
			{
				if (0 == carry
				&&  ii < last
				&&  !hasZeroElement<ElementT>(_ptr[ii])
				&&  !hasZeroElement<ElementT>(_ptr[ii+1]) )
				{
					const uint32_t next = findZeroElementWord<ElementT, SimdT>(_ptr, ii + 2, last + 1);

					if (next > last)
					{
						return kInvalid;
					}

					ii = max(ii, next - 1);
				}
			}

			occupied = _ptr[ii];

			if (UINT64_MAX == occupied
			&&  ii < last)
			{
				ii       = findWord<Match::NotFull, SimdT>(_ptr, ii + 1, last);
				occupied = _ptr[ii];
				carry    = 0;
			}
		}
	}

	template<bool SimdT>
	inline uint32_t bitSetFindShortT(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		// Run of 2*N-1 or more zero bits always contains whole N-bit aligned zero element.
		if (_numBits <   4) { return bitSetFindShortStepsT<SimdT, 1, uint64_t>(_ptr, _beginBit, _endBit, _numBits); }
		if (_numBits <   8) { return bitSetFindShortStepsT<SimdT, 2, uint64_t>(_ptr, _beginBit, _endBit, _numBits); }
		if (_numBits <  15) { return bitSetFindShortStepsT<SimdT, 3, uint64_t>(_ptr, _beginBit, _endBit, _numBits); }
		if (_numBits <  16) { return bitSetFindShortStepsT<SimdT, 3, uint8_t >(_ptr, _beginBit, _endBit, _numBits); }
		if (_numBits <  31) { return bitSetFindShortStepsT<SimdT, 4, uint8_t >(_ptr, _beginBit, _endBit, _numBits); }
		if (_numBits <  32) { return bitSetFindShortStepsT<SimdT, 4, uint16_t>(_ptr, _beginBit, _endBit, _numBits); }
		if (_numBits <  63) { return bitSetFindShortStepsT<SimdT, 5, uint16_t>(_ptr, _beginBit, _endBit, _numBits); }
		if (_numBits <  64) { return bitSetFindShortStepsT<SimdT, 5, uint32_t>(_ptr, _beginBit, _endBit, _numBits); }
		if (_numBits <  65) { return bitSetFindShortStepsT<SimdT, 6, uint32_t>(_ptr, _beginBit, _endBit, _numBits); }

		return bitSetFindShortStepsT<SimdT, 7, uint32_t>(_ptr, _beginBit, _endBit, _numBits);
	}

	template<bool SimdT>
	inline uint32_t bitSetFindLongT(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		for (uint32_t ii = alignUp(_beginBit, 64)/64, end = alignUp(_endBit, 64)/64; ii < end; ++ii)
		{
			ii = findWord<Match::Zero, SimdT>(_ptr, ii, end);

			if (ii == end)
			{
				break;
			}

			uint32_t idx = ii*64;

			if (idx != _beginBit)
			{
				idx = max(idx - countLeadingZeros(_ptr[ii-1]), _beginBit);
			}

			const uint32_t next   = findWord<Match::NonZero, SimdT>(_ptr, ii+1, end);
			const uint32_t tz     = next == end ? 0 : countTrailingZeros(_ptr[next]);
			const uint32_t runEnd = min(next*64 + tz, _endBit);

			if (runEnd - idx >= _numBits)
			{
				return idx;
			}

			ii = next;
		}

		return kInvalid;
	}

	template<bool SimdT>
	BX_NO_INLINE uint32_t bitSetFindRunT(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		if (_numBits < 127)
		{
			return bitSetFindShortT<SimdT>(_ptr, _beginBit, _endBit, _numBits);
		}

		return bitSetFindLongT<SimdT>(_ptr, _beginBit, _endBit, _numBits);
	}

	template<bool SimdT>
	BX_NO_INLINE uint32_t bitSetFindZeroFromT(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit)
	{
		const uint32_t endWord = alignUp(_endBit, 64)/64;
		const uint32_t ii      = findWord<Match::NotFull, SimdT>(_ptr, _beginBit/64, endWord);

		if (ii == endWord)
		{
			return kInvalid;
		}

		const uint32_t idx = ii*64 + countTrailingZeros(~_ptr[ii]);
		return idx < _endBit ? idx : kInvalid;
	}

	template<bool SimdT>
	inline uint32_t bitSetFindZeroT(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit)
	{
		const uint32_t begin = _beginBit/64;
		const uint32_t end   = alignUp(_endBit, 64)/64;

		if (begin >= end)
		{
			return kInvalid;
		}

		const uint64_t zeros = ~_ptr[begin] & (UINT64_MAX << (_beginBit & 63) );

		if (0 == zeros)
		{
			return bitSetFindZeroFromT<SimdT>(_ptr, (begin + 1)*64, _endBit);
		}

		const uint32_t idx = begin*64 + countTrailingZeros(zeros);
		return idx < _endBit ? idx : kInvalid;
	}

	template<bool SimdT>
	inline uint32_t bitSetFindRangeT(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		if (_beginBit >= _endBit
		||  _endBit - _beginBit < _numBits)
		{
			return kInvalid;
		}

		if (1 == _numBits)
		{
			return bitSetFindZeroT<SimdT>(_ptr, _beginBit, _endBit);
		}

		if (_numBits <= 64)
		{
			const uint32_t shift   = _beginBit & 63;
			const uint32_t numFree = min(64 - shift, _endBit - _beginBit);
			const uint64_t valid   = UINT64_MAX >> (64 - numFree);
			const uint64_t avail   = (~_ptr[_beginBit/64] >> shift) & valid;
			const uint64_t first   = UINT64_MAX >> (64 - _numBits);

			if (first == (avail & first) )
			{
				return _beginBit;
			}

			uint64_t runs = avail;
			uint32_t span = 1;

			for (; span*2 <= _numBits; span *= 2)
			{
				runs &= runs >> span;
			}

			runs &= runs >> (_numBits - span);

			if (0 != runs)
			{
				return _beginBit + countTrailingZeros(runs);
			}
		}

		return bitSetFindRunT<SimdT>(_ptr, _beginBit, _endBit, _numBits);
	}

	uint32_t bitSetFindRef(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		return bitSetFindRangeT<false>(_ptr, _beginBit, _endBit, _numBits);
	}

	uint32_t bitSetFindSimd(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		return bitSetFindRangeT<true>(_ptr, _beginBit, _endBit, _numBits);
	}

	inline uint32_t bitSetFind(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		return bitSetFindRangeT<kSimdSupported>(_ptr, _beginBit, _endBit, _numBits);
	}

	uint64_t BitArrayViewBase::get(uint32_t _beginBit, uint32_t _endBit) const
	{
		BX_ASSERT(true
			&& _beginBit <= m_count
			&& _endBit   <= m_count
			&& _beginBit <= _endBit
			, "Incorrect usage `%s`. _beginBit %d, _endBit %d, m_count %d [%d%d%d]"
			, BX_FUNCTION
			, _beginBit
			, _endBit
			, m_count
			, _beginBit <= m_count
			, _endBit   <= m_count
			, _beginBit <= _endBit
			);
		return bitSet_getBits(m_ptr, _beginBit, _endBit-_beginBit);
	}

	uint32_t BitArrayViewBase::findRange(uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits) const
	{
		return bitSetFind(m_ptr, _beginBit, _endBit, _numBits);
	}

	uint32_t BitArrayViewBase::findSet(Element _element) const
	{
		const uint32_t num = m_sizeInBytes/8;

		if (kLast == _element)
		{
			const uint32_t idx = findLastWord<Match::NonZero, kSimdSupported>(m_ptr, 0, num);

			return kInvalid != idx
				? idx*64 + findLastSet(m_ptr[idx]) - 1
				: kInvalid
				;
		}

		const uint32_t idx = findWord<Match::NonZero, kSimdSupported>(m_ptr, 0, num);

		return idx != num
			? idx*64 + findFirstSet(m_ptr[idx]) - 1
			: kInvalid
			;
	}

	uint32_t BitArrayViewBase::findSetFrom(uint32_t _beginBit, uint32_t _endBit) const
	{
		const uint32_t beginWord = _beginBit/64;
		const uint32_t endWord   = alignUp(_endBit, 64)/64;

		if (beginWord >= endWord)
		{
			return kInvalid;
		}

		const uint32_t idx = findWord<Match::NonZero, kSimdSupported>(m_ptr, beginWord, endWord);

		if (idx == endWord)
		{
			return kInvalid;
		}

		const uint32_t found = idx*64 + findFirstSet(m_ptr[idx]) - 1;

		return found < _endBit
			? found
			: kInvalid
			;
	}

	uint32_t BitArrayViewBase::findClearFrom(uint32_t _beginBit, uint32_t _endBit) const
	{
		const uint32_t beginWord = _beginBit/64;
		const uint32_t endWord   = alignUp(_endBit, 64)/64;

		if (beginWord >= endWord)
		{
			return kInvalid;
		}

		const uint32_t idx = findWord<Match::NotFull, kSimdSupported>(m_ptr, beginWord, endWord);

		if (idx == endWord)
		{
			return kInvalid;
		}

		const uint32_t found = idx*64 + findFirstSet(~m_ptr[idx]) - 1;

		return found < _endBit
			? found
			: kInvalid
			;
	}

	uint32_t BitArrayViewBase::countBits(uint32_t _beginBit, uint32_t _endBit) const
	{
		BX_ASSERT(true
			&& _beginBit <= m_count
			&& _endBit   <= m_count
			&& _beginBit <= _endBit
			, "Incorrect usage `%s`. _beginBit %d, _endBit %d, m_count %d [%d%d%d]"
			, BX_FUNCTION
			, _beginBit
			, _endBit
			, m_count
			, _beginBit <= m_count
			, _endBit   <= m_count
			, _beginBit <= _endBit
			);
		return bitSetCountBits(m_ptr, _beginBit, _endBit);
	}

	bool BitArrayViewBase::testAny(uint32_t _beginBit, uint32_t _endBit) const
	{
		BX_ASSERT(true
			&& _beginBit <= m_count
			&& _endBit   <= m_count
			&& _beginBit <= _endBit
			, "Incorrect usage `%s`. _beginBit %d, _endBit %d, m_count %d [%d%d%d]"
			, BX_FUNCTION
			, _beginBit
			, _endBit
			, m_count
			, _beginBit <= m_count
			, _endBit   <= m_count
			, _beginBit <= _endBit
			);
		return bitSetTestAny<kSimdSupported>(m_ptr, _beginBit, _endBit);
	}

	bool BitArrayViewBase::testAll(uint32_t _beginBit, uint32_t _endBit) const
	{
		BX_ASSERT(true
			&& _beginBit <= m_count
			&& _endBit   <= m_count
			&& _beginBit <= _endBit
			, "Incorrect usage `%s`. _beginBit %d, _endBit %d, m_count %d [%d%d%d]"
			, BX_FUNCTION
			, _beginBit
			, _endBit
			, m_count
			, _beginBit <= m_count
			, _endBit   <= m_count
			, _beginBit <= _endBit
			);
		return bitSetTestAll<kSimdSupported>(m_ptr, _beginBit, _endBit);
	}

	bool BitArrayViewBase::testNone(uint32_t _beginBit, uint32_t _endBit) const
	{
		BX_ASSERT(true
			&& _beginBit <= m_count
			&& _endBit   <= m_count
			&& _beginBit <= _endBit
			, "Incorrect usage `%s`. _beginBit %d, _endBit %d, m_count %d [%d%d%d]"
			, BX_FUNCTION
			, _beginBit
			, _endBit
			, m_count
			, _beginBit <= m_count
			, _endBit   <= m_count
			, _beginBit <= _endBit
			);
		return bitSetTestNone<kSimdSupported>(m_ptr, _beginBit, _endBit);
	}

	void MutableBitArrayView::set(uint32_t _beginBit, uint32_t _endBit, bool _value)
	{
		BX_ASSERT(true
			&& _beginBit <= m_count
			&& _endBit   <= m_count
			&& _beginBit <= _endBit
			, "Incorrect usage `%s`. _beginBit %d, _endBit %d, m_count %d [%d%d%d]"
			, BX_FUNCTION
			, _beginBit
			, _endBit
			, m_count
			, _beginBit <= m_count
			, _endBit   <= m_count
			, _beginBit <= _endBit
			);
		bitSetSet(m_ptr, _beginBit, _endBit, _value);
	}

	void MutableBitArrayView::set(uint32_t _beginBit, uint32_t _endBit, uint64_t _value)
	{
		BX_ASSERT(true
			&& _beginBit <= m_count
			&& _endBit   <= m_count
			&& _beginBit <= _endBit
			, "Incorrect usage `%s`. _beginBit %d, _endBit %d, m_count %d [%d%d%d]"
			, BX_FUNCTION
			, _beginBit
			, _endBit
			, m_count
			, _beginBit <= m_count
			, _endBit   <= m_count
			, _beginBit <= _endBit
			);
		return bitSet_setBits(m_ptr, _beginBit, _value, _endBit-_beginBit);
	}

	void MutableBitArrayView::blit(uint32_t _dstBit, uint32_t _srcBit, uint32_t _numBits)
	{
		bitSetBlit(m_ptr, _dstBit, _srcBit, _numBits);
	}

	void MippedBitArrayBase::updateMipFilled(uint32_t _beginWord, uint32_t _endWord, bool _value)
	{
		updateMip(_beginWord, _beginWord + 1);
		updateMip(_endWord - 1, _endWord);

		if (_beginWord + 2 < _endWord)
		{
			const uint32_t numMipWords = getNumMipWords(m_sizeInBytes/8);
			MutableBitArrayView(m_mip, numMipWords*sizeof(uint64_t) ).set(_beginWord + 1, _endWord - 1, _value);
		}
	}

} // namespace bx
