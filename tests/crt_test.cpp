/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"

#include <bx/allocator.h>
#include <bx/rng.h>
#include <bx/string.h>

#include <string.h>

namespace bx
{
	void memCopyRef(void* _dst, const void* _src, size_t _numBytes);
	void memCopySimd(void* _dst, const void* _src, size_t _numBytes);
	void memMoveRef(void* _dst, const void* _src, size_t _numBytes);
	void memMoveSimd(void* _dst, const void* _src, size_t _numBytes);
	void memSetRef(void* _dst, uint8_t _ch, size_t _numBytes);
	void memSetSimd(void* _dst, uint8_t _ch, size_t _numBytes);
	int32_t memCmpRef(const void* _lhs, const void* _rhs, size_t _numBytes);
	int32_t memCmpSimd(const void* _lhs, const void* _rhs, size_t _numBytes);
}

TEST_CASE("memSet", "")
{
	char temp[] =  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };

	bx::memSet(temp, 0, 0);
	REQUIRE(temp[0] == 1);

	bx::memSet(temp, 0, 5);
	REQUIRE(temp[0] == 0);
	REQUIRE(temp[1] == 0);
	REQUIRE(temp[2] == 0);
	REQUIRE(temp[3] == 0);
	REQUIRE(temp[4] == 0);
	REQUIRE(temp[5] == 6);
}

TEST_CASE("memMove", "")
{
	const char* original = "xxxxabvgd";
	char str[] = { 'x', 'x', 'x', 'x', 'a', 'b', 'v', 'g', 'd' };

	bx::memMove(&str[4], &str[4], 0);
	REQUIRE(0 == bx::memCmp(str, original, 9) );

	bx::memMove(&str[4], &str[4], 5);
	REQUIRE(0 == bx::memCmp(str, original, 9) );

	bx::memMove(str, &str[4], 5);
	REQUIRE(0 == bx::memCmp(str, "abvgd", 5) );

	bx::memMove(&str[4], str, 5);
	REQUIRE(str[4] == 'a' );

	bx::memSet(str, 'x', 4);
	REQUIRE(0 == bx::memCmp(str, original, 9) );
}

TEST_CASE("scatter/gather", "")
{
	const char* str = "a\0b\0v\0g\0d";

	char tmp0[64];
	bx::gather(tmp0, str, 2, 1, 5);
	REQUIRE(0 == bx::memCmp(tmp0, "abvgd", 5) );

	char tmp1[64];
	bx::scatter(tmp1, 2, tmp0, 1, 5);
	bx::memSet(&tmp1[1], 2, 0, 1, 5);
	REQUIRE(0 == bx::memCmp(tmp1, str, 5) );

}

TEST_CASE("memCmpRef", "")
{
	const uint8_t lhs[] = { 1, 2, 0x80 };
	const uint8_t rhs[] = { 1, 2, 0x7f };

	REQUIRE(0 <  bx::memCmpRef(lhs, rhs, 3) );
	REQUIRE(0 >  bx::memCmpRef(rhs, lhs, 3) );
	REQUIRE(0 == bx::memCmpRef(lhs, rhs, 2) );
	REQUIRE(0 <  ::memcmp(lhs, rhs, 3) );
}

namespace
{
	constexpr uint32_t kMaxSize  = 300;
	constexpr uint32_t kGuard    = 32;
	constexpr uint32_t kMaxShift = 32;
	constexpr uint32_t kBuffer   = kGuard + kMaxShift + kMaxSize + kGuard;
	constexpr uint8_t  kGuardVal = 0xcd;

	void fillRandom(uint8_t* _data, uint32_t _num, bx::RngMwc& _rng)
	{
		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			_data[ii] = uint8_t(_rng.gen() );
		}
	}

	int32_t sign(int32_t _value)
	{
		return (0 < _value) - (0 > _value);
	}

	uint32_t mismatch(const uint8_t* _lhs, const uint8_t* _rhs, uint32_t _size)
	{
		return sign(bx::memCmpSimd(_lhs, _rhs, _size) ) != sign(::memcmp(_lhs, _rhs, _size) );
	}

} // namespace

