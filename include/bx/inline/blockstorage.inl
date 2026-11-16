/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_BLOCKSTORAGE_H_HEADER_GUARD
#	error "Must be included from bx/blockstorage.h!"
#endif // BX_BLOCKSTORAGE_H_HEADER_GUARD

namespace bx
{
	inline void BlockStorage::setAllocatorHandle(AllocatorHandle _allocator)
	{
		m_blocks.setAllocatorHandle(_allocator);
	}

	inline AllocatorHandle BlockStorage::getAllocatorHandle() const
	{
		return m_blocks.getAllocatorHandle();
	}

	inline uint32_t BlockStorage::getCapacity() const
	{
		return m_numBlocks*m_blockCapacity;
	}

	inline uint32_t BlockStorage::getBlockCapacity() const
	{
		return m_blockCapacity;
	}

	inline uint32_t BlockStorage::getNumBlocks() const
	{
		return m_numBlocks;
	}

	inline bool BlockStorage::isBlockAllocated(uint32_t _blockIdx) const
	{
		BX_ASSERT(_blockIdx < m_numBlocks, "Block index %u out of range (%u).", _blockIdx, m_numBlocks);
		return isValid(*m_blocks.getPtr(_blockIdx) );
	}

	inline void* BlockStorage::getBlockPtr(uint32_t _blockIdx)
	{
		BX_ASSERT(isBlockAllocated(_blockIdx), "Accessing released block %d.", _blockIdx);
		return (void*)*m_blocks.getPtr(_blockIdx);
	}

	inline const void* BlockStorage::getBlockPtr(uint32_t _blockIdx) const
	{
		return const_cast<BlockStorage*>(this)->getBlockPtr(_blockIdx);
	}

	inline uint32_t BlockStorage::getBlockIdx(uint32_t _idx) const
	{
		return _idx>>m_blockShift;
	}

	inline void* BlockStorage::getPtr(uint32_t _idx)
	{
		const uint32_t blockIdx = _idx>>m_blockShift;
		return reinterpret_cast<uint8_t*>(getBlockPtr(blockIdx) ) + (_idx & (m_blockCapacity-1) )*m_elementSize;
	}

	inline const void* BlockStorage::getPtr(uint32_t _idx) const
	{
		return const_cast<BlockStorage*>(this)->getPtr(_idx);
	}

	inline void* BlockStorage::acquirePtr(uint32_t _idx)
	{
		const uint32_t blockIdx = _idx>>m_blockShift;

		if (blockIdx >= m_numBlocks)
		{
			setCapacity(_idx + 1);
		}

		allocateBlock(blockIdx);

		return getPtr(_idx);
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline BlockStorageT<Ty, BlockCapacityT>::BlockStorageT()
		: BlockStorage(sizeof(Ty), alignof(Ty), BlockCapacityT)
	{
		debugPtrUpdate();
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline BlockStorageT<Ty, BlockCapacityT>& BlockStorageT<Ty, BlockCapacityT>::operator=(BlockStorageT<Ty, BlockCapacityT>&& _other)
	{
		BlockStorage::operator=(bx::move(_other) );
		debugPtrUpdate();
		_other.debugPtrUpdate();
		return *this;
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline void BlockStorageT<Ty, BlockCapacityT>::setCapacity(uint32_t _capacity)
	{
		BlockStorage::setCapacity(_capacity);
		debugPtrUpdate();
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline void BlockStorageT<Ty, BlockCapacityT>::debugPtrUpdate()
	{
#if BX_CONFIG_DEBUG
		m_debugBlocks = reinterpret_cast<Ty**>(m_blocks.getPtr() );
#endif // BX_CONFIG_DEBUG
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline Ty* BlockStorageT<Ty, BlockCapacityT>::getPtr(uint32_t _idx)
	{
		return reinterpret_cast<Ty*>(getBlockPtr(_idx >> kBlockShift) ) + (_idx & (BlockCapacityT - 1) );
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline const Ty* BlockStorageT<Ty, BlockCapacityT>::getPtr(uint32_t _idx) const
	{
		return reinterpret_cast<const Ty*>(getBlockPtr(_idx >> kBlockShift) ) + (_idx & (BlockCapacityT - 1) );
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline Ty* BlockStorageT<Ty, BlockCapacityT>::acquirePtr(uint32_t _idx)
	{
		const uint32_t blockIdx = _idx >> kBlockShift;

		if (blockIdx >= getNumBlocks() )
		{
			setCapacity(_idx + 1);
		}

		allocateBlock(blockIdx);

		return getPtr(_idx);
	}

} // namespace bx
