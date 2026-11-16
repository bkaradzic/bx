/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "test.h"

#include <bx/allocators.h>
#include <bx/array.h>
#include <bx/handletable.h>
#include <bx/blockstorage.h>
#include <bx/rng.h>

#include <map>
#include <string>
#include <vector>

TEST_CASE("HandleTableT basic operations", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<int32_t, 64> array(allocator.getHandle(), 1);

	REQUIRE(array.getCapacity() >= 64);
	REQUIRE(0 == array.getCount() );
	REQUIRE(array.isEmpty() );
	REQUIRE(!array.isUsed(0) );

	const uint32_t idx1 = array.add(11);
	const uint32_t idx2 = array.add(22);

	REQUIRE(idx1 > 0);
	REQUIRE(idx2 > 0);
	REQUIRE(idx1 != idx2);
	REQUIRE(2 == array.getCount() );
	REQUIRE(!array.isEmpty() );
	REQUIRE(array.isUsed(idx1) );
	REQUIRE(array.isUsed(idx2) );
	REQUIRE(11 == array[idx1]);
	REQUIRE(22 == array[idx2]);

	array.remove(idx1);
	REQUIRE(!array.isUsed(idx1) );
	REQUIRE(1 == array.getCount() );

	const uint32_t idx3 = array.add(33);
	REQUIRE(idx3 > 0);
	REQUIRE(array.isUsed(idx3) );
	REQUIRE(2 == array.getCount() );
	REQUIRE(33 == array[idx3]);

	const int32_t popped = array.pop(idx2);
	REQUIRE(22 == popped);
	REQUIRE(!array.isUsed(idx2) );
	REQUIRE(1 == array.getCount() );
}

TEST_CASE("HandleTableT grows by block", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<uint32_t, 64> array(allocator.getHandle(), 64);
	bx::FixedArrayT<uint32_t, 96> indices;

	for (uint32_t ii = 0; ii < 80; ++ii)
	{
		const uint32_t idx = array.add(ii + 100);
		indices.add(idx);
	}

	REQUIRE(80 == array.getCount() );
	REQUIRE(array.getCapacity() >= 128);

	for (uint32_t ii = 0, count = indices.getCount(); ii < count; ++ii)
	{
		const uint32_t idx = indices[ii];
		REQUIRE(array.isUsed(idx) );
		REQUIRE(array[idx] == ii + 100);
	}
}

TEST_CASE("HandleTableT reallocate released middle block", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<uint32_t, 64> array(allocator.getHandle(), 192);
	bx::FixedArrayT<uint32_t, 256> indices;

	for (uint32_t ii = 0; ii < 150; ++ii)
	{
		indices.add(array.add(ii + 1) );
	}

	for (uint32_t ii = 0, count = indices.getCount(); ii < count; ++ii)
	{
		const uint32_t idx = indices[ii];
		if (idx >= 64 && idx < 128)
		{
			array.remove(idx);
		}
	}

	for (uint32_t ii = 151; ii < 192; ++ii)
	{
		const uint32_t idx = array.add(ii);
		REQUIRE(idx >= 128);
		REQUIRE(idx < 192);
	}

	const uint32_t reusedIdx = array.add(9999);
	REQUIRE(reusedIdx >= 64);
	REQUIRE(reusedIdx < 128);
	REQUIRE(array[reusedIdx] == 9999);
	REQUIRE(array.getCapacity() >= 192);
}

TEST_CASE("HandleTableT shrink when last block released", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<uint32_t, 64> array(allocator.getHandle(), 64);
	bx::FixedArrayT<uint32_t, 192> indices;

	for (uint32_t ii = 0; ii < 191; ++ii)
	{
		indices.add(array.add(ii + 10) );
	}

	REQUIRE(192 == array.getCapacity() );

	array.remove(indices[10]);
	REQUIRE(indices[10] == array.add(20) );

	for (uint32_t ii = 0, count = indices.getCount(); ii < count; ++ii)
	{
		const uint32_t idx = indices[ii];
		if (idx >= 128)
		{
			array.remove(idx);
		}
	}

	REQUIRE(128 == array.getCapacity() );

	for (uint32_t ii = 0, count = indices.getCount(); ii < count; ++ii)
	{
		const uint32_t idx = indices[ii];
		if (idx >= 64 && idx < 128)
		{
			array.remove(idx);
		}
	}

	REQUIRE(64 == array.getCapacity() );
}