TEST_CASE("memCopySimd", "")
{
	bx::RngMwc rng;

	uint8_t src[kBuffer];
	uint8_t dst[kBuffer];
	fillRandom(src, kBuffer, rng);

	for (uint32_t size = 0; size <= kMaxSize; ++size)
	{
		for (uint32_t srcOffset = 0; srcOffset < kMaxShift; ++srcOffset)
		{
			for (uint32_t dstOffset = 0; dstOffset < kMaxShift; ++dstOffset)
			{
				::memset(dst, kGuardVal, kBuffer);

				const uint8_t* from = src + kGuard + srcOffset;
				uint8_t*       to   = dst + kGuard + dstOffset;
				bx::memCopySimd(to, from, size);

				REQUIRE(0 == ::memcmp(to, from, size) );

				bool guardsIntact = true;
				for (uint32_t ii = 0; ii < kBuffer; ++ii)
				{
					const bool inside = dst + ii >= to && dst + ii < to + size;
					guardsIntact &= inside || kGuardVal == dst[ii];
				}

				REQUIRE(guardsIntact);
			}
		}
	}
}

TEST_CASE("memMoveSimd", "")
{
	bx::RngMwc rng;

	uint8_t initial[kBuffer];
	uint8_t actual[kBuffer];
	uint8_t expected[kBuffer];
	fillRandom(initial, kBuffer, rng);

	const uint32_t kMaxDistance = 40;

	for (uint32_t size = 0; size <= kMaxSize; size += 1 + size/32)
	{
		uint32_t numMismatch = 0;

		for (uint32_t srcPos = kGuard; srcPos <= kGuard + kMaxDistance; ++srcPos)
		{
			for (uint32_t dstPos = kGuard; dstPos <= kGuard + kMaxDistance; ++dstPos)
			{
				if (srcPos + size + kGuard > kBuffer
				||  dstPos + size + kGuard > kBuffer)
				{
					continue;
				}

				::memcpy(actual,   initial, kBuffer);
				::memcpy(expected, initial, kBuffer);

				bx::memMoveSimd(actual + dstPos, actual + srcPos, size);
				::memmove(expected + dstPos, expected + srcPos, size);

				numMismatch += 0 != ::memcmp(actual, expected, kBuffer);
			}
		}

		CAPTURE(size);
		REQUIRE(0 == numMismatch);
	}

	for (uint32_t size = 0; size <= 100; ++size)
	{
		::memcpy(actual,   initial, kBuffer);
		::memcpy(expected, initial, kBuffer);

		bx::memMoveSimd(actual + kBuffer - kGuard - size, actual + kGuard, size);
		::memmove(expected + kBuffer - kGuard - size, expected + kGuard, size);
		REQUIRE(0 == ::memcmp(actual, expected, kBuffer) );

		bx::memMoveSimd(actual + kGuard, actual + kBuffer - kGuard - size, size);
		::memmove(expected + kGuard, expected + kBuffer - kGuard - size, size);
		REQUIRE(0 == ::memcmp(actual, expected, kBuffer) );
	}
}

TEST_CASE("memSetSimd", "")
{
	const uint8_t values[] = { 0x00, 0x01, 0x7f, 0x80, 0xa5, 0xff };

	uint8_t dst[kBuffer];

	for (uint8_t value : values)
	{
		for (uint32_t size = 0; size <= kMaxSize; ++size)
		{
			for (uint32_t offset = 0; offset < kMaxShift; ++offset)
			{
				::memset(dst, kGuardVal, kBuffer);

				uint8_t* to = dst + kGuard + offset;
				bx::memSetSimd(to, value, size);

				bool filled = true;
				for (uint32_t ii = 0; ii < kBuffer; ++ii)
				{
					const bool inside = dst + ii >= to && dst + ii < to + size;
					filled &= (inside ? value : kGuardVal) == dst[ii];
				}

				REQUIRE(filled);
			}
		}
	}
}

