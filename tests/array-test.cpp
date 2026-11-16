/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "test.h"

#include <bx/allocators.h>
#include <bx/array.h>

#include <bx/math.h>
#include <bx/rng.h>
#include <bx/stringpool.h>

#include <algorithm>
#include <string>
#include <vector>

template<typename Ty>
void testForEach(const Ty& _container)
{
	uint32_t count = 0;
	for (auto x : _container)
	{
		REQUIRE( (x == _container[count]) );
		count++;
	}

	REQUIRE(count == _container.getCount() );
}

template<typename Ty>
void testForEach(const Ty& _container, int32_t _value)
{
	uint32_t count = 0;
	for (auto x : _container)
	{
		REQUIRE( (x == _value) );
		count++;
	}

	REQUIRE(count == _container.getCount() );
}

static_assert( bx::isConstructible<bx::MutableArrayViewT<int32_t>, int32_t*, uint32_t>() );
static_assert(!bx::isConstructible<bx::MutableArrayViewT<int32_t>, const int32_t*, uint32_t>() );
static_assert(!bx::isConstructible<bx::MutableArrayViewT<int32_t>, const bx::ArrayViewT<int32_t>&>() );
static_assert(!bx::isConstructible<bx::MutableArrayViewT<int32_t>, const bx::ArrayT<int32_t>&>() );
static_assert( bx::isSame<decltype(bx::declVal<bx::SortedArrayT<int32_t>&>()[0]), const int32_t&>() );
static_assert( bx::isSame<decltype(bx::declVal<bx::SortedArrayT<int32_t>&>().getPtr() ), const int32_t*>() );

TEST_CASE("FixedArrayT int32_t", "[container]")
{
	bx::FixedArrayT<int32_t, 32> array(bx::ArrayViewT<int32_t>({ 9, 8, 3, 1 }) );
	REQUIRE(!bx::isSorted(array) );

	bx::sort(array);
	REQUIRE( bx::isSorted(array) );

	REQUIRE(1 == bx::binarySearch(3, array) );

	REQUIRE(32 == array.getCapacity() );
	REQUIRE( 4 == array.getCount() );

	array.clear();
	REQUIRE(array.isEmpty() );
	REQUIRE(0 == array.getCount() );

	array.add({4096, 4688, 4688, 4944, 8192, 8384, 8592, 16384});
	REQUIRE(8 == array.getCount() );

	REQUIRE(4688 == array.pop(1, bx::kOrdered) );

	testForEach(array);

	REQUIRE(array[6] == 16384);
	REQUIRE(array == bx::ArrayViewT<int32_t>({ 4096, 4688, 4944, 8192, 8384, 8592, 16384 }) );

	array.clear();

	array.add(1389);
	REQUIRE(1 == array.getCount() );

	REQUIRE(1389 == array[0]);
	REQUIRE(1389 == *array.getPtr(0) );

	array.clear();
	REQUIRE(0 == array.getCount() );

	array.add(bx::ArrayViewT<int32_t>({ 1, 3, 8, 9 }) );
	REQUIRE(4 == array.getCount() );

	{
		bx::ArrayViewT<int32_t> tmp = array.get(bx::kFirst, 2);
		REQUIRE(2 == tmp.getCount() );
		REQUIRE(1 == tmp[0]);
		REQUIRE(3 == tmp[1]);
	}

	{
		bx::ArrayViewT<int32_t> tmp = array.get(bx::kLast, 2);
		REQUIRE(2 == tmp.getCount() );
		REQUIRE(8 == tmp[0]);
		REQUIRE(9 == tmp[1]);
	}

	array.remove(1, 2);
	REQUIRE(2 == array.getCount() );
	REQUIRE(1 == array[0]);
	REQUIRE(9 == array[1]);

	array.add(bx::ArrayViewT<int32_t>({ 3, 8, 9 }) );
	REQUIRE(5 == array.getCount() );

	array.remove(1, bx::kOrdered);
	REQUIRE(4 == array.getCount() );

	array.add(2, 1389);
	REQUIRE(1    == array[0]);
	REQUIRE(3    == array[1]);
	REQUIRE(1389 == array[2]);
	REQUIRE(8    == array[3]);
	REQUIRE(9    == array[4]);

	array.remove(2);
	REQUIRE(1 == array[0]);
	REQUIRE(3 == array[1]);
	REQUIRE(9 == array[2]);
	REQUIRE(8 == array[3]);

	array.remove(6, 10);
	REQUIRE(4 == array.getCount() );

	array.clear();

	array.add(bx::ArrayViewT<int32_t>({1, 3, 8, 9}) );
	REQUIRE(4 == array.getCount() );
	REQUIRE(1 == array[0]);
	REQUIRE(3 == array[1]);
	REQUIRE(8 == array[2]);
	REQUIRE(9 == array[3]);

	array.remove(1);
	REQUIRE(3 == array.getCount() );
	REQUIRE(1 == array[0]);
	REQUIRE(9 == array[1]);
	REQUIRE(8 == array[2]);

	for (uint32_t ii = 0, count = array.getCount(); ii < count; ++ii)
	{
		REQUIRE(ii == array.getIndex(array.getPtr(ii) ) );
	}

	array.remove(0, UINT32_MAX);

	array.add(bx::ArrayViewT<int32_t>({1, 3, 8, 9}) );
	REQUIRE(4 == array.getCount() );

	{
		bx::ArrayViewT<int32_t> otherView;
		REQUIRE(otherView.isEmpty() );
	}

	{
		bx::FixedArrayT<int32_t, 2> other(bx::ArrayViewT<int32_t>({ 13, 89 }) );
		bx::ArrayViewT<int32_t> otherView(other);

		REQUIRE(13 == otherView[0]);
		REQUIRE(89 == otherView[1]);

		array.add(other);
		REQUIRE(6 == array.getCount() );
	}

	{
		bx::FixedArrayT<int32_t, 6> other;
		other.add(bx::ArrayViewT<int32_t>({ 1, 3, 8, 9, 13, 89 }) );
		REQUIRE(array.getCount() == other.getCount() );

		for (uint32_t ii = 0, count = array.getCount(); ii < count; ++ii)
		{
			REQUIRE(array[ii] == other[ii]);
		}
	}

	{ // non-const C-array
		int32_t other[] = { 1, 3, 8, 9, 13, 89 };
		bx::ArrayViewT<int32_t> otherView(other);
		REQUIRE(array.getCount() == otherView.getCount() );
	}

	{ // const C-array
		const int32_t other[] = { 1, 3, 8, 9, 13, 89 };
		bx::ArrayViewT<int32_t> otherView(other);
		REQUIRE(array.getCount() == otherView.getCount() );

		for (uint32_t ii = 0, count = array.getCount(); ii < count; ++ii)
		{
			REQUIRE(array[ii] == otherView[ii]);
		}
	}
}

#define DEBUG BX_UNUSED

class TestClassNtm
{
public:
	BX_NO_INLINE TestClassNtm()
		: m_valid(true)
	{
		s_instanceCount++;
		DEBUG("\tctor %3d: [%d] TestClassNtm()", s_instanceCount, this-s_firstPtr);
	}

	BX_NO_INLINE TestClassNtm(const char* _a)
		: m_str(_a)
		, m_valid(true)
	{
		s_instanceCount++;
		DEBUG("\tctor %3d: [%d] TestClassNtm(const char* \"%s\")"
				, s_instanceCount, this-s_firstPtr, _a);
	}

	BX_NO_INLINE TestClassNtm(const std::string& _a)
		: m_str(_a)
		, m_valid(true)
	{
		s_instanceCount++;
		DEBUG("\tctor %3d: [%d] TestClassNtm(const std::string& \"%s\")"
				, s_instanceCount, this-s_firstPtr, _a.c_str() );
	}

	BX_NO_INLINE TestClassNtm(const TestClassNtm& _other)
		: m_valid(true)
	{
		s_instanceCount++;
		DEBUG("\tctor %3d: [%d] TestClassNtm(const TestClassNtm& _other /* %s */)"
				, s_instanceCount, this-s_firstPtr, _other.m_str.c_str() );

		*this = _other;
	}

	BX_NO_INLINE TestClassNtm(TestClassNtm&& _other)
		: m_valid(true)
	{
		s_instanceCount++;
		DEBUG("\tctor %3d: [%d] TestClassNtm(TestClassNtm&& _other /* %s */)"
				, s_instanceCount, this-s_firstPtr, _other.m_str.c_str() );

		*this = bx::move(_other);
	}

	BX_NO_INLINE ~TestClassNtm()
	{
		s_instanceCount--;
		BX_ASSERT(m_valid, "Deleting invalid instance [%d].", this - s_firstPtr);

		DEBUG("\tdtor %3d: [%d] ~TestClassNtm( /* %s */ )"
				, s_instanceCount, this-s_firstPtr, m_str.c_str() );

		m_str = "delete me!";
		m_valid = false;
	}

	BX_NO_INLINE TestClassNtm& operator=(const TestClassNtm& _other)
	{
		BX_ASSERT(_other.m_valid, "Copying invalid instance [%d].", &_other - s_firstPtr);

		BX_ASSERT(this != &_other, "");

		DEBUG("\tcopy [%d] <- [%d] operator=(const TestClassNtm& /* %s */)"
				, this-s_firstPtr, &_other - s_firstPtr, _other.m_str.c_str() );

		m_str   = _other.m_str;
		m_valid = _other.m_valid;

		return *this;
	}

	BX_NO_INLINE TestClassNtm& operator=(TestClassNtm&& _other)
	{
		BX_ASSERT(this != &_other, "");

		BX_ASSERT(_other.m_valid, "Moving invalid instance [%d].", &_other - s_firstPtr);

		DEBUG("\tmove [%d] <- [%d] operator=(TestClassNtm&& /* %s */)"
				, this-s_firstPtr, &_other-s_firstPtr, _other.m_str.c_str() );

		m_str   = bx::move(_other.m_str);
		m_valid = _other.m_valid;

		return *this;
	}

