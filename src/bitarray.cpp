/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include <bx/bitarray.h>
#include <bx/simd_t.h>
#include <bx/debug.h>

namespace bx
{
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
		const simd64_t orbitsB   = simd64_and(maskB, orbits);
		const simd64_t orbitsE   = simd64_and(maskE, orbits);
		const simd64_t cmaskB    = simd64_not(maskB);
		const simd64_t cmaskE    = simd64_not(maskE);

		if (idxB == idxE)
		{
			const simd64_t maskBE   = simd64_and(maskB, maskE);
			const simd64_t cmaskBE  = simd64_not(maskBE);
			const simd64_t orbitsBE = simd64_and(orbitsB, orbitsE);
			const simd64_t prev     = simd64_splat(_ptr[idxB]);
			const simd64_t cleared  = simd64_and(prev, cmaskBE);
			const simd64_t result   = simd64_or(cleared, orbitsBE);
			_ptr[idxB] = result.u64;
			return;
		}

		{
			const simd64_t prev    = simd64_splat(_ptr[idxB]);
			const simd64_t cleared = simd64_and(prev, cmaskB);
			const simd64_t result  = simd64_or(cleared, orbitsB);
			_ptr[idxB] = result.u64;
		}

		for (uint32_t idx = idxB+1; idx < idxE; ++idx)
		{
			_ptr[idx] = orbits.u64;
		}

		{
			const simd64_t prev    = simd64_splat(_ptr[idxE]);
			const simd64_t cleared = simd64_and(prev, cmaskE);
			const simd64_t result  = simd64_or(cleared, orbitsE);
			_ptr[idxE] = result.u64;
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

		for (uint32_t idx = idxB+1; idx < idxE; ++idx)
		{
			count += countBits(_ptr[idx]);
		}

		const simd64_t prevE   = simd64_splat(_ptr[idxE]);
		const simd64_t maskedE = simd64_and(prevE, maskE);
		count += countBits(maskedE.u64);

		return count;
	}

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

		for (uint32_t idx = idxB+1; idx < idxE; ++idx)
		{
			if (0 != _ptr[idx])
			{
				return true;
			}
		}

