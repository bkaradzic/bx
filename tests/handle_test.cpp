/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/handlealloc.h>
#include <bx/hash.h>
#include <bx/rng.h>

#include <set>

TEST_CASE("HandleAllocT", "")
{
	constexpr int32_t kMax = 64;
	bx::HandleAllocT<kMax> alloc;

	REQUIRE(sizeof(alloc) == sizeof(uint16_t) * kMax * 2 + sizeof(bx::HandleAlloc) );

	for (uint16_t ii = 0; ii < kMax; ++ii)
	{
		REQUIRE(!alloc.isValid(ii) );
	}

	bx::RngMwc random;
	std::set<uint16_t> handleSet;

	int32_t count = 0;

	for (int32_t ii = 0; ii < 200000; ++ii)
	{
		const bool add = random.gen() % 2;

		if (add && count < kMax)
		{
			count++;
			uint16_t handle = alloc.alloc();
			handleSet.insert(handle);
		}
		else if (count > 0)
		{
			count--;

			const int32_t idx = rand() % handleSet.size();
			auto it = handleSet.begin();

			for (int32_t it_idx = 0; it_idx < idx; ++it_idx)
			{
				it++;
				REQUIRE(alloc.isValid(*it) );
			}

			uint16_t handleToRemove = *it;
			alloc.free(handleToRemove);

			REQUIRE(!alloc.isValid(handleToRemove) );

			handleSet.erase(it);
		}

		// Check if it's still correct
		for (auto it = handleSet.begin(); it != handleSet.end(); ++it)
		{
			REQUIRE(alloc.isValid(*it) );
		}
	}

	// Finally delete all
	for (auto it = handleSet.begin(); it != handleSet.end(); ++it)
	{
		REQUIRE(alloc.isValid(*it) );

		alloc.free(*it);
	}

	handleSet.clear();

	for (uint16_t ii = 0; ii < kMax; ++ii)
	{
		REQUIRE(!alloc.isValid(ii) );
	}
}

TEST_CASE("HandleListT", "")
{
	bx::HandleListT<32> list;

	list.pushBack(16);
	REQUIRE(list.getFront() == 16);
	REQUIRE(list.getBack()  == 16);

	list.pushFront(7);
	REQUIRE(list.getFront() ==  7);
	REQUIRE(list.getBack()  == 16);

	uint16_t expected0[] = { 15, 31, 7, 16, 17, 11, 13 };
	list.pushBack(17);
	list.pushBack(11);
	list.pushBack(13);
	list.pushFront(31);
	list.pushFront(15);
	uint16_t count = 0;
	for (uint16_t it = list.getFront(); it != UINT16_MAX; it = list.getNext(it), ++count)
	{
		REQUIRE(it == expected0[count]);
	}
	REQUIRE(count == BX_COUNTOF(expected0) );

	list.remove(17);
	list.remove(31);
	list.remove(16);
	list.pushBack(16);
	uint16_t expected1[] = { 15, 7, 11, 13, 16 };
	count = 0;
	for (uint16_t it = list.getFront(); it != UINT16_MAX; it = list.getNext(it), ++count)
	{
		REQUIRE(it == expected1[count]);
	}
	REQUIRE(count == BX_COUNTOF(expected1) );

	list.popBack();
	list.popFront();
	list.popBack();
	list.popBack();

	REQUIRE(list.getFront() ==  7);
	REQUIRE(list.getBack()  ==  7);

	list.popBack();
	REQUIRE(list.getFront() ==  UINT16_MAX);
	REQUIRE(list.getBack()  ==  UINT16_MAX);
}

TEST_CASE("HandleAllocLruT", "")
{
	bx::HandleAllocLruT<16> lru;

	uint16_t handle[4] =
	{
		lru.alloc(),
		lru.alloc(),
		lru.alloc(),
		lru.alloc(),
	};

	lru.touch(handle[1]);

	uint16_t expected0[] = { handle[1], handle[3], handle[2], handle[0] };
	uint16_t count = 0;
	for (uint16_t it = lru.getFront(); it != UINT16_MAX; it = lru.getNext(it), ++count)
	{
		REQUIRE(it == expected0[count]);
	}
}