TEST_CASE("memCmpSimd", "")
{
	bx::RngMwc rng;

	uint8_t lhs[kBuffer];
	uint8_t rhs[kBuffer];
	fillRandom(lhs, kBuffer, rng);

	const uint32_t offsets[] = { 0, 1, 7, 15, 31 };

	for (uint32_t size = 0; size <= kMaxSize; ++size)
	{
		uint32_t numMismatch = 0;

		for (uint32_t lhsOffset : offsets)
		{
			for (uint32_t rhsOffset : offsets)
			{
				uint8_t* left  = lhs + kGuard + lhsOffset;
				uint8_t* right = rhs + kGuard + rhsOffset;

				fillRandom(rhs, kBuffer, rng);
				::memcpy(right, left, size);

				numMismatch += 0 != bx::memCmpSimd(left, right, size);

				for (uint32_t pos = 0; pos < size; ++pos)
				{
					const uint8_t saved = right[pos];

					right[pos] = uint8_t(left[pos] ^ 0x80);
					numMismatch += mismatch(left, right, size);
					numMismatch += mismatch(right, left, size);

					right[pos] = uint8_t(left[pos] + 1);
					numMismatch += mismatch(left, right, size);

					if (pos + 1 < size)
					{
						right[size - 1] = uint8_t(left[size - 1] - 1);
						numMismatch += mismatch(left, right, size);
						right[size - 1] = left[size - 1];
					}

					right[pos] = saved;
				}
			}
		}

		CAPTURE(size);
		REQUIRE(0 == numMismatch);
	}
}

TEST_CASE("memSimd benchmark", "[.][!benchmark]")
{
	const uint32_t sizes[] = { 7, 16, 64, 256, 4<<10, 64<<10, 1<<20 };
	const uint32_t kMax = 1<<20;

	static uint8_t s_src[kMax + 4096];
	static uint8_t s_dst[kMax + 8192];
	uint8_t* src = (uint8_t*)bx::alignPtr(s_src, 0, 4096) + 1;
	uint8_t* dst = (uint8_t*)bx::alignPtr(s_dst, 0, 4096) + 2048 + 3;

	::memset(src, 0x5a, kMax);
	::memset(dst, 0x5a, kMax);

	char name[64];

	for (uint32_t size : sizes)
	{
		const uint32_t moveSize = size - 5;

		bx::snprintf(name, sizeof(name), "::memcpy %u", size);
		BENCHMARK(name) { ::memcpy(dst, src, size); return dst[0]; };
		bx::snprintf(name, sizeof(name), "bx::memCopyRef %u", size);
		BENCHMARK(name) { bx::memCopyRef(dst, src, size); return dst[0]; };
		bx::snprintf(name, sizeof(name), "bx::memCopySimd %u", size);
		BENCHMARK(name) { bx::memCopySimd(dst, src, size); return dst[0]; };
		bx::snprintf(name, sizeof(name), "bx::memCopy %u", size);
		BENCHMARK(name) { bx::memCopy(dst, src, size); return dst[0]; };

		bx::snprintf(name, sizeof(name), "::memmove %u", size);
		BENCHMARK(name) { ::memmove(dst, dst + 5, moveSize); return dst[0]; };
		bx::snprintf(name, sizeof(name), "bx::memMoveRef %u", size);
		BENCHMARK(name) { bx::memMoveRef(dst, dst + 5, moveSize); return dst[0]; };
		bx::snprintf(name, sizeof(name), "bx::memMoveSimd %u", size);
		BENCHMARK(name) { bx::memMoveSimd(dst, dst + 5, moveSize); return dst[0]; };
		bx::snprintf(name, sizeof(name), "bx::memMove %u", size);
		BENCHMARK(name) { bx::memMove(dst, dst + 5, moveSize); return dst[0]; };

		bx::snprintf(name, sizeof(name), "::memset %u", size);
		BENCHMARK(name) { ::memset(dst, 0x5a, size); return dst[0]; };
		bx::snprintf(name, sizeof(name), "bx::memSetRef %u", size);
		BENCHMARK(name) { bx::memSetRef(dst, 0x5a, size); return dst[0]; };
		bx::snprintf(name, sizeof(name), "bx::memSetSimd %u", size);
		BENCHMARK(name) { bx::memSetSimd(dst, 0x5a, size); return dst[0]; };
		bx::snprintf(name, sizeof(name), "bx::memSet %u", size);
		BENCHMARK(name) { bx::memSet(dst, 0x5a, size); return dst[0]; };

		bx::snprintf(name, sizeof(name), "::memcmp %u", size);
		BENCHMARK(name) { return ::memcmp(dst, src, size); };
		bx::snprintf(name, sizeof(name), "bx::memCmpRef %u", size);
		BENCHMARK(name) { return bx::memCmpRef(dst, src, size); };
		bx::snprintf(name, sizeof(name), "bx::memCmpSimd %u", size);
		BENCHMARK(name) { return bx::memCmpSimd(dst, src, size); };
		bx::snprintf(name, sizeof(name), "bx::memCmp %u", size);
		BENCHMARK(name) { return bx::memCmp(dst, src, size); };
	}
}
