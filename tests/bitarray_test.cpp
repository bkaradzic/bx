/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "test.h"

#include <bx/allocator.h>
#include <bx/bitarray.h>
#include <bx/math.h>

static void testRange(bx::MutableBitArrayView& _bs, uint32_t _beginBit, uint32_t _endBit)
{
	_bs.clear();
	_bs.set(_beginBit, _endBit, true);

	const uint32_t beginBitMinus1 = bx::max<int32_t>(_beginBit-1, 0);
	const uint32_t endBitPlus1    = bx::min<int32_t>(_endBit+1, _bs.getCount() );

	if (beginBitMinus1 != _beginBit)
	{
		REQUIRE(!_bs.get(beginBitMinus1) );
	}

	if (endBitPlus1 != _endBit)
	{
		REQUIRE(!_bs.get(endBitPlus1) );
	}

	REQUIRE(_bs.testAll(_beginBit, _endBit) );

	const uint32_t bitCount = _endBit - _beginBit;

	REQUIRE(bitCount == _bs.countBits(beginBitMinus1, endBitPlus1) );
	REQUIRE(bitCount == _bs.countBits() );
}

TEST_CASE("BitArray", "[container]")
{
	uint64_t bits[64];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );
	REQUIRE(bs.getCapacity() == 4096);

	bs.clear(true);

	REQUIRE(!bs.get(0) );

	REQUIRE(0 == bs.find(128) );

	bs.set(0, true);
	REQUIRE(bs.get(0) );

	bs.set(0, false);
	REQUIRE(!bs.get(0) );

	REQUIRE(bs.testNone() );
	REQUIRE(!bs.testAny() );

	REQUIRE(bs.testNone(1, 5) );

	bs.set(1, 5, true);

	REQUIRE(4 == bs.countBits() );
	REQUIRE(4 == bs.countBits(1, 5) );

	bs.set(4095, true);
	REQUIRE(5 == bs.countBits() );
	REQUIRE(4 == bs.countBits(1, 5) );

	REQUIRE(5 == bs.find(32) );

	REQUIRE(bs.testNone(1, 1) );
	REQUIRE(bs.testNone(2, 2) );
	REQUIRE(bs.testNone(0, 0) );
	REQUIRE(!bs.testAny(0, 0) );

	REQUIRE(bs.testAll(1, 4) );
	REQUIRE(bs.testAny(1, 5) );

	REQUIRE(bs.testNone(6, 63) );
	REQUIRE(!bs.testAny(6, 63) );

	REQUIRE(bs.testAny(0, 63) );
	REQUIRE(bs.testNone(61, 75) );

	bs.set(61, 75, true);
	REQUIRE(75 == bs.find(128) );

	REQUIRE(bs.testNone(60, 60) );
	REQUIRE(!bs.testAny(60, 60) );

	REQUIRE(bs.testAll(61, 75) );
	REQUIRE(bs.testAny(61, 75) );

	REQUIRE(bs.testNone(76, 127) );
	REQUIRE(!bs.testAny(76, 127) );

	REQUIRE(bs.testAny(0, 127) );

	bs.clear();
	REQUIRE(bs.testNone() );
	REQUIRE(!bs.testAny() );

	uint32_t idx = bs.find(8);
	REQUIRE(0 == idx);
	bs.set(0, 7, true);

	idx = bs.find(512);
	REQUIRE(7 == idx);
	bs.set(7, 519, true);

	idx = bs.find(16);
	REQUIRE(519 == idx);
	bs.set(519, 535, true);

	bs.clear();
	bs.set(32, 64, true);
	bs.blit(1, 32, 30);
	REQUIRE(bs.testAll(8, 23) );

	bs.clear();
	bs.set(64, 255, true);
	bs.blit(1, 65, 128);
	REQUIRE(bs.testAll(8, 23) );

	bs.clear();
	bs.set(64, 192, true);
	bs.blit(3, 65, 127);
	REQUIRE(bs.testAll(3, 130) );

	testRange(bs, 0, 65);
	testRange(bs, 63, 65);
}

