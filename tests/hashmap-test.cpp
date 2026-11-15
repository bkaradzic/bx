/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"

#include <bx/allocators.h>
#include <bx/hashmap.h>
#include <bx/rng.h>
#include <bx/stringpool.h>

#include <unordered_map>
#include <string>
#include <unordered_set>

namespace bx
{
	template<>
	inline uint32_t hashKey(const std::string& _key)
	{
		return hashKey(StringView(_key.c_str(), int32_t(_key.size() ) ) );
	}

} // namespace bx

uint32_t swizzleBits(uint32_t _x)
{
	const uint32_t x1   = ( ( (_x & 0x001) >> 0) << 5);
	const uint32_t x2   = ( ( (_x & 0x002) >> 1) << 8);
	const uint32_t x4   = ( ( (_x & 0x004) >> 2) << 3);
	const uint32_t x8   = ( ( (_x & 0x008) >> 3) << 1);
	const uint32_t x10  = ( ( (_x & 0x010) >> 4) << 0);
	const uint32_t x20  = ( ( (_x & 0x020) >> 5) << 2);
	const uint32_t x40  = ( ( (_x & 0x040) >> 6) << 7);
	const uint32_t x80  = ( ( (_x & 0x080) >> 7) << 9);
	const uint32_t x100 = ( ( (_x & 0x100) >> 8) << 4);
	const uint32_t x200 = ( ( (_x & 0x200) >> 9) << 6);

	return 0
		| x1
		| x2
		| x4
		| x8
		| x10
		| x20
		| x40
		| x80
		| x100
		| x200
		;
}

template<typename Ty>
void testForEach(const Ty& _container)
{
	DBG("testForEach %d", _container.getCount() );

	uint32_t count = 0;
	for (auto x : _container)
	{
		BX_UNUSED(x);
		count++;
	}

	REQUIRE(count == _container.getCount() );
}

TEST_CASE("HashMapT", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HashMapT<uint32_t, uint32_t> hashMap(allocator.getHandle(), 10);

	testForEach(hashMap);

	REQUIRE(hashMap.tryAdd(13, 89) );
	REQUIRE( (NULL != hashMap.find(13)) );
	REQUIRE(1 == hashMap.getCount() );

	testForEach(hashMap);

	REQUIRE(hashMap.tryAdd(89, 13) );
	REQUIRE( (NULL != hashMap.find(89)) );
	REQUIRE(2 == hashMap.getCount() );

	testForEach(hashMap);

	REQUIRE( (NULL != hashMap.find(13)) );

	REQUIRE(!hashMap.tryRemove(1) );
	REQUIRE(!hashMap.tryRemove(12) );
	REQUIRE(!hashMap.tryRemove(40) );
	REQUIRE(2 == hashMap.getCount() );

	REQUIRE(hashMap.tryRemove(13) );
	REQUIRE(1 == hashMap.getCount() );
	REQUIRE(!hashMap.tryRemove(13) );
	REQUIRE(1 == hashMap.getCount() );

	REQUIRE(hashMap.tryRemove(89) );
	REQUIRE(0 == hashMap.getCount() );
	REQUIRE(!hashMap.tryRemove(89) );
	REQUIRE(0 == hashMap.getCount() );

	for (uint32_t ii = 0; ii < 16; ++ii)
	{
		REQUIRE(hashMap.tryAdd(13+ii, 89+ii) );
		REQUIRE( (NULL != hashMap.find(13+ii)) );
		REQUIRE(89+ii == *hashMap.find(13+ii) );
	}
	REQUIRE(16 == hashMap.getCount() );

	testForEach(hashMap);

	for (auto x : hashMap)
	{
		DBG("%d: key %d, value %d", x.getSlot(), x.getKey(), x.getValue() );
	}

	for (uint32_t ii = 0; ii < 16; ++ii)
	{
		REQUIRE( (NULL != hashMap.find(13+ii)) );
		REQUIRE(89+ii == *hashMap.find(13+ii) );
	}
	REQUIRE(16 == hashMap.getCount() );

	for (uint32_t ii = 0; ii < 16; ++ii)
	{
		for (uint32_t jj = ii; jj < 16; ++jj)
		{
			REQUIRE( (NULL != hashMap.find(13+jj)) );
			REQUIRE(89+jj == *hashMap.find(13+jj) );
		}
		REQUIRE(hashMap.tryRemove(13+ii) );
	}
	REQUIRE(0 == hashMap.getCount() );

	hashMap.setCapacity(10);

	for (uint32_t ii = 0; ii < 1024; ++ii)
	{
		REQUIRE(hashMap.tryAdd(13+ii, 89+ii) );
		REQUIRE( (NULL != hashMap.find(13+ii)) );
		REQUIRE(89+ii == *hashMap.find(13+ii) );
	}
	REQUIRE(1024 == hashMap.getCount() );

	{
		const uint32_t oldCapacity = hashMap.getCapacity();
		hashMap.setCapacity(oldCapacity + 1);
		REQUIRE(oldCapacity != hashMap.getCapacity() );
	}

	for (uint32_t ii = 0; ii < 1024; ++ii)
	{
		const uint32_t mask = swizzleBits(ii);

		REQUIRE(hashMap.tryRemove(13+mask) );
	}
	REQUIRE(0 == hashMap.getCount() );

	DBG("Capacity: %d", hashMap.getCapacity() );

	hashMap.clear();
	REQUIRE(0 == hashMap.getCount() );

	DBG("Capacity: %d", hashMap.getCapacity() );
	hashMap.trim();

	DBG("Trim/Capacity: %d", hashMap.getCapacity() );

	uint32_t prev = 0;
	for (uint32_t ii = 1; ii < 4096; ++ii)
	{
		uint32_t curr = (~0u >> bx::countLeadingZeros(ii) ) + 1;
		uint32_t npot = bx::nextPow2(ii);
		if (prev != curr
		||  curr != npot)
		{
			BX_TRACE("%4d: %4d %4d", ii, curr, npot);
			prev = curr;
		}
	}
}