	static int32_t s_instanceCount;
	static TestClassNtm* s_firstPtr;

	std::string m_str;
	bool m_valid;
};

#undef DEBUG

int32_t       TestClassNtm::s_instanceCount = 0;
TestClassNtm* TestClassNtm::s_firstPtr = NULL;

template<typename StorageT>
void testClassNmt(bx::ArrayBaseT<TestClassNtm, StorageT>& array)
{
#define DEBUG(_a) DBG("" BX_STRINGIZE(_a) ); _a

	TestClassNtm::s_firstPtr = array.getPtr(0);
	{
		REQUIRE(array.isEmpty() );

		array.setCount(10);
		REQUIRE(10 == array.getCount() );

		array.setCount(5);
		REQUIRE(5 == array.getCount() );

		array.setCount(7);
		REQUIRE(7 == array.getCount() );

		array.setCount(0);
		REQUIRE(array.isEmpty() );

		DEBUG(array.add(0, bx::Emplace,    "9") );
		DEBUG(array.add(0,                 "8") );
		DEBUG(array.add(0,                 "3") );
		DEBUG(array.add(0, bx::Emplace,    "1") );
		DEBUG(array.add(0, bx::Emplace, "1389") );
		DEBUG(array.add("13") );
		DEBUG(array.add("89") );
		DEBUG(array.add("9831") );

		array.setCapacity(32);

		{
			bx::MutableArrayViewT<TestClassNtm> mutableArrayView = array.get(0, 4);
			REQUIRE("1" == mutableArrayView[1].m_str);

			bx::ArrayViewT<TestClassNtm> arrayView = array.get(1, 4);
			REQUIRE("1" == arrayView[0].m_str);

			mutableArrayView[1] = "0";
			REQUIRE("0" == arrayView[0].m_str);
			REQUIRE("0" == mutableArrayView[1].m_str);
		}

		{
			const TestClassNtm abcd[] = { "#", "a", "b", "c", "d", "e", "f", "g", "h", "#" };
			bx::ArrayViewT<TestClassNtm> abcdView(abcd, BX_COUNTOF(abcd) );
			DEBUG(array.add(4, abcdView.get(1, 8) ) );
			REQUIRE(16 == array.getCount() );
		}

		DEBUG(array.remove(1, bx::kOrdered, 11) );
		REQUIRE(5 == array.getCount() );

		DEBUG(array.remove(1, 2) );
		REQUIRE(3 == array.getCount() );

		DEBUG(array.remove(0, 2) );
		REQUIRE(1 == array.getCount() );

		array[0] = "1389";

		DEBUG(array.add(bx::Emplace,      "13") );
		DEBUG(array.add(                  "89") );
		DEBUG(array.add(                   "8") );
		DEBUG(array.add(bx::Emplace,       "9") );
		REQUIRE("1389" == array[0].m_str);

//		bx::sort(array, [](const void* _lhs, const void* _rhs) -> int32_t
//			{
//				const TestClassNtm& lhs = *static_cast<const TestClassNtm*>(_lhs);
//				const TestClassNtm& rhs = *static_cast<const TestClassNtm*>(_rhs);
//				return bx::strCmp(lhs.m_str.c_str(), rhs.m_str.c_str() );
//			});

		DEBUG(array.remove(1, 2) );
		REQUIRE("1389" == array[0].m_str);

		DEBUG(array.remove(0, bx::kOrdered, 2) );
		REQUIRE("9" == array[0].m_str);
	}

	array.setCapacity(18);
	REQUIRE(array.getCapacity() >= 18);

	DEBUG(array.trim() );
	REQUIRE(array.getCapacity() >= array.getCount() );

	DEBUG(array.clear() );

	REQUIRE(0 == TestClassNtm::s_instanceCount);

	DEBUG(array.trim() );

	DBG("---");

#undef DEBUG
}

TEST_CASE("FixedArrayT TestClassNtm", "[container]")
{
	STATIC_REQUIRE(!bx::isTriviallyMoveAssignable<TestClassNtm>() );
	STATIC_REQUIRE( bx::isMoveAssignable<TestClassNtm>() );
	STATIC_REQUIRE( bx::isMoveConstructible<TestClassNtm>() );

	REQUIRE(0 == TestClassNtm::s_instanceCount);

	bx::CrtAllocator allocator;
	bx::ArrayT<TestClassNtm> dynamicArray(allocator.getHandle() );
	testClassNmt(dynamicArray);

	bx::FixedArrayT<TestClassNtm, 32> fixedArray;
	testClassNmt(fixedArray);
}

TEST_CASE("ArrayT ctors", "[container]")
{
	bx::CrtAllocator allocator;

	{ // Default
		bx::ArrayT<int32_t> array(allocator.getHandle() );

		REQUIRE(array.isEmpty() );
		REQUIRE(0 == array.getCount() );
	}

	{ // Static unitialized data as content.
		int32_t test[13] = {};
		bx::ArrayT<int32_t> array(allocator.getHandle(), test);

		REQUIRE(!array.isEmpty() );
		REQUIRE(13 == array.getCount() );
	}

	{ // Static initialized data as content.
		int32_t test[] = { 1389 };
		bx::ArrayT<int32_t> array(allocator.getHandle(), test);

		REQUIRE(!array.isEmpty() );
		REQUIRE(1 == array.getCount() );
		REQUIRE(1389 == test[0]);
	}

	{ // Create and set default capacity.
		bx::ArrayT<int32_t> array(allocator.getHandle(), 1389);

		REQUIRE(array.isEmpty() );
		REQUIRE(0 == array.getCount() );
		REQUIRE(1389 <= array.getCapacity() );
	}

	{ // Create set capacity/count and initialize all elements to 123.
		bx::ArrayT<int32_t> array(allocator.getHandle(), 1389, bx::Emplace, 123);

		REQUIRE(!array.isEmpty() );
		REQUIRE(1389 == array.getCount() );
		testForEach(array, 123);
	}
}

namespace
{
	struct EmplacePayload
	{
		int32_t value;
	};

	struct EmplaceProbe
	{
		EmplaceProbe(const EmplacePayload& _payload)
			: value(_payload.value)
		{
		}

		EmplaceProbe(EmplacePayload&& _payload)
			: value(_payload.value)
		{
			_payload.value = 0;
		}

		int32_t value;
	};

} // namespace

TEST_CASE("ArrayT emplace count constructs every element from rvalue arguments", "[container]")
{
	bx::CrtAllocator allocator;

	constexpr uint32_t kCount = 4;

	bx::ArrayT<EmplaceProbe> array(allocator.getHandle(), kCount, bx::Emplace, EmplacePayload{ 1389 });
	REQUIRE(kCount == array.getCount() );

	bx::FixedArrayT<EmplaceProbe, kCount> fixed(kCount, bx::Emplace, EmplacePayload{ 1389 });
	REQUIRE(kCount == fixed.getCount() );

	bx::FixedArrayT<EmplaceProbe, kCount> filled(bx::Emplace, EmplacePayload{ 1389 });
	REQUIRE(kCount == filled.getCount() );

	for (uint32_t ii = 0; ii < kCount; ++ii)
	{
		REQUIRE(1389 == array[ii].value);
		REQUIRE(1389 == fixed[ii].value);
		REQUIRE(1389 == filled[ii].value);
	}

	bx::ArrayT<EmplaceProbe> single(allocator.getHandle() );
	EmplacePayload payload = { 9 };
	single.add(bx::Emplace, bx::move(payload) );
	REQUIRE(9 == single[0].value);
	REQUIRE(0 == payload.value);
}

template<size_t SizeT>
void testCopy(const bx::ArrayViewT<int32_t>& _array, const int32_t (&_src)[SizeT])
{
	REQUIRE(SizeT == _array.getCount() );
	for (uint32_t ii = 0; ii < SizeT; ++ii)
	{
		REQUIRE(_src[ii] == _array[ii]);
	}
}

TEST_CASE("ArrayT copy", "[container]")
{
	bx::CrtAllocator allocator;

	const int32_t test[] = { 3, 1389 };

	bx::ArrayT<int32_t> da(allocator.getHandle(), test);

	bx::FixedArrayT<int32_t, 32> fa(da);
	da.clear();
	testCopy(fa, test);

	bx::SortedArrayT<int32_t> sda(allocator.getHandle(), fa);
	fa.clear();
	testCopy(sda, test);

	bx::FixedSortedArrayT<int32_t, 32> sfa(sda);
	sda.clear();
	testCopy(sfa, test);

BX_PRAGMA_DIAGNOSTIC_PUSH();
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG("-Wself-assign-overloaded");

	da.set(sfa);
	da = da;
	sfa.clear();
	testCopy(da, test);

	fa.set(da);
	fa = fa;
	da.clear();
	testCopy(fa, test);

	sda.set(fa);
	sda = sda;
	fa.clear();
	testCopy(sda, test);

	sfa.set(sda);
	sfa = sfa;
	sda.clear();
	testCopy(sfa, test);

BX_PRAGMA_DIAGNOSTIC_POP();

	da.set(sfa);
	sfa.clear();
	testCopy(da, test);

	fa = da;
	da.clear();
	testCopy(fa, test);

	sda = fa;
	fa.clear();
	testCopy(sda, test);

	sfa = sda;
	sda.clear();
	testCopy(sfa, test);

	da = sfa;
	sfa.clear();
	testCopy(da, test);

	REQUIRE(2 == da.getCount() );
	REQUIRE(3 == da[0]);
	REQUIRE(1389 == da[1]);
}