TEST_CASE("HandleTableT keeps current block when empty", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<uint32_t, 64> array(allocator.getHandle(), 64);
	bx::FixedArrayT<uint32_t, 128> indices;

	for (uint32_t ii = 0; ii < 80; ++ii)
	{
		indices.add(array.add(ii + 10) );
	}

	for (uint32_t ii = 0, count = indices.getCount(); ii < count; ++ii)
	{
		const uint32_t idx = indices[ii];
		if (idx >= 64)
		{
			array.remove(idx);
		}
	}

	REQUIRE(128 == array.getCapacity() );

	const uint32_t idx = array.add(99);
	REQUIRE(idx >= 64);
	REQUIRE(idx < 128);
}

TEST_CASE("HandleTableT reuses lowest block after current block fills", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<int32_t, 64> array(allocator.getHandle(), 1);

	uint32_t idx[200];

	for (uint32_t ii = 0; ii < BX_COUNTOF(idx); ++ii)
	{
		idx[ii] = array.add(int32_t(ii) );
		REQUIRE(ii + 1 == idx[ii]);
	}

	array.remove(idx[150]);
	array.remove(idx[10]);
	array.remove(idx[70]);

	for (uint32_t ii = BX_COUNTOF(idx) + 1; ii < 256; ++ii)
	{
		REQUIRE(ii == array.add(int32_t(ii) ) );
	}

	REQUIRE(idx[10]  == array.add(10) );
	REQUIRE(idx[70]  == array.add(70) );
	REQUIRE(idx[150] == array.add(150) );
	REQUIRE(256 == array.add(256) );

	for (uint32_t ii = 0; ii < BX_COUNTOF(idx); ++ii)
	{
		REQUIRE(int32_t(ii) == array[idx[ii] ]);
	}
}

namespace
{
	struct Tracked
	{
		static int32_t s_numDefault;
		static int32_t s_numCopy;
		static int32_t s_numMove;
		static int32_t s_numAssign;
		static int32_t s_numLive;

		Tracked()                      : value(0)            { ++s_numDefault; ++s_numLive; }
		Tracked(int32_t _value)        : value(_value)       { ++s_numLive; }
		Tracked(const Tracked& _other) : value(_other.value) { ++s_numCopy;    ++s_numLive; }
		Tracked(Tracked&& _other)      : value(_other.value) { ++s_numMove;    ++s_numLive; _other.value = -1; }
		~Tracked()                                           { --s_numLive; }

		Tracked& operator=(const Tracked& _other) { ++s_numAssign; value = _other.value; return *this; }
		Tracked& operator=(Tracked&& _other)      { ++s_numAssign; value = _other.value; return *this; }

		static void reset()
		{
			s_numDefault = 0;
			s_numCopy    = 0;
			s_numMove    = 0;
			s_numAssign  = 0;
			s_numLive    = 0;
		}

		int32_t value;
	};

	int32_t Tracked::s_numDefault;
	int32_t Tracked::s_numCopy;
	int32_t Tracked::s_numMove;
	int32_t Tracked::s_numAssign;
	int32_t Tracked::s_numLive;

} // namespace