TEST_CASE("HashMapT small capacity auto-grow", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HashMapT<uint32_t, uint32_t> hashMap(allocator.getHandle(), 2);

	// Insert more entries than the initial small table can hold.
	// Before the calcGrowth fix, this would deadlock in findInternal
	// because all slots (including the sentinel) could be filled,
	// leaving no empty slot to terminate the probe loop.
	for (uint32_t ii = 0; ii < 64; ++ii)
	{
		REQUIRE(hashMap.tryAdd(ii, ii * 10) );
		REQUIRE( (NULL != hashMap.find(ii)) );
		REQUIRE(ii * 10 == *hashMap.find(ii) );
	}
	REQUIRE(64 == hashMap.getCount() );

	for (uint32_t ii = 0; ii < 64; ++ii)
	{
		REQUIRE( (NULL != hashMap.find(ii)) );
		REQUIRE(ii * 10 == *hashMap.find(ii) );
	}

	// Also verify clear + re-insert works at small capacity.
	hashMap.clear();
	REQUIRE(0 == hashMap.getCount() );

	for (uint32_t ii = 0; ii < 32; ++ii)
	{
		REQUIRE(hashMap.tryAdd(100 + ii, ii) );
	}
	REQUIRE(32 == hashMap.getCount() );
}

TEST_CASE("HashMapT tryAdd", "[container]")
{
	{
		bx::CrtAllocator allocator;
		bx::HashMapT<bx::StringLiteral, int32_t> hashMap(allocator.getHandle(), 0);

		hashMap.add("1389", 1389);
		REQUIRE(1 == hashMap.getCount() );
		REQUIRE( (NULL != hashMap.find("1389")) );
		REQUIRE(1389 == *hashMap.find("1389") );

		REQUIRE_ASSERTS(hashMap.add("1389", 9831) );
		hashMap.add("1389", 9831, true);
		REQUIRE(9831 == *hashMap.find("1389") );

		hashMap.add("1389", 1389, true);
		REQUIRE(1389 == *hashMap.find("1389") );

		hashMap.add("9831", 9831);
		REQUIRE(2 == hashMap.getCount() );
		REQUIRE( (NULL != hashMap.find("9831")) );
		REQUIRE(9831 == *hashMap.find("9831") );
	}
}

TEST_CASE("HashMapControlT slots", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HashMapControlT<uint32_t> control(allocator.getHandle(), 16);

	constexpr uint32_t kNum = 100;

	REQUIRE(0 == control.getSlotCapacity() );
	REQUIRE(bx::HashMapControlT<uint32_t>::kInvalidSlot == control.findSlot(0) );

	bool used[kNum] = {};

	for (uint32_t ii = 0; ii < kNum; ++ii)
	{
		uint32_t slot = UINT32_MAX;
		REQUIRE(control.tryAdd(ii*7, slot) );
		REQUIRE(slot == ii);
		REQUIRE(ii+1 == control.getSlotCapacity() );
		REQUIRE(ii+1 == control.getCount() );
		used[slot] = true;
	}

	for (uint32_t ii = 0; ii < kNum; ++ii)
	{
		REQUIRE(used[ii]);
		REQUIRE(ii == control.findSlot(ii*7) );

		uint32_t slot = UINT32_MAX;
		REQUIRE(!control.tryAdd(ii*7, slot) );
		REQUIRE(ii == slot);
	}

	// Removing keys releases their slots, and adding keys reuses released slots
	// before reserving new ones.
	uint32_t removedSlot = UINT32_MAX;
	REQUIRE(control.tryRemove(13*7, removedSlot) );
	REQUIRE(13 == removedSlot);
	REQUIRE(control.tryRemove(89*7, removedSlot) );
	REQUIRE(89 == removedSlot);
	REQUIRE(!control.tryRemove(13*7, removedSlot) );
	REQUIRE(!control.tryRemove(13*7) );
	REQUIRE(kNum-2 == control.getCount() );
	REQUIRE(kNum == control.getSlotCapacity() );

	REQUIRE(89 == control.add(1000) );
	REQUIRE(13 == control.add(1001) );
	REQUIRE(kNum == control.getSlotCapacity() );
	REQUIRE(kNum == control.add(1002) );
	REQUIRE(kNum+1 == control.getSlotCapacity() );

	REQUIRE(89 == control.remove(1000) );
	REQUIRE(89 == control.add(1003) );

	// Rehash keeps slots.
	control.setCapacity(control.getCapacity()*4);
	REQUIRE(89 == control.findSlot(1003) );
	REQUIRE(13 == control.findSlot(1001) );
	REQUIRE(kNum == control.findSlot(1002) );
	REQUIRE(kNum+1 == control.getSlotCapacity() );

	uint32_t numIter = 0;
	bool seen[kNum+1] = {};

	for (const auto it : control)
	{
		REQUIRE(it.getSlot() < control.getSlotCapacity() );
		REQUIRE(!seen[it.getSlot()]);
		REQUIRE(it.getSlot() == control.findSlot(it.getKey() ) );
		seen[it.getSlot()] = true;
		++numIter;
	}

	REQUIRE(numIter == control.getCount() );

	control.clear();
	REQUIRE(0 == control.getCount() );
	REQUIRE(0 == control.getSlotCapacity() );
	REQUIRE(0 == control.add(5) );
}