TEST_CASE("BitArray-argument-validation", "[container]")
{
	uint64_t bits[1];
	bx::MutableBitArrayView bs(bits, sizeof(bits), 1);

	REQUIRE_ASSERTS(bs.find(0, 0, 1) ); // Invalid range.
	REQUIRE_ASSERTS(bs.find(0, 8, 1) ); // Invalid range.
	REQUIRE_ASSERTS(bs.find(1, 2, 1) ); // Invalid range.
	REQUIRE_ASSERTS(bs.find(0, 1, 0) ); // Invalid number of bits.
	REQUIRE_ASSERTS(bs.find(0, 1, 2) ); // Number of bits larger than range.
	REQUIRE_ASSERTS(bs.find(1, 0, 1) ); // Range invalid / backwards.
}

TEST_CASE("BitArray-6-bits", "[container]")
{
	constexpr uint32_t numBits = 64;

	uint64_t bits[1];
	bx::MutableBitArrayView bs(bits, sizeof(bits), numBits);
	REQUIRE(bs.getCapacity() == 64);

	bs.set(0, 64, (UINT64_MAX << 16) | 0b1100000110101000ul);

	REQUIRE(9 == bs.find(0, 63, 4) );
	REQUIRE(9 == bs.find(0, 63, 5) );
	REQUIRE(bx::kInvalid == bs.find(0, 63, 6) );
	REQUIRE(bx::kInvalid == bs.find(0, 63, 7) );
}

TEST_CASE("BitArray-7-bits", "[container]")
{
	const uint32_t numBits = 7;

	uint64_t bits[1];
	bx::MutableBitArrayView bs(bits, sizeof(bits), numBits);
	REQUIRE(bs.getCapacity() == 64);

	bs.clear(true);

	REQUIRE(!bs.get(3) );
	bs.set(3, true);
	REQUIRE(bs.get(3) );

	REQUIRE(!bs.testAll(0, numBits) );
	REQUIRE(!bs.testNone(0, numBits) );
	REQUIRE(bs.testAny(0, numBits) );
	REQUIRE(1 == bs.countBits(0, numBits) );

	REQUIRE(!bs.testAll(0, 2) );
	REQUIRE(bs.testNone(0, 2) );
	REQUIRE(!bs.testAny(0, 2) );

	bs.clear();
	REQUIRE(!bs.get(3) );
	REQUIRE(0 == bs.countBits(0, numBits) );

	bs.set(3, 7, true);
	REQUIRE(4 == bs.countBits(0, numBits) );

	bs.set(0, numBits, true);
	REQUIRE(!bs.testNone(0, numBits) );
	REQUIRE(bs.testAll(0, numBits) );

	bs.set(0, numBits, false);
	REQUIRE(bs.testNone(0, numBits) );
	REQUIRE(!bs.testAll(0, numBits) );

	testRange(bs, 0, 3);
	testRange(bs, 1, 3);
	testRange(bs, 2, 5);
	testRange(bs, 3, 4);
	testRange(bs, 4, 7);
	testRange(bs, 5, 6);
	testRange(bs, 6, 7);
}

TEST_CASE("BitArray-31-bits", "[container]")
{
	constexpr uint32_t numBits = 31;

	uint64_t bits[1];
	bx::MutableBitArrayView bs(bits, sizeof(bits), numBits);
	REQUIRE(bs.getCapacity() == 64);

	bs.clear(true);

	REQUIRE(!bs.get(13) );
	bs.set(13, true);
	REQUIRE(bs.get(13) );

	REQUIRE(!bs.testAll(0, numBits) );
	REQUIRE(!bs.testNone(0, numBits) );
	REQUIRE(bs.testAny(0, numBits) );
	REQUIRE(1 == bs.countBits(0, numBits) );

	REQUIRE(!bs.testAll(0, 12) );
	REQUIRE(bs.testNone(0, 12) );
	REQUIRE(!bs.testAny(0, 12) );

	bs.clear();
	REQUIRE(!bs.get(13) );
	REQUIRE(0 == bs.countBits(0, numBits) );

	bs.set(4, 8, true);
	REQUIRE(4 == bs.countBits(0, numBits) );

	bs.set(0, numBits, true);
	REQUIRE(!bs.testNone(0, numBits) );
	REQUIRE(bs.testAll(0, numBits) );

	bs.set(0, numBits, false);
	REQUIRE(bs.testNone(0, numBits) );
	REQUIRE(!bs.testAll(0, numBits) );

	bs.set(13, 20, true);
	REQUIRE(bs.testAll(13, 20) );

	testRange(bs, 0, 3);
	testRange(bs, 1, 3);
	testRange(bs, 2, 5);
	testRange(bs, 3, 4);
	testRange(bs, 4, 7);
	testRange(bs, 5, 6);
	testRange(bs, 6, 10);
}