TEST_CASE("HandleTableT constructs elements in place", "[container]")
{
	bx::CrtAllocator crt;

	Tracked::reset();

	{
		bx::HandleTableT<Tracked, 64> array(crt.getHandle(), 64);
		REQUIRE(0 == Tracked::s_numLive);
		REQUIRE(0 == Tracked::s_numDefault);

		const uint32_t idx0 = array.add(Tracked(11) );
		REQUIRE(1 == Tracked::s_numMove);
		REQUIRE(1 == Tracked::s_numLive);

		const Tracked src(22);
		const uint32_t idx1 = array.add(src);
		REQUIRE(1 == Tracked::s_numCopy);
		REQUIRE(3 == Tracked::s_numLive);

		REQUIRE(11 == array[idx0].value);
		REQUIRE(22 == array[idx1].value);

		array.remove(idx0);
		REQUIRE(2 == Tracked::s_numLive);

		uint32_t idx[100];
		for (uint32_t ii = 0; ii < BX_COUNTOF(idx); ++ii)
		{
			idx[ii] = array.add(Tracked(int32_t(ii) ) );
		}

		REQUIRE(102 == Tracked::s_numLive);
		REQUIRE(0 == Tracked::s_numDefault);
		REQUIRE(0 == Tracked::s_numAssign);

		for (uint32_t ii = 0; ii < BX_COUNTOF(idx); ++ii)
		{
			REQUIRE(int32_t(ii) == array[idx[ii] ].value);
		}
	}

	// Destructor destroys elements that are left.
	REQUIRE(0 == Tracked::s_numLive);
}

TEST_CASE("BlockStorageT allocates blocks on demand", "[container]")
{
	bx::CrtAllocator allocator;
	bx::BlockStorageT<uint64_t, 64> storage;
	storage.setAllocatorHandle(allocator.getHandle() );

	REQUIRE(0 == storage.getCapacity() );
	REQUIRE(0 == storage.getNumBlocks() );

	uint64_t* ptr = storage.acquirePtr(130);
	*ptr = 130;

	REQUIRE(192 == storage.getCapacity() );
	REQUIRE(3 == storage.getNumBlocks() );
	REQUIRE(!storage.isBlockAllocated(0) );
	REQUIRE(!storage.isBlockAllocated(1) );
	REQUIRE(storage.isBlockAllocated(2) );
	REQUIRE(ptr == storage.getPtr(130) );
	REQUIRE(130 == *storage.getPtr(130) );

	storage.allocateBlock(0);
	REQUIRE(storage.isBlockAllocated(0) );
	*storage.getPtr(1) = 1;
	REQUIRE(ptr == storage.getPtr(130) );

	storage.setCapacity(64);
	REQUIRE(64 == storage.getCapacity() );
	REQUIRE(1 == storage.getNumBlocks() );
	REQUIRE(storage.isBlockAllocated(0) );
	REQUIRE(1 == *storage.getPtr(1) );

	storage.releaseBlock(0);
	REQUIRE(!storage.isBlockAllocated(0) );

	storage.setCapacity(0);
	REQUIRE(0 == storage.getNumBlocks() );
}

TEST_CASE("HandleTableT iterator", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<uint32_t, 64> array(allocator.getHandle(), 1);

	{
		uint32_t numVisited = 0;

		for (const auto it : array)
		{
			BX_UNUSED(it);
			++numVisited;
		}

		REQUIRE(0 == numVisited);
	}

	for (uint32_t ii = 0; ii < 200; ++ii)
	{
		const uint32_t idx = array.add(ii*10);
		REQUIRE(ii + 1 == idx);
	}

	{
		uint32_t numVisited = 0;
		uint32_t prevIdx    = 0;

		for (const auto it : array)
		{
			REQUIRE(prevIdx < it.getIndex() );
			REQUIRE( (it.getIndex() - 1)*10 == it.getValue() );
			prevIdx = it.getIndex();
			++numVisited;
		}

		REQUIRE(200 == numVisited);
	}

	for (const auto it : array)
	{
		it.getValue() += 1;
	}

	{
		const bx::HandleTableT<uint32_t, 64>& constArray = array;

		uint32_t numVisited = 0;

		for (const auto it : constArray)
		{
			REQUIRE( (it.getIndex() - 1)*10 + 1 == it.getValue() );
			++numVisited;
		}

		REQUIRE(200 == numVisited);
	}

	{
		uint32_t numVisited = 0;

		for (const auto it : array)
		{
			++numVisited;

			if (1 == (it.getIndex() & 1) )
			{
				array.remove(it);
			}
		}

		REQUIRE(200 == numVisited);
		REQUIRE(100 == array.getCount() );
	}

	{
		uint32_t numVisited = 0;

		for (const auto it : array)
		{
			REQUIRE(0 == (it.getIndex() & 1) );
			REQUIRE( (it.getIndex() - 1)*10 + 1 == it.getValue() );
			++numVisited;
		}

		REQUIRE(100 == numVisited);
	}

	{
		uint32_t numVisited = 0;

		for (const auto it : array)
		{
			++numVisited;
			array.remove(it);
		}

		REQUIRE(100 == numVisited);
		REQUIRE(0 == array.getCount() );
		REQUIRE(64 == array.getCapacity() );
	}

	{
		uint32_t numVisited = 0;

		for (const auto it : array)
		{
			BX_UNUSED(it);
			++numVisited;
		}

		REQUIRE(0 == numVisited);
	}
}