TEST_CASE("ArrayT int32_t", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<int32_t> array(allocator.getHandle() );

	array.add(0, bx::Emplace,    9);
	array.add(0,                 8);
	array.add(0,                 3);
	array.add(0, bx::Emplace,    1);
	array.add(0, bx::Emplace, 1389);

	REQUIRE(5 == array.getCount() );

	for (uint32_t ii = 0, num = array.getCount(); ii < num; ++ii)
	{
		array.remove(ii, 0);
		array.remove(ii, bx::kOrdered, 0);
		array.remove(ii, bx::kUnordered, 0);
	}

	array.remove(bx::kFirst, 0);
	array.remove(bx::kLast, 0);

	REQUIRE(5 == array.getCount() );
}

TEST_CASE("FixedSortedArrayT<int32_t, compareAscending>", "[container]")
{
	bx::FixedSortedArrayT<int32_t, 32, bx::compareAscending<int32_t>> array;

	array.add(9);
	array.add(3);
	array.add(8);
	array.add(1);
	array.add(1389);

	testForEach(array);

	REQUIRE(5 == array.getCount() );

	REQUIRE(bx::isSorted(array) );
	REQUIRE(bx::isSorted(array, /*using explicit comparator*/ bx::compareAscending<int32_t>) );

	REQUIRE(1 == array[0]);
	REQUIRE(3 == array[1]);
	REQUIRE(8 == array[2]);
	REQUIRE(9 == array[3]);
	REQUIRE(1389 == array[4]);

	REQUIRE(8 == array.pop(2) );

	REQUIRE(bx::isSorted(array) );
	REQUIRE(bx::isSorted(array, /*using explicit comparator*/ bx::compareAscending<int32_t>) );
}

TEST_CASE("FixedSortedArrayT<int32_t, compareDescending>", "[container]")
{
	bx::FixedSortedArrayT<int32_t, 32, bx::compareDescending<int32_t>> array;

	array.add(9);
	array.add(3);
	array.add(8);
	array.add(1);
	array.add(1389);

	testForEach(array);

	REQUIRE(5 == array.getCount() );

	REQUIRE(bx::isSorted(array) );
	REQUIRE(bx::isSorted(array, /*using explicit comparator*/ bx::compareDescending<int32_t>) );

	REQUIRE(1 == array[4]);
	REQUIRE(3 == array[3]);
	REQUIRE(8 == array[2]);
	REQUIRE(9 == array[1]);
	REQUIRE(1389 == array[0]);

	REQUIRE(8 == array.pop(2) );

	REQUIRE(bx::isSorted(array) );
	REQUIRE(bx::isSorted(array, /*using explicit comparator*/ bx::compareDescending<int32_t>) );
}

TEST_CASE("SortedArrayT ctors", "[container]")
{
	bx::CrtAllocator allocator;

	{ // Default
		bx::SortedArrayT<int32_t> array(allocator.getHandle() );
		REQUIRE(array.isEmpty() );
		REQUIRE(0 == array.getCount() );
	}

	{ // Static unitialized data as content.
		int32_t test[13] = {};
		bx::SortedArrayT<int32_t> array(allocator.getHandle(), test);
		REQUIRE(!array.isEmpty() );
		REQUIRE(13 == array.getCount() );
	}

	{ // Static initialized data as content.
		int32_t test[] = { 1389 };
		bx::SortedArrayT<int32_t> array(allocator.getHandle(), test);
		REQUIRE(!array.isEmpty() );
		REQUIRE(1 == array.getCount() );
		REQUIRE(1389 == test[0]);
	}

	{ // Create and set default capacity.
		bx::SortedArrayT<int32_t> array(allocator.getHandle(), 1389);
		REQUIRE(array.isEmpty() );
		REQUIRE(0 == array.getCount() );
		REQUIRE(1389 <= array.getCapacity() );
	}

	{ // Create set capacity/cound and initialize all elements to 0.
		bx::SortedArrayT<int32_t> array(allocator.getHandle(), 1389, bx::Emplace, 0);
		REQUIRE(!array.isEmpty() );
		REQUIRE(1389 == array.getCount() );
	}
}

TEST_CASE("SortedArrayT<int32_t, compareAscending>", "[container]")
{
	bx::CrtAllocator allocator;
	bx::SortedArrayT<int32_t, bx::compareAscending<int32_t>> array(allocator.getHandle(), {1389, 3});

	array.add(9);
	array.add(8);
	array.add(1);
	array.add(1389);

	testForEach(array);

	REQUIRE(6 == array.getCount() );
	REQUIRE(bx::isSorted(array, bx::compareAscending<int32_t>) );

	REQUIRE(1 == array[0]);
	REQUIRE(3 == array[1]);
	REQUIRE(8 == array[2]);
	REQUIRE(9 == array[3]);
	REQUIRE(1389 == array[4]);
	REQUIRE(1389 == array[5]);

	REQUIRE(8 == array.pop(2) );

	REQUIRE(bx::isSorted(array, bx::compareAscending<int32_t>) );
}

TEST_CASE("SortedArrayT<int32_t, compareDescending>", "[container]")
{
	bx::CrtAllocator allocator;
	bx::SortedArrayT<int32_t, bx::compareDescending<int32_t>> array(allocator.getHandle(), {1389, 3});

	array.add(9);
	array.add(8);
	array.add(1);
	array.add(1389);

	testForEach(array);

	REQUIRE(6 == array.getCount() );
	REQUIRE(bx::isSorted(array, bx::compareDescending<int32_t>) );

	REQUIRE(1 == array[5]);
	REQUIRE(3 == array[4]);
	REQUIRE(8 == array[3]);
	REQUIRE(9 == array[2]);
	REQUIRE(1389 == array[1]);
	REQUIRE(1389 == array[0]);

	REQUIRE(8 == array.pop(3) );

	REQUIRE(bx::isSorted(array, bx::compareDescending<int32_t>) );
}

TEST_CASE("ArrayT", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<bx::Vec3> array0(allocator.getHandle() );
	bx::ArrayT<bx::Quaternion> array1(allocator.getHandle() );
}

TEST_CASE("ArrayT move semantics", "[container]")
{
	bx::CrtAllocator allocator;

	{
		bx::ArrayT<int32_t> src(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 1, 3, 8, 9 }) );
		REQUIRE(4 == src.getCount() );

		bx::ArrayT<int32_t> dst(bx::move(src) );
		REQUIRE(4 == dst.getCount() );
		REQUIRE(1 == dst[0]);
		REQUIRE(9 == dst[3]);
		REQUIRE(0 == src.getCount() );
	}

	{
		bx::ArrayT<int32_t> src(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 13, 89 }) );
		bx::ArrayT<int32_t> dst(allocator.getHandle() );
		dst = bx::move(src);
		REQUIRE(2 == dst.getCount() );
		REQUIRE(13 == dst[0]);
		REQUIRE(89 == dst[1]);
		REQUIRE(0 == src.getCount() );
	}
}

TEST_CASE("ArrayT setCount and trim", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<int32_t> array(allocator.getHandle() );

	array.setCount(10);
	REQUIRE(10 == array.getCount() );

	array.setCount(5);
	REQUIRE(5 == array.getCount() );

	array.setCount(0);
	REQUIRE(array.isEmpty() );

	array.add(bx::ArrayViewT<int32_t>({ 1, 2, 3 }) );
	REQUIRE(3 == array.getCount() );

	array.trim();
	REQUIRE(array.getCapacity() >= array.getCount() );
	REQUIRE(3 == array.getCount() );

	array.clear();
	array.trim();
	REQUIRE(0 == array.getCount() );
}

TEST_CASE("ArrayT reserve and capacity", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<int32_t> array(allocator.getHandle() );

	array.setCapacity(100);
	REQUIRE(array.getCapacity() >= 100);
	REQUIRE(0 == array.getCount() );

	for (uint32_t ii = 0; ii < 50; ++ii)
	{
		array.add(int32_t(ii) );
	}

	REQUIRE(50 == array.getCount() );
	REQUIRE(array.getCapacity() >= 100);
}

TEST_CASE("binarySearch edge cases", "[container]")
{
	bx::FixedArrayT<int32_t, 32> array(bx::ArrayViewT<int32_t>({ 1, 3, 8, 9, 1389 }) );

	// Search for first element
	REQUIRE(0 == bx::binarySearch(1, array) );

	// Search for last element
	REQUIRE(4 == bx::binarySearch(1389, array) );

	// Search for non-existent element (returns ~insertionPoint)
	REQUIRE(~4 == bx::binarySearch(42, array) );

	// Search in single element array
	bx::FixedArrayT<int32_t, 4> single(bx::ArrayViewT<int32_t>({ 7 }) );
	REQUIRE(0 == bx::binarySearch(7, single) );
	REQUIRE(~1 == bx::binarySearch(8, single) );

	// Search in empty array
	bx::FixedArrayT<int32_t, 4> empty;
	REQUIRE(~0 == bx::binarySearch(1, empty) );
}

TEST_CASE("FixedArrayT pop first and last", "[container]")
{
	bx::FixedArrayT<int32_t, 32> array(bx::ArrayViewT<int32_t>({ 10, 20, 30, 40, 50, 60 }) );

	REQUIRE(10 == array.pop(bx::kFirst) );
	REQUIRE(5 == array.getCount() );
	REQUIRE(20 == array[0]);
	REQUIRE(60 == array[4]);

	REQUIRE(20 == array.pop(bx::kFirst) );
	REQUIRE(4 == array.getCount() );
	REQUIRE(30 == array[0]);
	REQUIRE(60 == array[3]);

	REQUIRE(60 == array.pop(bx::kLast) );
	REQUIRE(3 == array.getCount() );
	REQUIRE(30 == array[0]);
	REQUIRE(50 == array[2]);
}

TEST_CASE("FixedArrayT remove first and last", "[container]")
{
	bx::FixedArrayT<int32_t, 32> array(bx::ArrayViewT<int32_t>({ 1, 2, 3, 4, 5, 6, 7 }) );

	array.remove(bx::kFirst, 2);
	REQUIRE(5 == array.getCount() );
	REQUIRE(3 == array[0]);
	REQUIRE(4 == array[1]);
	REQUIRE(7 == array[4]);

	array.remove(bx::kLast, 2);
	REQUIRE(3 == array.getCount() );
	REQUIRE(3 == array[0]);
	REQUIRE(4 == array[1]);
	REQUIRE(5 == array[2]);
}

