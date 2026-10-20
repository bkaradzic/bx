/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "test.h"

#include <bx/allocator.h>
#include <bx/bitarray.h>
#include <bx/math.h>
#include <bx/rng.h>
#include <bx/string.h>

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

namespace bx
{
	uint32_t bitSetFindRef(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits);
	uint32_t bitSetFindSimd(const uint64_t* _ptr, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits);
	uint32_t bitSetFindWordRef(const uint64_t* _ptr, uint32_t _beginWord, uint32_t _endWord, uint32_t _match);
	uint32_t bitSetFindWordSimd(const uint64_t* _ptr, uint32_t _beginWord, uint32_t _endWord, uint32_t _match);
	uint32_t bitSetFindLastWordRef(const uint64_t* _ptr, uint32_t _beginWord, uint32_t _endWord, uint32_t _match);
	uint32_t bitSetFindLastWordSimd(const uint64_t* _ptr, uint32_t _beginWord, uint32_t _endWord, uint32_t _match);
}

static bool getBit(const uint64_t* _bits, uint32_t _idx)
{
	return 0 != ( (_bits[_idx/64] >> (_idx & 63) ) & 1);
}

static uint32_t findFirstFit(const uint64_t* _bits, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
{
	uint32_t run = 0;

	for (uint32_t ii = _beginBit; ii < _endBit; ++ii)
	{
		run = getBit(_bits, ii) ? 0 : run + 1;

		if (run == _numBits)
		{
			return ii + 1 - _numBits;
		}
	}

	return bx::kInvalid;
}

static uint32_t findRef(const bx::MutableBitArrayView& _bs, uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits)
{
	for (uint32_t ii = _beginBit; ii + _numBits <= _endBit; ++ii)
	{
		if (_bs.testNone(ii, ii + _numBits) )
		{
			return ii;
		}
	}

	return bx::kInvalid;
}

TEST_CASE("BitArray-find-random", "[container]")
{
	constexpr uint32_t kNumBits = 1024;

	uint64_t bits[kNumBits/64];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );

	bx::RngMwc rng;

	for (uint32_t iter = 0; iter < 20000; ++iter)
	{
		const uint32_t density = rng.gen() % 4;

		bs.clear();

		for (uint32_t ii = 0, num = 4 + rng.gen() % 64; ii < num; ++ii)
		{
			const uint32_t begin = rng.gen() % kNumBits;
			const uint32_t end   = bx::min(kNumBits, begin + 1 + rng.gen() % (8 << density) );
			bs.set(begin, end, true);
		}

		for (uint32_t ii = 0, num = rng.gen() % 32; ii < num; ++ii)
		{
			const uint32_t begin = rng.gen() % kNumBits;
			const uint32_t end   = bx::min(kNumBits, begin + 1 + rng.gen() % 64);
			bs.set(begin, end, false);
		}

		const uint32_t numBits  = 1 + rng.gen() % 160;
		const bool     full     = 0 == (rng.gen() & 3);
		const uint32_t beginBit = full ? 0        : rng.gen() % (kNumBits - numBits);
		const uint32_t endBit   = full ? kNumBits : beginBit + numBits + rng.gen() % (kNumBits - beginBit - numBits + 1);

		const uint32_t expected = findRef(bs, beginBit, endBit, numBits);
		const uint32_t idx      = full ? bs.find(numBits) : bs.find(beginBit, endBit, numBits);

		INFO("Iteration " << iter << ", range [" << beginBit << ", " << endBit << "), num bits " << numBits << ", found " << idx << ", expected " << expected);

		REQUIRE( (bx::kInvalid == idx) == (bx::kInvalid == expected) );

		if (bx::kInvalid != idx)
		{
			REQUIRE(beginBit <= idx);
			REQUIRE(idx + numBits <= endBit);
			REQUIRE(bs.testNone(idx, idx + numBits) );
		}
	}
}

