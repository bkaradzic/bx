/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_RNG_H__
#define __BX_RNG_H__

#include "bx.h"

namespace bx
{
	// George Marsaglia's MWC
	class RngMwc
	{
	public:
		RngMwc(uint32_t _z = 12345, uint32_t _w = 65435)
			: m_z(_z)
			, m_w(_w)
		{
		}

		void reset(uint32_t _z = 12345, uint32_t _w = 65435)
		{
			m_z = _z;
			m_w = _w;
		}

		uint32_t gen()
		{
			m_z = 36969*(m_z&65535)+(m_z>>16);
			m_w = 18000*(m_w&65535)+(m_w>>16);
			return (m_z<<16)+m_w;
		}

	private:
		uint32_t m_z;
		uint32_t m_w;
	};

	// George Marsaglia's FIB
	class RngFib
	{
	public:
		RngFib()
			: m_a(9983651)
			, m_b(95746118)
		{
		}

		void reset()
		{
			m_a = 9983651;
			m_b = 95746118;
		}

		uint32_t gen()
		{
			m_b = m_a+m_b;
			m_a = m_b-m_a;
			return m_a;
		}

	private:
		uint32_t m_a;
		uint32_t m_b;
	};

	// George Marsaglia's SHR3
	class RngShr3
	{
	public:
		RngShr3(uint32_t _jsr = 34221)
			: m_jsr(_jsr)
		{
		}

		void reset(uint32_t _jsr = 34221)
		{
			m_jsr = _jsr;
		}

		uint32_t gen()
		{
			m_jsr ^= m_jsr<<17;
			m_jsr ^= m_jsr>>13;
			m_jsr ^= m_jsr<<5;
			return m_jsr;
		}

	private:
		uint32_t m_jsr;
	};

	/// Returns random number between 0.0f and 1.0f.
	template <typename Ty>
	inline float frnd(Ty* _rng)
	{
		uint32_t rnd = _rng->gen() & UINT16_MAX;
		return float(rnd) * 1.0f/float(UINT16_MAX);
	}

	/// Returns random number between -1.0f and 1.0f.
	template <typename Ty>
	inline float frndh(Ty* _rng)
	{
		return 2.0f * bx::frnd(_rng) - 1.0f;
	}

} // namespace bx

#endif // __BX_RNG_H__