TEST_CASE("HandleHashTable", "")
{
	typedef bx::HandleHashMapT<512> HashMap;

	HashMap hm;

	REQUIRE(512 == hm.getMaxCapacity() );

	bx::StringView sv0("test0");

	bool ok = hm.insert(bx::hash<bx::HashMurmur3>(sv0), 0);
	REQUIRE(ok);

	ok = hm.insert(bx::hash<bx::HashMurmur3>(sv0), 0);
	REQUIRE(!ok);
	REQUIRE(1 == hm.getNumElements() );

	bx::StringView sv1("test1");

	ok = hm.insert(bx::hash<bx::HashMurmur3>(sv1), 0);
	REQUIRE(ok);
	REQUIRE(2 == hm.getNumElements() );

	hm.removeByHandle(0);
	REQUIRE(0 == hm.getNumElements() );

	ok = hm.insert(bx::hash<bx::HashMurmur3>(sv0), 0);
	REQUIRE(ok);

	hm.removeByKey(bx::hash<bx::HashMurmur3>(sv0) );
	REQUIRE(0 == hm.getNumElements() );

	for (uint32_t ii = 0, num = hm.getMaxCapacity(); ii < num; ++ii)
	{
		ok = hm.insert(ii, uint16_t(ii) );
		REQUIRE(ok);
	}
}

TEST_CASE("HandleAlloc2", "[container]")
{
	constexpr uint16_t kMaxHandles = 1024;

	bx::HandleAlloc2T<kMaxHandles> handleAlloc;

	REQUIRE(0 == handleAlloc.getNumHandles() );
	REQUIRE(kMaxHandles == handleAlloc.getMaxHandles() );

	uint16_t handle = handleAlloc.alloc();
	REQUIRE(handleAlloc.isValid(handle) );
	REQUIRE(1 == handleAlloc.getNumHandles() );

	handleAlloc.free(handle);
	REQUIRE(0 == handleAlloc.getNumHandles() );

	for (uint32_t ii = 0; ii < kMaxHandles; ++ii)
	{
		handle = handleAlloc.alloc();
		REQUIRE(handleAlloc.isValid(handle) );
	}

	handle = handleAlloc.alloc();
	REQUIRE(bx::kInvalidHandle == handle);
	REQUIRE(!handleAlloc.isValid(handle) );
}

TEST_CASE("HandleAlloc2 - lowest free first", "[container]")
{
	bx::HandleAlloc2T<200> handleAlloc;

	for (uint32_t ii = 0; ii < 200; ++ii)
	{
		REQUIRE(ii == handleAlloc.alloc() );
	}

	handleAlloc.free(130);
	handleAlloc.free(3);
	handleAlloc.free(70);

	REQUIRE(197 == handleAlloc.getNumHandles() );
	REQUIRE(3   == handleAlloc.alloc() );
	REQUIRE(70  == handleAlloc.alloc() );
	REQUIRE(130 == handleAlloc.alloc() );
	REQUIRE(bx::kInvalidHandle == handleAlloc.alloc() );
}

TEST_CASE("HandleAlloc2 - reset", "[container]")
{
	constexpr uint16_t kMaxHandles = 64;

	bx::HandleAlloc2T<kMaxHandles> handleAlloc;

	for (uint32_t ii = 0; ii < kMaxHandles; ++ii)
	{
		handleAlloc.alloc();
	}

	REQUIRE(kMaxHandles == handleAlloc.getNumHandles() );

	handleAlloc.reset();

	REQUIRE(0 == handleAlloc.getNumHandles() );
	REQUIRE(kMaxHandles == handleAlloc.getMaxHandles() );

	uint16_t handle = handleAlloc.alloc();
	REQUIRE(0 == handle);
	REQUIRE(handleAlloc.isValid(handle) );
	REQUIRE(1 == handleAlloc.getNumHandles() );
}