TEST_CASE("HandleTableT iterator destructs removed elements", "[container]")
{
	bx::CrtAllocator allocator;

	Tracked::reset();

	{
		bx::HandleTableT<Tracked, 64> array(allocator.getHandle(), 64);

		for (int32_t ii = 0; ii < 130; ++ii)
		{
			(void)array.add(Tracked(ii) );
		}

		REQUIRE(130 == Tracked::s_numLive);

		for (const auto it : array)
		{
			if (0 == (it.getValue().value % 3) )
			{
				array.remove(it);
			}
		}

		REQUIRE(86 == array.getCount() );
		REQUIRE(86 == Tracked::s_numLive);

		for (const auto it : array)
		{
			REQUIRE(0 != (it.getValue().value % 3) );
		}
	}

	REQUIRE(0 == Tracked::s_numLive);
}

TEST_CASE("HandleTableT default allocator", "[container]")
{
	bx::HandleTableT<int32_t, 64> array;
	REQUIRE(bx::getDefaultAllocatorBlk() == bx::getAllocator(array.getAllocatorHandle() ) );
	REQUIRE(0 == array.getCount() );
	REQUIRE(0 == array.getCapacity() );
	REQUIRE(!array.isUsed(0) );
	REQUIRE(!array.isUsed(1) );
	REQUIRE(bx::kInvalid == array.findUsed(1) );
	REQUIRE(!(array.begin() != array.end() ) );

	const uint32_t idx = array.add(11);
	REQUIRE(0 < idx);
	REQUIRE(1 == array.getCount() );
	REQUIRE(64 == array.getCapacity() );
	REQUIRE(!array.isUsed(0) );
	REQUIRE(11 == array[idx]);

	array.remove(idx);
	REQUIRE(0 == array.getCount() );
	REQUIRE(!array.isUsed(0) );
}

TEST_CASE("HandleTableT grows past many blocks", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<uint32_t, 64> array(allocator.getHandle(), 0);

	constexpr uint32_t kNum = 64*1000;

	for (uint32_t ii = 1; ii < kNum; ++ii)
	{
		REQUIRE(ii == array.add(ii*3) );
	}

	REQUIRE(kNum - 1 == array.getCount() );
	REQUIRE(kNum == array.getCapacity() );

	const uint32_t* ptr = &array[777];

	array.remove(5);

	for (uint32_t ii = 64*100; ii < kNum; ++ii)
	{
		array.remove(ii);
	}

	REQUIRE(64*100 == array.getCapacity() );
	REQUIRE(ptr == &array[777]);

	for (uint32_t ii = 64*10; ii < 64*90; ++ii)
	{
		array.remove(ii);
	}

	REQUIRE(64*100 == array.getCapacity() );

	for (uint32_t ii = 6; ii < 64*10; ++ii)
	{
		REQUIRE(ii*3 == array[ii]);
	}

	for (uint32_t ii = 64*90; ii < 64*100; ++ii)
	{
		REQUIRE(ii*3 == array[ii]);
	}

	REQUIRE(5 == array.add(15) );

	const uint32_t idx = array.add(1);
	REQUIRE(idx >= 64*10);
	REQUIRE(idx < 64*11);
}

