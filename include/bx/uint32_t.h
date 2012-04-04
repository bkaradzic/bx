/*
 * Copyright 2010-2012 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

// Copyright 2006 Mike Acton <macton@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE

#ifndef __BX_UINT32_T_H__
#define __BX_UINT32_T_H__

#include "bx.h"

#if BX_COMPILER_MSVC
#	if BX_PLATFORM_WINDOWS
#		include <math.h> // math.h is included because VS bitches:
						 // warning C4985: 'ceil': attributes not present on previous declaration.
						 // must be included before intrin.h.
#		include <intrin.h>
#		pragma intrinsic(_BitScanForward)
#		pragma intrinsic(_BitScanReverse)
#	endif // BX_PLATFORM_WINDOWS
#endif // BX_COMPILER_MSVC

namespace bx
{
	inline uint32_t uint32_li(uint32_t _a)
	{
		return _a;
	}

	inline uint32_t uint32_dec(uint32_t _a)
	{
		return _a - 1;
	}

	inline uint32_t uint32_inc(uint32_t _a)
	{
		return _a + 1;
	}

	inline uint32_t uint32_not(uint32_t _a)
	{
		return ~_a;
	}

	inline uint32_t uint32_neg(uint32_t _a)
	{
		return -(int32_t)_a;
	}

	inline uint32_t uint32_ext(uint32_t _a)
	{
		return ( (int32_t)_a)>>31;
	}

	inline uint32_t uint32_and(uint32_t _a, uint32_t _b)
	{
		return _a & _b;
	}

	inline uint32_t uint32_xor(uint32_t _a, uint32_t _b)
	{
		return _a ^ _b;
	}

	inline uint32_t uint32_xorl(uint32_t _a, uint32_t _b)
	{
		return !_a != !_b;
	}

	inline uint32_t uint32_andc(uint32_t _a, uint32_t _b)
	{
		return _a & ~_b;
	}

	inline uint32_t uint32_or(uint32_t _a, uint32_t _b)
	{
		return _a | _b;
	}

	inline uint32_t uint32_sll(uint32_t _a, int _sa)
	{
		return _a << _sa;
	}

	inline uint32_t uint32_srl(uint32_t _a, int _sa)
	{
		return _a >> _sa;
	}

	inline uint32_t uint32_sra(uint32_t _a, int _sa)
	{
		return ( (int32_t)_a) >> _sa;
	}

	inline uint32_t uint32_rol(uint32_t _a, int _sa)
	{
		return ( _a << _sa) | (_a >> (32-_sa) );
	}

	inline uint32_t uint32_ror(uint32_t _a, int _sa)
	{
		return ( _a >> _sa) | (_a << (32-_sa) );
	}

	inline uint32_t uint32_add(uint32_t _a, uint32_t _b)
	{
		return _a + _b;
	}

	inline uint32_t uint32_sub(uint32_t _a, uint32_t _b)
	{
		return _a - _b;
	}

	inline uint32_t uint32_mul(uint32_t _a, uint32_t _b)
	{
		return _a * _b;
	}

	inline uint32_t uint32_div(uint32_t _a, uint32_t _b)
	{
		return (_a / _b);
	}

	inline uint32_t uint32_mod(uint32_t _a, uint32_t _b)
	{
		return (_a % _b);
	}

	inline uint32_t uint32_cmpeq(uint32_t _a, uint32_t _b)
	{
		return -(_a == _b);
	}

	inline uint32_t uint32_cmpneq(uint32_t _a, uint32_t _b)
	{
		return -(_a != _b);
	}

	inline uint32_t uint32_cmplt(uint32_t _a, uint32_t _b)
	{
		return -(_a < _b);
	}

	inline uint32_t uint32_cmple(uint32_t _a, uint32_t _b)
	{
		return -(_a <= _b);
	}

	inline uint32_t uint32_cmpgt(uint32_t _a, uint32_t _b)
	{
		return -(_a > _b);
	}

	inline uint32_t uint32_cmpge(uint32_t _a, uint32_t _b)
	{
		return -(_a >= _b);
	}

	inline uint32_t uint32_setnz(uint32_t _a)
	{
		return -!!_a;
	}

	inline uint32_t uint32_satadd(uint32_t _a, uint32_t _b)
	{
		const uint32_t add    = uint32_add(_a, _b);
		const uint32_t lt     = uint32_cmplt(add, _a);
		const uint32_t result = uint32_or(add, lt);

		return result;
	}

	inline uint32_t uint32_satsub(uint32_t _a, uint32_t _b)
	{
		const uint32_t sub    = uint32_sub(_a, _b);
		const uint32_t le     = uint32_cmple(sub, _a);
		const uint32_t result = uint32_and(sub, le);

		return result;
	}

	inline uint32_t uint32_satmul(uint32_t _a, uint32_t _b)
	{
		const uint64_t mul    = (uint64_t)_a * (uint64_t)_b;
		const uint32_t hi     = mul >> 32;
		const uint32_t nz     = uint32_setnz(hi);
		const uint32_t result = uint32_or(uint32_t(mul), nz);

		return result;
	}

	inline uint32_t uint32_sels(uint32_t test, uint32_t _a, uint32_t _b)
	{
		const uint32_t mask   = uint32_ext(test);
		const uint32_t sel_a  = uint32_and(_a, mask);
		const uint32_t sel_b  = uint32_andc(_b, mask);
		const uint32_t result = uint32_or(sel_a, sel_b);

		return (result);
	}

	inline uint32_t uint32_selb(uint32_t _mask, uint32_t _a, uint32_t _b)
	{
		const uint32_t sel_a  = uint32_and(_a, _mask);
		const uint32_t sel_b  = uint32_andc(_b, _mask);
		const uint32_t result = uint32_or(sel_a, sel_b);

		return (result);
	}

	inline uint32_t uint32_imin(uint32_t _a, uint32_t _b)
	{
		const uint32_t a_sub_b = uint32_sub(_a, _b);
		const uint32_t result  = uint32_sels(a_sub_b, _a, _b);

		return result;
	}

	inline uint32_t uint32_imax(uint32_t _a, uint32_t _b)
	{
		const uint32_t b_sub_a = uint32_sub(_b, _a);
		const uint32_t result  = uint32_sels(b_sub_a, _a, _b);

		return result;
	}

	inline uint32_t uint32_min(uint32_t _a, uint32_t _b)
	{
		return _a > _b ? _b : _a;
	}

	inline uint32_t uint32_max(uint32_t _a, uint32_t _b)
	{
		return _a > _b ? _a : _b;
	}

	inline uint32_t uint32_incwrap(uint32_t _val, uint32_t _min, uint32_t _max)
	{
		const uint32_t inc          = uint32_inc(_val);
		const uint32_t max_diff     = uint32_sub(_max, _val);
		const uint32_t neg_max_diff = uint32_neg(max_diff);
		const uint32_t max_or       = uint32_or(max_diff, neg_max_diff);
		const uint32_t max_diff_nz  = uint32_ext(max_or);
		const uint32_t result       = uint32_selb(max_diff_nz, inc, _min);

		return result;
	}

	inline uint32_t uint32_decwrap(uint32_t _val, uint32_t _min, uint32_t _max)
	{
		const uint32_t dec          = uint32_dec(_val);
		const uint32_t min_diff     = uint32_sub(_min, _val);
		const uint32_t neg_min_diff = uint32_neg(min_diff);
		const uint32_t min_or       = uint32_or(min_diff, neg_min_diff);
		const uint32_t min_diff_nz  = uint32_ext(min_or);
		const uint32_t result       = uint32_selb(min_diff_nz, dec, _max);

		return result;
	}

	inline uint32_t uint32_cntbits_ref(uint32_t _val)
	{
		const uint32_t tmp0   = uint32_srl(_val, 1);
		const uint32_t tmp1   = uint32_and(tmp0, 0x55555555);
		const uint32_t tmp2   = uint32_sub(_val, tmp1);
		const uint32_t tmp3   = uint32_and(tmp2, 0xc30c30c3);
		const uint32_t tmp4   = uint32_srl(tmp2, 2);
		const uint32_t tmp5   = uint32_and(tmp4, 0xc30c30c3);
		const uint32_t tmp6   = uint32_srl(tmp2, 4);
		const uint32_t tmp7   = uint32_and(tmp6, 0xc30c30c3);
		const uint32_t tmp8   = uint32_add(tmp3, tmp5);
		const uint32_t tmp9   = uint32_add(tmp7, tmp8);
		const uint32_t tmpA   = uint32_srl(tmp9, 6);
		const uint32_t tmpB   = uint32_add(tmp9, tmpA);
		const uint32_t tmpC   = uint32_srl(tmpB, 12);
		const uint32_t tmpD   = uint32_srl(tmpB, 24);
		const uint32_t tmpE   = uint32_add(tmpB, tmpC);
		const uint32_t tmpF   = uint32_add(tmpD, tmpE);
		const uint32_t result = uint32_and(tmpF, 0x3f);

		return result;
	}

	/// Count number of bits set.
	inline uint32_t uint32_cntbits(uint32_t _val)
	{
#if BX_COMPILER_GCC
		return __builtin_popcount(_val);
#elif BX_COMPILER_MSVC && BX_PLATFORM_WINDOWS
		return __popcnt(_val);
#else
		return uint32_cntbits_ref(_val);
#endif // BX_COMPILER_GCC
	}

	inline uint32_t uint32_cntlz_ref(uint32_t _val)
	{
		const uint32_t tmp0   = uint32_srl(_val, 1);
		const uint32_t tmp1   = uint32_or(tmp0, _val);
		const uint32_t tmp2   = uint32_srl(tmp1, 2);
		const uint32_t tmp3   = uint32_or(tmp2, tmp1);
		const uint32_t tmp4   = uint32_srl(tmp3, 4);
		const uint32_t tmp5   = uint32_or(tmp4, tmp3);
		const uint32_t tmp6   = uint32_srl(tmp5, 8);
		const uint32_t tmp7   = uint32_or(tmp6, tmp5);
		const uint32_t tmp8   = uint32_srl(tmp7, 16);
		const uint32_t tmp9   = uint32_or(tmp8, tmp7);
		const uint32_t tmpA   = uint32_not(tmp9);
		const uint32_t result = uint32_cntbits(tmpA);

		return result;
	}

	/// Count number of leading zeros.
	inline uint32_t uint32_cntlz(uint32_t _val)
	{
#if BX_COMPILER_GCC
		return __builtin_clz(_val);
#elif BX_COMPILER_MSVC && BX_PLATFORM_WINDOWS
		unsigned long index;
		_BitScanReverse(&index, _val);
		return 31 - index;
#else
		return uint32_cntlz_ref(_val);
#endif // BX_COMPILER_
	}

	inline uint32_t uint32_cnttz_ref(uint32_t _val)
	{
		const uint32_t tmp0   = uint32_not(_val);
		const uint32_t tmp1   = uint32_dec(_val);
		const uint32_t tmp2   = uint32_and(tmp0, tmp1);
		const uint32_t result = uint32_cntbits(tmp2);

		return result;
	}

	inline uint32_t uint32_cnttz(uint32_t _val)
	{
#if BX_COMPILER_MSVC && BX_PLATFORM_WINDOWS
		unsigned long index;
		_BitScanForward(&index, _val);
		return index;
#else
		return uint32_cnttz_ref(_val);
#endif // BX_COMPILER_
	}

	// shuffle:
	// ---- ---- ---- ---- fedc ba98 7654 3210
	// to:
	// -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
	inline uint32_t uint32_part1by1(uint32_t _a)
	{
		const uint32_t val    = uint32_and(_a, 0xffff);

		const uint32_t tmp0   = uint32_sll(val, 8);
		const uint32_t tmp1   = uint32_xor(val, tmp0);
		const uint32_t tmp2   = uint32_and(tmp1, 0x00ff00ff);

		const uint32_t tmp3   = uint32_sll(tmp2, 4);
		const uint32_t tmp4   = uint32_xor(tmp2, tmp3);
		const uint32_t tmp5   = uint32_and(tmp4, 0x0f0f0f0f);

		const uint32_t tmp6   = uint32_sll(tmp5, 2);
		const uint32_t tmp7   = uint32_xor(tmp5, tmp6);
		const uint32_t tmp8   = uint32_and(tmp7, 0x33333333);

		const uint32_t tmp9   = uint32_sll(tmp8, 1);
		const uint32_t tmpA   = uint32_xor(tmp8, tmp9);
		const uint32_t result = uint32_and(tmpA, 0x55555555);

		return result;
	}

	// shuffle:
	// ---- ---- ---- ---- ---- --98 7654 3210
	// to:
	// ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
	inline uint32_t uint32_part1by2(uint32_t _a)
	{
		const uint32_t val    = uint32_and(_a, 0x3ff);

		const uint32_t tmp0   = uint32_sll(val, 16);
		const uint32_t tmp1   = uint32_xor(val, tmp0);
		const uint32_t tmp2   = uint32_and(tmp1, 0xff0000ff);

		const uint32_t tmp3   = uint32_sll(tmp2, 8);
		const uint32_t tmp4   = uint32_xor(tmp2, tmp3);
		const uint32_t tmp5   = uint32_and(tmp4, 0x0300f00f);

		const uint32_t tmp6   = uint32_sll(tmp5, 4);
		const uint32_t tmp7   = uint32_xor(tmp5, tmp6);
		const uint32_t tmp8   = uint32_and(tmp7, 0x030c30c3);

		const uint32_t tmp9   = uint32_sll(tmp8, 2);
		const uint32_t tmpA   = uint32_xor(tmp8, tmp9);
		const uint32_t result = uint32_and(tmpA, 0x09249249);

		return result;
	}

	inline uint32_t uint32_testpow2(uint32_t _a)
	{
		const uint32_t tmp0   = uint32_not(_a);
		const uint32_t tmp1   = uint32_inc(tmp0);
		const uint32_t tmp2   = uint32_and(_a, tmp1);
		const uint32_t tmp3   = uint32_cmpeq(tmp2, _a);
		const uint32_t tmp4   = uint32_cmpneq(_a, 0);
		const uint32_t result = uint32_and(tmp3, tmp4);

		return result;
	}

	inline uint32_t uint32_nextpow2(uint32_t _a)
	{
		const uint32_t tmp0   = uint32_dec(_a);
		const uint32_t tmp1   = uint32_srl(tmp0, 1);
		const uint32_t tmp2   = uint32_or(tmp0, tmp1);
		const uint32_t tmp3   = uint32_srl(tmp2, 2);
		const uint32_t tmp4   = uint32_or(tmp2, tmp3);
		const uint32_t tmp5   = uint32_srl(tmp4, 4);
		const uint32_t tmp6   = uint32_or(tmp4, tmp5);
		const uint32_t tmp7   = uint32_srl(tmp6, 8);
		const uint32_t tmp8   = uint32_or(tmp6, tmp7);
		const uint32_t tmp9   = uint32_srl(tmp8, 16);
		const uint32_t tmpA   = uint32_or(tmp8, tmp9);
		const uint32_t result = uint32_inc(tmpA);

		return result;
	}
} // namespace bx

#endif // __BX_UINT32_T_H__