		const simd64_t prev   = simd64_splat(_ptr[idxE]);
		const simd64_t masked = simd64_and(prev, maskE);
		return 0 != masked.u64;
	}

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

		for (uint32_t idx = idxB+1; idx < idxE; ++idx)
		{
			if (UINT64_MAX != _ptr[idx])
			{
				return false;
			}
		}

		const simd64_t prev   = simd64_splat(_ptr[idxE]);
		const simd64_t masked = simd64_and(prev, maskE);
		return maskE.u64 == masked.u64;
	}

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

		for (uint32_t idx = idxB+1; idx < idxE; ++idx)
		{
			if (0 != _ptr[idx])
			{
				return false;
			}
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
		}
		else if (_numBits <= 128)
		{
			copyBits(_ptr, _dstBit, _srcBit, 64);

			const uint32_t numBits = _numBits - 64;

			if (0 < numBits)
			{
				copyBits(_ptr, _dstBit + 64, _srcBit + 64, numBits);
			}
		}
		else if ( (_dstBit&7) == (_srcBit&7) )
		{
			const uint32_t dstByte     = alignUp(_dstBit, 8)>>3;
			const uint32_t srcByte     = alignUp(_srcBit, 8)>>3;
			const uint32_t numPreBits  = (dstByte<<3) - _dstBit;
			const uint32_t tmp0        = _numBits - numPreBits;
			const uint32_t numPostBits = tmp0&7;
			const uint32_t numBytes    = tmp0>>3;
			uint8_t* ptr = (uint8_t*)_ptr;

			if (0 < numPreBits)
			{
				copyBits(_ptr, _dstBit, _srcBit, numPreBits);
			}

			bx::memMove(ptr + dstByte, ptr + srcByte, numBytes);

			if (0 < numPostBits)
			{
				copyBits(_ptr, (dstByte + numBytes)<<3, (srcByte + numBytes)<<3, numPostBits);
			}
		}
		else
		{
			      uint32_t idxB = alignUp(_dstBit, 64)>>6;
			const uint32_t numPreBits = (idxB<<6) - _dstBit;

			uint32_t srcBit = _srcBit;

			if (0 < numPreBits)
			{
				copyBits(_ptr, _dstBit, srcBit, numPreBits);
				srcBit += numPreBits;
			}

			uint32_t numBits = _numBits - numPreBits;

			for (; numBits >= 64; numBits -= 64, srcBit += 64)
			{
				const uint64_t bits = bitSet_getBits(_ptr, srcBit, 64);
				_ptr[idxB++] = bits;
			}

			if (0 < numBits)
			{
				copyBits(_ptr, idxB<<6, srcBit, numBits);
			}
		}
	}

	template<typename Ty>
	inline uint32_t bitSetFindT(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numSteps)
	{
		const Ty* bits = (const Ty*)_ptr;
		constexpr uint32_t numBitsPerStep = sizeof(Ty)*8;
		const uint32_t numBits = _numSteps*numBitsPerStep;

		for (uint32_t ii = _beginBit/numBitsPerStep, end = alignUp(_endBit, numBitsPerStep)/numBitsPerStep; ii < end; ++ii)
		{
			if (bits[ii] != Ty(0) )
			{
				continue;
			}

			uint32_t idx = ii*numBitsPerStep;
			uint32_t num = min(idx + numBitsPerStep, _endBit) - idx;

			if (idx != _beginBit)
			{
				const Ty test = bits[idx/numBitsPerStep - 1];
				const uint32_t newIdx = max(idx - countLeadingZeros<Ty>(test), _beginBit);

				num += idx - newIdx;
				idx = newIdx;
			}

			for (++ii; ii < end; ++ii)
			{
				if (num >= numBits)
				{
					return idx;
				}

				const Ty test = bits[ii];
				const uint32_t pos = ii * numBitsPerStep;
				num += min(pos + countTrailingZeros(test), _endBit) - pos;

				if (test != Ty(0) )
				{
					break;
				}
			}

			if (num >= numBits)
			{
				return idx;
			}
		}

		return _endBit;
	}

	inline uint32_t bitSetFindUpto14(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		const uint8_t* bits = (const uint8_t*)_ptr;

		const uint8_t beginMask = uint8_t(~(0xff << (_beginBit & 7) ) );

		uint32_t lz = 0;

		for (uint32_t begin = _beginBit / 8, end = alignUp(_endBit, 8) / 8, ii = begin; ii < end; ++ii)
		{
			uint8_t test = bits[ii];
			test |= ii == begin ? beginMask : 0;

			if (0xff == test)
			{
				continue;
			}

			const uint32_t pos = ii * 8;
			const uint32_t tz = min(pos + countTrailingZeros(test), _endBit) - pos;

			if (lz + tz >= _numBits)
			{
				return pos - lz;
			}

			lz = countLeadingZeros(test);

			const uint32_t idx = pos + 8;
			const uint32_t newIdx = max(idx - lz, _beginBit);
			lz = min(idx, _endBit) - newIdx;

			if (lz >= _numBits)
			{
				return newIdx;
			}
		}

		return _endBit;
	}

	inline uint32_t bitSetFindUpto6(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		const uint8_t* bits = (const uint8_t*)_ptr;

		const uint8_t beginMask = uint8_t(~(0xff << (_beginBit & 7) ) );
		const uint8_t endMask   = uint8_t(~(0xff >> ( ( (8-(_endBit & 7) ) & 7) ) ) );
		const uint8_t mask      = (1<<_numBits) - 1;

		for (uint32_t begin = _beginBit / 8, end = alignUp(_endBit, 8) / 8, ii = begin; ii < end; ++ii)
		{
			uint16_t test = bits[ii];
			test |= ii == begin ? beginMask : 0;
			test |= ii == end - 1 ? endMask : 0;
			test |= 0xff00;

			if (0xffff == test)
			{
				continue;
			}

			for (uint32_t pos = ii*8, posEnd = pos+7; pos < posEnd; ++pos)
			{
				const uint8_t tz = countTrailingZeros(test^0xff);
				test = int16_t(test) >> tz;

				pos += tz;

				if (0 == (test & mask) )
				{
					return pos;
				}

				test = int16_t(test) >> 1;
			}
		}

		return _endBit;
	}

	inline uint32_t bitSetFindZero(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit)
	{
		const uint32_t begin = _beginBit/64;

		for (uint32_t ii = begin, end = alignUp(_endBit, 64)/64; ii < end; ++ii)
		{
			uint64_t zeros = ~_ptr[ii];
			zeros &= ii == begin ? UINT64_MAX << (_beginBit & 63) : UINT64_MAX;

			if (0 != zeros)
			{
				const uint32_t idx = ii*64 + countTrailingZeros(zeros);
				return idx < _endBit ? idx : kInvalid;
			}
		}

		return kInvalid;
	}

	inline uint32_t bitSetFind(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
	{
		if (1 == _numBits)
		{
			return bitSetFindZero(_ptr, _beginBit, _endBit);
		}

		if (_numBits >= 127)
		{
			const uint32_t idx = bitSetFindT<uint64_t>(_ptr, _beginBit, _endBit, (_numBits+63)>>6);
			if (idx != _endBit)
			{
				return idx;
			}
		}

		if (_numBits >= 63)
		{
			const uint32_t idx = bitSetFindT<uint32_t>(_ptr, _beginBit, _endBit, (_numBits+31)>>5);
			if (idx != _endBit)
			{
				return idx;
			}
		}

		if (_numBits >= 128)
		{
			return kInvalid;
		}

		if (_numBits >= 31)
		{
			const uint32_t idx = bitSetFindT<uint16_t>(_ptr, _beginBit, _endBit, (_numBits+15)>>4);
			if (idx != _endBit)
			{
				return idx;
			}
		}

		if (_numBits >= 64)
		{
			return kInvalid;
		}

		uint32_t idx = bitSetFindT<uint8_t>(_ptr, _beginBit, _endBit, (_numBits+7)>>3);
		if (idx != _endBit)
		{
			return idx;
		}

		if (_numBits < 15)
		{
			idx = bitSetFindUpto14(_ptr, _beginBit, _endBit, _numBits);

			if (idx != _endBit)
			{
				return idx;
			}

			if (_numBits < 7)
			{
				idx = bitSetFindUpto6(_ptr, _beginBit, _endBit, _numBits);

				if (idx != _endBit)
				{
					return idx;
				}
			}
		}

		return kInvalid;
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
		uint32_t from = 0;
		uint32_t mask = 0;
		const uint32_t num = m_sizeInBytes/8;

		if (kLast == _element)
		{
			from = num;
			mask = UINT32_MAX;
		}

		for (uint32_t ii = 0; ii < num; ++ii)
		{
			const uint32_t idx = ( (~from + 1) + ii) ^ mask;
			const uint64_t word = m_ptr[idx];

			if (0 == word)
			{
				continue;
			}

			if (kLast == _element)
			{
				return idx*64 + findLastSet(word) - 1;
			}

			return idx*64 + findFirstSet(word) - 1;
		}

		return kInvalid;
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
		return bitSetTestAny(m_ptr, _beginBit, _endBit);
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
		return bitSetTestAll(m_ptr, _beginBit, _endBit);
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
		return bitSetTestNone(m_ptr, _beginBit, _endBit);
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

} // namespace bx
