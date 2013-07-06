/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_HANDLE_ALLOC_H__
#define __BX_HANDLE_ALLOC_H__

#include "bx.h"

namespace bx
{
	class HandleAlloc
	{
	public:
		static const uint16_t invalid = 0xffff;

		HandleAlloc(uint16_t _maxHandles)
			: m_dense(new uint16_t[_maxHandles*2])
			, m_sparse(&m_dense[_maxHandles])
			, m_numHandles(0)
			, m_maxHandles(_maxHandles)
		{
			for (uint16_t ii = 0; ii < _maxHandles; ++ii)
			{
				m_dense[ii] = ii;
			}
		}

		~HandleAlloc()
		{
			delete [] m_dense;
		}

		const uint16_t* getHandles() const
		{
			return m_dense;
		}

		uint16_t getHandleAt(uint16_t _at) const
		{
			return m_dense[_at];
		}

		uint16_t getNumHandles() const
		{
			return m_numHandles;
		}

		uint16_t getMaxHandles() const
		{
			return m_maxHandles;
		}

		uint16_t alloc()
		{
			if (m_numHandles < m_maxHandles)
			{
				uint16_t index = m_numHandles;
				++m_numHandles;

				uint16_t handle = m_dense[index];
				m_sparse[handle] = index;
				return handle;
			}

			return invalid;
		}

		void free(uint16_t _handle)
		{
			uint16_t index = m_sparse[_handle];
			--m_numHandles;
			uint16_t temp = m_dense[m_numHandles];
			m_dense[m_numHandles] = _handle;
			m_sparse[temp] = index;
			m_dense[index] = temp;
		}

	private:
		uint16_t* m_dense;
		uint16_t* m_sparse;
		uint16_t m_numHandles;
		uint16_t m_maxHandles;
	};
} // namespace bx

#endif // __HANDLE_ALLOC_H__