static void findWithPattern(bx::MutableBitArrayView& _bs, uint8_t _pattern, uint32_t _numBits)
{
	BX_ASSERT(0 == _bs.getCount() % 8, "BitArray lenght must be multiple of 8.");

	uint8_t* bytes = (uint8_t*)_bs.getPtr();
	bx::memSet(bytes, _pattern, _bs.getCount()/8);

	const uint8_t bitCount = bx::countBits(_pattern);
	const uint32_t numEmptyBits = _bs.getCount() - (_bs.getCount() / 8 * bitCount);

	for (uint32_t ii = 0; ii < numEmptyBits; ii += _numBits)
	{
		const uint32_t idx = _bs.find(_numBits);
		REQUIRE(idx != bx::kInvalid);

		_bs.set(idx, idx+_numBits, true);
	}

	REQUIRE(_bs.testAll() );
}

TEST_CASE("BitArray-find-unset-bit", "[container]")
{
	uint64_t bits[8];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );
	REQUIRE(bs.getCapacity() == 512);

	for (uint32_t ii = 0, num = bs.getCount(); ii < num; ++ii)
	{
		bs.set(0, num, true);
		bs.set(ii, false);
		REQUIRE(ii == bs.find(1) );
	}
}

TEST_CASE("BitArray-find-unset-bit-in-range", "[container]")
{
	uint64_t bits[8];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );

	const uint32_t num = bs.getCount();

	for (uint32_t ii = 0; ii < num; ++ii)
	{
		bs.set(0, num, true);
		bs.set(ii, false);

		for (uint32_t begin : { 0u, 1u, 63u, 64u, 65u, 200u })
		{
			for (uint32_t end : { 66u, 128u, 201u, 511u, 512u })
			{
				if (begin >= end)
				{
					continue;
				}

				const uint32_t expected = ii >= begin && ii < end ? ii : bx::kInvalid;
				REQUIRE(expected == bs.find(begin, end, 1) );
				REQUIRE(expected == bs.findClear(begin, end) );
			}
		}
	}
}

TEST_CASE("BitArray-find-with-pattern", "[container]")
{
	uint64_t bits[8];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );
	REQUIRE(bs.getCapacity() == 512);

	findWithPattern(bs, 0x55, 1);
	findWithPattern(bs, 0x81, 1);
	findWithPattern(bs, 0x81, 2);
	findWithPattern(bs, 0x81, 3);
	findWithPattern(bs, 0x81, 6);
	findWithPattern(bs, 0x88, 3);
	findWithPattern(bs, 0x99, 2);
	findWithPattern(bs, 0xaa, 1);
	findWithPattern(bs, 0xc3, 1);
	findWithPattern(bs, 0xc3, 2);
	findWithPattern(bs, 0xc3, 4);
	findWithPattern(bs, 0xe7, 1);
	findWithPattern(bs, 0xe7, 2);

	for (uint32_t ii = 0; ii < 256; ++ii)
	{
		findWithPattern(bs, uint8_t(ii), 1);
	}
}

static void testGet(bx::MutableBitArrayView& _bs, uint32_t _pos, uint32_t _numBits)
{
	_bs.set(_pos, _pos+_numBits, true);
	REQUIRE(_numBits == _bs.countBits(0, _bs.getCount() ) );
	REQUIRE(UINT64_MAX >> (64 - _numBits) == _bs.get(_pos, _pos + _numBits) );
	_bs.clear();
}