TEST_CASE("HandleTableT move, clear and allocator", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<bx::ArrayT<int32_t>, 64> array(allocator.getHandle() );

	REQUIRE(allocator.getHandle() == array.getAllocatorHandle() );
	REQUIRE(allocator.getHandle().idx == array.getAllocatorHandle().idx);
	REQUIRE(0 == array.getCapacity() );

	uint32_t idx[100];

	for (uint32_t ii = 0; ii < BX_COUNTOF(idx); ++ii)
	{
		bx::ArrayT<int32_t> element(allocator.getHandle() );
		element.add(int32_t(ii) );
		idx[ii] = array.add(bx::move(element) );
	}

	bx::HandleTableT<bx::ArrayT<int32_t>, 64> moved(bx::move(array) );

	REQUIRE(0 == array.getCount() );
	REQUIRE(0 == array.getCapacity() );
	REQUIRE(BX_COUNTOF(idx) == moved.getCount() );

	for (uint32_t ii = 0; ii < BX_COUNTOF(idx); ++ii)
	{
		REQUIRE(int32_t(ii) == moved[idx[ii] ][0]);
	}

	const uint32_t idxAfterMove = array.add(bx::ArrayT<int32_t>(allocator.getHandle() ) );
	REQUIRE(1 == array.getCount() );
	REQUIRE(array.isUsed(idxAfterMove) );

	array = bx::move(moved);

	REQUIRE(BX_COUNTOF(idx) == array.getCount() );
	REQUIRE(0 == moved.getCount() );
	REQUIRE(int32_t(BX_COUNTOF(idx) - 1) == array[idx[BX_COUNTOF(idx) - 1] ][0]);

	array.clear();

	REQUIRE(0 == array.getCount() );
	REQUIRE(0 == array.getCapacity() );
	REQUIRE(!array.isUsed(idx[0]) );

	const uint32_t idxAfterClear = array.add(bx::ArrayT<int32_t>(allocator.getHandle() ) );
	REQUIRE(1 == array.getCount() );
	REQUIRE(array.isUsed(idxAfterClear) );
}

TEST_CASE("HandleTableT write", "[container]")
{
	bx::CrtAllocator allocator;
	bx::HandleTableT<uint32_t, 64> array(allocator.getHandle(), 1);
	bx::FixedArrayT<uint32_t, 200> indices;

	for (uint32_t ii = 0; ii < 200; ++ii)
	{
		indices.add(array.add(ii) );
	}

	for (uint32_t ii = 0; ii < 200; ii += 7)
	{
		array.remove(indices[ii]);
	}

	bx::ArrayT<uint32_t> expected(allocator.getHandle() );

	for (const auto it : array)
	{
		expected.add(it.getValue() );
	}

	bx::ArrayT<uint8_t> out(allocator.getHandle() );
	bx::ArrayWriterT<uint8_t> writer(&out);

	REQUIRE(int32_t(expected.getCount()*sizeof(uint32_t) ) == bx::write(&writer, array, bx::ErrorAssert{}) );
	REQUIRE(out.getCount() == expected.getCount()*sizeof(uint32_t) );
	REQUIRE(0 == bx::memCmp(out.getPtr(), expected.getPtr(), out.getCount() ) );

	bx::HandleTableT<uint32_t, 64> empty(allocator.getHandle(), 1);
	REQUIRE(0 == bx::write(&writer, empty, bx::ErrorAssert{}) );
}

TEST_CASE("HandleTableT std::string elements", "[container]")
{
	bx::CrtAllocator allocator;

	bx::HandleTableT<std::string, 64> array(allocator.getHandle(), 1);
	bx::FixedArrayT<uint32_t, 200> indices;

	const std::string value("copied");
	indices.add(array.add(value) );

	for (uint32_t ii = 1; ii < 200; ++ii)
	{
		indices.add(array.add(std::to_string(ii) ) );
	}

	for (uint32_t ii = 1; ii < 200; ii += 3)
	{
		array.remove(indices[ii]);
	}

	REQUIRE(value == array[indices[0]]);

	for (uint32_t ii = 2; ii < 200; ii += 3)
	{
		REQUIRE(std::to_string(ii) == array[indices[ii]]);
	}

	bx::HandleTableT<std::string, 64> moved(bx::move(array) );
	REQUIRE("197" == moved[indices[197]]);
}

namespace
{
	std::string makeBlockValue(int32_t _value)
	{
		return 0 == _value%3
			? std::to_string(_value) + std::string(32, 'b')
			: std::to_string(_value)
			;
	}

