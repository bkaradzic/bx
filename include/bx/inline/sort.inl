/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_SORT_H_HEADER_GUARD
#	error "Must be included from bx/sort.h!"
#endif // BX_SORT_H_HEADER_GUARD

namespace bx
{
	template<typename Ty>
	inline int32_t compareAscending(const void* _lhs, const void* _rhs)
	{
		const Ty lhs = *static_cast<const Ty*>(_lhs);
		const Ty rhs = *static_cast<const Ty*>(_rhs);
		return (lhs > rhs) - (lhs < rhs);
	}

	template<typename Ty>
	inline int32_t compareDescending(const void* _lhs, const void* _rhs)
	{
		return compareAscending<Ty>(_rhs, _lhs);
	}

	template<>
	inline int32_t compareAscending<const char*>(const void* _lhs, const void* _rhs)
	{
		return strCmp(*(const char**)_lhs, *(const char**)_rhs);
	}

	template<>
	inline int32_t compareAscending<StringLiteral>(const void* _lhs, const void* _rhs)
	{
		return strCmp(*(const StringLiteral*)_lhs, *(const StringLiteral*)_rhs);
	}

	template<>
	inline int32_t compareAscending<StringView>(const void* _lhs, const void* _rhs)
	{
		return strCmp(*(const StringView*)_lhs, *(const StringView*)_rhs);
	}

	template<typename Ty>
	inline void quickSort(void* _data, uint32_t _num, uint32_t _stride, const ComparisonFn _fn)
	{
		static_assert(isTriviallyMoveAssignable<Ty>(), "Element type must be trivially move assignable");
		quickSort(_data, _num, _stride, _fn);
	}

	template<typename Ty>
	inline void quickSort(Ty* _data, uint32_t _num, const ComparisonFn _fn)
	{
		static_assert(isTriviallyMoveAssignable<Ty>(), "Element type must be trivially move assignable");
		quickSort( (void*)_data, _num, sizeof(Ty), _fn);
	}

	constexpr uint32_t kStableSortInsertionRun = 32;

	template<typename Ty, typename LessFn>
	inline void insertionSort(Ty* _data, uint32_t _num, LessFn _less)
	{
		for (uint32_t ii = 1; ii < _num; ++ii)
		{
			Ty item = _data[ii];

			uint32_t jj = ii;
			for (; jj > 0 && _less(item, _data[jj-1]); --jj)
			{
				_data[jj] = _data[jj-1];
			}

			_data[jj] = item;
		}
	}

	template<typename Ty, typename LessFn>
	inline void stableSort(AllocatorI* _allocator, Ty* _data, uint32_t _num, LessFn _less)
	{
		if (2 > _num)
		{
			return;
		}

		for (uint32_t ii = 0; ii < _num; ii += kStableSortInsertionRun)
		{
			insertionSort(_data + ii, min(kStableSortInsertionRun, _num - ii), _less);
		}

		if (kStableSortInsertionRun >= _num)
		{
			return;
		}

		Ty* scratch = (Ty*)alloc(_allocator, _num*sizeof(Ty) );
		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			BX_PLACEMENT_NEW(&scratch[ii], Ty);
		}

		Ty* src = _data;
		Ty* dst = scratch;

		for (uint32_t width = kStableSortInsertionRun; width < _num; width <<= 1)
		{
			for (uint32_t ii = 0; ii < _num; ii += width<<1)
			{
				const uint32_t mid = min(ii +  width,     _num);
				const uint32_t end = min(ii + (width<<1), _num);

				uint32_t ll = ii;
				uint32_t rr = mid;
				uint32_t oo = ii;

				while (ll < mid
				&&     rr < end)
				{
					if (_less(src[rr], src[ll]) )
					{
						dst[oo] = src[rr];
						++rr;
					}
					else
					{
						dst[oo] = src[ll];
						++ll;
					}

					++oo;
				}

				for (; ll < mid; ++ll, ++oo)
				{
					dst[oo] = src[ll];
				}

				for (; rr < end; ++rr, ++oo)
				{
					dst[oo] = src[rr];
				}
			}

			swap(src, dst);
		}

