/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_RADIXSORT_H__
#define __BX_RADIXSORT_H__

#include "bx.h"

namespace bx
{
#define BX_RADIXSORT_BITS 11
#define BX_RADIXSORT_HISTOGRAM_SIZE (1<<BX_RADIXSORT_BITS)
#define BX_RADIXSORT_BIT_MASK (BX_RADIXSORT_HISTOGRAM_SIZE-1)

	template <typename Ty>
	void radixSort32(uint32_t* _keys, uint32_t* _tempKeys, Ty* _values, Ty* _tempValues, uint32_t _size)
	{
		uint16_t histogram[BX_RADIXSORT_HISTOGRAM_SIZE];
		uint16_t shift = 0;
		for (uint32_t pass = 0; pass < 3; ++pass)
		{
			memset(histogram, 0, sizeof(uint16_t)*BX_RADIXSORT_HISTOGRAM_SIZE);
			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				uint32_t key = _keys[ii];
				uint16_t index = (key>>shift)&BX_RADIXSORT_BIT_MASK;
				++histogram[index];
			}

			uint16_t offset = 0;
			for (uint32_t ii = 0; ii < BX_RADIXSORT_HISTOGRAM_SIZE; ++ii)
			{
				uint16_t count = histogram[ii];
				histogram[ii] = offset;
				offset += count;
			}

			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				uint32_t key = _keys[ii];
				uint16_t index = (key>>shift)&BX_RADIXSORT_BIT_MASK;
				uint16_t dest = histogram[index]++;
				_tempKeys[dest] = key;
				_tempValues[dest] = _values[ii];
			}

			uint32_t* swapKeys = _tempKeys;
			_tempKeys = _keys;
			_keys = swapKeys;

			Ty* swapValues = _tempValues;
			_tempValues = _values;
			_values = swapValues;

			shift += BX_RADIXSORT_BITS;
		}
	}

	template <typename Ty>
	void radixSort64(uint64_t* _keys, uint64_t* _tempKeys, Ty* _values, Ty* _tempValues, uint32_t _size)
	{
		uint16_t histogram[BX_RADIXSORT_HISTOGRAM_SIZE];
		uint16_t shift = 0;
		for (uint32_t pass = 0; pass < 6; ++pass)
		{
			memset(histogram, 0, sizeof(uint16_t)*BX_RADIXSORT_HISTOGRAM_SIZE);
			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				uint64_t key = _keys[ii];
				uint16_t index = (key>>shift)&BX_RADIXSORT_BIT_MASK;
				++histogram[index];
			}

			uint16_t offset = 0;
			for (uint32_t ii = 0; ii < BX_RADIXSORT_HISTOGRAM_SIZE; ++ii)
			{
				uint16_t count = histogram[ii];
				histogram[ii] = offset;
				offset += count;
			}

			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				uint64_t key = _keys[ii];
				uint16_t index = (key>>shift)&BX_RADIXSORT_BIT_MASK;
				uint16_t dest = histogram[index]++;
				_tempKeys[dest] = key;
				_tempValues[dest] = _values[ii];
			}

			uint64_t* swapKeys = _tempKeys;
			_tempKeys = _keys;
			_keys = swapKeys;

			Ty* swapValues = _tempValues;
			_tempValues = _values;
			_values = swapValues;

			shift += BX_RADIXSORT_BITS;
		}
	}

#undef BX_RADIXSORT_BITS
#undef BX_RADIXSORT_HISTOGRAM_SIZE
#undef BX_RADIXSORT_BIT_MASK

} // namespace bx

#endif // __BX_RADIXSORT_H__
