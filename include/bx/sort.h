/*
 * Copyright 2010-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_SORT_H_HEADER_GUARD
#define BX_SORT_H_HEADER_GUARD

#include "bx.h"

namespace bx
{
	/// The function compares the `_lhs` and `_rhs` values.
	///
	/// @returns Returns value:
	///   - less than zero if `_lhs` is less than `_rhs`
	///   - zero if `_lhs` is equivalent to `_rhs`
	///   - greater than zero if `_lhs` is greater than `_rhs`
	///
	typedef int32_t (*ComparisonFn)(const void* _lhs, const void* _rhs);

	/// Performs sort (Quick Sort algorithm).
	///
	/// @param _data Pointer to sorted array data.
	/// @param _num Number of elements.
	/// @param _stride Element stride in bytes.
	/// @param _fn Comparison function.
	///
	void quickSort(
		  void* _data
		, uint32_t _num
		, uint32_t _stride
		, const ComparisonFn _fn
		);

	///
	void radixSort(
		  uint32_t* _keys
		, uint32_t* _tempKeys
		, uint32_t _size
		);

	///
	template <typename Ty>
	void radixSort(
		  uint32_t* _keys
		, uint32_t* _tempKeys
		, Ty* _values
		, Ty* _tempValues
		, uint32_t _size
		);

	///
	void radixSort(
		  uint64_t* _keys
		, uint64_t* _tempKeys
		, uint32_t _size
		);

	///
	template <typename Ty>
	void radixSort(
		  uint64_t* _keys
		, uint64_t* _tempKeys
		, Ty* _values
		, Ty* _tempValues
		, uint32_t _size
		);

	/// Performs check if array is sorted.
	///
	/// @param _data Pointer to sorted array data.
	/// @param _num Number of elements.
	/// @param _stride Element stride in bytes.
	/// @param _fn Comparison function.
	///
	/// @returns Returns `true` if array is sorted, otherwise returns `false`.
	///
	bool isSorted(
		  const void* _data
		, uint32_t _num
		, uint32_t _stride
		, const ComparisonFn _fn
		);

	/// Performs binary search of a sorted array.
	///
	/// @param _key Pointer to the key to search for.
	/// @param _data Pointer to sorted array data.
	/// @param _num Number of elements.
	/// @param _stride Element stride in bytes.
	/// @param _fn Comparison function.
	///
	/// @remarks Array must be sorted!
	///
	/// @returns Returns index of element or -1 if the key is not found in sorted array.
	///
	int32_t binarySearch(
		  const void* _key
		, const void* _data
		, uint32_t _num
		, uint32_t _stride
		, const ComparisonFn _fn
		);

} // namespace bx

#include "inline/sort.inl"

#endif // BX_SORT_H_HEADER_GUARD