		if (src != _data)
		{
			for (uint32_t ii = 0; ii < _num; ++ii)
			{
				_data[ii] = src[ii];
			}
		}

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			scratch[ii].~Ty();
		}

		free(_allocator, scratch);
	}

	template<typename ContainerT, typename LessFn>
	inline void stableSort(AllocatorI* _allocator, ContainerT& _container, LessFn _less)
	{
		if (!_container.empty() )
		{
			stableSort(_allocator, &_container[0], uint32_t(_container.size() ), _less);
		}
	}

	template<typename Ty>
	inline uint32_t unique(void* _data, uint32_t _num, uint32_t _stride, const ComparisonFn _fn)
	{
		static_assert(isTriviallyMoveAssignable<Ty>(), "Element type must be trivially move assignable");
		return unique(_data, _num, _stride, _fn);
	}

	template<typename Ty>
	inline uint32_t unique(Ty* _data, uint32_t _num, const ComparisonFn _fn)
	{
		static_assert(isTriviallyMoveAssignable<Ty>(), "Element type must be trivially move assignable");
		return unique( (void*)_data, _num, sizeof(Ty), _fn);
	}

	template<typename Ty>
	inline uint32_t lowerBound(const Ty& _key, const Ty* _data, uint32_t _num, const ComparisonFn _fn)
	{
		return lowerBound( (const void*)&_key, _data, _num, sizeof(Ty), _fn);
	}

	template<typename Ty>
	inline uint32_t lowerBound(const Ty& _key, const void* _data, uint32_t _num, uint32_t _stride, const ComparisonFn _fn)
	{
		return lowerBound( (const void*)&_key, _data, _num, _stride, _fn);
	}

	template<typename Ty>
	inline uint32_t upperBound(const Ty& _key, const Ty* _data, uint32_t _num, const ComparisonFn _fn)
	{
		return upperBound( (const void*)&_key, _data, _num, sizeof(Ty), _fn);
	}

	template<typename Ty>
	inline uint32_t upperBound(const Ty& _key, const void* _data, uint32_t _num, uint32_t _stride, const ComparisonFn _fn)
	{
		return upperBound( (const void*)&_key, _data, _num, _stride, _fn);
	}

	template<typename Ty>
	inline bool isSorted(const Ty* _data, uint32_t _num, const ComparisonFn _fn)
	{
		return isSorted(_data, _num, sizeof(Ty), _fn);
	}

	template<typename Ty>
	inline bool isSorted(const void* _data, uint32_t _num, uint32_t _stride, const ComparisonFn _fn)
	{
		return isSorted(_data, _num, _stride, _fn);
	}

	template<typename Ty>
	inline int32_t binarySearch(const Ty& _key, const Ty* _data, uint32_t _num, const ComparisonFn _fn)
	{
		return binarySearch( (const void*)&_key, _data, _num, sizeof(Ty), _fn);
	}

	template<typename Ty>
	inline int32_t binarySearch(const Ty& _key, const void* _data, uint32_t _num, uint32_t _stride, const ComparisonFn _fn)
	{
		return binarySearch( (const void*)&_key, _data, _num, _stride, _fn);
	}

	namespace radix_sort_detail
	{
		constexpr uint32_t kBits          = 11;
		constexpr uint32_t kHistogramSize = 1<<kBits;
		constexpr uint32_t kBitMask       = kHistogramSize-1;

	} // namespace radix_sort_detail

	inline void radixSort(uint32_t* _keys, uint32_t* _tempKeys, uint32_t _size)
	{
		uint32_t* keys = _keys;
		uint32_t* tempKeys = _tempKeys;

		uint32_t histogram[radix_sort_detail::kHistogramSize];
		uint16_t shift = 0;
		uint32_t pass = 0;
		for (; pass < 3; ++pass)
		{
			memSet(histogram, 0, sizeof(uint32_t)*radix_sort_detail::kHistogramSize);

			bool sorted = true;
			{
				uint32_t key = keys[0];
				uint32_t prevKey = key;
				for (uint32_t ii = 0; ii < _size; ++ii, prevKey = key)
				{
					key = keys[ii];
					uint16_t index = (key>>shift)&radix_sort_detail::kBitMask;
					++histogram[index];
					sorted &= prevKey <= key;
				}
			}

			if (sorted)
			{
				goto done;
			}

			uint32_t offset = 0;
			for (uint32_t ii = 0; ii < radix_sort_detail::kHistogramSize; ++ii)
			{
				uint32_t count = histogram[ii];
				histogram[ii] = offset;
				offset += count;
			}

			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				uint32_t key = keys[ii];
				uint16_t index = (key>>shift)&radix_sort_detail::kBitMask;
				uint32_t dest = histogram[index]++;
				tempKeys[dest] = key;
			}

			uint32_t* swapKeys = tempKeys;
			tempKeys = keys;
			keys = swapKeys;

			shift += radix_sort_detail::kBits;
		}

done:
		if (0 != (pass&1) )
		{
			// Odd number of passes needs to do copy to the destination.
			memCopy(_keys, _tempKeys, _size*sizeof(uint32_t) );
		}
	}

	template <typename Ty>
	inline void radixSort(uint32_t* _keys, uint32_t* _tempKeys, Ty* _values, Ty* _tempValues, uint32_t _size)
	{
		static_assert(isTriviallyMoveAssignable<Ty>(), "Sort element type must be trivially move assignable");

		uint32_t* keys = _keys;
		uint32_t* tempKeys = _tempKeys;
		Ty* values = _values;
		Ty* tempValues = _tempValues;

		uint32_t histogram[radix_sort_detail::kHistogramSize];
		uint16_t shift = 0;
		uint32_t pass = 0;
		for (; pass < 3; ++pass)
		{
			memSet(histogram, 0, sizeof(uint32_t)*radix_sort_detail::kHistogramSize);

			bool sorted = true;
			{
				uint32_t key = keys[0];
				uint32_t prevKey = key;
				for (uint32_t ii = 0; ii < _size; ++ii, prevKey = key)
				{
					key = keys[ii];
					uint16_t index = (key>>shift)&radix_sort_detail::kBitMask;
					++histogram[index];
					sorted &= prevKey <= key;
				}
			}

			if (sorted)
			{
				goto done;
			}

			uint32_t offset = 0;
			for (uint32_t ii = 0; ii < radix_sort_detail::kHistogramSize; ++ii)
			{
				uint32_t count = histogram[ii];
				histogram[ii] = offset;
				offset += count;
			}

			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				uint32_t key = keys[ii];
				uint16_t index = (key>>shift)&radix_sort_detail::kBitMask;
				uint32_t dest = histogram[index]++;
				tempKeys[dest] = key;
				tempValues[dest] = values[ii];
			}

			uint32_t* swapKeys = tempKeys;
			tempKeys = keys;
			keys = swapKeys;

			Ty* swapValues = tempValues;
			tempValues = values;
			values = swapValues;

			shift += radix_sort_detail::kBits;
		}