TEST_CASE("HashMapControlT uint16_t slots", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HashMapControlT<uint64_t, uint16_t> control(allocator.getHandle(), 0);

	static_assert(UINT16_MAX == bx::HashMapControlT<uint64_t, uint16_t>::kInvalidSlot);

	constexpr uint32_t kNum = 3000;

	for (uint32_t ii = 0; ii < kNum; ++ii)
	{
		const uint16_t slot = control.add(uint64_t(ii)*0x9e3779b97f4a7c15ull);
		REQUIRE(slot == ii);
	}

	for (uint32_t ii = 0; ii < kNum; ii += 2)
	{
		REQUIRE(ii == control.remove(uint64_t(ii)*0x9e3779b97f4a7c15ull) );
	}

	for (uint32_t ii = 0; ii < kNum/2; ++ii)
	{
		const uint16_t slot = control.add(uint64_t(kNum+ii) );
		REQUIRE(slot < kNum);
		REQUIRE(0 == (slot & 1) );
	}

	REQUIRE(kNum == control.getCount() );
	REQUIRE(kNum == control.getSlotCapacity() );
}

namespace
{
	struct Counted
	{
		static int32_t s_live;
		static int32_t s_moves;

		Counted(uint32_t _value)
			: m_value(_value)
			, m_self(this)
		{
			++s_live;
		}

		Counted(const Counted& _other)
			: m_value(_other.m_value)
			, m_self(this)
		{
			REQUIRE(&_other == _other.m_self);
			++s_live;
		}

		Counted(Counted&& _other)
			: m_value(_other.m_value)
			, m_self(this)
		{
			REQUIRE(&_other == _other.m_self);
			++s_live;
			++s_moves;
		}

		Counted& operator=(const Counted& _other)
		{
			m_value = _other.m_value;
			return *this;
		}

		Counted& operator=(Counted&& _other)
		{
			m_value = _other.m_value;
			return *this;
		}

		~Counted()
		{
			REQUIRE(this == m_self);
			--s_live;
			m_self = NULL;
		}

		uint32_t m_value;
		Counted* m_self;
	};

	int32_t Counted::s_live  = 0;
	int32_t Counted::s_moves = 0;

	static_assert(!bx::isTriviallyMoveConstructible<Counted>() );
	static_assert(!bx::isTriviallyDestructible<Counted>() );

} // namespace

template<typename MapT, bool ExpectMovesT>
void testNonTrivialValue()
{
	bx::CrtAllocator allocator;

	Counted::s_live  = 0;
	Counted::s_moves = 0;

	{
		MapT hashMap(allocator.getHandle(), 0);

		constexpr uint32_t kNum = 2000;

		for (uint32_t ii = 0; ii < kNum; ++ii)
		{
			hashMap.add(ii, Counted(ii*3) );
			REQUIRE(int32_t(ii+1) == Counted::s_live);
		}

		// Every add moves its value into storage once. Array value storage grew several times
		// and moved values again; block storage never moves values.
		REQUIRE( (int32_t(kNum) < Counted::s_moves) == ExpectMovesT);

		for (uint32_t ii = 0; ii < kNum; ++ii)
		{
			const Counted* value = hashMap.find(ii);
			REQUIRE( (NULL != value) );
			REQUIRE(ii*3 == value->m_value);
			REQUIRE(value == value->m_self);
		}

		for (uint32_t ii = 0; ii < kNum; ii += 3)
		{
			hashMap.remove(ii);
		}

		REQUIRE(int32_t(hashMap.getCount() ) == Counted::s_live);

		hashMap.add(0, Counted(7), true);
		hashMap.add(1, Counted(8), true);
		REQUIRE(7 == hashMap.find(0)->m_value);
		REQUIRE(8 == hashMap.find(1)->m_value);
		REQUIRE(int32_t(hashMap.getCount() ) == Counted::s_live);

		hashMap.trim();
		REQUIRE(int32_t(hashMap.getCount() ) == Counted::s_live);

		for (uint32_t ii = 1; ii < kNum; ++ii)
		{
			if (0 != ii % 3
			||  1 == ii)
			{
				const Counted* value = hashMap.find(ii);
				REQUIRE( (NULL != value) );
				REQUIRE( (1 == ii ? 8 : ii*3) == value->m_value);
			}
			else
			{
				REQUIRE( (NULL == hashMap.find(ii) ) );
			}
		}

		hashMap.clear();
		REQUIRE(0 == Counted::s_live);

		hashMap.add(1, Counted(1) );
		REQUIRE(1 == Counted::s_live);
	}

	REQUIRE(0 == Counted::s_live);
}

TEST_CASE("HashMapT non-trivial value", "[container]")
{
	testNonTrivialValue<bx::HashMapT<uint32_t, Counted>, true>();
	testNonTrivialValue<bx::BlockHashMapT<uint32_t, Counted, 64>, false>();
}