TEST_CASE("BitArray-find-skips-full-byte", "[container]")
{
	uint64_t bits[1];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );

	bs.set(0, 64, true);
	bs.set(5, 8, false);
	bs.set(16, 23, false);

	REQUIRE(bx::kInvalid == bs.find(10) );
	REQUIRE(16 == bs.find(7) );
}

TEST_CASE("BitArray-find-bench", "[.][!benchmark]")
{
	constexpr uint32_t kNumBits = 4096;

	uint64_t bits[kNumBits/64];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );

	bx::RngMwc rng;

	for (uint32_t ii = 0; ii < kNumBits; ii += 1 + rng.gen() % 12)
	{
		bs.set(ii, true);
	}

	bs.set(kNumBits - 256, kNumBits, false);

	char name[64];

	for (uint32_t numBits : { 1u, 2u, 4u, 6u, 7u, 12u, 14u, 15u, 24u, 31u, 40u, 63u, 64u, 127u, 200u, 300u })
	{
		uint32_t idx = 0;

		bx::snprintf(name, sizeof(name), "find %u", numBits);
		BENCHMARK(name)
		{
			idx = bs.find(numBits);
			return idx;
		};

		bx::snprintf(name, sizeof(name), "find ref %u", numBits);
		BENCHMARK(name)
		{
			return bx::bitSetFindRef(bits, 0, kNumBits, numBits);
		};

		REQUIRE( (bx::kInvalid == idx) == (bx::kInvalid == findRef(bs, 0, kNumBits, numBits) ) );
	}
}

TEST_CASE("BitArray-find-simd-matches-ref", "[container]")
{
	constexpr uint32_t kMaxWords = 80;

	uint64_t bits[kMaxWords];

	bx::RngMwc rng;

	for (uint32_t iter = 0; iter < 50000; ++iter)
	{
		const uint32_t numWords = 1 + rng.gen() % kMaxWords;
		const uint32_t numBits  = numWords*64;
		bx::MutableBitArrayView bs(bits, numWords*sizeof(uint64_t) );

		const uint32_t density = rng.gen() % 6;

		bs.clear();

		for (uint32_t ii = 0, num = rng.gen() % (4 + numWords*4); ii < num; ++ii)
		{
			const uint32_t begin = rng.gen() % numBits;
			const uint32_t end   = bx::min(numBits, begin + 1 + rng.gen() % (4 << density) );
			bs.set(begin, end, true);
		}

		if (0 == (rng.gen() & 3) )
		{
			bs.set(0, numBits, true);

			for (uint32_t ii = 0, num = rng.gen() % 4; ii < num; ++ii)
			{
				const uint32_t begin = rng.gen() % numBits;
				const uint32_t end   = bx::min(numBits, begin + 1 + rng.gen() % 320);
				bs.set(begin, end, false);
			}
		}

		const uint32_t runBits  = 0 == (rng.gen() & 1) ? 1 + rng.gen() % 8 : 1 + rng.gen() % 300;
		const uint32_t beginBit = rng.gen() % numBits;
		const uint32_t endBit   = beginBit + 1 + rng.gen() % (numBits - beginBit);

		const uint32_t ref  = bx::bitSetFindRef (bits, beginBit, endBit, runBits);
		const uint32_t simd = bx::bitSetFindSimd(bits, beginBit, endBit, runBits);

		INFO("Iteration " << iter << ", words " << numWords << ", range [" << beginBit << ", " << endBit << "), num bits " << runBits << ", simd " << simd << ", ref " << ref);

		REQUIRE(ref == simd);
		REQUIRE(findFirstFit(bits, beginBit, endBit, runBits) == simd);
	}
}