done:
		if (0 != (pass&1) )
		{
			// Odd number of passes needs to do copy to the destination.
			memCopy(_keys, _tempKeys, _size*sizeof(uint32_t) );
			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				_values[ii] = _tempValues[ii];
			}
		}
	}

	inline void radixSort(uint64_t* _keys, uint64_t* _tempKeys, uint32_t _size)
	{
		uint64_t* keys = _keys;
		uint64_t* tempKeys = _tempKeys;

		uint32_t histogram[radix_sort_detail::kHistogramSize];
		uint16_t shift = 0;
		uint32_t pass = 0;
		for (; pass < 6; ++pass)
		{
			memSet(histogram, 0, sizeof(uint32_t)*radix_sort_detail::kHistogramSize);

			bool sorted = true;
			{
				uint64_t key = keys[0];
				uint64_t prevKey = key;
				for (uint32_t ii = 0; ii < _size; ++ii, prevKey = key)
				{
					key = keys[ii];
					uint16_t index = (key>>shift)&radix_sort_detail::kBitMask;
					++histogram[index];
					sorted &= prevKey <= key;
				}
			}

			if (sorted)
			{
				goto done;
			}

			uint32_t offset = 0;
			for (uint32_t ii = 0; ii < radix_sort_detail::kHistogramSize; ++ii)
			{
				uint32_t count = histogram[ii];
				histogram[ii] = offset;
				offset += count;
			}

			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				uint64_t key = keys[ii];
				uint16_t index = (key>>shift)&radix_sort_detail::kBitMask;
				uint32_t dest = histogram[index]++;
				tempKeys[dest] = key;
			}

			uint64_t* swapKeys = tempKeys;
			tempKeys = keys;
			keys = swapKeys;

			shift += radix_sort_detail::kBits;
		}

done:
		if (0 != (pass&1) )
		{
			// Odd number of passes needs to do copy to the destination.
			memCopy(_keys, _tempKeys, _size*sizeof(uint64_t) );
		}
	}

	template <typename Ty>
	inline void radixSort(uint64_t* _keys, uint64_t* _tempKeys, Ty* _values, Ty* _tempValues, uint32_t _size)
	{
		static_assert(isTriviallyMoveAssignable<Ty>(), "Sort element type must be trivially move assignable");

		uint64_t* keys = _keys;
		uint64_t* tempKeys = _tempKeys;
		Ty* values = _values;
		Ty* tempValues = _tempValues;

		uint32_t histogram[radix_sort_detail::kHistogramSize];
		uint16_t shift = 0;
		uint32_t pass = 0;
		for (; pass < 6; ++pass)
		{
			memSet(histogram, 0, sizeof(uint32_t)*radix_sort_detail::kHistogramSize);

			bool sorted = true;
			{
				uint64_t key = keys[0];
				uint64_t prevKey = key;
				for (uint32_t ii = 0; ii < _size; ++ii, prevKey = key)
				{
					key = keys[ii];
					uint16_t index = (key>>shift)&radix_sort_detail::kBitMask;
					++histogram[index];
					sorted &= prevKey <= key;
				}
			}

			if (sorted)
			{
				goto done;
			}

			uint32_t offset = 0;
			for (uint32_t ii = 0; ii < radix_sort_detail::kHistogramSize; ++ii)
			{
				uint32_t count = histogram[ii];
				histogram[ii] = offset;
				offset += count;
			}

			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				uint64_t key = keys[ii];
				uint16_t index = (key>>shift)&radix_sort_detail::kBitMask;
				uint32_t dest = histogram[index]++;
				tempKeys[dest] = key;
				tempValues[dest] = values[ii];
			}

			uint64_t* swapKeys = tempKeys;
			tempKeys = keys;
			keys = swapKeys;

			Ty* swapValues = tempValues;
			tempValues = values;
			values = swapValues;

			shift += radix_sort_detail::kBits;
		}

done:
		if (0 != (pass&1) )
		{
			// Odd number of passes needs to do copy to the destination.
			memCopy(_keys, _tempKeys, _size*sizeof(uint64_t) );
			for (uint32_t ii = 0; ii < _size; ++ii)
			{
				_values[ii] = _tempValues[ii];
			}
		}
	}

} // namespace bx