template<typename MapT>
void testRandomVsReference(MapT& hashMap)
{
	std::unordered_map<uint32_t, uint32_t> ref;

	bx::RngMwc rng;

	constexpr uint32_t kKeySpace = 4096;

	for (uint32_t round = 0; round < 64; ++round)
	{
		for (uint32_t op = 0; op < 512; ++op)
		{
			const uint32_t key   = rng.gen() % kKeySpace;
			const uint32_t value = rng.gen();

			switch (rng.gen() % 4)
			{
			case 0:
			case 1:
				{
					const bool added = hashMap.tryAdd(key, value, true);
					REQUIRE(added);
					ref[key] = value;
				}
				break;

			case 2:
				{
					const bool removed = hashMap.tryRemove(key);
					REQUIRE(removed == (0 != ref.erase(key) ) );
				}
				break;

			default:
				{
					const bool added = hashMap.tryAdd(key, value);
					REQUIRE(added == (ref.end() == ref.find(key) ) );
					ref.emplace(key, value);
				}
				break;
			}
		}

		switch (round % 8)
		{
		case 3: hashMap.setCapacity(hashMap.getCount()*3); break;
		case 5: hashMap.trim(); break;
		case 7: hashMap.clear(); ref.clear(); break;
		default: break;
		}

		REQUIRE(hashMap.getCount() == ref.size() );
		REQUIRE(hashMap.getControl().getSlotCapacity() <= 2*kKeySpace);

		for (const auto& kv : ref)
		{
			const uint32_t* value = hashMap.find(kv.first);
			REQUIRE( (NULL != value) );
			REQUIRE(kv.second == *value);
		}

		bool seen[kKeySpace*2] = {};
		uint32_t numIter = 0;

		for (const auto it : hashMap)
		{
			REQUIRE(it.getSlot() < hashMap.getControl().getSlotCapacity() );
			REQUIRE(!seen[it.getSlot()]);
			seen[it.getSlot()] = true;

			const auto kv = ref.find(it.getKey() );
			REQUIRE(ref.end() != kv);
			REQUIRE(kv->second == it.getValue() );
			REQUIRE(&it.getValue() == &hashMap.getValue(it.getSlot() ) );
			++numIter;
		}

		REQUIRE(numIter == hashMap.getCount() );
	}
}

TEST_CASE("HashMapT random vs reference", "[container]")
{
	bx::CrtAllocator allocator;

	{
		bx::HashMapT<uint32_t, uint32_t, uint16_t, 0, bx::HashMapGroupRef> hashMap(allocator.getHandle(), 0);
		testRandomVsReference(hashMap);
	}

	{
		bx::HashMapT<uint32_t, uint32_t, uint16_t, 0, bx::HashMapGroupSimd> hashMap(allocator.getHandle(), 0);
		testRandomVsReference(hashMap);
	}

	{
		bx::HashMapT<uint32_t, uint32_t, uint16_t, 0, bx::HashMapGroupSimd256> hashMap(allocator.getHandle(), 0);
		testRandomVsReference(hashMap);
	}

	{
		bx::BlockHashMapT<uint32_t, uint32_t, 64, uint16_t> hashMap(allocator.getHandle(), 0);
		testRandomVsReference(hashMap);
	}

	{
		// Key space fits, so the fixed map never fills up.
		bx::FixedHashMapT<uint32_t, uint32_t, 4096, uint16_t> hashMap;
		testRandomVsReference(hashMap);
	}
}

TEST_CASE("FixedHashMapT", "[container]")
{
	constexpr uint32_t kCapacity = 100;

	bx::FixedHashMapT<uint32_t, uint32_t, kCapacity> hashMap;

	REQUIRE(0 == hashMap.getCount() );

	for (uint32_t ii = 0; ii < kCapacity; ++ii)
	{
		REQUIRE(hashMap.tryAdd(ii, ii*2) );
	}

	REQUIRE(kCapacity == hashMap.getCount() );
	REQUIRE(!hashMap.tryAdd(1000, 0) );
	REQUIRE_ASSERTS(hashMap.add(1000, 0) );
	REQUIRE(kCapacity == hashMap.getCount() );

	// Updates and lookups work on a full map.
	REQUIRE(hashMap.tryAdd(5, 7, true) );
	REQUIRE(7 == *hashMap.find(5) );
	REQUIRE( (NULL == hashMap.find(1000) ) );

	REQUIRE(hashMap.tryRemove(5) );
	REQUIRE(hashMap.tryAdd(1000, 9) );
	REQUIRE(!hashMap.tryAdd(5, 5) );
	REQUIRE(9 == *hashMap.find(1000) );

	uint32_t numIter = 0;

	for (const auto it : hashMap)
	{
		REQUIRE(it.getSlot() < kCapacity);
		REQUIRE( (it.getKey()*2 == it.getValue() || 1000 == it.getKey() ) );
		++numIter;
	}

	REQUIRE(kCapacity == numIter);

	hashMap.setCapacity(kCapacity*4);
	hashMap.trim();
	REQUIRE(kCapacity == hashMap.getCount() );
	REQUIRE(9 == *hashMap.find(1000) );

	hashMap.clear();
	REQUIRE(0 == hashMap.getCount() );
	REQUIRE(hashMap.tryAdd(1, 1) );

	Counted::s_live = 0;

	{
		bx::FixedHashMapT<uint32_t, Counted, 64> counted;

		for (uint32_t ii = 0; ii < 64; ++ii)
		{
			REQUIRE(counted.tryAdd(ii, Counted(ii) ) );
		}

		REQUIRE(!counted.tryAdd(64, Counted(64) ) );
		REQUIRE(64 == Counted::s_live);

		counted.remove(3);
		REQUIRE(63 == Counted::s_live);
	}

	REQUIRE(0 == Counted::s_live);
}