TEST_CASE("BitArray-ops-bench", "[.][!benchmark]")
{
	constexpr uint32_t kNumBits = 64<<10;

	alignas(64) static uint64_t bits[kNumBits/64];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );

	char name[64];

	for (uint32_t numBits : { 7u, 64u, 300u, 4096u })
	{
		bs.clear();

		bx::snprintf(name, sizeof(name), "set %u", numBits);
		BENCHMARK(name)
		{
			bs.set(13, 13 + numBits, true);
			bs.set(13, 13 + numBits, false);
			return bits[0];
		};
	}

	bs.clear();

	BENCHMARK("countBits 64K")
	{
		return bs.countBits(1, kNumBits - 1);
	};

	BENCHMARK("testNone 64K")
	{
		return bs.testNone(1, kNumBits - 1);
	};

	BENCHMARK("testAny 64K")
	{
		return bs.testAny(1, kNumBits - 1);
	};

	BENCHMARK("findSet first 64K")
	{
		return bs.findSet(bx::kFirst);
	};

	BENCHMARK("findSet range 64K")
	{
		return bs.findSet(1, kNumBits - 1);
	};

	bs.set(0, kNumBits, true);

	BENCHMARK("testAll 64K")
	{
		return bs.testAll(1, kNumBits - 1);
	};

	BENCHMARK("findClear range 64K")
	{
		return bs.findClear(1, kNumBits - 1);
	};

	BENCHMARK("find 1 64K")
	{
		return bs.find(1);
	};

	BENCHMARK("find 12 64K")
	{
		return bs.find(12);
	};

	bx::RngMwc rng;

	for (uint32_t ii = 0; ii < kNumBits; ++ii)
	{
		bits[ii/64] = (uint64_t(rng.gen() ) << 32) | rng.gen();
	}

	BENCHMARK("countBits random 64K")
	{
		return bs.countBits(1, kNumBits - 1);
	};

	for (uint32_t numBits : { 300u, 4096u })
	{
		bx::snprintf(name, sizeof(name), "blit %u", numBits);
		BENCHMARK(name)
		{
			bs.blit(1000, 3, numBits);
			return bits[20];
		};

		bx::snprintf(name, sizeof(name), "blit aligned %u", numBits);
		BENCHMARK(name)
		{
			bs.blit(1024, 128, numBits);
			return bits[20];
		};
	}
}

static void fillRandom(bx::RngMwc& _rng, uint64_t* _bits, uint32_t _numWords)
{
	const uint32_t mode = _rng.gen() % 4;

	for (uint32_t ii = 0; ii < _numWords; ++ii)
	{
		const uint64_t word = (uint64_t(_rng.gen() ) << 32) | _rng.gen();
		const uint32_t pick = _rng.gen() % 8;

		switch (mode)
		{
		case 0:  _bits[ii] = word; break;
		case 1:  _bits[ii] = 0 == pick ? word : 0; break;
		case 2:  _bits[ii] = 0 == pick ? word : UINT64_MAX; break;
		default: _bits[ii] = 0 == pick ? word : 1 == pick ? 0 : UINT64_MAX; break;
		}
	}
}

TEST_CASE("BitArray-find-word-simd-matches-ref", "[container]")
{
	constexpr uint32_t kMaxWords = 80;

	uint64_t bits[kMaxWords];

	bx::RngMwc rng;

	for (uint32_t iter = 0; iter < 50000; ++iter)
	{
		const uint32_t numWords = 1 + rng.gen() % kMaxWords;
		fillRandom(rng, bits, numWords);

		const uint32_t begin = rng.gen() % (numWords + 1);
		const uint32_t end   = begin + rng.gen() % (numWords - begin + 1);
		const uint32_t match = rng.gen() % 3;

		INFO("Iteration " << iter << ", words " << numWords << ", range [" << begin << ", " << end << "), match " << match);

		REQUIRE(bx::bitSetFindWordRef(bits, begin, end, match) == bx::bitSetFindWordSimd(bits, begin, end, match) );
		REQUIRE(bx::bitSetFindLastWordRef(bits, begin, end, match) == bx::bitSetFindLastWordSimd(bits, begin, end, match) );
	}
}

