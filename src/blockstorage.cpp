/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/blockstorage.h>

namespace bx
{
	BlockStorage::BlockStorage(uint32_t _elementSize, uint32_t _elementAlign, uint32_t _blockCapacity)
		: m_numBlocks(0)
		, m_elementSize(_elementSize)
		, m_elementAlign(_elementAlign)
		, m_blockCapacity(_blockCapacity)
		, m_blockShift(0 < _blockCapacity ? uint32_t(floorLog2(_blockCapacity) ) : 0)
	{
		BX_ASSERT(0 < _blockCapacity && isPowerOf2(_blockCapacity), "Block capacity must be a power of two, got %u.", _blockCapacity);
	}

	BlockStorage::~BlockStorage()
	{
		setCapacity(0);
	}

	BlockStorage& BlockStorage::operator=(BlockStorage&& _other)
	{
		if (this != &_other)
		{
			BX_ASSERT(true
				&& m_elementSize   == _other.m_elementSize
				&& m_blockCapacity == _other.m_blockCapacity
				, "Block storage layout mismatch (element size %u vs %u, block capacity %u vs %u)."
				, m_elementSize
				, _other.m_elementSize
				, m_blockCapacity
				, _other.m_blockCapacity
				);

			setCapacity(0);

			m_blocks    = move(_other.m_blocks);
			m_numBlocks = _other.m_numBlocks;

			_other.m_numBlocks = 0;
		}

		return *this;
	}

	void BlockStorage::setCapacity(uint32_t _capacity)
	{
		const uint32_t numBlocks = (_capacity + m_blockCapacity - 1)/m_blockCapacity;

		if (numBlocks == m_numBlocks)
		{
			return;
		}

		for (uint32_t ii = numBlocks; ii < m_numBlocks; ++ii)
		{
			releaseBlock(ii);
			m_blocks.getPtr(ii)->~Blk();
		}

		const uint32_t numLive = min(numBlocks, m_numBlocks);

		m_blocks.setCapacity(numBlocks, numLive);

		StorageTraits::constructDefault(m_blocks.getPtr(numLive), numBlocks - numLive);

		m_numBlocks = numBlocks;
	}

	void BlockStorage::allocateBlock(uint32_t _blockIdx)
	{
		Blk* blk = m_blocks.getPtr(_blockIdx);

		if (!isValid(*blk) )
		{
			*blk = alloc(m_blocks.getAllocatorHandle(), size_t(m_elementSize)*m_blockCapacity, m_elementAlign);
		}
	}

	void BlockStorage::releaseBlock(uint32_t _blockIdx)
	{
		Blk* blk = m_blocks.getPtr(_blockIdx);

		if (isValid(*blk) )
		{
			*blk = free(move(*blk) );
		}
	}

} // namespace bx