TEST_CASE("FixedHashSetT", "[container]")
{
	bx::FixedHashSetT<uint64_t, 16> set;

	for (uint32_t ii = 0; ii < 16; ++ii)
	{
		REQUIRE(set.tryAdd(ii*0x9e3779b97f4a7c15ull) );
	}

	REQUIRE(!set.tryAdd(16*0x9e3779b97f4a7c15ull) );
	REQUIRE(16 == set.getCount() );
	REQUIRE(set.contains(3*0x9e3779b97f4a7c15ull) );
	REQUIRE(set.tryRemove(3*0x9e3779b97f4a7c15ull) );
	REQUIRE(set.tryAdd(16*0x9e3779b97f4a7c15ull) );
	REQUIRE(!set.contains(3*0x9e3779b97f4a7c15ull) );
	REQUIRE(16 == set.getCount() );
}

TEST_CASE("HashSetT", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HashSetT<uint64_t> set(allocator.getHandle(), 0);

	REQUIRE(0 == set.getCount() );
	REQUIRE(!set.contains(13) );

	REQUIRE(set.tryAdd(13) );
	REQUIRE(!set.tryAdd(13) );
	REQUIRE(set.contains(13) );
	REQUIRE(1 == set.getCount() );

	set.add(89);
	REQUIRE_ASSERTS(set.add(89) );
	REQUIRE(2 == set.getCount() );

	REQUIRE(!set.tryRemove(7) );
	REQUIRE(set.tryRemove(13) );
	REQUIRE(!set.contains(13) );
	REQUIRE(1 == set.getCount() );

	set.remove(89);
	REQUIRE_ASSERTS(set.remove(89) );
	REQUIRE(0 == set.getCount() );

	for (uint32_t ii = 0; ii < 5000; ++ii)
	{
		set.add(uint64_t(ii)*0x9e3779b97f4a7c15ull);
	}

	REQUIRE(5000 == set.getCount() );

	uint32_t numIter = 0;
	uint64_t sum     = 0;

	for (const auto it : set)
	{
		REQUIRE(set.contains(it.getKey() ) );
		sum += it.getKey() % 5000;
		++numIter;
	}

	REQUIRE(numIter == set.getCount() );
	BX_UNUSED(sum);

	set.trim();
	REQUIRE(5000 == set.getCount() );

	set.clear();
	REQUIRE(0 == set.getCount() );
	REQUIRE(!set.contains(0) );

	// A set keeps no slot indices: its block holds control bytes and keys only.
	{
		bx::HashMapControlT<uint64_t> control(allocator.getHandle(), 1000);
		bx::HashSetT<uint64_t>        sized(allocator.getHandle(), 1000);

		REQUIRE(control.getCapacity() == sized.getCapacity() );
		REQUIRE(control.getSizeInBytes() == sized.getControl().getSizeInBytes() + 2*control.getCapacity()*sizeof(uint32_t) );
	}
}

TEST_CASE("HashSetT random vs reference", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HashSetT<uint32_t> set(allocator.getHandle(), 0);
	std::unordered_set<uint32_t> ref;

	bx::RngMwc rng;

	constexpr uint32_t kKeySpace = 4096;

	for (uint32_t round = 0; round < 32; ++round)
	{
		for (uint32_t op = 0; op < 512; ++op)
		{
			const uint32_t key = rng.gen() % kKeySpace;

			if (0 == (rng.gen() & 1) )
			{
				REQUIRE(set.tryAdd(key) == ref.insert(key).second);
			}
			else
			{
				REQUIRE(set.tryRemove(key) == (0 != ref.erase(key) ) );
			}
		}

		REQUIRE(set.getCount() == ref.size() );

		for (uint32_t key : ref)
		{
			REQUIRE(set.contains(key) );
		}

		uint32_t numIter = 0;

		for (const auto it : set)
		{
			REQUIRE(ref.end() != ref.find(it.getKey() ) );
			++numIter;
		}

		REQUIRE(numIter == set.getCount() );
	}
}

TEST_CASE("HashMapT empty does not allocate", "[container]")
{
	bx::CrtAllocator allocator;

	bx::HashMapT<uint32_t, uint32_t> hashMap(allocator.getHandle(), 0);
	REQUIRE(hashMap.isEmpty() );
	REQUIRE(0 == hashMap.getCapacity() );
	REQUIRE(0 == hashMap.getControl().getSizeInBytes() );
	REQUIRE( (NULL == hashMap.find(13) ) );
	REQUIRE(hashMap.findSlot(13) == bx::HashMapT<uint32_t, uint32_t>::kInvalidSlot);
	REQUIRE(!hashMap.tryRemove(13) );
	testForEach(hashMap);

	hashMap.clear();
	hashMap.trim();
	REQUIRE(0 == hashMap.getCapacity() );

	hashMap.add(13, 31);
	REQUIRE(!hashMap.isEmpty() );
	REQUIRE(0 != hashMap.getCapacity() );
	REQUIRE(31 == *hashMap.find(13) );

	hashMap.remove(13);
	REQUIRE(hashMap.isEmpty() );
	hashMap.trim();
	REQUIRE(0 == hashMap.getCapacity() );
	REQUIRE(0 == hashMap.getControl().getSizeInBytes() );

	bx::HashSetT<uint32_t> set(allocator.getHandle(), 0);
	REQUIRE(set.isEmpty() );
	REQUIRE(0 == set.getCapacity() );
	REQUIRE(!set.contains(7) );
	REQUIRE(!set.tryRemove(7) );
	testForEach(set);

	set.add(7);
	REQUIRE(set.contains(7) );
	REQUIRE(!set.isEmpty() );
}

