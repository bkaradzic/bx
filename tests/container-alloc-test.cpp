/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"

#include <bx/allocators.h>
#include <bx/array.h>
#include <bx/handletable.h>
#include <bx/hashmap.h>
#include <bx/stringpool.h>

#include <string>

#if BX_CONFIG_DEBUG

namespace
{
	class BudgetAllocator final : public bx::AllocatorBlkI
	{
	public:
		BudgetAllocator(uint32_t _budget)
			: m_budget(_budget)
		{
		}

		bx::Blk reallocImpl(bx::Blk&& _blk, size_t _size, size_t _alignment) override
		{
			const bx::AllocatorOp::Enum op = bx::getAllocatorOp(_blk, _size, _alignment);

			if (bx::AllocatorOp::Allocate == op
			||  bx::AllocatorOp::Grow     == op)
			{
				if (0 == m_budget)
				{
					return bx::Blk(bx::Blk::InvalidReason::AllocateFailed);
				}

				--m_budget;
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

		uint32_t         m_budget;
		bx::CrtAllocator m_crt;
	};

	std::string makeLong(uint32_t _value)
	{
		return std::to_string(_value) + std::string(32, 'x');
	}

	void fillArray(bx::ArrayT<std::string>& _array)
	{
		for (uint32_t ii = 0; ii < 100000; ++ii)
		{
			_array.add(ii/2, makeLong(ii) );
		}
	}

	void fillSortedArray(bx::SortedArrayT<uint32_t>& _array)
	{
		for (uint32_t ii = 0; ii < 100000; ++ii)
		{
			_array.add(ii*7919);
		}
	}

	void fillHashMap(bx::HashMapT<uint32_t, std::string>& _map)
	{
		for (uint32_t ii = 0; ii < 100000; ++ii)
		{
			_map.add(ii, makeLong(ii) );
		}
	}

	void fillHandleTable(bx::HandleTableT<std::string, 64>& _array)
	{
		for (uint32_t ii = 0; ii < 100000; ++ii)
		{
			const uint32_t idx = _array.add(makeLong(ii) );
			BX_UNUSED(idx);
		}
	}

	void fillStringPool(bx::StringPool& _pool, bx::ArrayT<bx::String>& _strings)
	{
		for (uint32_t ii = 0; ii < 100000; ++ii)
		{
			const std::string str = makeLong(ii);
			_strings.add(_pool.intern(bx::StringView(str.c_str(), int32_t(str.size() ) ) ) );
		}
	}

	void fillStringBuilder(bx::StringBuilder& _builder)
	{
		for (uint32_t ii = 0; ii < 100000; ++ii)
		{
			_builder.setCursor(int32_t(ii) );
			_builder.addPrintf("%u,", ii);
		}
	}

} // namespace

TEST_CASE("Containers assert when allocation fails", "[container]")
{
	BudgetAllocator allocator(1000);
	bx::StringPool  pool(allocator.getHandle(), 0);
	allocator.m_budget = 0;

	bx::ArrayT<std::string> array(allocator.getHandle() );
	REQUIRE_ASSERTS(fillArray(array) );

	bx::SortedArrayT<uint32_t> sorted(allocator.getHandle() );
	REQUIRE_ASSERTS(fillSortedArray(sorted) );

	bx::HashMapT<uint32_t, std::string> map(allocator.getHandle() );
	REQUIRE_ASSERTS(fillHashMap(map) );

	bx::HandleTableT<std::string, 64> handleTable(allocator.getHandle() );
	REQUIRE_ASSERTS(fillHandleTable(handleTable) );

	bx::CrtAllocator crt;
	bx::ArrayT<bx::String> strings(crt.getHandle() );
	REQUIRE_ASSERTS(fillStringPool(pool, strings) );

	bx::StringBuilder builder(&pool);
	REQUIRE_ASSERTS(fillStringBuilder(builder) );
}

#endif // BX_CONFIG_DEBUG

const bx::StringPoolEntry& stringLiteralExprOtherTu()
{
	return "literal_expr"_str;
}

namespace
{
	struct NoAddressOf
	{
		void operator&() const = delete;

		bool operator==(const NoAddressOf& _rhs) const
		{
			return m_value == _rhs.m_value;
		}

		int32_t m_value;
	};

} // namespace

template class bx::ArrayT<NoAddressOf>;
template class bx::HashMapT<uint32_t, NoAddressOf>;
template class bx::BlockHashMapT<uint32_t, NoAddressOf>;
template class bx::HandleTableT<NoAddressOf>;