TEST_CASE("BitArray-range-ops-random", "[container]")
{
	constexpr uint32_t kMaxWords = 80;

	uint64_t bits[kMaxWords];

	bx::RngMwc rng;

	for (uint32_t iter = 0; iter < 50000; ++iter)
	{
		const uint32_t numWords = 1 + rng.gen() % kMaxWords;
		const uint32_t numBits  = numWords*64;
		fillRandom(rng, bits, numWords);

		bx::MutableBitArrayView bs(bits, numWords*sizeof(uint64_t) );

		const uint32_t beginBit = rng.gen() % (numBits + 1);
		const uint32_t endBit   = beginBit + rng.gen() % (numBits - beginBit + 1);

		uint32_t count    = 0;
		uint32_t firstSet = bx::kInvalid;
		uint32_t firstClr = bx::kInvalid;

		for (uint32_t ii = beginBit; ii < endBit; ++ii)
		{
			const bool bit = getBit(bits, ii);
			count += bit;
			firstSet = bit  && bx::kInvalid == firstSet ? ii : firstSet;
			firstClr = !bit && bx::kInvalid == firstClr ? ii : firstClr;
		}

		const uint32_t numRange = endBit - beginBit;

		INFO("Iteration " << iter << ", words " << numWords << ", range [" << beginBit << ", " << endBit << ")");

		REQUIRE(count == bs.countBits(beginBit, endBit) );
		REQUIRE( (0 != count) == bs.testAny(beginBit, endBit) );
		REQUIRE( (0 == count) == bs.testNone(beginBit, endBit) );
		REQUIRE( (numRange == count) == bs.testAll(beginBit, endBit) );
		REQUIRE(firstSet == bs.findSet(beginBit, endBit) );
		REQUIRE(firstClr == bs.findClear(beginBit, endBit) );

		uint32_t first = bx::kInvalid;
		uint32_t last  = bx::kInvalid;

		for (uint32_t ii = 0; ii < numBits; ++ii)
		{
			if (getBit(bits, ii) )
			{
				first = bx::kInvalid == first ? ii : first;
				last  = ii;
			}
		}

		REQUIRE(first == bs.findSet(bx::kFirst) );
		REQUIRE(last  == bs.findSet(bx::kLast) );
	}
}

TEST_CASE("BitArray-blit-random", "[container]")
{
	constexpr uint32_t kNumWords = 40;
	constexpr uint32_t kNumBits  = kNumWords*64;

	uint64_t bits[kNumWords];
	uint64_t expected[kNumWords];

	bx::RngMwc rng;

	for (uint32_t iter = 0; iter < 50000; ++iter)
	{
		fillRandom(rng, bits, kNumWords);
		bx::memCopy(expected, bits, sizeof(bits) );

		const uint32_t numBits = 1 + rng.gen() % (0 == (rng.gen() & 1) ? 160 : kNumBits/2);
		const uint32_t srcBit  = rng.gen() % (kNumBits - numBits + 1);
		const uint32_t dstBit  = 0 == (rng.gen() & 3)
			? bx::min(kNumBits - numBits, (srcBit & ~7u) + (rng.gen() % 64)*8)
			: rng.gen() % (kNumBits - numBits + 1)
			;

		bool tmp[kNumBits];

		for (uint32_t ii = 0; ii < numBits; ++ii)
		{
			tmp[ii] = getBit(expected, srcBit + ii);
		}

		for (uint32_t ii = 0; ii < numBits; ++ii)
		{
			const uint32_t idx = dstBit + ii;
			expected[idx/64] = (expected[idx/64] & ~(UINT64_C(1) << (idx & 63) ) ) | (uint64_t(tmp[ii]) << (idx & 63) );
		}

		bx::MutableBitArrayView bs(bits, sizeof(bits) );
		bs.blit(dstBit, srcBit, numBits);

		INFO("Iteration " << iter << ", dst " << dstBit << ", src " << srcBit << ", num bits " << numBits);

		REQUIRE(0 == bx::memCmp(bits, expected, sizeof(bits) ) );
	}
}

