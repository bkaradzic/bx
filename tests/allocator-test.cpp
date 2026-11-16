/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"

#include <bx/allocator.h>
#include <bx/allocators.h>
#include <bx/heap.h>

struct MockNonFreeingAllocator : public bx::AllocatorI
{
	MockNonFreeingAllocator()
		: m_sbrk(1)
	{
	}

	~MockNonFreeingAllocator() override
	{
	}

	void* realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line) override
	{
		BX_ASSERT(_ptr == NULL, "MockNonFreeingAllocator can't realloc or free.");
		BX_ASSERT(m_sbrk + _size < BX_COUNTOF(m_storage), "");
		BX_UNUSED(_ptr, _file, _line);

		const uintptr_t sbrk = bx::alignUp(m_sbrk, bx::max(int32_t(_align), 1) );
		m_sbrk = sbrk + _size;

		return &m_storage[sbrk];
	}

	uintptr_t m_sbrk;
	BX_ALIGN_DECL(256, uint8_t) m_storage[0x10000];
};

bool testAlignment(size_t _expected, void* _ptr)
{
	bool aligned = bx::isAligned(_ptr, int32_t(_expected) );
//	BX_TRACE("%p, %d", _ptr, _expected);
	BX_WARN(aligned, "%p not aligned to %d bytes.", _ptr, _expected);
	return aligned;
}

TEST_CASE("Allocator", "")
{
	MockNonFreeingAllocator mnfa;

	REQUIRE(testAlignment(1,   bx::alloc       (&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(2,   bx::alloc       (&mnfa, 1, 2  ) ) );
	REQUIRE(testAlignment(1,   bx::alloc       (&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(4,   bx::alloc       (&mnfa, 1, 4  ) ) );
	REQUIRE(testAlignment(1,   bx::alloc       (&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(8,   bx::alloc       (&mnfa, 1, 8  ) ) );
	REQUIRE(testAlignment(1,   bx::alloc       (&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(16,  bx::alloc       (&mnfa, 1, 16 ) ) );
	REQUIRE(testAlignment(1,   bx::alloc       (&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(32,  bx::alloc       (&mnfa, 1, 32 ) ) );
	REQUIRE(testAlignment(1,   bx::alloc       (&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(64,  bx::alloc       (&mnfa, 1, 64 ) ) );
	REQUIRE(testAlignment(1,   bx::alloc       (&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(128, bx::alloc       (&mnfa, 1, 128) ) );

	REQUIRE(testAlignment(1,   bx::alignedAlloc(&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(2,   bx::alignedAlloc(&mnfa, 1, 2  ) ) );
	REQUIRE(testAlignment(1,   bx::alignedAlloc(&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(4,   bx::alignedAlloc(&mnfa, 1, 4  ) ) );
	REQUIRE(testAlignment(1,   bx::alignedAlloc(&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(8,   bx::alignedAlloc(&mnfa, 1, 8  ) ) );
	REQUIRE(testAlignment(1,   bx::alignedAlloc(&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(16,  bx::alignedAlloc(&mnfa, 1, 16 ) ) );
	REQUIRE(testAlignment(1,   bx::alignedAlloc(&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(32,  bx::alignedAlloc(&mnfa, 1, 32 ) ) );
	REQUIRE(testAlignment(1,   bx::alignedAlloc(&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(64,  bx::alignedAlloc(&mnfa, 1, 64 ) ) );
	REQUIRE(testAlignment(1,   bx::alignedAlloc(&mnfa, 1, 1  ) ) );
	REQUIRE(testAlignment(128, bx::alignedAlloc(&mnfa, 1, 128) ) );
}

namespace
{
	struct PtrObject
	{
		PtrObject(int32_t* _numLive)
			: m_numLive(_numLive)
		{
			++*m_numLive;
		}

		~PtrObject()
		{
			--*m_numLive;
		}

		int32_t* m_numLive;
	};

} // namespace

TEST_CASE("allocPtr", "[blk]")
{
	bx::CrtAllocator allocator;

	for (size_t alignment : { size_t(0), size_t(8), size_t(16), size_t(64), size_t(4096) })
	{
		uint8_t* ptr = (uint8_t*)bx::allocPtr(&allocator, 100, alignment);
		REQUIRE(ptr != (uint8_t*)NULL);
		REQUIRE(bx::isAligned(ptr, bx::max<size_t>(alignment, 16) ) );

		for (uint32_t ii = 0; ii < 100; ++ii)
		{
			ptr[ii] = uint8_t(ii);
		}

		ptr = (uint8_t*)bx::reallocPtr(&allocator, ptr, 100000);
		REQUIRE(bx::isAligned(ptr, bx::max<size_t>(alignment, 16) ) );

		bool same = true;
		for (uint32_t ii = 0; ii < 100; ++ii)
		{
			same &= ptr[ii] == uint8_t(ii);
		}
		REQUIRE(same);

		bx::memSet(ptr, 0xcd, 100000);

		ptr = (uint8_t*)bx::reallocPtr(&allocator, ptr, 10);
		REQUIRE(0xcd == ptr[9]);

		REQUIRE(bx::reallocPtr(&allocator, ptr, 0) == (void*)NULL);
	}

	REQUIRE(bx::reallocPtr(&allocator, NULL, 0) == (void*)NULL);

	void* fromNull = bx::reallocPtr(&allocator, NULL, 32);
	REQUIRE(fromNull != (void*)NULL);
	bx::freePtr(fromNull);

	bx::freePtr(NULL);
}

namespace
{
	uint8_t allocatorHandleNext()
	{
		bx::CrtAllocator allocator;
		return allocator.getHandle().idx;
	}

} // namespace

TEST_CASE("Allocator handle reused in turn", "[blk]")
{
	const uint8_t first  = allocatorHandleNext();
	const uint8_t second = allocatorHandleNext();
	REQUIRE(first != second);
	REQUIRE(!bx::isAllocatorPermanent({ first }) );
	REQUIRE(!bx::isAllocatorPermanent({ bx::kAllocatorHandleDefault }) );
}

TEST_CASE("Allocator handle permanent", "[blk]")
{
	bx::AllocatorHandle handle;

	{
		bx::CrtAllocator permanent;
		handle = bx::setAllocatorPermanent(&permanent)->getHandle();
		REQUIRE(bx::isAllocatorPermanent(handle) );
	}

	bool reused = false;
	for (uint32_t ii = 0; ii < bx::kMaxAllocatorHandles; ++ii)
	{
		reused |= handle.idx == allocatorHandleNext();
	}

	REQUIRE(!reused);
	REQUIRE(bx::isAllocatorPermanent(handle) );
	REQUIRE(bx::getHeapAllocator() == bx::getAllocator(bx::getHeapAllocatorHandle() ) );
	REQUIRE(bx::isAllocatorPermanent(bx::getHeapAllocatorHandle() ) );
}

TEST_CASE("BX_NEW_PTR and deletePtr", "[blk]")
{
	bx::CrtAllocator allocator;
	int32_t numLive = 0;

	PtrObject* object = BX_NEW_PTR(&allocator, PtrObject)(&numLive);
	REQUIRE(1 == numLive);

	bx::deletePtr(object);
	REQUIRE(0 == numLive);

	object = NULL;
	bx::deletePtr(object);
}