TEST_CASE("SortedArrayT add maintains order with duplicates", "[container]")
{
	bx::CrtAllocator allocator;
	bx::SortedArrayT<int32_t> array(allocator.getHandle() );

	array.add(5);
	array.add(3);
	array.add(5);
	array.add(1);
	array.add(3);
	array.add(5);

	REQUIRE(6 == array.getCount() );
	REQUIRE(bx::isSorted(array) );

	REQUIRE(1 == array[0]);
	REQUIRE(3 == array[1]);
	REQUIRE(3 == array[2]);
	REQUIRE(5 == array[3]);
	REQUIRE(5 == array[4]);
	REQUIRE(5 == array[5]);
}

TEST_CASE("SortedArrayT copy assignment allocator", "[container]")
{
	bx::CrtAllocator allocatorA;
	bx::CrtAllocator allocatorB;

	bx::SortedArrayT<int32_t> src(allocatorA.getHandle() );
	src.add(3);
	src.add(1);

	bx::SortedArrayT<int32_t> adopts;
	adopts = src;
	REQUIRE(allocatorA.getHandle() == adopts.getAllocatorHandle() );
	REQUIRE(bx::isEqual(adopts, src) );

	bx::SortedArrayT<int32_t> keeps(allocatorB.getHandle() );
	keeps = src;
	REQUIRE(allocatorB.getHandle() == keeps.getAllocatorHandle() );
	REQUIRE(bx::isEqual(keeps, src) );
}

TEST_CASE("SortedArrayT add at unchecked position", "[container]")
{
	bx::CrtAllocator allocator;
	bx::SortedArrayT<int32_t> array(allocator.getHandle() );

	array.add(bx::Unchecked, 0, 3);
	array.add(bx::Unchecked, 0, 1);
	array.add(bx::Unchecked, 2, 5);
	array.add(bx::Unchecked, 1, 3);

	REQUIRE(4 == array.getCount() );
	REQUIRE(1 == array[0]);
	REQUIRE(3 == array[1]);
	REQUIRE(3 == array[2]);
	REQUIRE(5 == array[3]);

	REQUIRE_ASSERTS(array.add(bx::Unchecked, 0, 4) );
	REQUIRE_ASSERTS(array.add(bx::Unchecked, 4, 4) );
	REQUIRE_ASSERTS(array.add(bx::Unchecked, 5, 9) );
}

TEST_CASE("FixedSortedArrayT remove and re-add", "[container]")
{
	bx::FixedSortedArrayT<int32_t, 32> array;

	array.add(10);
	array.add(30);
	array.add(20);
	REQUIRE(bx::isSorted(array) );

	(void)array.pop(1); // remove 20
	REQUIRE(2 == array.getCount() );
	REQUIRE(bx::isSorted(array) );

	array.add(15);
	array.add(25);
	REQUIRE(4 == array.getCount() );
	REQUIRE(bx::isSorted(array) );

	REQUIRE(10 == array[0]);
	REQUIRE(15 == array[1]);
	REQUIRE(25 == array[2]);
	REQUIRE(30 == array[3]);
}

TEST_CASE("ArrayViewT from FixedArrayT subrange", "[container]")
{
	bx::FixedArrayT<int32_t, 32> array(bx::ArrayViewT<int32_t>({ 10, 20, 30, 40, 50 }) );

	{
		bx::ArrayViewT<int32_t> view = array.get(1, 3);
		REQUIRE(3 == view.getCount() );
		REQUIRE(20 == view[0]);
		REQUIRE(30 == view[1]);
		REQUIRE(40 == view[2]);
	}

	{
		bx::ArrayViewT<int32_t> view = array.get(0, 1);
		REQUIRE(1 == view.getCount() );
		REQUIRE(10 == view[0]);
	}

	{
		bx::ArrayViewT<int32_t> view = array.get(4, 1);
		REQUIRE(1 == view.getCount() );
		REQUIRE(50 == view[0]);
	}
}

TEST_CASE("ArrayT comparison operator", "[container]")
{
	bx::CrtAllocator allocator;

	bx::ArrayT<int32_t> a(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 1, 2, 3 }) );
	bx::ArrayT<int32_t> b(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 1, 2, 3 }) );
	bx::ArrayT<int32_t> c(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 1, 2, 4 }) );
	bx::ArrayT<int32_t> d(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 1, 2 }) );

	REQUIRE(a == bx::ArrayViewT<int32_t>({ 1, 2, 3 }) );
	REQUIRE(!(a == bx::ArrayViewT<int32_t>({ 1, 2, 4 }) ));
	REQUIRE(!(a == bx::ArrayViewT<int32_t>({ 1, 2 }) ));
}

TEST_CASE("FixedArrayT add with insert", "[container]")
{
	bx::FixedArrayT<int32_t, 32> array;

	array.add(1);
	array.add(2);
	array.add(2, 3); // insert at index 2 (end)
	REQUIRE(3 == array.getCount() );
	REQUIRE(1 == array[0]);
	REQUIRE(2 == array[1]);
	REQUIRE(3 == array[2]);

	array.add(1, 99); // insert in middle
	REQUIRE(4 == array.getCount() );
	REQUIRE(1  == array[0]);
	REQUIRE(99 == array[1]);
	REQUIRE(2  == array[2]);
	REQUIRE(3  == array[3]);
}

TEST_CASE("ArrayT TestClassNtm move semantics", "[container]")
{
	REQUIRE(0 == TestClassNtm::s_instanceCount);

	bx::CrtAllocator allocator;

	{
		bx::ArrayT<TestClassNtm> src(allocator.getHandle() );
		src.add(bx::Emplace, "hello");
		src.add(bx::Emplace, "world");
		REQUIRE(2 == src.getCount() );
		REQUIRE(2 == TestClassNtm::s_instanceCount);

		bx::ArrayT<TestClassNtm> dst(bx::move(src) );
		REQUIRE(2 == dst.getCount() );
		REQUIRE(0 == src.getCount() );
		REQUIRE("hello" == dst[0].m_str);
		REQUIRE("world" == dst[1].m_str);
	}

	REQUIRE(0 == TestClassNtm::s_instanceCount);
}

TEST_CASE("FixedArrayT getIndex", "[container]")
{
	bx::FixedArrayT<int32_t, 32> array(bx::ArrayViewT<int32_t>({ 10, 20, 30 }) );

	for (uint32_t ii = 0, num = array.getCount(); ii < num; ++ii)
	{
		REQUIRE(ii == array.getIndex(array.getPtr(ii) ));
	}
}

TEST_CASE("SortedArrayT remove preserves order", "[container]")
{
	bx::CrtAllocator allocator;
	bx::SortedArrayT<int32_t> array(allocator.getHandle() );

	array.add(5);
	array.add(1);
	array.add(9);
	array.add(3);
	array.add(7);

	REQUIRE(bx::isSorted(array) );

	(void)array.pop(2); // remove middle element (5)
	REQUIRE(4 == array.getCount() );
	REQUIRE(bx::isSorted(array) );

	(void)array.pop(bx::kFirst);
	REQUIRE(3 == array.getCount() );
	REQUIRE(bx::isSorted(array) );

	(void)array.pop(bx::kLast);
	REQUIRE(2 == array.getCount() );
	REQUIRE(bx::isSorted(array) );
}

TEST_CASE("ArrayT remove at iterator unordered", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<uint32_t> array(allocator.getHandle() );

	for (uint32_t ii = 0; ii < 100; ++ii)
	{
		array.add(ii);
	}

	uint32_t numVisited = 0;

	for (uint32_t& value : array)
	{
		++numVisited;

		if (1 == (value & 1) )
		{
			array.remove(&value);
		}
	}

	REQUIRE(100 == numVisited);
	REQUIRE(50 == array.getCount() );

	uint32_t sum = 0;

	for (const uint32_t value : array)
	{
		REQUIRE(0 == (value & 1) );
		sum += value;
	}

	REQUIRE(2450 == sum);
}

TEST_CASE("ArrayT remove at iterator ordered", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<uint32_t> array(allocator.getHandle() );

	for (uint32_t ii = 0; ii < 100; ++ii)
	{
		array.add(ii);
	}

	uint32_t numVisited = 0;

	for (uint32_t& value : array)
	{
		++numVisited;

		if (0 == (value % 3) )
		{
			array.remove(&value, bx::kOrdered);
		}
	}

	REQUIRE(100 == numVisited);
	REQUIRE(66 == array.getCount() );

	uint32_t expected = 1;

	for (const uint32_t value : array)
	{
		REQUIRE(expected == value);
		expected += 1 + (0 == ( (expected + 1) % 3) );
	}
}

TEST_CASE("ArrayT remove at iterator runs", "[container]")
{
	bx::CrtAllocator allocator;

	{
		bx::ArrayT<uint32_t> array(allocator.getHandle() );

		for (uint32_t ii = 0; ii < 7; ++ii)
		{
			array.add(ii);
		}

		uint32_t numVisited = 0;

		for (uint32_t& value : array)
		{
			++numVisited;
			array.remove(&value);
		}

		REQUIRE(7 == numVisited);
		REQUIRE(0 == array.getCount() );
	}

	{
		bx::ArrayT<uint32_t> array(allocator.getHandle() );
		array.add(42u);

		uint32_t numVisited = 0;

		for (uint32_t& value : array)
		{
			++numVisited;
			REQUIRE(42 == value);
			array.remove(&value, bx::kOrdered);
		}

		REQUIRE(1 == numVisited);
		REQUIRE(0 == array.getCount() );
	}

	{
		bx::ArrayT<uint32_t> array(allocator.getHandle() );

		for (uint32_t ii = 0; ii < 10; ++ii)
		{
			array.add(ii);
		}

		uint32_t numVisited = 0;

		for (uint32_t& value : array)
		{
			++numVisited;

			if (9 == value)
			{
				array.remove(&value);
			}
		}

		REQUIRE(10 == numVisited);
		REQUIRE(9 == array.getCount() );

		for (uint32_t& value : array)
		{
			if (0 == value)
			{
				array.remove(&value, bx::kOrdered);
			}
		}

		REQUIRE(8 == array.getCount() );
		REQUIRE(1 == array[0]);
		REQUIRE(8 == array[7]);
	}
}