TEST_CASE("BitArray-get", "[container]")
{
	uint64_t bits[4];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );
	REQUIRE(bs.getCapacity() == 256);

	bs.clear(true);

	testGet(bs,   0,   5);
	testGet(bs,   1,   3);
	testGet(bs,   0,  63);
	testGet(bs,   0,  64);
	testGet(bs,   1,  63);
	testGet(bs,   1,  64);
	testGet(bs,  62,   5);
	testGet(bs, 127,   3);
	testGet(bs, 250,   6);
	testGet(bs, 254,   2);
}

static void testSet(bx::MutableBitArrayView& _bs, uint32_t _pos, uint64_t _value, uint32_t _numBits)
{
	_bs.set(_pos, _pos+_numBits, _value);

	REQUIRE(_value == _bs.get(_pos, _pos + _numBits) );
	_bs.clear();
}

TEST_CASE("BitArray-set", "[container]")
{
	uint64_t bits[4];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );
	REQUIRE(bs.getCapacity() == 256);

	bs.clear(true);

	testSet(bs,   0,    1,  32);

	testSet(bs,   0,   16,  5);
	testSet(bs,   1,    7,  3);
	testSet(bs,   1,   32, 64);
	testSet(bs,  62,    6,  5);
	testSet(bs, 127,    1,  3);
	testSet(bs,  60, 0xaa,  8);
}

TEST_CASE("BitArray-tight-pack", "[container]")
{
	uint64_t bits[2];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );
	REQUIRE(bs.getCapacity() == 128);

	bs.clear(true);
	bs.set(0, 80, true);

	REQUIRE(bx::kInvalid != bs.find(48) );

	bs.clear(true);
	bs.set(32, 128, true);

	REQUIRE(bx::kInvalid != bs.find(32) );
}

TEST_CASE("BitArray-set-all-bits-one-by-one", "[container]")
{
	bx::DefaultAllocator crtAllocator;

	constexpr uint32_t kCount = 64;

	bx::BitArray bs(&crtAllocator);
	bs.setCount(kCount);

	REQUIRE(kCount == bs.getCount() );

	bs.clear(true);

	for (uint32_t ii = 0, num = bs.getCount(); ii < num; ++ii)
	{
		uint32_t idx = bs.find(1);
		REQUIRE(bx::kInvalid != idx);

		bs.set(idx, true);
	}
}

TEST_CASE("BitArray-resize", "[container]")
{
	bx::DefaultAllocator allocator;
	bx::BitArray array(&allocator);

	array.setCapacity(1024);
	REQUIRE(1024 == array.getCapacity() );
	REQUIRE(0 == array.getCount() );

	array.setCount(2048);
	REQUIRE(2048 == array.getCount() );
	REQUIRE(2048 == array.getCapacity() );

	array.set(0, 2048, true);
	REQUIRE(array.testAll() );

	array.clear(true);
	REQUIRE(array.testNone() );

	testGet(array,  0,   5);
	testGet(array,  1,   3);
	testGet(array,  0,  63);
	testGet(array,  0,  64);
	testGet(array,  1,  63);
	testGet(array,  1,  64);
	testGet(array,  62,  5);
	testGet(array, 127,  3);
	testGet(array, 250,  6);
	testGet(array, 254,  2);
}

TEST_CASE("BitArray-findSet", "[container]")
{
	bx::DefaultAllocator allocator;
	bx::BitArray array(&allocator);

	constexpr uint32_t kCapacity = 1024;
	array.setCount(kCapacity);

	for (uint32_t ii = 0; ii < kCapacity/3; ++ii)
	{
		constexpr uint32_t kFirstIdx = 32;
		constexpr uint32_t kLastIdx  = kCapacity-kFirstIdx;

		array.clear();

		array.set(kFirstIdx, true);
		REQUIRE(kFirstIdx == array.findSet(bx::kFirst) );
		REQUIRE(kFirstIdx == array.findSet(bx::kLast) );

		array.set(kLastIdx, true);
		REQUIRE(kFirstIdx == array.findSet(bx::kFirst) );
		REQUIRE(kLastIdx  == array.findSet(bx::kLast) );

		array.set(kFirstIdx, false);
		REQUIRE(kLastIdx == array.findSet(bx::kFirst) );
		REQUIRE(kLastIdx == array.findSet(bx::kLast) );
	}
}