	using StringHandleTable = bx::HandleTableT<std::string, 64>;

	void requireHandleTableEqual(const StringHandleTable& _array, const std::map<uint32_t, std::string>& _ref)
	{
		REQUIRE(_ref.size() == _array.getCount() );

		auto refIt = _ref.begin();

		for (const auto& it : _array)
		{
			REQUIRE(refIt != _ref.end() );
			REQUIRE(refIt->first  == it.getIndex() );
			REQUIRE(refIt->second == it.getValue() );
			++refIt;
		}

		REQUIRE(refIt == _ref.end() );
	}

	uint32_t pickUsed(bx::RngMwc& _rng, const std::map<uint32_t, std::string>& _ref)
	{
		auto it = _ref.begin();
		std::advance(it, _rng.gen() % _ref.size() );
		return it->first;
	}

} // namespace

TEST_CASE("HandleTableT std::string elements against model", "[container]")
{
	bx::CrtAllocator allocator;
	StringHandleTable array(allocator.getHandle() );
	std::map<uint32_t, std::string> ref;

	bx::RngMwc rng;

	for (int32_t op = 0; op < 20000; ++op)
	{
		const uint32_t count = uint32_t(ref.size() );
		const bool     grow  = (op/2000)%2 == 0;

		switch (rng.gen() % 12)
		{
		case 0:
		case 1:
		case 2:
			if (grow
			||  0 == rng.gen() % 2)
			{
				const std::string value = makeBlockValue(op);
				const uint32_t idx = 0 == rng.gen() % 2
					? array.add(value)
					: array.add(std::string(value) )
					;
				REQUIRE(0 != idx);
				REQUIRE(ref.end() == ref.find(idx) );
				ref[idx] = value;
			}
			break;

		case 3:
			if (0 != count)
			{
				const uint32_t src = pickUsed(rng, ref);
				const uint32_t idx = array.add(array[src]);
				REQUIRE(ref.end() == ref.find(idx) );
				ref[idx] = ref[src];
			}
			break;

		case 4:
		case 5:
			if (0 != count)
			{
				const uint32_t idx = pickUsed(rng, ref);
				array.remove(idx);
				ref.erase(idx);
			}
			break;

		case 6:
			if (0 != count)
			{
				const uint32_t idx = pickUsed(rng, ref);
				REQUIRE(ref[idx] == array.pop(idx) );
				ref.erase(idx);
			}
			break;

		case 7:
			{
				const uint32_t idx = rng.gen() % (array.getCapacity() + 64);
				auto it = ref.lower_bound(idx);

				REQUIRE( (ref.end() != ref.find(idx) ) == array.isUsed(idx) );
				REQUIRE( (ref.end() == it ? bx::kInvalid : it->first) == array.findUsed(idx) );
			}
			break;

		case 8:
			if (0 != count
			&&  0 == rng.gen() % 8)
			{
				const uint32_t mod = 1 + rng.gen() % 4;

				for (auto it = array.begin(), itEnd = array.end(); it != itEnd; ++it)
				{
					const uint32_t idx = (*it).getIndex();

					if (0 == idx % mod)
					{
						array.remove(it);
						ref.erase(idx);
					}
				}
			}
			break;

		case 9:
			if (0 != count
			&&  0 == rng.gen() % 8)
			{
				uint32_t visited = 0;
				uint32_t last    = 0;

				for (const auto& it : array)
				{
					const uint32_t idx = it.getIndex();
					REQUIRE(last < idx);
					last = idx;
					++visited;

					auto next = ref.upper_bound(idx);

					if (ref.end() != next
					&&  0 == rng.gen() % 4)
					{
						array.remove(next->first);
						ref.erase(next);
					}
				}

				REQUIRE(visited <= count);
			}
			break;

		case 10:
			if (0 == rng.gen() % 64)
			{
				StringHandleTable other(bx::move(array) );
				REQUIRE(0 == array.getCount() );
				array = bx::move(other);
			}
			break;

		default:
			if (0 == rng.gen() % 256)
			{
				array.clear();
				ref.clear();
			}
			break;
		}

		requireHandleTableEqual(array, ref);
	}
}