TEST_CASE("HandleAlloc2 - free and realloc", "[container]")
{
	constexpr uint16_t kMaxHandles = 4;

	bx::HandleAlloc2T<kMaxHandles> handleAlloc;

	uint16_t handles[kMaxHandles];
	for (uint32_t ii = 0; ii < kMaxHandles; ++ii)
	{
		handles[ii] = handleAlloc.alloc();
	}

	REQUIRE(kMaxHandles == handleAlloc.getNumHandles() );

	handleAlloc.free(handles[1]);
	REQUIRE(kMaxHandles - 1 == handleAlloc.getNumHandles() );
	REQUIRE(!handleAlloc.isValid(handles[1]) );

	uint16_t reused = handleAlloc.alloc();
	REQUIRE(handles[1] == reused);
	REQUIRE(handleAlloc.isValid(reused) );
	REQUIRE(kMaxHandles == handleAlloc.getNumHandles() );
}

TEST_CASE("HandleAlloc2 - isValid out of range", "[container]")
{
	constexpr uint16_t kMaxHandles = 8;

	bx::HandleAlloc2T<kMaxHandles> handleAlloc;

	REQUIRE(!handleAlloc.isValid(0) );
	REQUIRE(!handleAlloc.isValid(kMaxHandles) );
	REQUIRE(!handleAlloc.isValid(kMaxHandles + 100) );
	REQUIRE(!handleAlloc.isValid(UINT16_MAX) );
}

TEST_CASE("HandleAlloc2 - double free", "[container]")
{
	constexpr uint16_t kMaxHandles = 8;

	bx::HandleAlloc2T<kMaxHandles> handleAlloc;

	uint16_t handle = handleAlloc.alloc();
	REQUIRE(1 == handleAlloc.getNumHandles() );

	handleAlloc.free(handle);
	REQUIRE(0 == handleAlloc.getNumHandles() );

	handleAlloc.free(handle);
	REQUIRE(0 == handleAlloc.getNumHandles() );
}

TEST_CASE("HandleAlloc2 - interleaved alloc free", "[container]")
{
	constexpr uint16_t kMaxHandles = 16;

	bx::HandleAlloc2T<kMaxHandles> handleAlloc;

	uint16_t handles[kMaxHandles / 2];
	for (uint32_t ii = 0; ii < kMaxHandles / 2; ++ii)
	{
		handles[ii] = handleAlloc.alloc();
		REQUIRE(handleAlloc.isValid(handles[ii]) );
	}

	REQUIRE(kMaxHandles / 2 == handleAlloc.getNumHandles() );

	for (uint32_t ii = 0; ii < kMaxHandles / 2; ii += 2)
	{
		handleAlloc.free(handles[ii]);
	}

	REQUIRE(kMaxHandles / 2 - kMaxHandles / 4 == handleAlloc.getNumHandles() );

	const uint32_t remaining = kMaxHandles - handleAlloc.getNumHandles();
	for (uint32_t ii = 0; ii < remaining; ++ii)
	{
		uint16_t handle = handleAlloc.alloc();
		REQUIRE(handleAlloc.isValid(handle) );
	}

	REQUIRE(kMaxHandles == handleAlloc.getNumHandles() );

	uint16_t overflow = handleAlloc.alloc();
	REQUIRE(!handleAlloc.isValid(overflow) );
}