TEST_CASE("BitArray-findSet-empty", "[container]")
{
	bx::DefaultAllocator allocator;
	bx::BitArray array(&allocator);

	array.setCount(128);
	array.clear(true);

	// All zeros — findSet should return kInvalid.
	REQUIRE(bx::kInvalid == array.findSet(bx::kFirst) );
	REQUIRE(bx::kInvalid == array.findSet(bx::kLast) );

	// Set one bit, verify findSet finds it.
	array.set(64, true);
	REQUIRE(64 == array.findSet(bx::kFirst) );
	REQUIRE(64 == array.findSet(bx::kLast) );
}

TEST_CASE("FixedBitArrayT", "[container]")
{
	bx::FixedBitArrayT<128> ba;
	REQUIRE(0 == ba.getCount() );
	REQUIRE(128 == ba.getCapacity() );

	ba.setCount(100);
	REQUIRE(100 == ba.getCount() );

	ba.clear(true);
	REQUIRE(ba.testNone() );

	// Set individual bits and verify.
	ba.set(0, true);
	ba.set(50, true);
	ba.set(99, true);

	REQUIRE(ba.get(0) );
	REQUIRE(ba.get(50) );
	REQUIRE(ba.get(99) );
	REQUIRE(!ba.get(1) );
	REQUIRE(!ba.get(51) );

	REQUIRE(3 == ba.countBits() );

	// Find unset run after setting a range.
	ba.clear(true);
	REQUIRE(0 == ba.find(1) );

	ba.set(0, 64, true);
	REQUIRE(64 == ba.find(1) );

	// Set range and verify.
	ba.clear(true);
	ba.set(10, 20, true);
	REQUIRE(ba.testAll(10, 20) );
	REQUIRE(10 == ba.countBits() );

	// testAny/testNone/testAll full array.
	ba.clear(true);
	REQUIRE(ba.testNone() );
	REQUIRE(!ba.testAny() );
	REQUIRE(!ba.testAll() );

	ba.set(0, 100, true);
	REQUIRE(ba.testAll() );
	REQUIRE(ba.testAny() );
	REQUIRE(!ba.testNone() );
}

TEST_CASE("BitArray-copy-ctor", "[container]")
{
	bx::DefaultAllocator allocator;
	bx::BitArray src(&allocator);
	src.setCount(256);
	src.clear(true);
	src.set(0, 64, true);
	src.set(200, true);

	bx::BitArray dst(src);

	REQUIRE(dst.getCount() == src.getCount() );
	REQUIRE(dst.countBits() == src.countBits() );
	REQUIRE(dst.testAll(0, 64) );
	REQUIRE(dst.get(200) );
	REQUIRE(!dst.get(100) );

	// Verify they are independent copies.
	dst.set(100, true);
	REQUIRE(!src.get(100) );
	REQUIRE(dst.get(100) );
}

TEST_CASE("BitArray-copy-assignment", "[container]")
{
	bx::DefaultAllocator allocator;

	bx::BitArray src(&allocator);
	src.setCount(128);
	src.clear(true);
	src.set(0, 32, true);

	bx::BitArray dst(&allocator);
	dst.setCount(64);
	dst.clear(true);

	dst = src;

	REQUIRE(128 == dst.getCount() );
	REQUIRE(dst.testAll(0, 32) );
	REQUIRE(dst.testNone(32, 128) );

	// Verify independence.
	src.set(64, true);
	REQUIRE(!dst.get(64) );
}

TEST_CASE("BitArray-empty-predicates", "[container]")
{
	bx::DefaultAllocator allocator;

	// A default-constructed BitArray owns no storage. Range predicates over an
	// empty range must answer without touching the (NULL) buffer.
	const bx::BitArray ba(&allocator);

	REQUIRE(0 == ba.getCount() );
	REQUIRE(0 == ba.countBits() );
	REQUIRE(!ba.testAny() );
	REQUIRE(ba.testAll() );
	REQUIRE(ba.testNone() );
}

