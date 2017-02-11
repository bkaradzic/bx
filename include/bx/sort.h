/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_SORT_H_HEADER_GUARD
#define BX_SORT_H_HEADER_GUARD

#include "bx.h"

namespace bx
{
	///
	typedef int32_t (*ComparisonFn)(const void* _lhs, const void* _rhs);

	///
	void quickSort(
		  void* _data
		, uint32_t _num
		, uint32_t _stride
		, const ComparisonFn _fn
		);

	///
	void radixSort(
		  uint32_t* __restrict _keys
		, uint32_t* __restrict _tempKeys
		, uint32_t _size
		);

	///
	template <typename Ty>
	void radixSort(
		  uint32_t* __restrict _keys
		, uint32_t* __restrict _tempKeys
		, Ty* __restrict _values
		, Ty* __restrict _tempValues
		, uint32_t _size
		);

	///
	void radixSort(
		  uint64_t* __restrict _keys
		, uint64_t* __restrict _tempKeys
		, uint32_t _size
		);

	///
	template <typename Ty>
	void radixSort(
		  uint64_t* __restrict _keys
		, uint64_t* __restrict _tempKeys
		, Ty* __restrict _values
		, Ty* __restrict _tempValues
		, uint32_t _size
		);

} // namespace bx

#include "sort.inl"

#endif // BX_SORT_H_HEADER_GUARD
