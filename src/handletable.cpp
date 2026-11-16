/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/handletable.h>

namespace bx
{
	static uint32_t s_noFreeHead = kInvalid;

	HandleTableBase::HandleTableBase(uint32_t _elementSize, uint32_t _elementAlign, uint32_t _blockCapacity)
		: HandleTableBase(getDefaultAllocatorBlk()->getHandle(), _elementSize, _elementAlign, _blockCapacity, 0)
	{
	}

	HandleTableBase::HandleTableBase(HandleTableBase&& _src)
		: m_base(_src.m_base)
		, m_freeHead(_src.m_freeHead)
		, m_blockCount(_src.m_blockCount)
		, m_blk(_src.m_blk)
		, m_table(move(_src.m_table) )
		, m_usedSlots(move(_src.m_usedSlots) )
		, m_nonFullBlocks(move(_src.m_nonFullBlocks) )
		, m_allocatedBlocks(move(_src.m_allocatedBlocks) )
		, m_elementSize(_src.m_elementSize)
		, m_elementAlign(_src.m_elementAlign)
		, m_blockCapacity(_src.m_blockCapacity)
		, m_blockShift(_src.m_blockShift)
		, m_numBlocks(_src.m_numBlocks)
		, m_tableCapacity(_src.m_tableCapacity)
		, m_count(_src.m_count)
		, m_curBlock(_src.m_curBlock)
	{
		_src.reset();
	}

	HandleTableBase::HandleTableBase(AllocatorHandle _allocator, uint32_t _elementSize, uint32_t _elementAlign, uint32_t _blockCapacity, uint32_t _capacity)
		: m_base(NULL)
		, m_freeHead(&s_noFreeHead)
		, m_blockCount(NULL)
		, m_blk(NULL)
		, m_usedSlots(_allocator)
		, m_nonFullBlocks(_allocator)
		, m_allocatedBlocks(_allocator)
		, m_elementSize(_elementSize)
		, m_elementAlign(_elementAlign)
		, m_blockCapacity(_blockCapacity)
		, m_blockShift(uint32_t(floorLog2(_blockCapacity) ) )
		, m_numBlocks(0)
		, m_tableCapacity(0)
		, m_count(0)
		, m_curBlock(0)
	{
		BX_ASSERT(isPowerOf2(_blockCapacity) && 64 <= _blockCapacity, "Block capacity must be a power of two of at least 64, got %u.", _blockCapacity);
		BX_ASSERT(sizeof(uint32_t) <= _elementSize, "Element size must be at least %zu, got %u.", sizeof(uint32_t), _elementSize);
		BX_ASSERT(alignof(uint32_t) <= _elementAlign, "Element alignment must be at least %zu, got %u.", alignof(uint32_t), _elementAlign);

		for (uint32_t ii = 0, num = (_capacity + _blockCapacity - 1)>>m_blockShift; ii < num; ++ii)
		{
			allocateBlock();
		}
	}

	HandleTableBase::~HandleTableBase()
	{
		releaseBlocks();

		if (isValid(m_table) )
		{
			m_table = free(move(m_table) );
		}
	}

	void HandleTableBase::set(HandleTableBase&& _src)
	{
		if (this == &_src)
		{
			return;
		}

		releaseBlocks();

		if (isValid(m_table) )
		{
			m_table = free(move(m_table) );
		}

		m_base          = _src.m_base;
		m_freeHead      = _src.m_freeHead;
		m_blockCount    = _src.m_blockCount;
		m_blk           = _src.m_blk;
		m_table         = move(_src.m_table);
		m_usedSlots          = move(_src.m_usedSlots);
		m_nonFullBlocks       = move(_src.m_nonFullBlocks);
		m_allocatedBlocks     = move(_src.m_allocatedBlocks);
		m_elementSize   = _src.m_elementSize;
		m_elementAlign  = _src.m_elementAlign;
		m_blockCapacity = _src.m_blockCapacity;
		m_blockShift    = _src.m_blockShift;
		m_numBlocks     = _src.m_numBlocks;
		m_tableCapacity = _src.m_tableCapacity;
		m_count         = _src.m_count;
		m_curBlock      = _src.m_curBlock;

		_src.reset();
	}

	void HandleTableBase::clear()
	{
		releaseBlocks();

		for (uint32_t ii = 0; ii < m_tableCapacity; ++ii)
		{
			m_base[ii]       = 0;
			m_freeHead[ii]   = kInvalid;
			m_blockCount[ii] = 0;
		}

		m_usedSlots.setCount(0);
		m_nonFullBlocks.setCount(0);
		m_nonFullBlocks.setCount(m_tableCapacity);
		m_allocatedBlocks.setCount(0);
		m_allocatedBlocks.setCount(m_tableCapacity);

		m_numBlocks = 0;
		m_count     = 0;
		m_curBlock  = 0;
	}

	void HandleTableBase::releaseBlocks()
	{
		for (uint32_t ii = 0; ii < m_numBlocks; ++ii)
		{
			if (m_allocatedBlocks.get(ii) )
			{
				m_blk[ii] = free(move(m_blk[ii]) );
			}
		}
	}

	void HandleTableBase::reset()
	{
		m_base          = NULL;
		m_freeHead      = &s_noFreeHead;
		m_blockCount    = NULL;
		m_blk           = NULL;
		m_numBlocks     = 0;
		m_tableCapacity = 0;
		m_count         = 0;
		m_curBlock      = 0;
	}