TEST_CASE("BitArray-copy-assignment from empty", "[container]")
{
	bx::DefaultAllocator allocator;

	// Assigning an empty array releases the destination's storage. The
	// destination must end up empty rather than keeping a count that refers
	// to the freed block.
	const bx::BitArray src(&allocator);

	bx::BitArray dst(&allocator);
	dst.setCount(128);
	dst.clear(true);
	dst.set(0, 128, true);

	dst = src;

	REQUIRE(0 == dst.getCount() );
	REQUIRE(0 == dst.countBits() );

	// Destination must be reusable after the release.
	dst.setCount(64);
	dst.clear(true);
	REQUIRE(64 == dst.getCount() );
	REQUIRE(dst.testNone() );
}

TEST_CASE("BitArray-move-assignment", "[container]")
{
	bx::DefaultAllocator allocator;

	bx::BitArray src(&allocator);
	src.setCount(256);
	src.clear(true);
	src.set(0, 128, true);

	bx::BitArray dst(&allocator);
	dst = bx::move(src);

	REQUIRE(256 == dst.getCount() );
	REQUIRE(dst.testAll(0, 128) );
	REQUIRE(dst.testNone(128, 256) );

	// Source should be empty after move.
	REQUIRE(0 == src.getCount() );
}

TEST_CASE("BitArray-construct-from-view", "[container]")
{
	uint64_t bits[2];
	bx::MutableBitArrayView view(bits, sizeof(bits), 100);
	view.clear(true);
	view.set(10, 50, true);

	bx::DefaultAllocator allocator;
	bx::BitArray array(&allocator, view);

	REQUIRE(100 == array.getCount() );
	REQUIRE(array.testAll(10, 50) );
	REQUIRE(array.testNone(0, 10) );
	REQUIRE(array.testNone(50, 100) );
}

TEST_CASE("BitArray-set-from-view", "[container]")
{
	uint64_t bits[4];
	bx::MutableBitArrayView view(bits, sizeof(bits), 200);
	view.clear(true);
	view.set(0, 100, true);

	bx::DefaultAllocator allocator;
	bx::BitArray array(&allocator);

	array.set(static_cast<const bx::BitArrayViewBase&>(view) );

	REQUIRE(200 == array.getCount() );
	REQUIRE(array.testAll(0, 100) );
	REQUIRE(array.testNone(100, 200) );
}

TEST_CASE("BitArray-set-move", "[container]")
{
	bx::DefaultAllocator allocator;

	bx::BitArray src(&allocator);
	src.setCount(512);
	src.clear(true);
	src.set(0, 256, true);

	bx::BitArray dst(&allocator);
	dst.set(bx::move(src) );

	REQUIRE(512 == dst.getCount() );
	REQUIRE(dst.testAll(0, 256) );
	REQUIRE(dst.testNone(256, 512) );

	// Source should be empty after move.
	REQUIRE(0 == src.getCount() );
}

TEST_CASE("MutableBitArrayView-reassign", "[container]")
{
	uint64_t bits1[1];
	uint64_t bits2[2];

	bx::MutableBitArrayView view(bits1, sizeof(bits1), 64);
	REQUIRE(64 == view.getCount() );

	view.clear(true);
	view.set(0, true);
	view.set(63, true);
	REQUIRE(view.get(0) );
	REQUIRE(view.get(63) );

	// Reassign to second buffer.
	view.set(bits2, 100);
	REQUIRE(100 == view.getCount() );

	bx::memSet(bits2, 0, sizeof(bits2) );
	view.set(99, true);
	REQUIRE(view.get(99) );
	REQUIRE(!view.get(0) );
}

TEST_CASE("BitArray-getPtr-getTerm", "[container]")
{
	uint64_t bits[64];
	bx::MutableBitArrayView view(bits, sizeof(bits) );

	REQUIRE(bits == view.getPtr() );
	REQUIRE(static_cast<void*>(bits + 64) == view.getTerm() );

	const bx::MutableBitArrayView& constView = view;
	REQUIRE(static_cast<const void*>(bits) == constView.getPtr() );
	REQUIRE(static_cast<const void*>(bits + 64) == constView.getTerm() );
}