TEST_CASE("ArrayT range-for mutates elements", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<uint32_t> array(allocator.getHandle() );

	for (uint32_t ii = 0; ii < 16; ++ii)
	{
		array.add(ii);
	}

	for (uint32_t& value : array)
	{
		value *= 2;
	}

	const bx::ArrayT<uint32_t>& constArray = array;

	uint32_t idx = 0;

	for (const uint32_t& value : constArray)
	{
		REQUIRE(idx*2 == value);
		++idx;
	}

	REQUIRE(16 == idx);
}

TEST_CASE("FixedArrayT remove at iterator", "[container]")
{
	bx::FixedArrayT<int32_t, 8> array;

	for (int32_t ii = 0; ii < 8; ++ii)
	{
		array.add(ii);
	}

	for (int32_t& value : array)
	{
		if (0 == (value & 1) )
		{
			array.remove(&value, bx::kOrdered);
		}
	}

	REQUIRE(4 == array.getCount() );

	for (uint32_t ii = 0, num = array.getCount(); ii < num; ++ii)
	{
		REQUIRE(int32_t(ii*2 + 1) == array[ii]);
	}
}

TEST_CASE("SortedArrayT remove at iterator", "[container]")
{
	bx::CrtAllocator allocator;
	bx::SortedArrayT<int32_t> array(allocator.getHandle() );

	for (int32_t ii = 99; ii >= 0; --ii)
	{
		array.add(ii);
	}

	uint32_t numVisited = 0;

	for (const int32_t& value : array)
	{
		++numVisited;

		if (value < 50)
		{
			array.remove(&value);
		}
	}

	REQUIRE(100 == numVisited);
	REQUIRE(50 == array.getCount() );
	REQUIRE(bx::isSorted(array) );
	REQUIRE(50 == array[0]);
	REQUIRE(99 == array[49]);
}

TEST_CASE("ArrayT nested arrays", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<bx::ArrayT<int32_t> > outer(allocator.getHandle(), 4, bx::Emplace, allocator.getHandle() );

	REQUIRE(4 == outer.getCount() );

	for (uint32_t ii = 0, num = outer.getCount(); ii < num; ++ii)
	{
		REQUIRE(allocator.getHandle() == outer[ii].getAllocatorHandle() );
		REQUIRE(outer[ii].isEmpty() );

		for (int32_t jj = 0; jj <= int32_t(ii); ++jj)
		{
			outer[ii].add(jj);
		}
	}

	bx::ArrayT<int32_t> inner(allocator.getHandle() );
	inner.add(7);
	inner.add(8);

	outer.add(inner);
	outer.add(bx::move(inner) );

	REQUIRE(6 == outer.getCount() );
	REQUIRE(2 == outer[4].getCount() );
	REQUIRE(2 == outer[5].getCount() );
	REQUIRE(8 == outer[5][1]);

	bx::ArrayT<bx::ArrayT<int32_t> > copy(allocator.getHandle() );
	copy = outer;

	REQUIRE(6 == copy.getCount() );

	for (uint32_t ii = 0, num = copy.getCount(); ii < num; ++ii)
	{
		REQUIRE(allocator.getHandle() == copy[ii].getAllocatorHandle() );
		REQUIRE(bx::isEqual(copy[ii], outer[ii]) );
	}

	copy[2].add(100);
	REQUIRE(4 == copy[2].getCount() );
	REQUIRE(3 == outer[2].getCount() );

	for (uint32_t ii = 0; ii < 100; ++ii)
	{
		bx::ArrayT<int32_t> row(allocator.getHandle() );
		row.add(int32_t(ii) );
		outer.add(bx::move(row) );
	}

	REQUIRE(106 == outer.getCount() );
	REQUIRE(99 == outer[105][0]);
}

TEST_CASE("ArrayT default allocator", "[container]")
{
	bx::ArrayT<int32_t> array;
	REQUIRE(bx::kAllocatorHandleDefault == array.getAllocatorHandle().idx);
	REQUIRE(bx::getDefaultAllocatorBlk() == bx::getAllocator(array.getAllocatorHandle() ) );
	REQUIRE(0 == array.getCapacity() );

	for (int32_t ii = 0; ii < 100; ++ii)
	{
		array.add(ii);
	}

	REQUIRE(100 == array.getCount() );
	REQUIRE(bx::getDefaultAllocatorBlk() == bx::getAllocator(array.getAllocatorHandle() ) );
	REQUIRE(bx::getDefaultAllocatorBlk()->getHandle().idx == array.getAllocatorHandle().idx);

	bx::ArrayT<bx::ArrayT<int32_t> > nested;
	nested.setCount(4);
	nested[2].add(5);
	nested[2].add(6);
	REQUIRE(2 == nested[2].getCount() );
	REQUIRE(0 == nested[3].getCount() );
	REQUIRE(bx::getDefaultAllocatorBlk() == bx::getAllocator(nested[2].getAllocatorHandle() ) );

	bx::ArrayT<int32_t> moved(bx::move(array) );
	REQUIRE(100 == moved.getCount() );
	REQUIRE(0 == array.getCount() );
	array.add(1);
	REQUIRE(1 == array.getCount() );
}

TEST_CASE("ArrayT keeps allocator across trim", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<int32_t> array(allocator.getHandle() );

	array.add(1);
	REQUIRE(allocator.getHandle() == array.getAllocatorHandle() );

	array.clear();
	array.trim();
	REQUIRE(0 == array.getCapacity() );
	REQUIRE(allocator.getHandle() == array.getAllocatorHandle() );

	array.add(2);
	REQUIRE(allocator.getHandle() == array.getAllocatorHandle() );
	REQUIRE(allocator.getHandle().idx == array.getAllocatorHandle().idx);

	bx::ArrayT<int32_t> copy(array);
	REQUIRE(allocator.getHandle() == copy.getAllocatorHandle() );

	bx::ArrayT<int32_t> assigned;
	assigned = array;
	REQUIRE(allocator.getHandle() == assigned.getAllocatorHandle() );

	bx::ArrayT<int32_t> pinned(bx::getDefaultAllocatorBlk()->getHandle() );
	pinned = array;
	REQUIRE(bx::getDefaultAllocatorBlk() == bx::getAllocator(pinned.getAllocatorHandle() ) );
}

TEST_CASE("ArrayT add grows geometrically", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<uint32_t> array(allocator.getHandle() );
	bx::SortedArrayT<uint32_t> sorted(allocator.getHandle() );

	uint32_t numGrows       = 0;
	uint32_t numSortedGrows = 0;
	uint32_t capacity       = 0;
	uint32_t sortedCapacity = 0;

	for (uint32_t ii = 0; ii < 100000; ++ii)
	{
		array.add(ii);
		numGrows += capacity != array.getCapacity();
		capacity  = array.getCapacity();

		sorted.add(ii);
		numSortedGrows += sortedCapacity != sorted.getCapacity();
		sortedCapacity  = sorted.getCapacity();
	}

	REQUIRE(100000 == array.getCount() );
	REQUIRE(100000 == sorted.getCount() );
	REQUIRE(32 > numGrows);
	REQUIRE(32 > numSortedGrows);

	bx::ArrayT<uint32_t> reserved(allocator.getHandle(), 1000);
	REQUIRE(1000 <= reserved.getCapacity() );
	REQUIRE(1024 >= reserved.getCapacity() );
}

TEST_CASE("ArrayT setCount initializes new elements", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<int32_t> array(allocator.getHandle() );

	array.setCount(64);
	bx::memSet(array.getPtr(), 0xff, 64*sizeof(int32_t) );

	array.setCount(0);
	array.setCount(64);
	testForEach(array, 0);

	array.setCount(0);
	array.setCount(32, 1389);
	REQUIRE(32 == array.getCount() );
	testForEach(array, 1389);

	array.setCount(48, array[0]);
	REQUIRE(48 == array.getCount() );
	testForEach(array, 1389);

	array.setCount(16, 7);
	REQUIRE(16 == array.getCount() );
	REQUIRE(1389 == array[15]);

	array.setCount(0);
	array.setCount(48, bx::InitNone);
	REQUIRE(48 == array.getCount() );
	REQUIRE(1389 == array[47]);

	bx::FixedArrayT<int32_t, 32> fixed;
	fixed.setCount(32, -1);
	fixed.setCount(0);
	fixed.setCount(32);
	testForEach(fixed, 0);
}

TEST_CASE("ArrayT add own element", "[container]")
{
	bx::CrtAllocator allocator;
	bx::ArrayT<int32_t> array(allocator.getHandle() );

	array.add(100);

	while (array.getCount() < array.getCapacity() )
	{
		array.add(int32_t(array.getCount() + 100) );
	}

	const uint32_t count = array.getCount();
	array.add(array[0]);
	REQUIRE(count + 1 == array.getCount() );
	REQUIRE(100 == array[count]);

	array.add(0, array[3]);
	REQUIRE(103 == array[0]);
	REQUIRE(100 == array[1]);
	REQUIRE(103 == array[4]);

	bx::SortedArrayT<int32_t> sorted(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 1, 3, 5, 7 }) );
	sorted.add(sorted[3]);
	sorted.add(sorted[0]);
	REQUIRE(sorted == bx::ArrayViewT<int32_t>({ 1, 1, 3, 5, 7, 7 }) );

	bx::ArrayT<int32_t> self(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 1, 2, 3, 4 }) );
	self.trim();
	self.add(bx::ArrayViewT<int32_t>(self.getPtr(1), 2) );
	self.add(0, bx::ArrayViewT<int32_t>(self.getPtr(4), 2) );
	REQUIRE(self == bx::ArrayViewT<int32_t>({ 2, 3, 1, 2, 3, 4, 2, 3 }) );

	bx::ArrayT<bx::ArrayT<int32_t> > outer(allocator.getHandle() );
	outer.setCount(1);
	outer[0].add(7);
	outer[0].add(8);
	outer.setCount(64, outer[0]);
	REQUIRE(64 == outer.getCount() );

	for (uint32_t ii = 0, num = outer.getCount(); ii < num; ++ii)
	{
		REQUIRE(outer[ii] == bx::ArrayViewT<int32_t>({ 7, 8 }) );
	}
}