template<typename MapT>
static void testMoveMap(bx::AllocatorHandle _allocator)
{
	constexpr uint32_t kNum = 300;

	Counted::s_live = 0;

	{
		MapT src(_allocator, 0);

		for (uint32_t ii = 0; ii < kNum; ++ii)
		{
			src.add(ii, bx::Emplace, ii*3);
		}

		REQUIRE(kNum == src.getCount() );
		REQUIRE(kNum == uint32_t(Counted::s_live) );

		MapT dst(bx::move(src) );
		REQUIRE(src.isEmpty() );
		REQUIRE(0 == src.getCapacity() );
		REQUIRE(kNum == dst.getCount() );
		REQUIRE(kNum == uint32_t(Counted::s_live) );

		for (uint32_t ii = 0; ii < kNum; ++ii)
		{
			REQUIRE(ii*3 == dst.find(ii)->m_value);
		}

		src.add(1000, bx::Emplace, 1u);
		REQUIRE(1 == src.getCount() );

		MapT other(_allocator, 0);

		for (uint32_t ii = 0; ii < 10; ++ii)
		{
			other.add(ii + 5000, bx::Emplace, ii);
		}

		REQUIRE(kNum + 11 == uint32_t(Counted::s_live) );

		other = bx::move(dst);
		REQUIRE(dst.isEmpty() );
		REQUIRE(kNum == other.getCount() );
		REQUIRE(kNum + 1 == uint32_t(Counted::s_live) );
		REQUIRE( (NULL == other.find(5000) ) );

		bx::swap(src, other);
		REQUIRE(kNum == src.getCount() );
		REQUIRE(1 == other.getCount() );
		REQUIRE(1 == other.find(1000)->m_value);

		for (uint32_t ii = 0; ii < kNum; ++ii)
		{
			REQUIRE(ii*3 == src.find(ii)->m_value);
		}

		dst.add(7, bx::Emplace, 7u);
		REQUIRE(1 == dst.getCount() );
		REQUIRE(kNum + 2 == uint32_t(Counted::s_live) );
	}

	REQUIRE(0 == Counted::s_live);
}

TEST_CASE("HashMapT move and swap", "[container]")
{
	bx::CrtAllocator allocator;

	testMoveMap<bx::HashMapT<uint32_t, Counted> >(allocator.getHandle() );
	testMoveMap<bx::BlockHashMapT<uint32_t, Counted, 64> >(allocator.getHandle() );
}

TEST_CASE("HashSetT move and swap", "[container]")
{
	bx::CrtAllocator allocator;

	bx::HashSetT<uint32_t> src(allocator.getHandle(), 0);

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		src.add(ii);
	}

	bx::HashSetT<uint32_t> dst(bx::move(src) );
	REQUIRE(src.isEmpty() );
	REQUIRE(0 == src.getCapacity() );
	REQUIRE(1000 == dst.getCount() );

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		REQUIRE(dst.contains(ii) );
	}

	src.add(5000);
	bx::swap(src, dst);
	REQUIRE(1000 == src.getCount() );
	REQUIRE(1 == dst.getCount() );
	REQUIRE(dst.contains(5000) );

	dst = bx::move(src);
	REQUIRE(src.isEmpty() );
	REQUIRE(1000 == dst.getCount() );
	REQUIRE(!dst.contains(5000) );
}

TEST_CASE("HashMapT emplace", "[container]")
{
	bx::CrtAllocator allocator;

	Counted::s_live = 0;

	{
		bx::HashMapT<uint32_t, Counted> hashMap(allocator.getHandle(), 0);

		REQUIRE(hashMap.tryAdd(1, bx::Emplace, 10u) );
		REQUIRE(!hashMap.tryAdd(1, bx::Emplace, 11u) );
		REQUIRE(1 == Counted::s_live);
		REQUIRE(10 == hashMap.find(1)->m_value);

		hashMap.add(2, bx::Emplace, 20u);
		REQUIRE_ASSERTS(hashMap.add(2, bx::Emplace, 21u) );
		REQUIRE(2 == Counted::s_live);
		REQUIRE(20 == hashMap.find(2)->m_value);
	}

	REQUIRE(0 == Counted::s_live);
}

TEST_CASE("HashMapT remove by iterator", "[container]")
{
	bx::CrtAllocator allocator;

	Counted::s_live = 0;

	{
		bx::HashMapT<uint32_t, Counted> hashMap(allocator.getHandle(), 0);

		for (uint32_t ii = 0; ii < 1000; ++ii)
		{
			hashMap.add(ii, bx::Emplace, ii);
		}

		uint32_t numRemoved = 0;

		for (const auto it : hashMap)
		{
			if (1 == (it.getKey() & 1) )
			{
				hashMap.remove(it);
				++numRemoved;
			}
		}

		REQUIRE(500 == numRemoved);
		REQUIRE(500 == hashMap.getCount() );
		REQUIRE(500 == Counted::s_live);

		for (uint32_t ii = 0; ii < 1000; ++ii)
		{
			REQUIRE( (0 == (ii & 1) ) == (NULL != hashMap.find(ii) ) );
		}

		for (const auto it : hashMap)
		{
			REQUIRE(0 == (it.getKey() & 1) );
			REQUIRE(it.getKey() == it.getValue().m_value);
		}

		for (uint32_t ii = 1; ii < 1000; ii += 2)
		{
			hashMap.add(ii, bx::Emplace, ii);
		}

		REQUIRE(1000 == hashMap.getCount() );
		REQUIRE(1000 == Counted::s_live);
	}

	REQUIRE(0 == Counted::s_live);

	bx::HashSetT<uint32_t> set(allocator.getHandle(), 0);

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		set.add(ii);
	}

	for (const auto it : set)
	{
		if (0 == (it.getKey() % 3) )
		{
			set.remove(it);
		}
	}

	REQUIRE(666 == set.getCount() );

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		REQUIRE( (0 != (ii % 3) ) == set.contains(ii) );
	}
}