TEST_CASE("BitArray-getAllocator", "[container]")
{
	bx::DefaultAllocator allocator;
	bx::BitArray array(&allocator);

	REQUIRE(&allocator == array.getAllocator() );
}

TEST_CASE("BitArray-findSet-findClear-range", "[container]")
{
	bx::FixedBitArrayT<200> array;
	array.setCount(200);

	REQUIRE(bx::kInvalid == array.findSet(0, 200) );
	REQUIRE(0 == array.findClear(0, 200) );

	array.set(0, true);
	array.set(63, true);
	array.set(64, true);
	array.set(130, true);
	array.set(199, true);

	REQUIRE(0   == array.findSet(0, 200) );
	REQUIRE(63  == array.findSet(1, 200) );
	REQUIRE(64  == array.findSet(64, 200) );
	REQUIRE(130 == array.findSet(65, 200) );
	REQUIRE(199 == array.findSet(131, 200) );
	REQUIRE(bx::kInvalid == array.findSet(131, 199) );
	REQUIRE(bx::kInvalid == array.findSet(200, 200) );

	REQUIRE(1   == array.findClear(0, 200) );
	REQUIRE(65  == array.findClear(63, 200) );
	REQUIRE(bx::kInvalid == array.findClear(199, 200) );

	array.set(0, 200, true);
	REQUIRE(bx::kInvalid == array.findClear(0, 200) );
	array.set(77, false);
	REQUIRE(77 == array.findClear(0, 200) );
	REQUIRE(bx::kInvalid == array.findClear(78, 200) );
}

TEST_CASE("MippedBitArray-fixed", "[container]")
{
	bx::FixedMippedBitArrayT<4096> array;
	REQUIRE(4096 == array.getCapacity() );
	array.setCount(4096);

	REQUIRE(0 == array.findClear(0, 4096) );

	array.set(0, 4096, true);
	REQUIRE(bx::kInvalid == array.findClear(0, 4096) );
	REQUIRE(array.testAll() );

	array.set(4000, false);
	REQUIRE(4000 == array.findClear(0, 4096) );
	REQUIRE(bx::kInvalid == array.findClear(4001, 4096) );

	array.set(64, 128, false);
	REQUIRE(64   == array.findClear(0, 4096) );
	REQUIRE(100  == array.findClear(100, 4096) );
	REQUIRE(4000 == array.findClear(128, 4096) );

	array.set(64, 128, true);
	REQUIRE(4000 == array.findClear(0, 4096) );

	array.set(4000, true);
	REQUIRE(bx::kInvalid == array.findClear(0, 4096) );

	array.set(200, 232, uint64_t(0) );
	REQUIRE(200 == array.findClear(0, 4096) );
	REQUIRE(bx::kInvalid == array.findClear(232, 4096) );
	array.set(200, 232, true);

	array.set(0, 64, false);
	array.blit(2048, 0, 64);
	REQUIRE(2048 == array.findClear(64, 4096) );
	array.set(0, 64, true);
	array.set(2048, 2112, true);
	REQUIRE(bx::kInvalid == array.findClear(0, 4096) );

	array.clear();
	REQUIRE(0 == array.findClear(0, 4096) );
	REQUIRE(array.testNone() );

	bx::FixedMippedBitArrayT<65> odd;
	odd.setCount(65);
	odd.set(0, 65, true);
	REQUIRE(bx::kInvalid == odd.findClear(0, 65) );
	odd.set(64, false);
	REQUIRE(64 == odd.findClear(0, 65) );
	odd.set(3, false);
	REQUIRE(3 == odd.findClear(0, 65) );
	REQUIRE(64 == odd.findClear(4, 65) );
}