TEST_CASE("ArrayWriterT", "[container]")
{
	bx::CrtAllocator allocator;

	bx::ArrayT<char> text(allocator.getHandle() );
	bx::ArrayWriterT<char> textWriter(&text);
	bx::write(&textWriter, bx::ErrorAssert{}, "%s=%d;", "x", 1389);
	bx::write(&textWriter, "abc", 3, bx::ErrorAssert{});
	REQUIRE(text == bx::ArrayViewT<char>("x=1389;abc", 10) );

	bx::ArrayT<uint8_t> bytes(allocator.getHandle() );
	bx::ArrayWriterT<uint8_t> byteWriter(&bytes);

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		bx::write(&byteWriter, ii, bx::ErrorAssert{});
	}

	REQUIRE(4000 == bytes.getCount() );
	REQUIRE(999 == bx::loadUnaligned<uint32_t>(bytes.getPtr(3996) ) );
}

TEST_CASE("ArrayT write", "[container]")
{
	bx::CrtAllocator allocator;

	bx::ArrayT<uint8_t> out(allocator.getHandle() );
	bx::ArrayWriterT<uint8_t> writer(&out);

	bx::ArrayT<uint16_t> array(allocator.getHandle(), bx::ArrayViewT<uint16_t>({ 1, 2 }) );
	bx::FixedArrayT<uint16_t, 4> fixed(bx::ArrayViewT<uint16_t>({ 3 }) );
	bx::SortedArrayT<uint16_t> sorted(allocator.getHandle(), bx::ArrayViewT<uint16_t>({ 5, 4 }) );
	bx::FixedSortedArrayT<uint16_t, 4> fixedSorted(bx::ArrayViewT<uint16_t>({ 6 }) );
	bx::StaticArrayT<uint16_t, 1> fixedStatic;
	fixedStatic[0] = 7;

	REQUIRE(4 == bx::write(&writer, array,       bx::ErrorAssert{}) );
	REQUIRE(2 == bx::write(&writer, fixed,       bx::ErrorAssert{}) );
	REQUIRE(4 == bx::write(&writer, sorted,      bx::ErrorAssert{}) );
	REQUIRE(2 == bx::write(&writer, fixedSorted, bx::ErrorAssert{}) );
	REQUIRE(2 == bx::write(&writer, fixedStatic, bx::ErrorAssert{}) );
	REQUIRE(2 == bx::write(&writer, bx::ArrayViewT<uint16_t>({ 8 }), bx::ErrorAssert{}) );
	REQUIRE(0 == bx::write(&writer, bx::MutableArrayViewT<uint16_t>(), bx::ErrorAssert{}) );

	REQUIRE(16 == out.getCount() );

	for (uint16_t ii = 0; ii < 8; ++ii)
	{
		REQUIRE(ii+1 == bx::loadUnaligned<uint16_t>(out.getPtr(ii*2) ) );
	}
}

TEST_CASE("ArrayT stableSort", "[container]")
{
	struct Pair
	{
		uint32_t key;
		uint32_t order;
	};

	bx::CrtAllocator allocator;
	bx::ArrayT<Pair> array(allocator.getHandle() );

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		array.add({ (ii*7919) % 10, ii });
	}

	bx::stableSort(array, [](const Pair& _lhs, const Pair& _rhs) { return _lhs.key < _rhs.key; });

	REQUIRE(1000 == array.getCount() );

	for (uint32_t ii = 1, num = array.getCount(); ii < num; ++ii)
	{
		const Pair& prev = array[ii-1];
		const Pair& curr = array[ii];
		REQUIRE( (prev.key < curr.key || (prev.key == curr.key && prev.order < curr.order) ) );
	}
}

TEST_CASE("isEqual C string", "[container]")
{
	char str[] = "abc";
	const char* lhs = str;
	const char* rhs = "abc";
	REQUIRE(lhs != rhs);
	REQUIRE(bx::isEqual(lhs, rhs) );
}

TEST_CASE("ArrayT pop clamps index", "[container]")
{
	bx::CrtAllocator allocator;

	bx::ArrayT<int32_t> array(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 1, 2, 3 }) );
	REQUIRE(3 == array.pop(100, bx::kOrdered) );
	REQUIRE(array == bx::ArrayViewT<int32_t>({ 1, 2 }) );

	bx::SortedArrayT<int32_t> sorted(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 3, 1, 2 }) );
	REQUIRE(3 == sorted.pop(100) );
	REQUIRE(sorted == bx::ArrayViewT<int32_t>({ 1, 2 }) );
}

struct RecursiveNode
{
	uint32_t m_value = 0;
	bx::ArrayT<RecursiveNode> m_children;
};

TEST_CASE("ArrayT recursive element type", "[container]")
{
	RecursiveNode root;

	for (uint32_t ii = 0; ii < 100; ++ii)
	{
		root.m_children.add(bx::Emplace);
		root.m_children[ii].m_value = ii;
		root.m_children[ii].m_children.add(bx::Emplace);
	}

	REQUIRE(100 == root.m_children.getCount() );
	REQUIRE(99 == root.m_children[99].m_value);
	REQUIRE(1 == root.m_children[99].m_children.getCount() );
}

TEST_CASE("ArrayT emplace ctors and set", "[container]")
{
	bx::CrtAllocator allocator;

	bx::FixedArrayT<int32_t, 4> full(bx::Emplace, 7);
	REQUIRE(full == bx::ArrayViewT<int32_t>({ 7, 7, 7, 7 }) );

	bx::FixedArrayT<int32_t, 8> three(3, bx::Emplace, 9);
	REQUIRE(three == bx::ArrayViewT<int32_t>({ 9, 9, 9 }) );

	bx::ArrayT<int32_t> array(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 1, 2, 3 }) );
	array.set(bx::Emplace, 1389);
	REQUIRE(array == bx::ArrayViewT<int32_t>({ 1389 }) );

	REQUIRE(0 == TestClassNtm::s_instanceCount);
	{
		bx::ArrayT<TestClassNtm> objects(allocator.getHandle(), 4, bx::Emplace);
		REQUIRE(4 == objects.getCount() );
		REQUIRE(4 == TestClassNtm::s_instanceCount);

		objects.set(bx::Emplace);
		REQUIRE(1 == objects.getCount() );
		REQUIRE(1 == TestClassNtm::s_instanceCount);
	}
	REQUIRE(0 == TestClassNtm::s_instanceCount);
}

TEST_CASE("SortedArrayT copy, move and emplace ctors", "[container]")
{
	bx::CrtAllocator allocator;

	bx::SortedArrayT<int32_t> sorted(allocator.getHandle(), 5, bx::Emplace, 3);
	REQUIRE(sorted == bx::ArrayViewT<int32_t>({ 3, 3, 3, 3, 3 }) );

	sorted.add(1);
	sorted.add(8);

	bx::SortedArrayT<int32_t> copy(sorted);
	REQUIRE(copy == bx::ArrayViewT<int32_t>({ 1, 3, 3, 3, 3, 3, 8 }) );
	REQUIRE(allocator.getHandle() == copy.getAllocatorHandle() );

	bx::SortedArrayT<int32_t> moved(bx::move(copy) );
	REQUIRE(moved == bx::ArrayViewT<int32_t>({ 1, 3, 3, 3, 3, 3, 8 }) );
	REQUIRE(allocator.getHandle() == moved.getAllocatorHandle() );
	REQUIRE(copy.isEmpty() );

	copy.add(2);
	REQUIRE(copy == bx::ArrayViewT<int32_t>({ 2 }) );

	moved.set(bx::Emplace, 5);
	REQUIRE(moved == bx::ArrayViewT<int32_t>({ 5 }) );

	moved.add(9);
	moved.add(1);
	REQUIRE(moved == bx::ArrayViewT<int32_t>({ 1, 5, 9 }) );

	bx::SortedArrayT<int32_t> empty(allocator.getHandle() );
	empty.set(bx::Emplace, 7);
	REQUIRE(empty == bx::ArrayViewT<int32_t>({ 7 }) );
}

namespace
{
	struct MoveOnly
	{
		BX_CLASS_NO_COPY(MoveOnly);

		static int32_t s_live;

		MoveOnly()
			: m_value(0)
		{
			++s_live;
		}

		explicit MoveOnly(int32_t _value)
			: m_value(_value)
		{
			++s_live;
		}

		MoveOnly(MoveOnly&& _other)
			: m_value(_other.m_value)
		{
			_other.m_value = -1;
			++s_live;
		}

		~MoveOnly()
		{
			--s_live;
		}

		MoveOnly& operator=(MoveOnly&& _other)
		{
			m_value = _other.m_value;
			_other.m_value = -1;
			return *this;
		}

		int32_t m_value;
	};

	int32_t MoveOnly::s_live = 0;
}

