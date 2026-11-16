/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"

#include <bx/allocators.h>
#include <bx/math.h>
#include <bx/uniqueptr.h>

namespace
{
	struct CountingAllocator : public bx::AllocatorBlkI
	{
		bx::Blk reallocImpl(bx::Blk&& _blk, size_t _size, size_t _alignment) override
		{
			const bx::AllocatorOp::Enum op = bx::getAllocatorOp(_blk, _size, _alignment);

			if (bx::AllocatorOp::Allocate == op)
			{
				++m_numAllocs;
			}
			else if (bx::AllocatorOp::Free == op)
			{
				++m_numFrees;
			}

			return m_crt.realloc(bx::move(_blk), _size, _alignment);
		}

		bool owns(const bx::Blk& _blk) override
		{
			return m_crt.owns(_blk);
		}

		bool isCpuAccessible() const override
		{
			return true;
		}

		int32_t getNumLive() const
		{
			return int32_t(m_numAllocs - m_numFrees);
		}

		uint32_t         m_numAllocs = 0;
		uint32_t         m_numFrees  = 0;
		bx::CrtAllocator m_crt;
	};

	constexpr int32_t kCountedDefault = -1;

	struct Counted
	{
		Counted()
			: m_value(kCountedDefault)
		{
			++s_numLive;
		}

		Counted(int32_t _value)
			: m_value(_value)
		{
			++s_numLive;
		}

		~Counted()
		{
			--s_numLive;
		}

		int32_t m_value;

		static int32_t s_numLive;
	};

	int32_t Counted::s_numLive = 0;

	constexpr size_t kOverAlignment = 64;

	struct alignas(kOverAlignment) OverAligned
	{
		uint8_t m_data[kOverAlignment];
	};

	constexpr int32_t kInitTaggedDefault  = 1;
	constexpr int32_t kInitTaggedNone     = 2;
	constexpr int32_t kInitTaggedZero     = 3;
	constexpr int32_t kInitTaggedIdentity = 4;

	struct InitTagged
	{
		InitTagged()
			: m_value(kInitTaggedDefault)
		{
		}

		InitTagged(bx::InitNoneTag)
			: m_value(kInitTaggedNone)
		{
		}

		InitTagged(bx::InitZeroTag)
			: m_value(kInitTaggedZero)
		{
		}

		InitTagged(bx::InitIdentityTag)
			: m_value(kInitTaggedIdentity)
		{
		}

		int32_t m_value;
	};

} // namespace

TEST_CASE("UniquePtrT destroys and frees on destruction", "[uniqueptr]")
{
	CountingAllocator allocator;

	{
		bx::UniquePtrT<Counted> ptr = BX_MAKE_UNIQUE(allocator.getHandle(), Counted, 1389);
		REQUIRE(ptr.isValid() );
		REQUIRE(1389 == ptr->m_value);
		REQUIRE(1389 == (*ptr).m_value);
		REQUIRE(ptr.getPtr() == &*ptr);
		REQUIRE(1 == Counted::s_numLive);
		REQUIRE(1 == allocator.getNumLive() );
	}

	REQUIRE(0 == Counted::s_numLive);
	REQUIRE(0 == allocator.getNumLive() );
}

TEST_CASE("UniquePtrT default is invalid", "[uniqueptr]")
{
	bx::UniquePtrT<Counted> ptr;
	REQUIRE(!ptr.isValid() );
	REQUIRE(ptr.getPtr() == (Counted*)NULL);

	ptr.reset();
	REQUIRE(!ptr.isValid() );

	bx::UniquePtrT<Counted[]> array;
	REQUIRE(!array.isValid() );
	REQUIRE(array.getPtr() == (Counted*)NULL);
	REQUIRE(0 == array.getCount() );
}

TEST_CASE("UniquePtrT move", "[uniqueptr]")
{
	CountingAllocator allocator;

	{
		bx::UniquePtrT<Counted> aa = BX_MAKE_UNIQUE(allocator.getHandle(), Counted, 1);
		bx::UniquePtrT<Counted> bb = BX_MAKE_UNIQUE(allocator.getHandle(), Counted, 2);
		REQUIRE(2 == Counted::s_numLive);

		aa = bx::move(bb);
		REQUIRE(!bb.isValid() );
		REQUIRE(2 == aa->m_value);
		REQUIRE(1 == Counted::s_numLive);
		REQUIRE(1 == allocator.getNumLive() );

		bx::UniquePtrT<Counted> cc(bx::move(aa) );
		REQUIRE(!aa.isValid() );
		REQUIRE(2 == cc->m_value);
		REQUIRE(1 == Counted::s_numLive);
	}

	REQUIRE(0 == Counted::s_numLive);
	REQUIRE(0 == allocator.getNumLive() );
}

