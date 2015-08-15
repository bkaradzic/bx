/*
 * Copyright 2010-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef BX_HASH_H_HEADER_GUARD
#define BX_HASH_H_HEADER_GUARD

#include "bx.h"

namespace bx
{
// MurmurHash2 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#define MURMUR_M 0x5bd1e995
#define MURMUR_R 24
#define mmix(_h, _k) { _k *= MURMUR_M; _k ^= _k >> MURMUR_R; _k *= MURMUR_M; _h *= MURMUR_M; _h ^= _k; }

	class HashMurmur2A
	{
	public:
		void begin(uint32_t _seed = 0)
		{
			m_hash = _seed;
			m_tail = 0;
			m_count = 0;
			m_size = 0;
		}

		void add(const void* _data, int _len)
		{
			const uint8_t* data = (uint8_t*)_data;
			m_size += _len;

			mixTail(data, _len);

			while(_len >= 4)
			{
#if BX_PLATFORM_EMSCRIPTEN
				uint32_t kk = data[0]<<24|data[1]<<16|data[2]<<8|data[3];
#else
				uint32_t kk = *(uint32_t*)data;
#endif

				mmix(m_hash, kk);

				data += 4;
				_len -= 4;
			}

			mixTail(data, _len);
		}

		template<typename Ty>
		void add(Ty _value)
		{
			add(&_value, sizeof(Ty) );
		}

		uint32_t end()
		{
			mmix(m_hash, m_tail);
			mmix(m_hash, m_size);

			m_hash ^= m_hash >> 13;
			m_hash *= MURMUR_M;
			m_hash ^= m_hash >> 15;

			return m_hash;
		}

	private:
		void mixTail(const uint8_t*& _data, int& _len)
		{
			while( _len && ((_len<4) || m_count) )
			{
				m_tail |= (*_data++) << (m_count * 8);

				m_count++;
				_len--;

				if(m_count == 4)
				{
					mmix(m_hash, m_tail);
					m_tail = 0;
					m_count = 0;
				}
			}
		}

		uint32_t m_hash;
		uint32_t m_tail;
		uint32_t m_count;
		uint32_t m_size;
	};

#undef MURMUR_M
#undef MURMUR_R
#undef mmix

	inline uint32_t hashMurmur2A(const void* _data, uint32_t _size)
	{
		HashMurmur2A murmur;
		murmur.begin();
		murmur.add(_data, (int)_size);
		return murmur.end();
	}

	template <typename Ty>
	inline uint32_t hashMurmur2A(const Ty& _data)
	{
		return hashMurmur2A(&_data, sizeof(Ty) );
	}

} // namespace bx

#endif // BX_HASH_H_HEADER_GUARD
