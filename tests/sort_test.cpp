/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/bx.h>
#include <bx/sort.h>
#include <bx/string.h>
#include <bx/rng.h>

TEST_CASE("quickSort", "[sort]")
{
	const char* str[] =
	{
		"jabuka",
		"kruska",
		"malina",
		"jagoda",
	};

	REQUIRE(!bx::isSorted(str, BX_COUNTOF(str) ) );

	bx::quickSort(str, BX_COUNTOF(str) );

	REQUIRE(0 == bx::strCmp(str[0], "jabuka") );
	REQUIRE(0 == bx::strCmp(str[1], "jagoda") );
	REQUIRE(0 == bx::strCmp(str[2], "kruska") );
	REQUIRE(0 == bx::strCmp(str[3], "malina") );

	REQUIRE(bx::isSorted(str, BX_COUNTOF(str) ) );

	int8_t byte[128];
	bx::RngMwc rng;
	for (uint32_t ii = 0; ii < BX_COUNTOF(byte); ++ii)
	{
		byte[ii] = rng.gen()&0xff;
	}

	REQUIRE(!bx::isSorted(byte, BX_COUNTOF(byte) ) );

	bx::quickSort(byte, BX_COUNTOF(byte) );

	for (uint32_t ii = 1; ii < BX_COUNTOF(byte); ++ii)
	{
		REQUIRE(byte[ii-1] <= byte[ii]);
	}

	REQUIRE(bx::isSorted(byte, BX_COUNTOF(byte) ) );
}