TEST_CASE("UniquePtrT reset and release", "[uniqueptr]")
{
	CountingAllocator allocator;

	bx::UniquePtrT<Counted> ptr = BX_MAKE_UNIQUE(allocator.getHandle(), Counted, 7);
	ptr.reset();
	REQUIRE(!ptr.isValid() );
	REQUIRE(0 == Counted::s_numLive);
	REQUIRE(0 == allocator.getNumLive() );

	ptr = BX_MAKE_UNIQUE(allocator.getHandle(), Counted, 8);
	bx::Blk blk = ptr.release();
	REQUIRE(!ptr.isValid() );
	REQUIRE(bx::isValid(blk) );
	REQUIRE(1 == Counted::s_numLive);
	REQUIRE(1 == allocator.getNumLive() );

	Counted* counted = (Counted*)blk;
	REQUIRE(8 == counted->m_value);
	counted->~Counted();
	blk = bx::free(bx::move(blk) );
	REQUIRE(0 == Counted::s_numLive);
	REQUIRE(0 == allocator.getNumLive() );
}

TEST_CASE("UniquePtrT keeps allocator handle while empty", "[uniqueptr]")
{
	CountingAllocator allocator;
	const bx::AllocatorHandle handle = allocator.getHandle();

	bx::UniquePtrT<Counted> ptr(handle);
	REQUIRE(!ptr.isValid() );
	REQUIRE(handle == ptr.getAllocatorHandle() );

	ptr = BX_MAKE_UNIQUE(ptr.getAllocatorHandle(), Counted, 1);
	REQUIRE(1 == allocator.getNumLive() );

	ptr.reset();
	REQUIRE(!ptr.isValid() );
	REQUIRE(handle == ptr.getAllocatorHandle() );
	REQUIRE(0 == allocator.getNumLive() );

	ptr = BX_MAKE_UNIQUE(ptr.getAllocatorHandle(), Counted, 2);
	bx::UniquePtrT<Counted> moved(bx::move(ptr) );
	REQUIRE(handle == ptr.getAllocatorHandle() );

	bx::Blk blk = moved.release();
	REQUIRE(handle == moved.getAllocatorHandle() );
	( (Counted*)blk)->~Counted();
	blk = bx::free(bx::move(blk) );

	bx::UniquePtrT<Counted[]> array = BX_MAKE_UNIQUE(handle, Counted[], 0);
	REQUIRE(!array.isValid() );
	REQUIRE(handle == array.getAllocatorHandle() );

	array = BX_MAKE_UNIQUE(array.getAllocatorHandle(), Counted[], 3);
	REQUIRE(1 == allocator.getNumLive() );

	array.reset();
	REQUIRE(handle == array.getAllocatorHandle() );
	REQUIRE(0 == Counted::s_numLive);
	REQUIRE(0 == allocator.getNumLive() );
}

TEST_CASE("UniquePtrT array", "[uniqueptr]")
{
	CountingAllocator allocator;

	{
		bx::UniquePtrT<Counted[]> array = BX_MAKE_UNIQUE(allocator.getHandle(), Counted[], 5);
		REQUIRE(array.isValid() );
		REQUIRE(5 == array.getCount() );
		REQUIRE(5 == Counted::s_numLive);
		REQUIRE(1 == allocator.getNumLive() );

		for (uint32_t ii = 0, num = array.getCount(); ii < num; ++ii)
		{
			REQUIRE(kCountedDefault == array[ii].m_value);
			array[ii].m_value = int32_t(ii);
		}

		REQUIRE(&array[0] == array.getPtr() );
		REQUIRE(4 == array.getPtr()[4].m_value);

		bx::UniquePtrT<Counted[]> other = BX_MAKE_UNIQUE(allocator.getHandle(), Counted[], 3);
		REQUIRE(8 == Counted::s_numLive);

		array = bx::move(other);
		REQUIRE(!other.isValid() );
		REQUIRE(0 == other.getCount() );
		REQUIRE(3 == array.getCount() );
		REQUIRE(3 == Counted::s_numLive);
		REQUIRE(1 == allocator.getNumLive() );
	}

	REQUIRE(0 == Counted::s_numLive);
	REQUIRE(0 == allocator.getNumLive() );
}