TEST_CASE("HashMapT default allocator", "[container]")
{
	bx::HashMapT<uint32_t, uint32_t> hashMap;
	REQUIRE(bx::getDefaultAllocatorBlk() == bx::getAllocator(hashMap.getAllocatorHandle() ) );
	REQUIRE(0 == hashMap.getCount() );

	for (uint32_t ii = 0; ii < 100; ++ii)
	{
		REQUIRE(hashMap.tryAdd(ii, ii * 3) );
	}

	REQUIRE(100 == hashMap.getCount() );
	REQUIRE( (NULL != hashMap.find(42) ) );
	REQUIRE(126 == *hashMap.find(42) );

	bx::BlockHashMapT<uint32_t, uint32_t, 64> blockHashMap;
	REQUIRE(bx::getDefaultAllocatorBlk() == bx::getAllocator(blockHashMap.getAllocatorHandle() ) );

	for (uint32_t ii = 0; ii < 100; ++ii)
	{
		REQUIRE(blockHashMap.tryAdd(ii, ii * 3) );
	}

	REQUIRE(100 == blockHashMap.getCount() );
	REQUIRE(126 == *blockHashMap.find(42) );

	bx::HashSetT<uint64_t> set;
	REQUIRE(bx::getDefaultAllocatorBlk() == bx::getAllocator(set.getAllocatorHandle() ) );

	for (uint64_t ii = 0; ii < 100; ++ii)
	{
		set.add(ii);
	}

	REQUIRE(100 == set.getCount() );
	REQUIRE(set.contains(42) );
	REQUIRE(!set.contains(100) );
}

TEST_CASE("HashMapT getOrAdd", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HashMapT<uint32_t, uint32_t> hashMap(allocator.getHandle(), 0);

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		hashMap.getOrAdd(ii % 10) += 1;
	}

	REQUIRE(10 == hashMap.getCount() );

	for (uint32_t ii = 0; ii < 10; ++ii)
	{
		REQUIRE(100 == *hashMap.find(ii) );
	}

	REQUIRE(0 == hashMap.getOrAdd(1389) );
	REQUIRE(11 == hashMap.getCount() );

	for (auto& it : hashMap)
	{
		it.getValue() *= 2;
	}

	REQUIRE(200 == *hashMap.find(7) );
	REQUIRE(  0 == *hashMap.find(1389) );

	bx::FixedHashMapT<uint32_t, uint32_t, 16> fixed;
	fixed.getOrAdd(3) = 33;
	fixed.getOrAdd(3) += 1;
	REQUIRE(1 == fixed.getCount() );
	REQUIRE(34 == *fixed.find(3) );
}

TEST_CASE("HashMapT add own value", "[container]")
{
	bx::CrtAllocator allocator;
	{
		bx::HashMapT<uint32_t, Counted> hashMap(allocator.getHandle(), 0);

		hashMap.add(0, Counted(1389) );

		for (uint32_t ii = 1; ii < 1000; ++ii)
		{
			hashMap.add(ii, *hashMap.find(ii-1) );
		}

		for (uint32_t ii = 1000; ii < 2000; ++ii)
		{
			hashMap.add(ii, bx::move(*hashMap.find(ii-1000) ) );
		}

		for (uint32_t ii = 0; ii < 2000; ++ii)
		{
			REQUIRE(1389 == hashMap.find(ii)->m_value);
		}
	}

	REQUIRE(0 == Counted::s_live);
}

TEST_CASE("HashMapT string handle key", "[container]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	struct Entry
	{
		bx::String name;
		uint32_t   id;
	};

	bx::HashMapT<uint32_t, Entry> ids(allocator.getHandle(), 0);
	const uint32_t poolCount = pool.getCount();

	const char* names[] = { "float4", "uint", "float4", "sampler", "uint", "float4" };
	uint32_t result[BX_COUNTOF(names)];

	for (uint32_t ii = 0; ii < BX_COUNTOF(names); ++ii)
	{
		bx::String name = pool.intern(names[ii]);

		Entry& entry = ids.getOrAdd(name.getHandle() );
		if (entry.name.isEmpty() )
		{
			entry.name = name;
			entry.id   = ids.getCount() - 1;
		}

		result[ii] = entry.id;
	}

	REQUIRE(3 == ids.getCount() );
	REQUIRE(poolCount + 3 == pool.getCount() );
	REQUIRE(0 == result[0]);
	REQUIRE(1 == result[1]);
	REQUIRE(0 == result[2]);
	REQUIRE(2 == result[3]);
	REQUIRE(1 == result[4]);
	REQUIRE(0 == result[5]);

	const Entry* entry = ids.find(pool.intern("sampler").getHandle() );
	REQUIRE( (NULL != entry) );
	REQUIRE(entry->name == bx::StringView("sampler") );

	ids.clear();
	REQUIRE(3 == pool.trim() );
	REQUIRE(poolCount == pool.getCount() );
}

