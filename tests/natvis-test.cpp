/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"

#include <bx/allocators.h>
#include <bx/array.h>
#include <bx/bitarray.h>
#include <bx/debug.h>
#include <bx/handlealloc.h>
#include <bx/handletable.h>
#include <bx/hashmap.h>
#include <bx/stringpool.h>
#include <bx/uniqueptr.h>

namespace
{
	struct NatvisItem
	{
		NatvisItem(uint32_t _id, float _weight)
			: id(_id)
			, weight(_weight)
		{
		}

		uint32_t id;
		float    weight;
	};

	struct NatvisKey
	{
		bool operator==(const NatvisKey& _rhs) const
		{
			return group == _rhs.group && index == _rhs.index;
		}

		uint32_t group;
		uint32_t index;
	};

} // namespace

TEST_CASE("natvis", "[.][natvis]")
{
	bx::CrtAllocator     allocator;
	bx::DefaultAllocator defaultAllocator;

	const bx::AllocatorOp::Enum allocatorOp = bx::AllocatorOp::Allocate;

	bx::Blk blk = bx::alloc(&allocator, 64);
	REQUIRE(isValid(blk) );

	bx::ArrayT<uint32_t> array(allocator.getHandle() );
	for (uint32_t ii = 0; ii < 10; ++ii)
	{
		array.add(ii * 10);
	}

	bx::FixedArrayT<uint16_t, 8> fixedArray(bx::ArrayViewT<uint16_t>({ 3, 1, 4, 1, 5 }) );

	bx::StaticArrayT<uint16_t, 4> staticArray;
	staticArray[0] = 7;
	staticArray[1] = 8;
	staticArray[2] = 9;
	staticArray[3] = 10;

	bx::SortedArrayT<int32_t> sortedArray(allocator.getHandle(), bx::ArrayViewT<int32_t>({ 9, -2, 5, 0, 13 }) );

	bx::FixedSortedArrayT<int32_t, 8> fixedSortedArray(bx::ArrayViewT<int32_t>({ 6, 2, 4 }) );

	const bx::ArrayViewT<uint32_t> arrayView = array;

	uint32_t mutableData[] = { 11, 22, 33 };
	const bx::MutableArrayViewT<uint32_t> mutableArrayView(mutableData, BX_COUNTOF(mutableData) );

	const bx::StorageViewT<uint32_t> storageView(mutableData, BX_COUNTOF(mutableData) );

	bx::UniquePtrT<NatvisItem> uniquePtr = BX_MAKE_UNIQUE(allocator.getHandle(), NatvisItem, 42, 0.5f);
	bx::UniquePtrT<NatvisItem> uniquePtrEmpty;

	bx::UniquePtrT<uint32_t[]> uniqueArray = BX_MAKE_UNIQUE(allocator.getHandle(), uint32_t[], 3);
	uniqueArray[0] = 11;
	uniqueArray[1] = 22;
	uniqueArray[2] = 33;

	const bx::StringView stringView("string view");

	const bx::StringPoolEntry& stringLiteral = "string literal"_str;

	const bx::String string(bx::StringView("pooled string") );
	const bx::String stringEmpty;

	bx::StringBuilder stringBuilder;
	stringBuilder.add("string ");
	stringBuilder.add("builder");

	bx::HandleTableT<NatvisItem> handleTable(allocator.getHandle() );
	const uint32_t handle0 = handleTable.add(NatvisItem(1, 1.0f) );
	const uint32_t handle1 = handleTable.add(NatvisItem(2, 2.0f) );
	const uint32_t handle2 = handleTable.add(NatvisItem(3, 3.0f) );
	handleTable.remove(handle1);

	bx::HashMapT<uint32_t, uint32_t> hashMap(allocator.getHandle(), 0);
	bx::FixedHashMapT<uint32_t, uint32_t, 16> fixedHashMap;
	bx::BlockHashMapT<uint32_t, uint32_t> blockHashMap(allocator.getHandle(), 0);
	bx::HashSetT<uint32_t> hashSet(allocator.getHandle(), 0);
	bx::FixedHashSetT<uint32_t, 16> fixedHashSet;

	for (uint32_t ii = 0; ii < 8; ++ii)
	{
		REQUIRE(hashMap.tryAdd(ii, ii * 100) );
		REQUIRE(fixedHashMap.tryAdd(ii, ii * 100) );
		REQUIRE(blockHashMap.tryAdd(ii, ii * 100) );
		REQUIRE(hashSet.tryAdd(ii * 7) );
		REQUIRE(fixedHashSet.tryAdd(ii * 7) );
	}

	bx::HashMapT<bx::String, uint32_t> stringHashMap(allocator.getHandle(), 0);
	REQUIRE(stringHashMap.tryAdd(bx::String(bx::StringView("albedo") ), 0) );
	REQUIRE(stringHashMap.tryAdd(bx::String(bx::StringView("normal") ), 1) );

	bx::HashMapT<bx::String, bx::String> stringToStringHashMap(allocator.getHandle(), 0);
	REQUIRE(stringToStringHashMap.tryAdd(bx::String(bx::StringView("albedo") ), bx::String(bx::StringView("textures/albedo.png") ) ) );
	REQUIRE(stringToStringHashMap.tryAdd(bx::String(bx::StringView("normal") ), bx::String(bx::StringView("textures/normal.png") ) ) );

	bx::HashMapT<bx::StringView, bx::StringView> viewHashMap(allocator.getHandle(), 0);
	REQUIRE(viewHashMap.tryAdd(bx::StringView("vs"), bx::StringView("vs_mesh.bin") ) );
	REQUIRE(viewHashMap.tryAdd(bx::StringView("fs"), bx::StringView("fs_mesh.bin") ) );

	bx::HashMapT<bx::String, bx::ArrayT<uint32_t> > stringToArrayHashMap(allocator.getHandle(), 0);
	REQUIRE(stringToArrayHashMap.tryAdd(bx::String(bx::StringView("indices") ), bx::Emplace, allocator.getHandle() ) );
	REQUIRE(stringToArrayHashMap.tryAdd(bx::String(bx::StringView("empty") ), bx::Emplace, allocator.getHandle() ) );
	bx::ArrayT<uint32_t>* indices = stringToArrayHashMap.find(bx::String(bx::StringView("indices") ) );
	REQUIRE( (NULL != indices) );
	indices->add(0);
	indices->add(1);
	indices->add(2);

	bx::HashMapT<uint32_t, NatvisItem> itemHashMap(allocator.getHandle(), 0);
	REQUIRE(itemHashMap.tryAdd(7, bx::Emplace, 7u, 0.7f) );
	REQUIRE(itemHashMap.tryAdd(9, bx::Emplace, 9u, 0.9f) );

	bx::FixedHashMapT<uint32_t, NatvisItem, 8> fixedItemHashMap;
	REQUIRE(fixedItemHashMap.tryAdd(3, bx::Emplace, 3u, 0.3f) );
	REQUIRE(fixedItemHashMap.tryAdd(5, bx::Emplace, 5u, 0.5f) );

	bx::BlockHashMapT<uint32_t, NatvisItem, 64> blockItemHashMap(allocator.getHandle(), 0);
	for (uint32_t ii = 0; ii < 100; ++ii)
	{
		REQUIRE(blockItemHashMap.tryAdd(ii, bx::Emplace, ii, float(ii) ) );
	}

	bx::HashMapT<NatvisKey, NatvisItem> keyItemHashMap(allocator.getHandle(), 0);
	REQUIRE(keyItemHashMap.tryAdd(NatvisKey{ 1, 2 }, bx::Emplace, 12u, 1.2f) );
	REQUIRE(keyItemHashMap.tryAdd(NatvisKey{ 3, 4 }, bx::Emplace, 34u, 3.4f) );

	bx::HashSetT<NatvisKey> keyHashSet(allocator.getHandle(), 0);
	REQUIRE(keyHashSet.tryAdd(NatvisKey{ 5, 6 }) );
	REQUIRE(keyHashSet.tryAdd(NatvisKey{ 7, 8 }) );

	bx::BitArray bitArray(allocator.getHandle() );
	bitArray.setCount(100);
	bitArray.set(3, true);
	bitArray.set(64, true);
	bitArray.set(99, true);

	bx::FixedBitArrayT<128> fixedBitArray;
	fixedBitArray.setCount(128);
	fixedBitArray.set(5, true);
	fixedBitArray.set(127, true);

	const bx::BitArrayViewBase& bitArrayView = fixedBitArray;

	bx::MippedBitArray mippedBitArray(allocator.getHandle() );
	mippedBitArray.setCount(300);
	mippedBitArray.set(1, true);
	mippedBitArray.set(200, true);

	bx::HandleAllocT<16> handleAlloc;
	const uint16_t handleAlloc0 = handleAlloc.alloc();
	const uint16_t handleAlloc1 = handleAlloc.alloc();
	handleAlloc.free(handleAlloc0);

	bx::FixedMippedBitArrayT<32> handleAllocBits;
	handleAllocBits.setCount(32);
	bx::HandleAlloc handleAllocExternal(handleAllocBits, 32);
	const uint16_t handleAllocExternal0 = handleAllocExternal.alloc();

	BX_UNUSED(allocatorOp, arrayView, mutableArrayView, storageView, stringView, stringLiteral, stringEmpty);
	BX_UNUSED(handle0, handle2, handleAlloc1, handleAllocExternal0, bitArrayView);

	BX_TRACE("natvis: break here and inspect locals.");

	blk = bx::free(&allocator, bx::move(blk) );
}