	void HandleTableBase::selectBlock()
	{
		uint32_t blockIdx = kInvalid;

		if (0 < m_numBlocks)
		{
			m_nonFullBlocks.set(m_curBlock, false);
			blockIdx = m_nonFullBlocks.findSet(0, m_numBlocks);
		}

		if (kInvalid == blockIdx)
		{
			blockIdx = allocateBlock();
		}

		m_curBlock = blockIdx;
	}

	uint32_t HandleTableBase::allocateBlock()
	{
		uint32_t blockIdx = 0 < m_tableCapacity
			? m_allocatedBlocks.findClear(0, m_tableCapacity)
			: kInvalid
			;

		if (kInvalid == blockIdx)
		{
			blockIdx = m_tableCapacity;
			growTable(max<uint32_t>(m_tableCapacity*2, 8) );
		}

		Blk blk = alloc(m_usedSlots.getAllocatorHandle(), size_t(m_elementSize)<<m_blockShift, m_elementAlign);
		BX_ASSERT(isValid(blk), "Failed to allocate block of %u elements of %u bytes.", m_blockCapacity, m_elementSize);

		const uint32_t  begin = blockIdx<<m_blockShift;
		const uint32_t  end   = begin + m_blockCapacity;
		const uintptr_t base  = uintptr_t(blk.ptr) - size_t(begin)*m_elementSize;

		m_blk[blockIdx]  = move(blk);
		m_base[blockIdx] = base;

		if (blockIdx >= m_numBlocks)
		{
			m_numBlocks = blockIdx + 1;
			m_usedSlots.setCount(m_numBlocks<<m_blockShift);
		}

		const uint32_t first = 0 == blockIdx ? 1 : begin;

		uint32_t freeHead = kInvalid;

		for (uint32_t idx = end; first < idx; --idx)
		{
			*(uint32_t*)(base + size_t(idx-1)*m_elementSize) = freeHead;
			freeHead = idx-1;
		}

		m_freeHead[blockIdx]   = freeHead;
		m_blockCount[blockIdx] = first - begin;

		if (0 == blockIdx)
		{
			m_usedSlots.set(0, true);
		}

		m_allocatedBlocks.set(blockIdx, true);
		m_nonFullBlocks.set(blockIdx, true);

		return blockIdx;
	}

	void HandleTableBase::releaseBlock(uint32_t _blockIdx)
	{
		m_nonFullBlocks.set(_blockIdx, false);

		if (_blockIdx == m_curBlock)
		{
			const uint32_t blockIdx = m_nonFullBlocks.findSet(0, m_numBlocks);

			if (kInvalid == blockIdx)
			{
				m_nonFullBlocks.set(_blockIdx, true);
				return;
			}

			m_curBlock = blockIdx;
		}

		m_blk[_blockIdx]      = free(move(m_blk[_blockIdx]) );
		m_freeHead[_blockIdx] = kInvalid;

		m_allocatedBlocks.set(_blockIdx, false);

		if (_blockIdx + 1 == m_numBlocks)
		{
			uint32_t numBlocks = _blockIdx;

			while (0 < numBlocks
			&&     !m_allocatedBlocks.get(numBlocks - 1) )
			{
				--numBlocks;
			}

			m_numBlocks = numBlocks;

			const uint32_t newBitCount = numBlocks<<m_blockShift;
			m_usedSlots.setCapacity(newBitCount);
			m_usedSlots.setCount(newBitCount);
		}
	}

	void HandleTableBase::growTable(uint32_t _tableCapacity)
	{
		const size_t baseSize  = size_t(_tableCapacity)*sizeof(uintptr_t);
		const size_t indexSize = size_t(_tableCapacity)*sizeof(uint32_t);

		Blk table = alloc(m_usedSlots.getAllocatorHandle(), baseSize + 2*indexSize + _tableCapacity*sizeof(Blk), alignof(Blk) );
		BX_ASSERT(isValid(table), "Failed to allocate block table of %u entries.", _tableCapacity);

		uint8_t*   ptr        = (uint8_t*)uintptr_t(table.ptr);
		uintptr_t* base       = (uintptr_t*)ptr;
		uint32_t*  freeHead   = (uint32_t*)(ptr + baseSize);
		uint32_t*  blockCount = freeHead + _tableCapacity;
		Blk*       blk        = (Blk*)(ptr + baseSize + 2*indexSize);

		const uint32_t tableCapacity = m_tableCapacity;

		if (0 < tableCapacity)
		{
			memCopy(base,       m_base,       tableCapacity*sizeof(uintptr_t) );
			memCopy(freeHead,   m_freeHead,   tableCapacity*sizeof(uint32_t) );
			memCopy(blockCount, m_blockCount, tableCapacity*sizeof(uint32_t) );
			memCopy(blk,        m_blk,        tableCapacity*sizeof(Blk) );

			m_table = free(move(m_table) );
		}

		for (uint32_t ii = tableCapacity; ii < _tableCapacity; ++ii)
		{
			base[ii]       = 0;
			freeHead[ii]   = kInvalid;
			blockCount[ii] = 0;
		}

		StorageTraits::constructDefault(&blk[tableCapacity], _tableCapacity - tableCapacity);

		m_table         = move(table);
		m_base          = base;
		m_freeHead      = freeHead;
		m_blockCount    = blockCount;
		m_blk           = blk;
		m_tableCapacity = _tableCapacity;

		m_nonFullBlocks.setCount(_tableCapacity);
		m_allocatedBlocks.setCount(_tableCapacity);
	}

} // namespace bx