TEST_CASE("HashMapT String key", "[container]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);
	bx::StringPool other(allocator.getHandle(), 0);

	const uint32_t poolCount = pool.getCount();

	{
		bx::HashMapT<bx::String, uint32_t> ids(allocator.getHandle(), 0);
		char tmp[32];

		for (uint32_t ii = 0; ii < 1000; ++ii)
		{
			bx::snprintf(tmp, sizeof(tmp), "name_%u", ii);
			ids.add(pool.intern(tmp), ii);
		}

		REQUIRE(1000 == ids.getCount() );
		REQUIRE(0 == pool.trim() );

		for (uint32_t ii = 0; ii < 1000; ++ii)
		{
			bx::snprintf(tmp, sizeof(tmp), "name_%u", ii);
			REQUIRE(ii == *ids.find(pool.intern(tmp) ) );
			REQUIRE(ii == *ids.find(other.intern(tmp) ) );
		}

		for (uint32_t ii = 0; ii < 1000; ii += 2)
		{
			bx::snprintf(tmp, sizeof(tmp), "name_%u", ii);
			ids.remove(pool.intern(tmp) );
		}

		REQUIRE(500 == pool.trim() );

		for (auto it = ids.begin(), itEnd = ids.end(); it != itEnd; ++it)
		{
			if (0 == it.getValue() % 3)
			{
				ids.remove(it);
			}
		}

		REQUIRE(167 == pool.trim() );

		bx::HashMapT<bx::String, uint32_t> moved(allocator.getHandle(), 0);
		moved.add(pool.intern("moved"), 1389);
		moved = bx::move(ids);
		REQUIRE(1 == pool.trim() );

		bx::HashSetT<bx::String> set(allocator.getHandle(), 0);
		set.add(pool.intern("set") );
		REQUIRE(set.contains(other.intern("set") ) );
		set.clear();
		REQUIRE(1 == pool.trim() );
	}

	REQUIRE(333 == pool.trim() );
	REQUIRE(poolCount == pool.getCount() );
}

TEST_CASE("HashMapT iterators and allocator", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HashMapT<uint32_t, uint32_t> hashMap(allocator.getHandle() );

	REQUIRE(allocator.getHandle() == hashMap.getAllocatorHandle() );
	REQUIRE(allocator.getHandle().idx == hashMap.getAllocatorHandle().idx);

	for (uint32_t ii = 0; ii < 16; ++ii)
	{
		hashMap.add(ii, ii);
	}

	for (auto& it : hashMap)
	{
		it.getValue() += 100;
	}

	const bx::HashMapT<uint32_t, uint32_t>& constMap = hashMap;
	uint32_t count = 0;

	for (const auto& it : constMap)
	{
		REQUIRE(it.getKey() + 100 == it.getValue() );
		REQUIRE(&it.getValue() == &constMap.getValue(it.getSlot() ) );
		++count;
	}

	REQUIRE(16 == count);

	bx::HashSetT<uint32_t> hashSet(allocator.getHandle() );
	REQUIRE(allocator.getHandle().idx == hashSet.getAllocatorHandle().idx);

	bx::BlockHashMapT<uint32_t, uint32_t> blockMap(allocator.getHandle() );
	REQUIRE(allocator.getHandle().idx == blockMap.getAllocatorHandle().idx);

	bx::HashMapControlT<uint32_t> control(allocator.getHandle() );
	REQUIRE(allocator.getHandle().idx == control.getAllocatorHandle().idx);
}

TEST_CASE("HashMapT std::string value", "[container]")
{
	bx::CrtAllocator allocator;

	bx::HashMapT<uint32_t, std::string> map(allocator.getHandle() );

	for (uint32_t ii = 0; ii < 500; ++ii)
	{
		map.add(ii, std::to_string(ii) );
	}

	const std::string value("updated");
	map.add(7, value, true);
	map.add(8, std::string("moved"), true);
	map.remove(9);

	REQUIRE(499 == map.getCount() );
	REQUIRE( (NULL == map.find(9)) );
	REQUIRE(value == *map.find(7) );
	REQUIRE("moved" == *map.find(8) );

	for (uint32_t ii = 10; ii < 500; ++ii)
	{
		REQUIRE(std::to_string(ii) == *map.find(ii) );
	}
}

TEST_CASE("HashMapT std::string key", "[container]")
{
	bx::CrtAllocator allocator;

	bx::HashMapT<std::string, std::string> map(allocator.getHandle() );
	std::unordered_map<std::string, std::string> ref;

	bx::RngMwc rng;

	for (uint32_t op = 0; op < 20000; ++op)
	{
		const uint32_t id = rng.gen() % 1024;

		const std::string key = 0 == id%3
			? std::to_string(id) + std::string(32, 'k')
			: std::to_string(id)
			;

		if (0 == rng.gen()%3)
		{
			map.tryRemove(key);
			ref.erase(key);
		}
		else
		{
			const std::string value = std::to_string(op);
			map.add(key, value, true);
			ref[key] = value;
		}
	}

	REQUIRE(ref.size() == map.getCount() );

	for (const auto& it : ref)
	{
		const std::string* value = map.find(it.first);
		REQUIRE( (NULL != value) );
		REQUIRE(it.second == *value);
	}

	bx::HashMapT<std::string, std::string> moved(allocator.getHandle() );
	moved.add("short", "value");
	moved = bx::move(map);
	REQUIRE(ref.size() == moved.getCount() );
	REQUIRE( (NULL == moved.find("short")) );

	bx::HashSetT<std::string> set(allocator.getHandle() );

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		set.add(std::to_string(ii) );
	}

	for (uint32_t ii = 0; ii < 1000; ii += 2)
	{
		set.remove(std::to_string(ii) );
	}

	REQUIRE(500 == set.getCount() );
	REQUIRE(!set.contains("998") );
	REQUIRE(set.contains("999") );
}