TEST_CASE("BitArray-set-random", "[container]")
{
	constexpr uint32_t kNumWords = 40;
	constexpr uint32_t kNumBits  = kNumWords*64;

	uint64_t bits[kNumWords];
	uint64_t expected[kNumWords];

	bx::RngMwc rng;

	for (uint32_t iter = 0; iter < 50000; ++iter)
	{
		fillRandom(rng, bits, kNumWords);
		bx::memCopy(expected, bits, sizeof(bits) );

		const uint32_t beginBit = rng.gen() % (kNumBits + 1);
		const uint32_t endBit   = beginBit + rng.gen() % (kNumBits - beginBit + 1);
		const bool     value    = 0 != (rng.gen() & 1);

		for (uint32_t idx = beginBit; idx < endBit; ++idx)
		{
			expected[idx/64] = (expected[idx/64] & ~(UINT64_C(1) << (idx & 63) ) ) | (uint64_t(value) << (idx & 63) );
		}

		bx::MutableBitArrayView bs(bits, sizeof(bits) );
		bs.set(beginBit, endBit, value);

		INFO("Iteration " << iter << ", range [" << beginBit << ", " << endBit << "), value " << value);

		REQUIRE(0 == bx::memCmp(bits, expected, sizeof(bits) ) );
	}
}

TEST_CASE("MippedBitArray-random", "[container]")
{
	constexpr uint32_t kNumBits  = 64*130;
	constexpr uint32_t kNumWords = kNumBits/64;

	bx::FixedMippedBitArrayT<kNumBits> array;
	array.setCount(kNumBits);

	bx::RngMwc rng;

	for (uint32_t iter = 0; iter < 20000; ++iter)
	{
		const uint32_t op       = rng.gen() % 6;
		const uint32_t beginBit = rng.gen() % kNumBits;
		const uint32_t numBits  = 1 + rng.gen() % (0 == (rng.gen() & 3) ? kNumBits - beginBit : bx::min<uint32_t>(kNumBits - beginBit, 300) );
		const uint32_t endBit   = beginBit + numBits;

		switch (op)
		{
		case 0:  array.set(beginBit, endBit, true);  break;
		case 1:  array.set(beginBit, endBit, false); break;
		case 2:  array.set(beginBit, 0 != (rng.gen() & 1) ); break;
		case 3:  array.set(beginBit, bx::min(endBit, beginBit + 64), (uint64_t(rng.gen() ) << 32) | rng.gen() ); break;
		case 4:  array.blit(rng.gen() % (kNumBits - numBits + 1), beginBit, numBits); break;
		default: if (0 == rng.gen() % 64) { array.clear(); } break;
		}

		const uint64_t* words = (const uint64_t*)array.getPtr();
		const uint64_t* mip   = array.getMip();

		INFO("Iteration " << iter << ", op " << op << ", range [" << beginBit << ", " << endBit << ")");

		for (uint32_t ii = 0; ii < kNumWords; ++ii)
		{
			const bool full    = UINT64_MAX == words[ii];
			const bool mipFull = 0 != ( (mip[ii/64] >> (ii & 63) ) & 1);
			REQUIRE(full == mipFull);
		}

		for (uint32_t ii = kNumWords; ii < bx::alignUp(kNumWords, 64u); ++ii)
		{
			REQUIRE(0 != ( (mip[ii/64] >> (ii & 63) ) & 1) );
		}

		const uint32_t findBegin = rng.gen() % kNumBits;
		uint32_t expected = bx::kInvalid;

		for (uint32_t ii = findBegin; ii < kNumBits && bx::kInvalid == expected; ++ii)
		{
			expected = getBit(words, ii) ? bx::kInvalid : ii;
		}

		REQUIRE(expected == array.findClear(findBegin, kNumBits) );
	}
}