TEST_CASE("ArrayT move-only elements", "[container]")
{
	bx::CrtAllocator allocator;

	{
		bx::ArrayT<MoveOnly> array(allocator.getHandle() );

		for (int32_t ii = 0; ii < 100; ++ii)
		{
			array.add(MoveOnly(ii) );
		}

		MoveOnly value(100);
		array.add(bx::move(value) );
		REQUIRE(-1 == value.m_value);

		array.add(bx::Emplace, 101);
		array.add(0, MoveOnly(-1) );
		array.add(1, bx::Emplace, -2);
		REQUIRE(104 == array.getCount() );
		REQUIRE(-1 == array[0].m_value);
		REQUIRE(-2 == array[1].m_value);
		REQUIRE(0  == array[2].m_value);

		array.remove(0, bx::kOrdered, 2);
		array.remove(5, bx::kOrdered, 10);
		array.remove(0);
		REQUIRE(91  == array.getCount() );
		REQUIRE(101 == array[0].m_value);
		REQUIRE(1   == array[1].m_value);
		REQUIRE(15  == array[5].m_value);

		bx::ArrayT<MoveOnly> moved(bx::move(array) );
		REQUIRE(91 == moved.getCount() );

		bx::ArrayT<MoveOnly> assigned(allocator.getHandle() );
		assigned.add(MoveOnly(7) );
		assigned = bx::move(moved);
		REQUIRE(91  == assigned.getCount() );
		REQUIRE(101 == assigned[0].m_value);

		assigned.setCount(10);
		assigned.setCount(200);
		REQUIRE(0 == assigned[199].m_value);
		assigned.trim();

		bx::ArrayT<bx::ArrayT<MoveOnly> > nested(allocator.getHandle() );

		for (int32_t ii = 0; ii < 20; ++ii)
		{
			nested.add(bx::ArrayT<MoveOnly>(allocator.getHandle() ) );
			nested[ii].add(MoveOnly(ii) );
		}

		nested.remove(3, bx::kOrdered);
		REQUIRE(19 == nested.getCount() );
		REQUIRE(4  == nested[3][0].m_value);
	}

	REQUIRE(0 == MoveOnly::s_live);
}

namespace
{
	struct NoDefault
	{
		static int32_t s_live;

		explicit NoDefault(int32_t _value)
			: m_value(_value)
			, m_self(this)
		{
			++s_live;
		}

		NoDefault(const NoDefault& _other)
			: m_value(_other.m_value)
			, m_self(this)
		{
			REQUIRE(&_other == _other.m_self);
			++s_live;
		}

		NoDefault(NoDefault&& _other)
			: m_value(_other.m_value)
			, m_self(this)
		{
			REQUIRE(&_other == _other.m_self);
			_other.m_value = -1;
			++s_live;
		}

		~NoDefault()
		{
			REQUIRE(this == m_self);
			m_self = NULL;
			--s_live;
		}

		NoDefault& operator=(const NoDefault& _other)
		{
			REQUIRE(this == m_self);
			REQUIRE(&_other == _other.m_self);
			m_value = _other.m_value;
			return *this;
		}

		NoDefault& operator=(NoDefault&& _other)
		{
			REQUIRE(this == m_self);
			REQUIRE(&_other == _other.m_self);
			m_value = _other.m_value;
			_other.m_value = -1;
			return *this;
		}

		bool operator==(const NoDefault& _rhs) const
		{
			return m_value == _rhs.m_value;
		}

		int32_t    m_value;
		NoDefault* m_self;
	};

	int32_t NoDefault::s_live = 0;

	template<typename Ty, typename MakeFn>
	void testArrayVsVector(MakeFn _make)
	{
		bx::CrtAllocator allocator;
		bx::ArrayT<Ty> array(allocator.getHandle() );
		std::vector<Ty> ref;

		bx::RngMwc rng;

		for (int32_t op = 0; op < 4000; ++op)
		{
			const uint32_t count = uint32_t(ref.size() );
			const uint32_t pos   = rng.gen() % (count + 1);

			switch (rng.gen() % 11)
			{
			case 0:
				array.add(_make(op) );
				ref.push_back(_make(op) );
				break;

			case 7:
				if (pos < count)
				{
					const uint32_t idx = rng.gen() % count;
					array.add(pos, array[idx]);
					const Ty value(ref[idx]);
					ref.insert(ref.begin() + pos, value);
				}
				break;

			case 8:
				if (pos < count
				&&  count < 512)
				{
					const uint32_t src = rng.gen() % count;
					const uint32_t num = 1 + rng.gen() % (count - src);
					array.add(pos, bx::ArrayViewT<Ty>(&array[src], num) );
					const std::vector<Ty> copy(ref.begin() + src, ref.begin() + src + num);
					ref.insert(ref.begin() + pos, copy.begin(), copy.end() );
				}
				break;

			case 9:
				if (pos < count
				&&  0 == rng.gen() % 4)
				{
					const uint32_t num = rng.gen() % (count - pos + 1);

					if (0 == rng.gen() % 2)
					{
						array.set(bx::ArrayViewT<Ty>(&array[pos], num) );
					}
					else
					{
						array = bx::ArrayViewT<Ty>(&array[pos], num);
					}

					ref = std::vector<Ty>(ref.begin() + pos, ref.begin() + pos + num);
				}
				break;

			case 1:
				{
					const Ty value = _make(op);
					array.add(pos, value);
					ref.insert(ref.begin() + pos, value);
				}
				break;

			case 2:
				array.add(pos, bx::Emplace, _make(op) );
				ref.insert(ref.begin() + pos, _make(op) );
				break;

			case 3:
				{
					std::vector<Ty> src;
					const uint32_t num = rng.gen() % 12;

					for (uint32_t ii = 0; ii < num; ++ii)
					{
						src.push_back(_make(op*16 + int32_t(ii) ) );
					}

					array.add(pos, bx::ArrayViewT<Ty>(src.data(), num) );
					ref.insert(ref.begin() + pos, src.begin(), src.end() );
				}
				break;

			case 4:
				if (pos < count)
				{
					const uint32_t num = 1 + rng.gen() % bx::min<uint32_t>(count - pos, 8);
					array.remove(pos, bx::kOrdered, num);
					ref.erase(ref.begin() + pos, ref.begin() + pos + num);
				}
				break;

			case 5:
				if (pos < count)
				{
					array.remove(pos);
					ref[pos] = std::move(ref.back() );
					ref.pop_back();
				}
				break;

			case 6:
				{
					const uint32_t num = rng.gen() % (count + 16);
					array.setCount(num, _make(op) );
					ref.resize(num, _make(op) );
				}
				break;

			default:
				if (64 < count)
				{
					bx::ArrayT<Ty> copy(array);
					array = bx::move(copy);
					array.trim();
				}
				break;
			}

			REQUIRE(ref.size() == array.getCount() );

			for (uint32_t ii = 0, num = array.getCount(); ii < num; ++ii)
			{
				REQUIRE(ref[ii] == array[ii]);
			}
		}
	}

	NoDefault makeNoDefault(int32_t _value)
	{
		return NoDefault(_value);
	}

	std::string makeString(int32_t _value)
	{
		return 0 == _value%3
			? std::to_string(_value) + std::string(32, 's')
			: std::to_string(_value)
			;
	}

	struct Pod12
	{
		bool operator==(const Pod12& _rhs) const
		{
			return x == _rhs.x && y == _rhs.y && z == _rhs.z;
		}

		int32_t x;
		int32_t y;
		int32_t z;
	};

	uint32_t makeU32(int32_t _value)
	{
		return uint32_t(_value);
	}

	Pod12 makePod12(int32_t _value)
	{
		return { _value, -_value, _value*3 };
	}

	struct Reloc
	{
		BX_CLASS(Reloc, TRIVIALLY_RELOCATABLE);

		Reloc(int32_t _key, int32_t _tag)
			: m_data(new int32_t[2]{ _key, _tag })
		{
			++s_live;
		}

		Reloc(const Reloc& _other)
			: m_data(new int32_t[2]{ _other.m_data[0], _other.m_data[1] })
		{
			++s_live;
		}

		Reloc(Reloc&& _other)
			: m_data(_other.m_data)
		{
			_other.m_data = NULL;
			++s_live;
		}

		~Reloc()
		{
			delete [] m_data;
			--s_live;
		}

		Reloc& operator=(const Reloc& _other)
		{
			if (this != &_other)
			{
				delete [] m_data;
				m_data = new int32_t[2]{ _other.m_data[0], _other.m_data[1] };
			}

			return *this;
		}

		Reloc& operator=(Reloc&& _other)
		{
			if (this != &_other)
			{
				delete [] m_data;
				m_data = _other.m_data;
				_other.m_data = NULL;
			}

			return *this;
		}

		bool operator==(const Reloc& _rhs) const
		{
			return m_data[0] == _rhs.m_data[0]
				&& m_data[1] == _rhs.m_data[1]
				;
		}

		static int32_t s_live;

		int32_t* m_data;
	};

	struct RelocDerived : Reloc
	{
	};

	int32_t Reloc::s_live = 0;

	Reloc makeReloc(int32_t _value)
	{
		return Reloc(_value, _value*3);
	}

	bx::String makeBxString(int32_t _value)
	{
		return bx::String(makeString(_value).c_str() );
	}
}

static_assert(bx::isTriviallyRelocatable<Reloc>());
static_assert(bx::isTriviallyRelocatable<bx::Blk>());
static_assert(bx::isTriviallyRelocatable<bx::ArrayT<std::string>>());
static_assert(!bx::isTriviallyRelocatable<RelocDerived>());
static_assert(!bx::isTriviallyRelocatable<std::string>());

TEST_CASE("ArrayT element without default constructor", "[container]")
{
	testArrayVsVector<NoDefault>(makeNoDefault);
	REQUIRE(0 == NoDefault::s_live);

	bx::FixedArrayT<NoDefault, 16> fixed;
	fixed.add(NoDefault(1) );
	fixed.add(0, NoDefault(0) );
	fixed.add(bx::Emplace, 2);
	REQUIRE(3 == fixed.getCount() );
	REQUIRE(0 == fixed[0].m_value);
	REQUIRE(2 == fixed[2].m_value);
	fixed.clear();
	REQUIRE(0 == NoDefault::s_live);
}