TEST_CASE("UniquePtrT trivial array", "[uniqueptr]")
{
	CountingAllocator allocator;

	bx::UniquePtrT<uint8_t[]> empty = BX_MAKE_UNIQUE(allocator.getHandle(), uint8_t[], 0);
	REQUIRE(!empty.isValid() );
	REQUIRE(0 == empty.getCount() );
	REQUIRE(0 == allocator.m_numAllocs);

	constexpr uint32_t kSize = 4096;

	bx::UniquePtrT<uint8_t[]> data = BX_MAKE_UNIQUE(allocator.getHandle(), uint8_t[], kSize);
	REQUIRE(data.isValid() );
	REQUIRE(kSize == data.getCount() );
	REQUIRE(bx::isAligned(data.getPtr(), alignof(uint8_t) ) );

	bx::memSet(data.getPtr(), 0xa5, kSize);
	REQUIRE(0xa5 == data[kSize-1]);

	bx::UniquePtrT<OverAligned[]> aligned = BX_MAKE_UNIQUE(allocator.getHandle(), OverAligned[], 4);
	REQUIRE(bx::isAligned(aligned.getPtr(), kOverAlignment) );

	data.reset();
	aligned.reset();
	REQUIRE(0 == allocator.getNumLive() );
}

TEST_CASE("UniquePtrT array constructs elements with InitNone", "[uniqueptr]")
{
	CountingAllocator allocator;

	constexpr uint32_t kCount = 4;

	bx::UniquePtrT<InitTagged[]> tagged = BX_MAKE_UNIQUE(allocator.getHandle(), InitTagged[], kCount);
	REQUIRE(kCount == tagged.getCount() );

	for (uint32_t ii = 0; ii < kCount; ++ii)
	{
		REQUIRE(kInitTaggedNone == tagged[ii].m_value);
	}

	bx::UniquePtrT<bx::Vec3[]> vec = BX_MAKE_UNIQUE(allocator.getHandle(), bx::Vec3[], kCount);
	REQUIRE(kCount == vec.getCount() );

	tagged.reset();
	vec.reset();
	REQUIRE(0 == allocator.getNumLive() );
}

TEST_CASE("UniquePtrT array constructs elements with arguments", "[uniqueptr]")
{
	CountingAllocator allocator;

	constexpr uint32_t kCount = 4;

	bx::UniquePtrT<InitTagged[]> zero     = BX_MAKE_UNIQUE(allocator.getHandle(), InitTagged[], kCount, bx::InitZero);
	bx::UniquePtrT<InitTagged[]> identity = BX_MAKE_UNIQUE(allocator.getHandle(), InitTagged[], kCount, bx::InitIdentity);
	bx::UniquePtrT<Counted[]>    counted  = BX_MAKE_UNIQUE(allocator.getHandle(), Counted[], kCount, 7);
	REQUIRE(kCount == zero.getCount() );
	REQUIRE(kCount == identity.getCount() );
	REQUIRE(kCount == counted.getCount() );

	for (uint32_t ii = 0; ii < kCount; ++ii)
	{
		REQUIRE(kInitTaggedZero     == zero[ii].m_value);
		REQUIRE(kInitTaggedIdentity == identity[ii].m_value);
		REQUIRE(7                   == counted[ii].m_value);
	}

	bx::UniquePtrT<bx::Plane[]> planes = BX_MAKE_UNIQUE(allocator.getHandle(), bx::Plane[], kCount, bx::InitZero);
	REQUIRE(kCount == planes.getCount() );

	for (uint32_t ii = 0; ii < kCount; ++ii)
	{
		REQUIRE(0.0f == planes[ii].dist);
	}

	zero.reset();
	identity.reset();
	counted.reset();
	planes.reset();
	REQUIRE(0 == Counted::s_numLive);
	REQUIRE(0 == allocator.getNumLive() );
}