TEST_CASE("BitArray-iterator", "[container]")
{
	constexpr uint32_t kNumWords = 40;
	constexpr uint32_t kNumBits  = kNumWords*64;

	uint64_t bits[kNumWords];
	bx::MutableBitArrayView bs(bits, sizeof(bits) );

	bs.clear();

	{
		uint32_t numVisited = 0;

		for (const uint32_t idx : bs)
		{
			BX_UNUSED(idx);
			++numVisited;
		}

		REQUIRE(0 == numVisited);
	}

	bs.set(0, true);
	bs.set(63, true);
	bs.set(64, true);
	bs.set(1000, true);
	bs.set(kNumBits - 1, true);

	{
		const uint32_t expected[] = { 0, 63, 64, 1000, kNumBits - 1 };

		uint32_t numVisited = 0;

		for (const uint32_t idx : bs)
		{
			REQUIRE(numVisited < BX_COUNTOF(expected) );
			REQUIRE(expected[numVisited] == idx);
			++numVisited;
		}

		REQUIRE(BX_COUNTOF(expected) == numVisited);
	}

	bx::RngMwc rng;

	for (uint32_t iter = 0; iter < 1000; ++iter)
	{
		fillRandom(rng, bits, kNumWords);

		const uint32_t count = 1 + rng.gen() % kNumBits;
		bx::MutableBitArrayView view(bits, sizeof(bits), count);

		uint32_t expected = view.findSet(0, count);

		for (const uint32_t idx : view)
		{
			REQUIRE(expected == idx);
			REQUIRE(view.get(idx) );
			expected = view.findSet(idx + 1, count);
		}

		REQUIRE(bx::kInvalid == expected);
	}

	fillRandom(rng, bits, kNumWords);

	{
		const uint32_t numSet = bs.countBits();

		uint32_t numVisited = 0;

		for (const uint32_t idx : bs)
		{
			++numVisited;

			if (0 == (idx & 1) )
			{
				bs.set(idx, false);
			}
		}

		REQUIRE(numSet == numVisited);

		for (const uint32_t idx : bs)
		{
			REQUIRE(1 == (idx & 1) );
			bs.set(idx, false);
		}

		REQUIRE(bs.testNone() );
	}
}

template<typename MaskT>
static void testBitMaskToIndexIterator(MaskT _mask)
{
	uint32_t numVisited = 0;
	uint32_t prevIdx    = 0;

	for (bx::BitMaskToIndexIteratorT<MaskT> it(_mask); !it.isDone(); it.next() )
	{
		const uint32_t idx = it.getIndex();

		REQUIRE(idx < sizeof(MaskT)*8);
		REQUIRE( (0 == numVisited || prevIdx < idx) );
		REQUIRE(0 != ( (_mask >> idx) & 1) );
		REQUIRE(1 == (it.getMask() & 1) );

		prevIdx = idx;
		++numVisited;
	}

	REQUIRE(bx::countBits<uint64_t>(uint64_t(_mask) ) == numVisited);
}

TEST_CASE("BitMaskToIndexIteratorT", "[container]")
{
	testBitMaskToIndexIterator<uint8_t>(0);
	testBitMaskToIndexIterator<uint8_t>(1);
	testBitMaskToIndexIterator<uint8_t>(0x80);
	testBitMaskToIndexIterator<uint8_t>(0xff);
	testBitMaskToIndexIterator<uint8_t>(0xa5);
	testBitMaskToIndexIterator<uint16_t>(0x8001);
	testBitMaskToIndexIterator<uint32_t>(0);
	testBitMaskToIndexIterator<uint32_t>(0x80000001);
	testBitMaskToIndexIterator<uint32_t>(UINT32_MAX);
	testBitMaskToIndexIterator<uint64_t>(0);
	testBitMaskToIndexIterator<uint64_t>(1);
	testBitMaskToIndexIterator<uint64_t>(UINT64_C(0x8000000000000000) );
	testBitMaskToIndexIterator<uint64_t>(UINT64_MAX);

	bx::RngMwc rng;

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		testBitMaskToIndexIterator<uint32_t>(rng.gen() );
		testBitMaskToIndexIterator<uint64_t>( (uint64_t(rng.gen() ) << 32) | rng.gen() );
	}
}