TEST_CASE("binarySearch", "[sort]")
{
	const char* str[] =
	{
		"jabuka",
		"kruska",
		"malina",
		"jagoda",
	};

	REQUIRE(!bx::isSorted(str, BX_COUNTOF(str) ) );

	bx::quickSort(str, BX_COUNTOF(str) );
	REQUIRE(bx::isSorted(str, BX_COUNTOF(str) ) );

	auto bsearchStrCmpFn = [](const void* _lhs, const void* _rhs)
	{
		const char* lhs = (const char*)_lhs;
		const char* rhs = *(const char**)_rhs;
		return bx::strCmp(lhs, rhs);
	};

	REQUIRE(~4 == bx::binarySearch("sljiva", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 0 == bx::binarySearch("jabuka", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 1 == bx::binarySearch("jagoda", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 2 == bx::binarySearch("kruska", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 3 == bx::binarySearch("malina", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE(~3 == bx::binarySearch("kupina", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );

	REQUIRE( 0 == bx::lowerBound("jabuka", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 1 == bx::upperBound("jabuka", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );

	REQUIRE( 1 == bx::lowerBound("jagoda", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 2 == bx::upperBound("jagoda", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );

	REQUIRE( 2 == bx::lowerBound("kruska", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 3 == bx::upperBound("kruska", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );

	REQUIRE( 3 == bx::lowerBound("malina", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
	REQUIRE( 4 == bx::upperBound("malina", str, BX_COUNTOF(str), sizeof(str[0]), bsearchStrCmpFn) );
}

TEST_CASE("unique", "[sort]")
{
	//                   0    1    2    3    4    5    6    7    8    9   10   11   12   13 | 14
	int32_t test[] = { 100, 101, 101, 101, 103, 104, 105, 105, 105, 106, 106, 107, 108, 109 };
	REQUIRE(bx::isSorted(test, BX_COUNTOF(test) ) );

	REQUIRE(0 == bx::unique(test, 0) );
	REQUIRE(1 == bx::unique(test, 1) );

	REQUIRE(2 == bx::unique(test, 4) );
	bx::quickSort(test, BX_COUNTOF(test) );

	REQUIRE(3 == bx::unique(test, 5) );
	bx::quickSort(test, BX_COUNTOF(test) );

	uint32_t last = bx::unique(test, BX_COUNTOF(test) );
	REQUIRE(9 == last);

	REQUIRE(9 == bx::unique(test, last) );
}

TEST_CASE("lowerBound, upperBound int32_t", "[sort]")
{
	//                         0    1    2    3    4    5    6    7    8    9   10   11   12   13 | 14
	const int32_t test[] = { 100, 101, 101, 101, 103, 104, 105, 105, 105, 106, 106, 107, 108, 109 };
	REQUIRE(bx::isSorted(test, BX_COUNTOF(test) ) );

	const uint32_t resultLowerBound[] = { 0, 1, 4, 4, 5, 6,  9, 11, 12, 13 };
	const uint32_t resultUpperBound[] = { 1, 4, 4, 5, 6, 9, 11, 12, 13, 14 };

	STATIC_REQUIRE(10 == BX_COUNTOF(resultLowerBound) );
	STATIC_REQUIRE(10 == BX_COUNTOF(resultUpperBound) );

	for (int32_t key = test[0], keyMax = test[BX_COUNTOF(test)-1], ii = 0; key <= keyMax; ++key, ++ii)
	{
		REQUIRE(resultLowerBound[ii] == bx::lowerBound(key, test, BX_COUNTOF(test) ) );
		REQUIRE(resultUpperBound[ii] == bx::upperBound(key, test, BX_COUNTOF(test) ) );
	}
}

template<typename Ty>
int32_t compareAscendingTest(const Ty& _lhs, const Ty& _rhs)
{
	return bx::compareAscending<Ty>(&_lhs, &_rhs);
}

template<typename Ty>
int32_t compareDescendingTest(const Ty& _lhs, const Ty& _rhs)
{
	return bx::compareDescending<Ty>(&_lhs, &_rhs);
}

template<typename Ty>
void compareTest(const Ty& _min, const Ty& _max)
{
	REQUIRE(_min < _max);

	REQUIRE(-1 == compareAscendingTest<Ty>(bx::min<Ty>(), bx::max<Ty>() ) );
	REQUIRE(-1 == compareAscendingTest<Ty>(Ty(0),         bx::max<Ty>() ) );
	REQUIRE( 0 == compareAscendingTest<Ty>(bx::min<Ty>(), bx::min<Ty>() ) );
	REQUIRE( 0 == compareAscendingTest<Ty>(bx::max<Ty>(), bx::max<Ty>() ) );
	REQUIRE( 1 == compareAscendingTest<Ty>(bx::max<Ty>(), Ty(0)         ) );
	REQUIRE( 1 == compareAscendingTest<Ty>(bx::max<Ty>(), bx::min<Ty>() ) );

	REQUIRE(-1 == compareAscendingTest<Ty>(_min, _max) );
	REQUIRE( 0 == compareAscendingTest<Ty>(_min, _min) );
	REQUIRE( 0 == compareAscendingTest<Ty>(_max, _max) );
	REQUIRE( 1 == compareAscendingTest<Ty>(_max, _min) );

	REQUIRE( 1 == compareDescendingTest<Ty>(_min, _max) );
	REQUIRE( 0 == compareDescendingTest<Ty>(_min, _min) );
	REQUIRE( 0 == compareDescendingTest<Ty>(_max, _max) );
	REQUIRE(-1 == compareDescendingTest<Ty>(_max, _min) );
}

TEST_CASE("ComparisonFn", "[sort]")
{
	compareTest< int8_t>(  -13,   89);
	compareTest<int16_t>(-1389, 1389);
	compareTest<int32_t>(-1389, 1389);
	compareTest<int64_t>(-1389, 1389);

	compareTest< uint8_t>(  13,   89);
	compareTest<uint16_t>(  13, 1389);
	compareTest<uint32_t>(  13, 1389);
	compareTest<uint64_t>(  13, 1389);

	compareTest< float>(-13.89f, 1389.0f);
	compareTest<double>(-13.89f, 1389.0f);
}

#include <bx/allocator.h>
#include <tinystl/allocator.h>
#include <tinystl/string.h>
#include <tinystl/vector.h>

namespace
{
	struct Item
	{
		uint32_t key;
		uint32_t seq;
	};

	bool byKey(const Item& _a, const Item& _b)
	{
		return _a.key < _b.key;
	}

	tinystl::vector<Item> makeItems(uint32_t _num, uint32_t _numKeys)
	{
		tinystl::vector<Item> items;
		uint32_t rng = 12345;
		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			rng = rng*1664525 + 1013904223;
			const Item item =
			{
				.key = (rng >> 16) % _numKeys,
				.seq = ii,
			};

			items.push_back(item);
		}

		return items;
	}

	void checkSortedAndStable(tinystl::vector<Item>& _items)
	{
		bx::DefaultAllocator allocator;

		const uint32_t num = uint32_t(_items.size() );
		bx::stableSort(&allocator, _items, byKey);

		REQUIRE(num == _items.size() );

		for (uint32_t ii = 1; ii < num; ++ii)
		{
			REQUIRE(_items[ii-1].key <= _items[ii].key);

			if (_items[ii-1].key == _items[ii].key)
			{
				REQUIRE(_items[ii-1].seq < _items[ii].seq);
			}
		}
	}

} // namespace

TEST_CASE("insertionSort", "[sort]")
{
	Item items[] =
	{
		{ 3, 0 }, { 1, 1 }, { 3, 2 }, { 0, 3 }, { 1, 4 }, { 3, 5 },
	};

	bx::insertionSort(items, BX_COUNTOF(items), byKey);

	const uint32_t expectedKey[] = { 0, 1, 1, 3, 3, 3 };
	const uint32_t expectedSeq[] = { 3, 1, 4, 0, 2, 5 };
	for (uint32_t ii = 0; ii < BX_COUNTOF(items); ++ii)
	{
		REQUIRE(expectedKey[ii] == items[ii].key);
		REQUIRE(expectedSeq[ii] == items[ii].seq);
	}

	bx::insertionSort(items, 0, byKey);
	bx::insertionSort(items, 1, byKey);
	REQUIRE(0 == items[0].key);
}

TEST_CASE("stableSort is stable across the insertion/merge threshold", "[sort]")
{
	const uint32_t sizes[] = { 0, 1, 2, 3, 31, 32, 33, 63, 64, 65, 127, 128, 129, 1000 };

	for (uint32_t ii = 0; ii < BX_COUNTOF(sizes); ++ii)
	{
		tinystl::vector<Item> items = makeItems(sizes[ii], 4);
		checkSortedAndStable(items);
	}
}

TEST_CASE("stableSort handles degenerate orderings", "[sort]")
{
	bx::DefaultAllocator allocator;

	{
		tinystl::vector<Item> items;
		for (uint32_t ii = 0; ii < 100; ++ii)
		{
			const Item item = { ii, ii };
			items.push_back(item);
		}

		checkSortedAndStable(items);
		REQUIRE(0  == items[0].key);
		REQUIRE(99 == items[99].key);
	}

	{
		tinystl::vector<Item> items;
		for (uint32_t ii = 0; ii < 100; ++ii)
		{
			const Item item = { 100-ii, ii };
			items.push_back(item);
		}

		bx::stableSort(&allocator, items, byKey);
		for (uint32_t ii = 1; ii < 100; ++ii)
		{
			REQUIRE(items[ii-1].key < items[ii].key);
		}
	}

	{
		tinystl::vector<Item> items;
		for (uint32_t ii = 0; ii < 100; ++ii)
		{
			const Item item = { 7, ii };
			items.push_back(item);
		}

		bx::stableSort(&allocator, items, byKey);
		for (uint32_t ii = 0; ii < 100; ++ii)
		{
			REQUIRE(ii == items[ii].seq);
		}
	}
}

TEST_CASE("stableSort moves elements by assignment", "[sort]")
{
	struct Named
	{
		uint32_t         key;
		tinystl::string name;
	};

	const tinystl::string prefix("a-long-name-that-will-not-fit-in-a-small-string-buffer-");

	bx::DefaultAllocator allocator;

	tinystl::vector<Named> items;
	for (uint32_t ii = 0; ii < 200; ++ii)
	{
		const char suffix[] = { char('0' + ii%10) };

		Named item;
		item.key  = (200 - ii) % 10;
		item.name = prefix;
		item.name.append(suffix, suffix + 1);

		items.push_back(item);
	}

	bx::stableSort(&allocator, items, [](const Named& _a, const Named& _b) { return _a.key < _b.key; });

	REQUIRE(200 == items.size() );
	for (uint32_t ii = 1; ii < items.size(); ++ii)
	{
		REQUIRE(items[ii-1].key <= items[ii].key);
	}

	for (uint32_t ii = 0; ii < items.size(); ++ii)
	{
		REQUIRE(prefix.size() + 1 == items[ii].name.size() );
		REQUIRE(0 == bx::memCmp(items[ii].name.c_str(), prefix.c_str(), prefix.size() ) );
	}
}

TEST_CASE("stableSort handles empty and single-element containers", "[sort]")
{
	bx::DefaultAllocator allocator;

	tinystl::vector<Item> empty;
	bx::stableSort(&allocator, empty, byKey);
	REQUIRE(empty.empty() );

	tinystl::vector<Item> one;
	const Item item = { 42, 0 };
	one.push_back(item);
	bx::stableSort(&allocator, one, byKey);
	REQUIRE(1  == one.size() );
	REQUIRE(42 == one[0].key);
}