TEST_CASE("MippedBitArray-dynamic", "[container]")
{
	bx::DefaultAllocator allocator;
	bx::MippedBitArray array(&allocator);

	REQUIRE(0 == array.getCapacity() );
	REQUIRE(bx::kInvalid == array.findClear(0, 0) );

	array.setCount(100);
	array.set(0, 100, true);
	REQUIRE(bx::kInvalid == array.findClear(0, 100) );

	array.setCount(200);
	REQUIRE(100 == array.findClear(0, 200) );
	array.set(100, 200, true);
	REQUIRE(bx::kInvalid == array.findClear(0, 200) );

	array.setCount(5000);
	REQUIRE(200 == array.findClear(0, 5000) );
	array.set(200, 5000, true);
	REQUIRE(bx::kInvalid == array.findClear(0, 5000) );
	REQUIRE(array.testAll() );

	array.set(4999, false);
	REQUIRE(4999 == array.findClear(0, 5000) );
	array.set(63, false);
	REQUIRE(63 == array.findClear(0, 5000) );
	REQUIRE(4999 == array.findClear(64, 5000) );

	array.setCapacity(128);
	REQUIRE(128 == array.getCapacity() );
	array.setCount(128);
	REQUIRE(63 == array.findClear(0, 128) );
	REQUIRE(bx::kInvalid == array.findClear(64, 128) );

	array.setCount(65);
	array.setCount(300);
	REQUIRE(array.get(64) );
	REQUIRE(65 == array.findClear(64, 300) );
	REQUIRE(bx::kInvalid == array.findClear(64, 65) );
}

TEST_CASE("BitArray-ranges-ending-on-word-boundary", "[container]")
{
	uint64_t words[4];
	bx::MutableBitArrayView bs(words, sizeof(words), 256);

	bs.clear();
	bs.set(0, 64, true);
	REQUIRE(UINT64_MAX == words[0]);
	REQUIRE(0 == words[1]);

	bs.clear();
	bs.set(10, 192, true);
	REQUIRE( (UINT64_MAX >> 10) == (words[0] >> 10) );
	REQUIRE(UINT64_MAX == words[1]);
	REQUIRE(UINT64_MAX == words[2]);
	REQUIRE(0 == words[3]);
	REQUIRE(182 == bs.countBits(10, 192) );
	REQUIRE(64  == bs.countBits(128, 192) );

	bs.set(0, 256, true);
	bs.set(0, 256, false);
	REQUIRE(bs.testNone() );

	bs.set(70, true);
	REQUIRE( bs.testAny(64, 128) );
	REQUIRE(!bs.testNone(64, 128) );
	REQUIRE(!bs.testAll(64, 128) );
	REQUIRE( bs.testNone(0, 64) );
	REQUIRE(!bs.testAny(0, 64) );
	REQUIRE(1 == bs.countBits(64, 128) );

	bs.set(127, true);
	REQUIRE( bs.testAny(64, 128) );
	REQUIRE(2 == bs.countBits(64, 128) );

	bs.set(64, 128, true);
	REQUIRE( bs.testAll(64, 128) );
	REQUIRE(!bs.testAll(0, 128) );

	bs.set(0, 256, true);
	REQUIRE( bs.testAll(0, 256) );
	REQUIRE(256 == bs.countBits(0, 256) );

	bs.set(255, false);
	REQUIRE(!bs.testAll(0, 256) );
	REQUIRE( bs.testAny(192, 256) );
	REQUIRE( bs.testAll(192, 255) );

	bs.set(192, 256, false);
	REQUIRE( bs.testNone(192, 256) );
	REQUIRE(!bs.testAny(192, 256) );
	REQUIRE( bs.testAll(0, 192) );

	bs.clear();
	bs.set(60, 124, UINT64_MAX);
	REQUIRE(0xf == (words[0] >> 60) );
	REQUIRE( (UINT64_MAX >> 4) == words[1]);
	REQUIRE(UINT64_MAX == bs.get(60, 124) );

	bs.clear();
	bs.set(64, 128, UINT64_C(0x1234567890abcdef) );
	REQUIRE(UINT64_C(0x1234567890abcdef) == words[1]);
	REQUIRE(UINT64_C(0x1234567890abcdef) == bs.get(64, 128) );
	REQUIRE(0 == words[0]);
	REQUIRE(0 == words[2]);

	bs.clear();
	bs.set(100, 164, UINT64_MAX);
	REQUIRE(UINT64_MAX == bs.get(100, 164) );
	REQUIRE( (UINT64_MAX >> 36) == (words[1] >> 36) );
	REQUIRE( (UINT64_MAX >> 28) == words[2]);
}