TEST_CASE("ArrayT std::string elements", "[container]")
{
	testArrayVsVector<std::string>(makeString);
}

TEST_CASE("ArrayT trivially copyable elements", "[container]")
{
	testArrayVsVector<uint32_t>(makeU32);
	testArrayVsVector<Pod12>(makePod12);
}

TEST_CASE("ArrayT trivially relocatable elements", "[container]")
{
	testArrayVsVector<Reloc>(makeReloc);
	REQUIRE(0 == Reloc::s_live);

	testArrayVsVector<bx::String>(makeBxString);
	bx::StringPool::getDefault()->trim();
}

namespace
{
	int32_t compareFirstChar(const void* _lhs, const void* _rhs)
	{
		const std::string& lhs = *(const std::string*)_lhs;
		const std::string& rhs = *(const std::string*)_rhs;
		return int32_t(uint8_t(lhs[0]) ) - int32_t(uint8_t(rhs[0]) );
	}

	struct KeyTag
	{
		bool operator==(const KeyTag& _rhs) const
		{
			return key == _rhs.key && tag == _rhs.tag;
		}

		uint8_t key;
		int32_t tag;
	};

	int32_t compareKey(const void* _lhs, const void* _rhs)
	{
		return int32_t( ( (const KeyTag*)_lhs)->key) - int32_t( ( (const KeyTag*)_rhs)->key);
	}

	bool lessKey(const std::string& _lhs, const std::string& _rhs)
	{
		return uint8_t(_lhs[0]) < uint8_t(_rhs[0]);
	}

	bool lessKey(const KeyTag& _lhs, const KeyTag& _rhs)
	{
		return _lhs.key < _rhs.key;
	}

	template<typename Ty>
	Ty makeSorted(bx::RngMwc& _rng, int32_t _value);

	template<>
	KeyTag makeSorted<KeyTag>(bx::RngMwc& _rng, int32_t _value)
	{
		return { uint8_t('a' + _rng.gen() % 16), _value };
	}

	int32_t compareRelocKey(const void* _lhs, const void* _rhs)
	{
		return ( (const Reloc*)_lhs)->m_data[0] - ( (const Reloc*)_rhs)->m_data[0];
	}

	bool lessKey(const Reloc& _lhs, const Reloc& _rhs)
	{
		return _lhs.m_data[0] < _rhs.m_data[0];
	}

	template<>
	Reloc makeSorted<Reloc>(bx::RngMwc& _rng, int32_t _value)
	{
		return Reloc(int32_t('a' + _rng.gen() % 16), _value);
	}

	template<>
	std::string makeSorted<std::string>(bx::RngMwc& _rng, int32_t _value)
	{
		std::string result(1, char('a' + _rng.gen() % 16) );
		result += std::to_string(_value);

		if (0 == _value%3)
		{
			result += std::string(32, 's');
		}

		return result;
	}

	template<typename Ty>
	uint32_t refLowerBound(const std::vector<Ty>& _ref, const Ty& _key)
	{
		return uint32_t(std::lower_bound(_ref.begin(), _ref.end(), _key, [](const Ty& _lhs, const Ty& _rhs) { return lessKey(_lhs, _rhs); }) - _ref.begin() );
	}

	template<typename Ty>
	uint32_t refUpperBound(const std::vector<Ty>& _ref, const Ty& _key)
	{
		return uint32_t(std::upper_bound(_ref.begin(), _ref.end(), _key, [](const Ty& _lhs, const Ty& _rhs) { return lessKey(_lhs, _rhs); }) - _ref.begin() );
	}

	template<typename Ty, typename ArrayT>
	void testSortedArrayVsVector(ArrayT& _array, uint32_t _maxCount)
	{
		std::vector<Ty> ref;

		bx::RngMwc rng;

		for (int32_t op = 0; op < 6000; ++op)
		{
			const uint32_t    count = uint32_t(ref.size() );
			const Ty          value = makeSorted<Ty>(rng, op);
			const uint32_t    lower = refLowerBound(ref, value);
			const uint32_t    upper = refUpperBound(ref, value);
			const bool        room  = count < _maxCount;

			switch (rng.gen() % 14)
			{
			case 0:
			case 1:
				if (room)
				{
					_array.add(value);
					ref.insert(ref.begin() + upper, value);
				}
				break;

			case 2:
				if (room)
				{
					const uint32_t pos = rng.gen() % (upper + 1);
					_array.add(pos, value);
					ref.insert(ref.begin() + upper, value);
				}
				break;

			case 3:
				if (room)
				{
					const uint32_t pos = lower + rng.gen() % (upper - lower + 1);
					Ty moved(value);
					_array.add(bx::Unchecked, pos, bx::move(moved) );
					ref.insert(ref.begin() + pos, value);
				}
				break;

			case 4:
				if (room
				&&  0 != count)
				{
					const uint32_t idx = rng.gen() % count;
					_array.add(_array[idx]);
					const Ty copy(ref[idx]);
					ref.insert(ref.begin() + refUpperBound(ref, copy), copy);
				}
				break;

			case 5:
				{
					const uint32_t num = bx::min<uint32_t>(rng.gen() % 12, _maxCount - count);
					std::vector<Ty> src;

					for (uint32_t ii = 0; ii < num; ++ii)
					{
						src.push_back(makeSorted<Ty>(rng, op*16 + int32_t(ii) ) );
					}

					_array.add(bx::ArrayViewT<Ty>(src.data(), num) );

					for (const Ty& str : src)
					{
						ref.insert(ref.begin() + refUpperBound(ref, str), str);
					}
				}
				break;

			case 6:
				if (0 != count
				&&  2*count <= _maxCount)
				{
					const uint32_t pos = rng.gen() % count;
					const uint32_t num = 1 + rng.gen() % (count - pos);
					_array.add(bx::ArrayViewT<Ty>(&_array[pos], num) );

					const std::vector<Ty> src(ref.begin() + pos, ref.begin() + pos + num);

					for (const Ty& str : src)
					{
						ref.insert(ref.begin() + refUpperBound(ref, str), str);
					}
				}
				break;

			case 7:
				if (0 != count)
				{
					const uint32_t pos = rng.gen() % count;
					const uint32_t num = 1 + rng.gen() % bx::min<uint32_t>(count - pos, 8);
					_array.remove(pos, num);
					ref.erase(ref.begin() + pos, ref.begin() + pos + num);
				}
				break;

			case 8:
				if (0 != count)
				{
					const uint32_t num = 1 + rng.gen() % bx::min<uint32_t>(count, 4);

					if (0 == rng.gen() % 2)
					{
						_array.remove(bx::kFirst, num);
						ref.erase(ref.begin(), ref.begin() + num);
					}
					else
					{
						_array.remove(bx::kLast, num);
						ref.erase(ref.end() - num, ref.end() );
					}
				}
				break;

			case 9:
				if (0 != count)
				{
					const uint32_t idx = rng.gen() % count;

					if (0 == rng.gen() % 2)
					{
						_array.remove(&_array[idx]);
					}
					else
					{
						REQUIRE(ref[idx] == _array.pop(idx) );
					}

					ref.erase(ref.begin() + idx);
				}
				break;

			case 10:
				if (0 != count)
				{
					if (0 == rng.gen() % 2)
					{
						REQUIRE(ref.front() == _array.pop(bx::kFirst) );
						ref.erase(ref.begin() );
					}
					else
					{
						REQUIRE(ref.back() == _array.pop(bx::kLast) );
						ref.pop_back();
					}
				}
				break;

			case 11:
				{
					REQUIRE(lower == bx::lowerBound(value, _array) );
					REQUIRE(upper == bx::upperBound(value, _array) );

					const int32_t idx = bx::binarySearch(value, _array);

					if (lower == upper)
					{
						REQUIRE(int32_t(~lower) == idx);
					}
					else
					{
						REQUIRE(int32_t(lower) <= idx);
						REQUIRE(int32_t(upper) >  idx);
					}
				}
				break;

			case 12:
				if (0 != count)
				{
					const uint32_t pos = rng.gen() % count;
					const uint32_t num = rng.gen() % (count - pos + 1);
					_array.set(bx::ArrayViewT<Ty>(&_array[pos], num) );
					ref = std::vector<Ty>(ref.begin() + pos, ref.begin() + pos + num);
				}
				break;

			default:
				if (64 < count)
				{
					ArrayT copy(_array);
					_array = bx::move(copy);
					_array.trim();
				}
				break;
			}

			REQUIRE(ref.size() == _array.getCount() );

			for (uint32_t ii = 0, num = _array.getCount(); ii < num; ++ii)
			{
				REQUIRE(ref[ii] == _array[ii]);
			}
		}
	}

} // namespace

TEST_CASE("SortedArrayT std::string elements against model", "[container]")
{
	bx::CrtAllocator allocator;

	bx::SortedArrayT<std::string, compareFirstChar> array(allocator.getHandle() );
	testSortedArrayVsVector<std::string>(array, UINT32_MAX);

	bx::FixedSortedArrayT<std::string, 96, compareFirstChar> fixed;
	testSortedArrayVsVector<std::string>(fixed, 96);
}

TEST_CASE("SortedArrayT trivially copyable elements against model", "[container]")
{
	bx::CrtAllocator allocator;

	bx::SortedArrayT<KeyTag, compareKey> array(allocator.getHandle() );
	testSortedArrayVsVector<KeyTag>(array, UINT32_MAX);

	bx::FixedSortedArrayT<KeyTag, 96, compareKey> fixed;
	testSortedArrayVsVector<KeyTag>(fixed, 96);
}

TEST_CASE("SortedArrayT trivially relocatable elements against model", "[container]")
{
	bx::CrtAllocator allocator;

	{
		bx::SortedArrayT<Reloc, compareRelocKey> array(allocator.getHandle() );
		testSortedArrayVsVector<Reloc>(array, UINT32_MAX);
	}

	REQUIRE(0 == Reloc::s_live);
}