TEST_CASE("HandleAlloc2 - iteration", "[container]")
{
	bx::HandleAlloc2T<300> handleAlloc;

	REQUIRE(bx::kInvalidHandle == handleAlloc.findFirst() );

	const uint16_t expected[] = { 0, 1, 63, 64, 127, 200, 299 };

	for (uint32_t ii = 0; ii < 300; ++ii)
	{
		handleAlloc.alloc();
	}

	for (uint32_t ii = 0; ii < 300; ++ii)
	{
		bool keep = false;
		for (uint32_t jj = 0; jj < BX_COUNTOF(expected); ++jj)
		{
			keep |= expected[jj] == ii;
		}

		if (!keep)
		{
			handleAlloc.free(uint16_t(ii) );
		}
	}

	REQUIRE(BX_COUNTOF(expected) == handleAlloc.getNumHandles() );

	uint32_t num = 0;
	for (uint16_t handle = handleAlloc.findFirst(); bx::kInvalidHandle != handle; handle = handleAlloc.findNext(handle) )
	{
		REQUIRE(num < BX_COUNTOF(expected) );
		REQUIRE(expected[num] == handle);
		++num;
	}

	REQUIRE(BX_COUNTOF(expected) == num);
	REQUIRE(bx::kInvalidHandle == handleAlloc.findNext(299) );
}

TEST_CASE("HandleAlloc2 - external storage", "[container]")
{
	bx::FixedMippedBitArrayT<100> bits;
	bits.setCount(100);

	bx::HandleAlloc2 handleAlloc(bits, 100);

	for (uint32_t ii = 0; ii < 100; ++ii)
	{
		REQUIRE(ii == handleAlloc.alloc() );
	}

	REQUIRE(bx::kInvalidHandle == handleAlloc.alloc() );
	REQUIRE(100 == handleAlloc.getNumHandles() );
	REQUIRE(99  == handleAlloc.findNext(98) );
	REQUIRE(bx::kInvalidHandle == handleAlloc.findNext(99) );
}

TEST_CASE("HandleAlloc2 - full word summary", "[container]")
{
	bx::HandleAlloc2T<4096> handleAlloc;

	for (uint32_t ii = 0; ii < 4096; ++ii)
	{
		REQUIRE(ii == handleAlloc.alloc() );
	}

	REQUIRE(bx::kInvalidHandle == handleAlloc.alloc() );

	handleAlloc.free(4095);
	handleAlloc.free(2048);
	handleAlloc.free(63);
	REQUIRE(4093 == handleAlloc.getNumHandles() );

	REQUIRE(63   == handleAlloc.alloc() );
	REQUIRE(2048 == handleAlloc.alloc() );
	REQUIRE(4095 == handleAlloc.alloc() );
	REQUIRE(bx::kInvalidHandle == handleAlloc.alloc() );

	for (uint32_t ii = 0; ii < 4096; ii += 64)
	{
		handleAlloc.free(uint16_t(ii + 7) );
	}

	for (uint32_t ii = 0; ii < 4096; ii += 64)
	{
		REQUIRE(ii + 7 == handleAlloc.alloc() );
	}

	REQUIRE(4096 == handleAlloc.getNumHandles() );
	REQUIRE(bx::kInvalidHandle == handleAlloc.alloc() );

	handleAlloc.reset();
	REQUIRE(0 == handleAlloc.alloc() );
}

TEST_CASE("HandleAlloc2 - odd sizes", "[container]")
{
	bx::HandleAlloc2T<65> small;

	for (uint32_t ii = 0; ii < 65; ++ii)
	{
		REQUIRE(ii == small.alloc() );
	}

	REQUIRE(bx::kInvalidHandle == small.alloc() );
	small.free(64);
	REQUIRE(64 == small.alloc() );
	REQUIRE(64 == small.findNext(63) );
	REQUIRE(bx::kInvalidHandle == small.findNext(64) );

	bx::HandleAlloc2T<4097> big;

	for (uint32_t ii = 0; ii < 4097; ++ii)
	{
		REQUIRE(ii == big.alloc() );
	}

	REQUIRE(bx::kInvalidHandle == big.alloc() );
	big.free(4096);
	big.free(0);
	REQUIRE(0    == big.alloc() );
	REQUIRE(4096 == big.alloc() );
	REQUIRE(bx::kInvalidHandle == big.alloc() );
}
