/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_BLK_ALLOCATORS_H_HEADER_GUARD
#	error "Must be included from bx/blk-allocators.h!"
#endif // BX_BLK_ALLOCATORS_H_HEADER_GUARD

namespace bx
{
	template<>
	inline int32_t compareAscending<Blk>(const void* _lhs, const void* _rhs)
	{
		return compareAscending<uint64_t>(&( (const Blk*)_lhs)->ptr, &( (const Blk*)_rhs)->ptr);
	}

	template<>
	inline int32_t compareAscending<Range>(const void* _lhs, const void* _rhs)
	{
		return compareAscending<uint64_t>(&( (const Range*)_lhs)->blk.ptr, &( (const Range*)_rhs)->blk.ptr);
	}

	template<typename AllocatorBlkT>
	inline SecureAllocatorT<AllocatorBlkT>::SecureAllocatorT(AllocatorBlkT& _allocator)
		: m_allocator(&_allocator)
	{
		BX_ASSERT(_allocator.isCpuAccessible(), "Allocator must be CPU accessible!");
	}

	template<typename AllocatorBlkT>
	inline SecureAllocatorT<AllocatorBlkT>::~SecureAllocatorT()
	{
	}

	template<typename AllocatorBlkT>
	inline Blk SecureAllocatorT<AllocatorBlkT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		switch (getAllocatorOp(_blk, _size, _alignment) )
		{
		case AllocatorOp::Free:
			memSetSecure( (void*)uintptr_t(_blk.ptr), 0x00, _blk.size);
			break;

		case AllocatorOp::Grow:
		case AllocatorOp::Realign:
		case AllocatorOp::Truncate:
			{
				Blk blk = m_allocator->realloc(Blk(), _size, _alignment);

				if (!isValid(blk) )
				{
					return Blk(Blk::InvalidReason::ResizeFailed);
				}

				memCopy( (void*)blk.ptr, (void*)_blk.ptr, min(_blk.size, _size) );

				memSetSecure( (void*)uintptr_t(_blk.ptr), 0x00, _blk.size);

				(void)m_allocator->realloc(move(_blk), 0, 0);

				return blk;
			}
			break;

		default:
			break;
		}

		return m_allocator->realloc(move(_blk), _size, _alignment);
	}

	template<typename AllocatorBlkT>
	inline bool SecureAllocatorT<AllocatorBlkT>::owns(const Blk& _blk)
	{
		return m_allocator->owns(_blk);
	}

	template<typename AllocatorBlkT>
	inline bool SecureAllocatorT<AllocatorBlkT>::isCpuAccessible() const
	{
		return true;
	}

	template<uint32_t CapacityT, uint32_t AlignT>
	inline FixedCapacityAllocatorT<CapacityT, AlignT>::FixedCapacityAllocatorT()
		: m_used(false)
	{
	}

	template<uint32_t CapacityT, uint32_t AlignT>
	inline Blk FixedCapacityAllocatorT<CapacityT, AlignT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		BX_ASSERT(!isValid(_blk) || owns(_blk), "Blk passed is not owned by this allocator!");
		BX_ASSERT(0 == _alignment || _alignment == AlignT
				, "Not possible to change alignment (requested %d, required %d)!"
				, _alignment
				, AlignT
				);

		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate:
			if (m_used)
			{
				return Blk(Blk::InvalidReason::NoBlockLargeEnough);
			}

			m_used = true;
			return Blk(uint64_t(m_data), CapacityT, AlignT, isCpuAccessible(), allocatorOp);

		case AllocatorOp::Free:
			m_used = false;
			_blk.invalidate(Blk::InvalidReason::Freed);
			return move(_blk);

		case AllocatorOp::Grow:
		case AllocatorOp::Truncate:
			if (_blk.size >= _size)
			{
				return move(_blk);
			}
			return Blk(Blk::InvalidReason::NotSupported);

		case AllocatorOp::Noop:
			return Blk(Blk::InvalidReason::Noop);

		default:
			BX_ASSERT(false, "You should not be here!");
			break;
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	template<uint32_t CapacityT, uint32_t AlignT>
	inline bool FixedCapacityAllocatorT<CapacityT, AlignT>::owns(const Blk& _blk)
	{
		return contain(Blk(uint64_t(m_data), CapacityT, AlignT, bx::AllocatorOp::Alias), _blk);
	}

	template<uint32_t CapacityT, uint32_t AlignT>
	inline bool FixedCapacityAllocatorT<CapacityT, AlignT>::isCpuAccessible() const
	{
		return true;
	}

	template <typename PrimaryT, typename SecondaryT>
	inline OverflowAllocatorT<PrimaryT, SecondaryT>::OverflowAllocatorT()
		: m_cpuAccessible(true
			&&   m_primary.isCpuAccessible()
			&& m_secondary.isCpuAccessible()
			)
	{
	}

	template <typename PrimaryT, typename SecondaryT>
	inline Blk OverflowAllocatorT<PrimaryT, SecondaryT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		return Blk(reallocInternal(move(_blk), _size, _alignment), m_cpuAccessible);
	}

	template <typename PrimaryT, typename SecondaryT>
	inline bool OverflowAllocatorT<PrimaryT, SecondaryT>::owns(const Blk& _blk)
	{
		return m_primary.owns(_blk)
			|| m_secondary.owns(_blk)
			;
	}

	template <typename PrimaryT, typename SecondaryT>
	inline bool OverflowAllocatorT<PrimaryT, SecondaryT>::isCpuAccessible() const
	{
		return m_cpuAccessible;
	}

	template <typename PrimaryT, typename SecondaryT>
	inline Blk OverflowAllocatorT<PrimaryT, SecondaryT>::reallocInternal(Blk&& _blk, size_t _size, size_t _alignment)
	{
		BX_ASSERT(!isValid(_blk) || owns(_blk), "Blk passed is not owned by this allocator!");

		if (isValid(_blk) )
		{
			const bool primary = m_primary.owns(_blk);

			if (0 == _size)
			{
				if (primary)
				{
					return m_primary.realloc(move(_blk), 0, 0);
				}

				return m_secondary.realloc(move(_blk), 0, 0);
			}

			if (primary)
			{
				Blk blk = m_primary.realloc(move(_blk), _size, _alignment);

				if (!isValid(blk)
				&&   isCpuAccessible() )
				{
					blk = m_secondary.realloc(Blk(), _size, _alignment);

					if (!isValid(blk) )
					{
						return Blk(Blk::InvalidReason::ResizeFailed);
					}

					memCopy( (void*)blk.ptr, (void*)_blk.ptr, min(_blk.size, _size) );

					(void)m_primary.realloc(move(_blk), 0, 0);
				}

				return blk;
			}

			return m_secondary.realloc(move(_blk), _size, _alignment);
		}

		Blk blk = m_primary.realloc(Blk(), _size, _alignment);

		if (!isValid(blk) )
		{
			blk = m_secondary.realloc(Blk(), _size, _alignment);
		}

		return blk;
	}

	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT>
	inline ThresholdAllocatorT<ThresholdT, PrimaryT, SecondaryT, MaxAlignmentT>::ThresholdAllocatorT()
		: m_cpuAccessible(true
			&&   m_primary.isCpuAccessible()
			&& m_secondary.isCpuAccessible()
			)
	{
	}

	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT>
	inline ThresholdAllocatorT<ThresholdT, PrimaryT, SecondaryT, MaxAlignmentT>::ThresholdAllocatorT(PrimaryT _primary, SecondaryT _secondary)
		: m_primary(_primary)
		, m_secondary(_secondary)
		, m_cpuAccessible(true
			&&   m_primary.isCpuAccessible()
			&& m_secondary.isCpuAccessible()
			)
	{
	}

	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT>
	inline bool ThresholdAllocatorT<ThresholdT, PrimaryT, SecondaryT, MaxAlignmentT>::fitsPrimary(size_t _size, size_t _alignment)
	{
		return _alignment <= MaxAlignmentT
			&& alignUp(_size, max<size_t>(_alignment, 1) ) <= ThresholdT
			;
	}

	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT>
	inline Blk ThresholdAllocatorT<ThresholdT, PrimaryT, SecondaryT, MaxAlignmentT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		return Blk(reallocInternal(move(_blk), _size, _alignment), m_cpuAccessible);
	}

	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT>
	inline Blk ThresholdAllocatorT<ThresholdT, PrimaryT, SecondaryT, MaxAlignmentT>::alloc(size_t _size, size_t _alignment)
	{
		if (fitsPrimary(_size, _alignment) )
		{
			return Blk(internal::allocRaw(&m_primary, _size, _alignment), m_cpuAccessible);
		}

		return Blk(internal::allocRaw(&m_secondary, _size, _alignment), m_cpuAccessible);
	}

	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT>
	inline Blk ThresholdAllocatorT<ThresholdT, PrimaryT, SecondaryT, MaxAlignmentT>::free(Blk&& _blk)
	{
		BX_ASSERT(owns(_blk), "Blk passed is not owned by this allocator!");

		if (m_primary.owns(_blk) )
		{
			return Blk(internal::freeRaw(&m_primary, move(_blk) ), m_cpuAccessible);
		}

		return Blk(internal::freeRaw(&m_secondary, move(_blk) ), m_cpuAccessible);
	}

	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT>
	inline bool ThresholdAllocatorT<ThresholdT, PrimaryT, SecondaryT, MaxAlignmentT>::owns(const Blk& _blk)
	{
		return m_primary.owns(_blk)
			|| m_secondary.owns(_blk)
			;
	}

	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT>
	inline bool ThresholdAllocatorT<ThresholdT, PrimaryT, SecondaryT, MaxAlignmentT>::isCpuAccessible() const
	{
		return m_cpuAccessible;
	}

	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT>
	inline Blk ThresholdAllocatorT<ThresholdT, PrimaryT, SecondaryT, MaxAlignmentT>::reallocInternal(Blk&& _blk, size_t _size, size_t _alignment)
	{
		BX_ASSERT(!isValid(_blk) || owns(_blk), "Blk passed is not owned by this allocator!");

		if (!isValid(_blk) )
		{
			return fitsPrimary(_size, _alignment)
				? m_primary.realloc(move(_blk), _size, _alignment)
				: m_secondary.realloc(move(_blk), _size, _alignment)
				;
		}

		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);
		const bool primary = m_primary.owns(_blk);
		const bool fits    = fitsPrimary(_size, max<size_t>(_alignment, _blk.getAlignment() ) );
		Blk blk;

		if (0 == _size
		||  primary == fits)
		{
			if (primary)
			{
				return m_primary.realloc(move(_blk), _size, _alignment);
			}

			if (0 == _size)
			{
				return internal::freeNoTrack(&m_secondary, move(_blk) );
			}

			Error err;
			blk = internal::tryReallocNoTrack(&m_secondary, move(_blk), _size, _alignment, &err);

			if (err.isOk() )
			{
				return blk;
			}
		}
		else
		{
			blk = move(_blk);
		}

		Error errCopy;
		blk = internal::tryAllocCopyFreeNoTrack(this, move(blk), _size, _alignment, &errCopy);

		if (!errCopy.isOk() )
		{
			return Blk(Blk::InvalidReason::ResizeFailed);
		}

		blk.allocatorOp = allocatorOp;

		return blk;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::FirstFitAllocatorT(AllocatorBlkI* _metaDataAllocator)
		: m_range(_metaDataAllocator->getHandle() )
		, m_free(_metaDataAllocator->getHandle() )
		, m_total(0)
		, m_used(0)
	{
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::~FirstFitAllocatorT()
	{
		reset();
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate:
			return alloc(_size, _alignment);

		case AllocatorOp::Free:
			return free(move(_blk) );

		case AllocatorOp::Grow:
		case AllocatorOp::Truncate:
		case AllocatorOp::Realign:
			if (_size != _blk.size
			||  !isAligned(_blk.ptr, _alignment) )
			{
				if (isCpuAccessible() )
				{
					Error err;
					Blk blk = internal::tryAllocCopyFreeNoTrack(this, move(_blk), _size, _alignment, &err);

					if (!err.isOk() )
					{
						blk.invalidate();
						return Blk(Blk::InvalidReason::NoBlockLargeEnough);
					}

					blk.allocatorOp = allocatorOp;
					return blk;
				}

				return Blk(Blk::InvalidReason::NotImplemented);
			}
			[[fallthrough]];

		case AllocatorOp::Noop:
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);

		default:
			BX_ASSERT(false, "You should not be here (AllocatorOp: %d)!", allocatorOp);
			break;
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline bool FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::owns(const Blk& _blk)
	{
		const uint32_t idx = findRange(_blk);

		return idx < m_range.getCount()
			&& contain(m_range[idx].blk, _blk)
			;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline bool FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::isCpuAccessible() const
	{
		return CpuAccessibleT;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::addSlab(Blk&& _blk)
	{
		BX_ASSERT(isAligned(_blk.ptr, kTraits.minBlockAlignment)
			, "Slab must be aligned to %zd (ptr 0x%llx)."
			, kTraits.minBlockAlignment
			, _blk.ptr
			);

		m_total += _blk.size;

		Range range;
		range.blk   = move(_blk);
		range.inUse = false;
		m_range.add(move(range) );
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::removeUnusedSlab()
	{
		for (const Range* it = m_range.getPtr(), *itEnd = m_range.getTerm(); it != itEnd; ++it)
		{
			if (!it->inUse)
			{
				m_total -= it->blk.size;

				Range range = m_range.pop(m_range.getIndex(it) );

				return move(range.blk);
			}
		}

		return bx::Blk();
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline size_t FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::getTotal() const
	{
		return m_total;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline size_t FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::getUsed() const
	{
		return m_used;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::reset()
	{
		for (Blk blk = removeUnusedSlab(); isValid(blk); blk = removeUnusedSlab() )
		{
			blk.invalidate();
		}

		m_used = 0;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::allocFreeBlocks(size_t _size, size_t _alignment)
	{
		for (const Blk *it = m_free.getPtr(), *itEnd = m_free.getTerm(); it != itEnd; ++it)
		{
			if (it->size >= _size)
			{
				const uint64_t ptr       = alignUp(it->ptr, _alignment);
				const size_t   alignDiff = size_t(ptr - it->ptr);

				if (it->size - alignDiff < _size)
				{
					continue;
				}

				if (0 != alignDiff)
				{
					if (it->size - alignDiff != _size)
					{
						Blk blk(Copy, *it);
						blk.ptr  += alignDiff + _size;
						blk.size -= alignDiff + _size;

						Blk* modify  = const_cast<Blk*>(it);
						modify->size = alignDiff;

						m_free.add(Unchecked, m_free.getIndex(it)+1, move(blk) );
					}
					else
					{
						Blk* modify  = const_cast<Blk*>(it);
						modify->size = alignDiff;
					}
				}
				else if (it->size != _size)
				{
					Blk* modify   = const_cast<Blk*>(it);
					modify->ptr  += _size;
					modify->size -= _size;
				}
				else
				{
					remove(m_free, it);
				}

				m_used += _size;

				return Blk(ptr, _size, _alignment, CpuAccessibleT, AllocatorOp::Allocate);
			}
		}

		return Blk(Blk::InvalidReason::NoBlockLargeEnough);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::alloc(size_t _size, size_t _alignment)
	{
		const size_t minBlockSize = max(kTraits.minBlockSize, _alignment);

		_size = alignUp(max(_size, minBlockSize), minBlockSize);
		Blk blk = allocFreeBlocks(_size, _alignment);

		if (isValid(blk) )
		{
			return blk;
		}

		for (const Range* it = m_range.getPtr(), *itEnd = m_range.getTerm(); it != itEnd; ++it)
		{
			if (!it->inUse
			&&   it->blk.size >= _size)
			{
				Range* modify = const_cast<Range*>(it);
				modify->inUse = true;

				Blk free(modify->blk.ptr, modify->blk.size, 0, CpuAccessibleT, AllocatorOp::Allocate);
				m_free.add(move(free) );

				return allocFreeBlocks(_size, _alignment);
			}
		}

		return Blk(Blk::InvalidReason::NoBlockLargeEnough);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline uint32_t FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::findRange(const Blk& _blk) const
	{
		Range range;
		range.blk = Blk(Copy, _blk);

		return bx::satSub<uint32_t>(bx::upperBound(range, m_range), 1u);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::free(Blk&& _blk)
	{
		m_used -= _blk.size;

		const uint32_t rangeIdx = findRange(_blk);
		const Blk&     slab     = m_range[rangeIdx].blk;

		Blk blk(_blk.ptr, _blk.size, 0, CpuAccessibleT, AllocatorOp::Allocate);

		uint32_t idx = bx::lowerBound(_blk, m_free);

		if (idx < m_free.getCount()
		&&  blk.ptr + blk.size == m_free[idx].ptr
		&&  contain(slab, m_free[idx]) )
		{
			blk.size += m_free[idx].size;
			remove(m_free, m_free.getPtr(idx) );
		}

		if (0 < idx
		&&  m_free[idx-1].ptr + m_free[idx-1].size == blk.ptr
		&&  contain(slab, m_free[idx-1]) )
		{
			--idx;

			Blk& modify  = const_cast<Blk&>(m_free[idx]);
			modify.size += blk.size;

			blk.invalidate();
		}
		else
		{
			m_free.add(Unchecked, idx, move(blk) );
		}

		if (m_free[idx].ptr  == slab.ptr
		&&  m_free[idx].size == slab.size)
		{
			const_cast<Range&>(m_range[rangeIdx]).inUse = false;
			remove(m_free, m_free.getPtr(idx) );
		}

		_blk.invalidate(Blk::InvalidReason::Freed);
		_blk.allocatorOp = AllocatorOp::Free;

		return move(_blk);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void FirstFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::remove(FreeBlocks& _free, const Blk* _it)
	{
		const uint32_t idx = _free.getIndex(_it);
		Blk removed = _free.pop(idx);
		removed.invalidate();
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::BitArrayAllocatorT(Blk&& _blk, BitArrayT& _bitArray)
		: m_blk(move(_blk) )
		, m_bitArray(_bitArray)
	{
		BX_ASSERT(m_blk.size / kTraits.minBlockSize == m_bitArray.getCount()
			, "BitArray can't represent whole block (Blk.size %llu, kTraits.minBlockSize %zu, num blocks %llu, num bits %d)!"
			, uint64_t(m_blk.size)
			, size_t(kTraits.minBlockSize)
			, uint64_t(m_blk.size / kTraits.minBlockSize)
			, m_bitArray.getCount()
			);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::~BitArrayAllocatorT()
	{
		m_blk.invalidate();
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline Blk BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		BX_ASSERT(!isValid(_blk) || owns(_blk), "Blk passed is not owned by this allocator!");

		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Noop:
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);

		case AllocatorOp::Allocate:
			return alloc(_size, _alignment);

		case AllocatorOp::Free:
			return free(move(_blk) );

		case AllocatorOp::Grow:
			return grow(move(_blk), _size);

		case AllocatorOp::Realign:
			return Blk(Blk::InvalidReason::NotImplemented);

		case AllocatorOp::Truncate:
			return truncate(move(_blk), _size);

		default:
			BX_ASSERT(false, "You should not be here (AllocatorOp: %d)!", allocatorOp);
			break;
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline bool BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::owns(const Blk& _blk)
	{
		return contain(m_blk, _blk);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline bool BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::isCpuAccessible() const
	{
		return CpuAccessibleT;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline Blk BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::alloc(size_t _size, size_t _alignment)
	{
		const size_t alignment = max<size_t>(_alignment, 1);

		BX_ASSERT(isAligned(m_blk.ptr, min<size_t>(alignment, kTraits.minBlockSize) )
			, "Slab 0x%llx is not aligned to requested alignment %zu."
			, uint64_t(m_blk.ptr)
			, alignment
			);

		const size_t size    = alignUp(max(_size, kTraits.minBlockSize), kTraits.minBlockSize);
		const size_t numBits = size/kTraits.minBlockSize;
		const size_t padBits = alignment > kTraits.minBlockSize ? alignment/kTraits.minBlockSize - 1 : 0;

		if (numBits + padBits > m_bitArray.getCount() )
		{
			return Blk(Blk::InvalidReason::NoBlockLargeEnough);
		}

		const uint32_t pos = find(m_bitArray, narrowCast<uint32_t>(numBits + padBits) );

		if (kInvalid == pos)
		{
			return Blk(Blk::InvalidReason::NoBlockLargeEnough);
		}

		const uint64_t ptr       = alignUp(m_blk.ptr + uint64_t(pos)*kTraits.minBlockSize, alignment);
		const uint32_t posOffset = narrowCast<uint32_t>( (ptr - m_blk.ptr)/kTraits.minBlockSize);

		m_bitArray.set(posOffset, posOffset + narrowCast<uint32_t>(numBits), true);

		return Blk(ptr, size, _alignment, CpuAccessibleT, AllocatorOp::Allocate);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline Blk BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::free(Blk&& _blk)
	{
		BX_ASSERT(owns(_blk), "Blk passed is not owned by this allocator!");

		const uint64_t  offset  = _blk.ptr - m_blk.ptr;
		const uint32_t  pos     = narrowCast<uint32_t>(offset/kTraits.minBlockSize);
		const uint32_t  numBits = narrowCast<uint32_t>(_blk.size/kTraits.minBlockSize);

		m_bitArray.set(pos, pos+numBits, false);

		_blk.invalidate(Blk::InvalidReason::Freed);
		_blk.allocatorOp = AllocatorOp::Free;

		return move(_blk);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline Blk BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::grow(Blk&& _blk, size_t _size)
	{
		const size_t size = alignUp(max(_size, kTraits.minBlockSize), kTraits.minBlockSize);

		const uint64_t  offset     = _blk.ptr - m_blk.ptr;
		const uint32_t  pos        = narrowCast<uint32_t>(offset/kTraits.minBlockSize);
		const uint32_t  numBitsOld = narrowCast<uint32_t>(_blk.size/kTraits.minBlockSize);
		const uint32_t  numBitsNew = narrowCast<uint32_t>(size/kTraits.minBlockSize);

		if (m_bitArray.getCount() >= pos+numBitsNew
		&&  m_bitArray.testNone(pos+numBitsOld, pos+numBitsNew) )
		{
			m_bitArray.set(pos+numBitsOld, pos+numBitsNew, true);

			_blk.size = size;
			_blk.allocatorOp = AllocatorOp::Grow;

			return move(_blk);
		}

		if (!isCpuAccessible() )
		{
			return Blk(Blk::InvalidReason::ResizeFailed);
		}

		Error err;
		Blk blk = internal::tryAllocCopyFreeNoTrack(this, move(_blk), size, 0, &err);

		if (!err.isOk() )
		{
			blk.invalidate();
			return Blk(Blk::InvalidReason::NoBlockLargeEnough);
		}

		blk.allocatorOp = AllocatorOp::Grow;
		return blk;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline Blk BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::truncate(Blk&& _blk, size_t _size)
	{
		const size_t size = alignUp(max(_size, kTraits.minBlockSize), kTraits.minBlockSize);
		if (_size == _blk.size)
		{
			return move(_blk);
		}

		const uint64_t  offset     = _blk.ptr - m_blk.ptr;
		const uint32_t  pos        = narrowCast<uint32_t>(offset/kTraits.minBlockSize);
		const uint32_t  numBitsOld = narrowCast<uint32_t>(_blk.size/kTraits.minBlockSize);
		const uint32_t  numBitsNew = narrowCast<uint32_t>(size/kTraits.minBlockSize);

		m_bitArray.set(pos+numBitsNew, pos+numBitsOld, false);

		_blk.size = size;
		_blk.allocatorOp = AllocatorOp::Truncate;

		return move(_blk);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline uint32_t BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::find(const MippedBitArrayBase& _bitArray, uint32_t _numBits)
	{
		return 1 == _numBits
			? _bitArray.findClear(0, _bitArray.getCount() )
			: _bitArray.find(_numBits)
			;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT, typename BitArrayT>
	inline uint32_t BitArrayAllocatorT<MinBlockSizeT, CpuAccessibleT, BitArrayT>::find(const BitArrayViewBase& _bitArray, uint32_t _numBits)
	{
		return _bitArray.find(_numBits);
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::FrameRingAllocatorT(Blk&& _blk)
		: m_blk(move(_blk) )
		, m_head(0)
		, m_used(0)
		, m_frameSize(0)
		, m_firstFrame(0)
		, m_numFrames(0)
	{
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::~FrameRingAllocatorT()
	{
		m_blk.invalidate();
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline Blk FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Noop:
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);

		case AllocatorOp::Allocate:
			return alloc(_size, _alignment);

		case AllocatorOp::Free:
			return free(move(_blk) );

		default:
			break;
		}

		if (AllocatorOp::Truncate == allocatorOp
		&&  isAligned(_blk.ptr, _alignment) )
		{
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);
		}

		if (!CpuAccessibleT)
		{
			return Blk(Blk::InvalidReason::ResizeFailed);
		}

		Error err;
		Blk blk = internal::tryAllocCopyFreeNoTrack(this, move(_blk), _size, _alignment, &err);

		if (!err.isOk() )
		{
			blk.invalidate();
			return Blk(Blk::InvalidReason::NoBlockLargeEnough);
		}

		blk.allocatorOp = allocatorOp;
		return blk;
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline bool FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::owns(const Blk& _blk)
	{
		return contain(m_blk, _blk);
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline bool FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::isCpuAccessible() const
	{
		return CpuAccessibleT;
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline bool FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::endFrame(uint64_t _fence)
	{
		if (MaxFramesT == m_numFrames)
		{
			return false;
		}

		Frame& frame = m_frame[(m_firstFrame + m_numFrames) % MaxFramesT];
		frame.fence = _fence;
		frame.size  = m_frameSize;

		++m_numFrames;
		m_frameSize = 0;

		return true;
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline void FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::retire(uint64_t _completedFence)
	{
		while (0 != m_numFrames
		&&     m_frame[m_firstFrame].fence <= _completedFence)
		{
			m_used -= m_frame[m_firstFrame].size;
			m_firstFrame = (m_firstFrame + 1) % MaxFramesT;
			--m_numFrames;
		}

		if (0 == m_used)
		{
			m_head = 0;
		}
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline size_t FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::getUsed() const
	{
		return size_t(m_used);
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline uint32_t FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::getNumFramesInFlight() const
	{
		return m_numFrames;
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline Blk FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::free(Blk&& _blk)
	{
		_blk.invalidate(Blk::InvalidReason::Freed);
		_blk.allocatorOp = AllocatorOp::Free;
		return move(_blk);
	}

	template<bool CpuAccessibleT, uint32_t MaxFramesT>
	inline Blk FrameRingAllocatorT<CpuAccessibleT, MaxFramesT>::alloc(size_t _size, size_t _alignment)
	{
		const uint64_t capacity  = m_blk.size;
		const size_t   alignment = max<size_t>(_alignment, kTraits.minBlockAlignment);
		const uint64_t size      = alignUp(max<uint64_t>(_size, kTraits.minBlockSize), kTraits.minBlockAlignment);

		uint64_t pos  = alignUp(m_blk.ptr + m_head, alignment) - m_blk.ptr;
		uint64_t need = pos - m_head + size;

		if (pos + size > capacity)
		{
			pos  = alignUp(m_blk.ptr, alignment) - m_blk.ptr;
			need = capacity - m_head + pos + size;
		}

		if (m_used + need > capacity)
		{
			return Blk(Blk::InvalidReason::NoBlockLargeEnough);
		}

		m_used      += need;
		m_frameSize += need;
		m_head       = pos + size;

		return Blk(m_blk.ptr + pos, size_t(size), _alignment, CpuAccessibleT, AllocatorOp::Allocate);
	}

	template<bool CpuAccessibleT>
	inline DoubleEndedStackAllocatorT<CpuAccessibleT>::DoubleEndedStackAllocatorT(Blk&& _blk, AllocatorBlkI* _metaDataAllocator)
		: m_blk(move(_blk) )
		, m_low(_metaDataAllocator, m_blk.ptr, false)
		, m_high(_metaDataAllocator, m_blk.ptr + m_blk.size, true)
	{
		m_low.m_other  = &m_high;
		m_high.m_other = &m_low;
	}

	template<bool CpuAccessibleT>
	inline DoubleEndedStackAllocatorT<CpuAccessibleT>::~DoubleEndedStackAllocatorT()
	{
		m_blk.invalidate();
	}

	template<bool CpuAccessibleT>
	inline typename DoubleEndedStackAllocatorT<CpuAccessibleT>::End&
	DoubleEndedStackAllocatorT<CpuAccessibleT>::getLow()
	{
		return m_low;
	}

	template<bool CpuAccessibleT>
	inline typename DoubleEndedStackAllocatorT<CpuAccessibleT>::End&
	DoubleEndedStackAllocatorT<CpuAccessibleT>::getHigh()
	{
		return m_high;
	}

	template<bool CpuAccessibleT>
	inline size_t DoubleEndedStackAllocatorT<CpuAccessibleT>::getFree() const
	{
		return size_t(m_high.m_pos - m_low.m_pos);
	}

	template<bool CpuAccessibleT>
	inline DoubleEndedStackAllocatorT<CpuAccessibleT>::End::End(AllocatorBlkI* _metaDataAllocator, uint64_t _start, bool _high)
		: m_entry(_metaDataAllocator->getHandle() )
		, m_other(NULL)
		, m_start(_start)
		, m_pos(_start)
		, m_high(_high)
	{
	}

	template<bool CpuAccessibleT>
	inline Blk DoubleEndedStackAllocatorT<CpuAccessibleT>::End::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Noop:
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);

		case AllocatorOp::Allocate:
			return alloc(_size, _alignment);

		case AllocatorOp::Free:
			return free(move(_blk) );

		default:
			break;
		}

		return resize(move(_blk), _size, _alignment, allocatorOp);
	}

	template<bool CpuAccessibleT>
	inline bool DoubleEndedStackAllocatorT<CpuAccessibleT>::End::owns(const Blk& _blk)
	{
		const Blk used = m_high
			? Blk(m_pos,   size_t(m_start - m_pos),   0, CpuAccessibleT, AllocatorOp::Alias)
			: Blk(m_start, size_t(m_pos   - m_start), 0, CpuAccessibleT, AllocatorOp::Alias)
			;

		return contain(used, _blk);
	}

	template<bool CpuAccessibleT>
	inline bool DoubleEndedStackAllocatorT<CpuAccessibleT>::End::isCpuAccessible() const
	{
		return CpuAccessibleT;
	}

	template<bool CpuAccessibleT>
	inline void DoubleEndedStackAllocatorT<CpuAccessibleT>::End::reset()
	{
		m_entry.clear();
		m_pos = m_start;
	}

	template<bool CpuAccessibleT>
	inline size_t DoubleEndedStackAllocatorT<CpuAccessibleT>::End::getUsed() const
	{
		return size_t(m_high ? m_start - m_pos : m_pos - m_start);
	}

	template<bool CpuAccessibleT>
	inline Blk DoubleEndedStackAllocatorT<CpuAccessibleT>::End::alloc(size_t _size, size_t _alignment)
	{
		const size_t   alignment = max<size_t>(_alignment, kTraits.minBlockAlignment);
		const uint64_t size      = alignUp(max<uint64_t>(_size, kTraits.minBlockSize), kTraits.minBlockAlignment);

		uint64_t ptr;

		if (m_high)
		{
			if (m_pos - m_other->m_pos < size)
			{
				return Blk(Blk::InvalidReason::NoBlockLargeEnough);
			}

			ptr = alignDown(m_pos - size, alignment);

			if (ptr < m_other->m_pos)
			{
				return Blk(Blk::InvalidReason::NoBlockLargeEnough);
			}
		}
		else
		{
			ptr = alignUp(m_pos, alignment);

			if (ptr > m_other->m_pos
			||  m_other->m_pos - ptr < size)
			{
				return Blk(Blk::InvalidReason::NoBlockLargeEnough);
			}
		}

		Entry entry;
		entry.begin = m_pos;
		entry.ptr   = ptr;
		entry.size  = size;
		entry.freed = false;
		m_entry.add(entry);

		m_pos = m_high ? ptr : ptr + size;

		Blk blk(ptr, size_t(size), _alignment, CpuAccessibleT, AllocatorOp::Allocate);
		blk.bookKeeping = m_entry.getCount() - 1;

		return blk;
	}

	template<bool CpuAccessibleT>
	inline Blk DoubleEndedStackAllocatorT<CpuAccessibleT>::End::free(Blk&& _blk)
	{
		const uint32_t idx = _blk.bookKeeping;

		BX_ASSERT(idx < m_entry.getCount() && m_entry[idx].ptr == _blk.ptr && !m_entry[idx].freed
			, "Block 0x%llx isn't allocated from this end."
			, _blk.ptr
			);

		m_entry[idx].freed = true;

		for (uint32_t num = m_entry.getCount(); 0 < num && m_entry[num-1].freed; --num)
		{
			m_pos = m_entry[num-1].begin;
			m_entry.remove(kLast);
		}

		_blk.invalidate(Blk::InvalidReason::Freed);
		_blk.allocatorOp = AllocatorOp::Free;

		return move(_blk);
	}

	template<bool CpuAccessibleT>
	inline Blk DoubleEndedStackAllocatorT<CpuAccessibleT>::End::resize(Blk&& _blk, size_t _size, size_t _alignment, AllocatorOp::Enum _allocatorOp)
	{
		const uint32_t idx  = _blk.bookKeeping;
		const uint64_t size = alignUp(max<uint64_t>(_size, kTraits.minBlockSize), kTraits.minBlockAlignment);

		if (isAligned(_blk.ptr, _alignment) )
		{
			if (AllocatorOp::Truncate == _allocatorOp)
			{
				_blk.allocatorOp = AllocatorOp::Noop;
				return move(_blk);
			}

			if (!m_high
			&&  idx + 1 == m_entry.getCount()
			&&  m_other->m_pos - _blk.ptr >= size)
			{
				m_entry[idx].size = size;
				m_pos = _blk.ptr + size;

				_blk.size        = size_t(size);
				_blk.allocatorOp = _allocatorOp;
				return move(_blk);
			}
		}

		if (!CpuAccessibleT)
		{
			return Blk(Blk::InvalidReason::ResizeFailed);
		}

		Error err;
		Blk blk = internal::tryAllocCopyFreeNoTrack(this, move(_blk), _size, _alignment, &err);

		if (!err.isOk() )
		{
			blk.invalidate();
			return Blk(Blk::InvalidReason::NoBlockLargeEnough);
		}

		blk.allocatorOp = _allocatorOp;
		return blk;
	}

	namespace internal
	{
		inline TwoLevelSegregatedFitLookup::TwoLevelSegregatedFitLookup()
			: m_firstLevel(0)
		{
			for (uint32_t fl = 0; fl < kFirstLevelCount; ++fl)
			{
				m_secondLevel[fl] = 0;

				for (uint32_t sl = 0; sl < kSecondLevelCount; ++sl)
				{
					m_idx[fl][sl] = kInvalid;
				}
			}
		}

		inline uint32_t TwoLevelSegregatedFitLookup::find(size_t _size) const
		{
			uint32_t fl, sl;
			mapping(_size, &fl, &sl);

			uint32_t second = m_secondLevel[fl] & (~0u << sl);
			if (0 == second)
			{
				const uint32_t first = m_firstLevel & (~0u << (fl + 1) );
				if (0 == first)
				{
					return kInvalid;
				}

				fl = findFirstSet(first)-1;

				second = m_secondLevel[fl];
			}

			BX_ASSERT(0 != second, "");

			sl = findFirstSet(second)-1;

			return m_idx[fl][sl];
		}

		inline void TwoLevelSegregatedFitLookup::set(size_t _size, uint32_t _idx)
		{
			uint32_t fl, sl;
			mapping(_size, &fl, &sl);

			m_idx[fl][sl] = _idx;

			if (kInvalid == _idx)
			{
				m_secondLevel[fl] &= ~(1u << sl);

				if (0 == m_secondLevel[fl])
				{
					m_firstLevel &= ~(1u << fl);
				}
			}
			else
			{
				m_firstLevel      |= 1u << fl;
				m_secondLevel[fl] |= 1u << sl;
			}
		}

		inline uint32_t TwoLevelSegregatedFitLookup::get(size_t _size) const
		{
			uint32_t fl, sl;
			mapping(_size, &fl, &sl);

			return m_idx[fl][sl];
		}

		inline uint32_t TwoLevelSegregatedFitLookup::getBin(size_t _size)
		{
			uint32_t fl, sl;
			mapping(_size, &fl, &sl);

			return (fl << kSecondLevelShift) | sl;
		}

		inline uint32_t TwoLevelSegregatedFitLookup::getIdx(uint32_t _bin) const
		{
			return m_idx[_bin >> kSecondLevelShift][_bin & (kSecondLevelCount-1)];
		}

		inline void TwoLevelSegregatedFitLookup::setIdx(uint32_t _bin, uint32_t _idx)
		{
			const uint32_t fl = _bin >> kSecondLevelShift;
			const uint32_t sl = _bin & (kSecondLevelCount-1);

			m_idx[fl][sl] = _idx;

			if (kInvalid == _idx)
			{
				m_secondLevel[fl] &= ~(1u << sl);

				if (0 == m_secondLevel[fl])
				{
					m_firstLevel &= ~(1u << fl);
				}
			}
			else
			{
				m_firstLevel      |= 1u << fl;
				m_secondLevel[fl] |= 1u << sl;
			}
		}

		inline size_t TwoLevelSegregatedFitLookup::roundBlockSize(size_t _size)
		{
			const size_t mask = (size_t(1) << (floorLog2(_size) - kSecondLevelShift) ) - 1;
			return _size >= kBlockSizeSmall ? (_size + mask) & ~mask : _size;
		}

		inline void TwoLevelSegregatedFitLookup::mapping(size_t _size, uint32_t* _outFl, uint32_t* _outSl)
		{
			if (_size < kBlockSizeSmall)
			{
				*_outFl = 0;
				*_outSl = uint32_t(_size) / (kBlockSizeSmall / kSecondLevelCount);
			}
			else
			{
				const uint32_t sz = floorLog2(_size);
				*_outFl = sz - kFirstLevelShift + 1;
				*_outSl = bx::narrowCast<uint32_t>( (_size >> (sz - kSecondLevelShift) ) ^ kSecondLevelCount);
			}

			BX_ASSERT(*_outFl < kFirstLevelCount,  "Wrong first level %d, %zu!",  *_outFl, _size);
			BX_ASSERT(*_outSl < kSecondLevelCount, "Wrong second level %d, %zu!", *_outSl, _size);
		}

	} // namespace internal

	template<>
	inline int32_t compareAscending<internal::RangeTlsf>(const void* _lhs, const void* _rhs)
	{
		return compareAscending<uint64_t>(&( (const internal::RangeTlsf*)_lhs)->blk.ptr, &( (const internal::RangeTlsf*)_rhs)->blk.ptr);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::TwoLevelSegregatedFitAllocatorT(AllocatorBlkI* _metaDataAllocator)
		: m_blkFreeList(_metaDataAllocator->getHandle(), 256)
		, m_range(_metaDataAllocator->getHandle() )
		, m_total(0)
		, m_used(0)
		, m_lastCarveSize(0)
		, m_lastCarveIdx(kInvalid)
		, m_tailIdx(kInvalid)
	{
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::~TwoLevelSegregatedFitAllocatorT()
	{
		for (internal::RangeTlsf* it = m_range.getPtr(Unchecked), *itEnd = m_range.getTerm(Unchecked); it != itEnd; ++it)
		{
			for (uint32_t idx = it->sentinelIdx; idx != kInvalid; )
			{
				const uint32_t prevIdx = m_blkFreeList[idx].prevIdx;
				BlkMeta meta = m_blkFreeList.pop(idx);
				meta.blk.invalidate();

				idx = prevIdx;
			}

			it->blk.invalidate();
		}

		m_range.clear();
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		{
			const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

			switch (allocatorOp)
			{
			case AllocatorOp::Noop:
				_blk.allocatorOp = AllocatorOp::Noop;
				return move(_blk);

			case AllocatorOp::Free:
				return freeBlock(move(_blk) );

			default:
				break;
			}
		}

		{
			const size_t alignment = max(kTraits.minBlockAlignment, _alignment);
			const size_t adjusted  = adjustSize(_size, narrowCast<int32_t>(alignment) );
			const size_t size      = internal::TwoLevelSegregatedFitLookup::roundBlockSize(adjusted);

			const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, size, alignment);

			switch (allocatorOp)
			{
			case AllocatorOp::Noop:
				_blk.allocatorOp = AllocatorOp::Noop;
				return move(_blk);

			case AllocatorOp::Allocate:
				return allocBlock(size, alignment);

			case AllocatorOp::Grow:
				return grow(move(_blk), size, alignment);

			case AllocatorOp::Realign:
				return grow(move(_blk), size, alignment);

			case AllocatorOp::Truncate:
				return truncate(move(_blk), size, alignment);

			default:
				BX_ASSERT(false, "You should not be here (AllocatorOp: %d)!", allocatorOp);
				break;
			}
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::alloc(size_t _size, size_t _alignment)
	{
		const size_t alignment = max(kTraits.minBlockAlignment, _alignment);
		const size_t adjusted  = adjustSize(_size, narrowCast<int32_t>(alignment) );
		const size_t size      = internal::TwoLevelSegregatedFitLookup::roundBlockSize(adjusted);

		return allocBlock(size, alignment);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::free(Blk&& _blk)
	{
		return freeBlock(move(_blk) );
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline bool TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::owns(const Blk& _blk)
	{
		internal::RangeTlsf range;
		range.blk = Blk(Copy, _blk);

		const uint32_t idxLb = bx::satSub<uint32_t>(upperBound(range, m_range), 1u);

		if (idxLb < m_range.getCount() )
		{
			return contain(m_range[idxLb].blk, _blk);
		}

		return false;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline bool TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::isCpuAccessible() const
	{
		return CpuAccessibleT;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::addSlab(Blk&& _blk)
	{
		BX_ASSERT(isAligned(_blk.ptr, kTraits.minBlockAlignment)
			, "Slab must be aligned to %zd (ptr 0x%llx)."
			, kTraits.minBlockAlignment
			, _blk.ptr
			);

		m_total += _blk.size;

		Blk free(_blk.ptr, _blk.size, 0, CpuAccessibleT, AllocatorOp::Allocate);

		BlkMeta sentinel;
		sentinel.blk   = Blk(free.ptr + free.size, 0, 0, CpuAccessibleT, AllocatorOp::Allocate);
		const uint32_t sentinelIdx = m_blkFreeList.add(move(sentinel) );

		internal::RangeTlsf range;
		range.blk         = move(_blk);
		range.sentinelIdx = sentinelIdx;
		m_range.add(move(range) );

		BlkMeta meta;
		meta.blk     = move(free);
		meta.nextIdx = sentinelIdx;
		meta.flags   = BlkMeta::kFlagFree;
		const uint32_t idx = m_blkFreeList.add(move(meta) );

		m_blkFreeList[sentinelIdx].prevIdx = idx;

		linkFreeBlock(idx);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::removeUnusedSlab()
	{
		for (uint32_t ii = 0, num = m_range.getCount(); ii < num; ++ii)
		{
			const uint32_t sentinelIdx = m_range[ii].sentinelIdx;
			const uint32_t idx         = m_blkFreeList[sentinelIdx].prevIdx;
			const BlkMeta& meta        = m_blkFreeList[idx];

			if (meta.isFree()
			&&  kInvalid == meta.prevIdx
			&&  meta.blk.ptr == m_range[ii].blk.ptr)
			{
				unlinkFreeBlock(idx);
				m_blkFreeList[idx].blk.invalidate();
				m_blkFreeList.remove(idx);
				m_blkFreeList[sentinelIdx].blk.invalidate();
				m_blkFreeList.remove(sentinelIdx);

				internal::RangeTlsf range = m_range.pop(ii);
				m_total -= range.blk.size;

				return move(range.blk);
			}
		}

		return bx::Blk();
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline size_t TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::getTotal() const
	{
		return m_total;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline size_t TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::getUsed() const
	{
		return m_used;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::BlkMeta::BlkMeta()
		: prevIdx(kInvalid)
		, nextIdx(kInvalid)
		, freePrevIdx(kInvalid)
		, freeNextIdx(kInvalid)
		, flags(kFlagNone)
	{
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline bool TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::BlkMeta::isFree() const
	{
		return !!(flags & kFlagFree);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline bool TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::BlkMeta::isPrevFree() const
	{
		return !!(flags & kFlagPrevFree);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline bool TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::BlkMeta::isUnlinked() const
	{
		return true
			&& kInvalid == freePrevIdx
			&& kInvalid == freeNextIdx
			;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline bool TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::BlkMeta::isSentinel() const
	{
		return true
			&& 0 == blk.size
			&& !isFree()
			;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline bool TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::BlkMeta::isTail() const
	{
		return !!(flags & kFlagTail);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline size_t TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::adjustSize(size_t _size, size_t _alignment)
	{
		_size = alignUp(_size, _alignment);
		return max(_size, kTraits.minBlockSize);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::setFree(uint32_t _idx, bool _free)
	{
		BlkMeta& meta = m_blkFreeList[_idx];

		if (_free)
		{
			meta.flags |= BlkMeta::kFlagFree;
			m_blkFreeList[meta.nextIdx].flags |= BlkMeta::kFlagPrevFree;

			linkFreeBlock(_idx);
		}
		else
		{
			unlinkFreeBlock(_idx);

			meta.flags &= ~BlkMeta::kFlagFree;
			m_blkFreeList[meta.nextIdx].flags &= ~BlkMeta::kFlagPrevFree;
		}
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::splitFreeBlock(uint32_t _idx, size_t _size, size_t _alignment)
	{
		unlinkFreeBlock(_idx);

		BlkMeta& meta = m_blkFreeList[_idx];
		BX_ASSERT(!meta.isSentinel(), "");

		const uint64_t ptr = alignUp(meta.blk.ptr, _alignment);

		if (ptr != meta.blk.ptr)
		{
			const size_t   size    = size_t(ptr - meta.blk.ptr);
			const uint32_t prevIdx = meta.prevIdx;

			if (meta.isPrevFree() )
			{
				unlinkFreeBlock(prevIdx);
				m_blkFreeList[prevIdx].blk.size += size;
				linkFreeBlock(prevIdx);
			}
			else
			{
				BlkMeta free;
				free.blk = Blk(meta.blk.ptr, size, 0, CpuAccessibleT, AllocatorOp::Noop);
				free.prevIdx = prevIdx;
				free.nextIdx = _idx;
				free.flags   = BlkMeta::kFlagFree;
				const uint32_t freeIdx = m_blkFreeList.add(move(free) );

				if (kInvalid != prevIdx)
				{
					m_blkFreeList[prevIdx].nextIdx = freeIdx;
				}

				meta.prevIdx = freeIdx;
				meta.flags |= BlkMeta::kFlagPrevFree;

				linkFreeBlock(freeIdx);
			}

			meta.blk.ptr   = ptr;
			meta.blk.size -= size;
		}

		const uint32_t nextIdx = meta.nextIdx;

		BlkMeta free;
		free.blk = Blk(meta.blk.ptr + _size, meta.blk.size - _size, 0, CpuAccessibleT, AllocatorOp::Noop);
		free.prevIdx = _idx;
		free.nextIdx = nextIdx;
		free.flags   = BlkMeta::kFlagFree | BlkMeta::kFlagPrevFree;
		const uint32_t freeIdx = m_blkFreeList.add(move(free) );

		m_blkFreeList[nextIdx].prevIdx = freeIdx;

		meta.nextIdx  = freeIdx;
		meta.blk.size = _size;

		linkFreeBlock(_idx);
		linkFreeBlock(freeIdx);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::dump(const StringLiteral& _str, uint32_t _idx)
	{
		const BlkMeta& meta = m_blkFreeList[_idx];
		BX_TRACE("BlkMeta[%d] 0x%llx, %llu // %S", _idx, meta.blk.ptr, uint64_t(meta.blk.size), &_str);
		BX_TRACE("\tprev/next      %d, %d", meta.prevIdx, meta.nextIdx);
		BX_TRACE("\tprev/next free %d, %d", meta.freePrevIdx, meta.freeNextIdx);
		BX_TRACE("\t[%c] free, [%c] prev free"
			, meta.isFree() ? 'x' : ' '
			, meta.isPrevFree() ? 'x' : ' '
			);

		BX_UNUSED(_str, _idx, meta);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::linkFreeBlock(uint32_t _idx)
	{
		linkFreeBlock(_idx, internal::TwoLevelSegregatedFitLookup::getBin(m_blkFreeList[_idx].blk.size) );
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::linkFreeBlock(uint32_t _idx, uint32_t _bin)
	{
		linkFreeBlock(_idx, m_blkFreeList[_idx], _bin);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::linkFreeBlock(uint32_t _idx, BlkMeta& _meta, uint32_t _bin)
	{
		BX_ASSERT(_meta.isFree(), "Block isn't free (idx: %d).", _idx);

		BlkMeta& next = m_blkFreeList[_meta.nextIdx];

		if (next.isSentinel() )
		{
			_meta.freePrevIdx = kInvalid;
			_meta.freeNextIdx = kInvalid;
			_meta.flags      |= BlkMeta::kFlagTail;
			next.flags       |= BlkMeta::kFlagPrevFree;
			m_lastCarveIdx    = kInvalid;
			m_tailIdx         = _idx;
			return;
		}
		BX_ASSERT(_bin == internal::TwoLevelSegregatedFitLookup::getBin(_meta.blk.size), "");

		const uint32_t currentIdx = m_lookup.getIdx(_bin);

		BX_ASSERT(_meta.isUnlinked() && _meta.isFree()
			, "ptr 0x%llx, size %llu, u %d, f %d"
			, _meta.blk.ptr
			, uint64_t(_meta.blk.size)
			, _meta.isUnlinked()
			, _meta.isFree()
			);
		_meta.freePrevIdx = kInvalid;
		_meta.freeNextIdx = currentIdx;
		m_blkFreeList[_meta.nextIdx].flags |= BlkMeta::kFlagPrevFree;

		if (kInvalid != currentIdx)
		{
			BX_ASSERT(kInvalid == m_blkFreeList[currentIdx].freePrevIdx, "");
			m_blkFreeList[currentIdx].freePrevIdx = _idx;
		}

		m_lookup.setIdx(_bin, _idx);
		m_lastCarveIdx = kInvalid;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::unlinkFreeBlock(uint32_t _idx)
	{
		unlinkFreeBlock(_idx, internal::TwoLevelSegregatedFitLookup::getBin(m_blkFreeList[_idx].blk.size) );
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::unlinkFreeBlock(uint32_t _idx, uint32_t _bin)
	{
		unlinkFreeBlock(_idx, m_blkFreeList[_idx], _bin);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::unlinkFreeBlock(uint32_t _idx, BlkMeta& _meta, uint32_t _bin)
	{
		BX_ASSERT(_meta.isFree(), "Block isn't free (idx: %d).", _idx);

		if (_meta.isTail() )
		{
			_meta.flags   &= ~BlkMeta::kFlagTail;
			m_lastCarveIdx = kInvalid;

			if (m_tailIdx == _idx)
			{
				m_tailIdx = kInvalid;
			}

			return;
		}
		BX_ASSERT(_bin == internal::TwoLevelSegregatedFitLookup::getBin(_meta.blk.size), "");

		const uint32_t freePrevIdx = _meta.freePrevIdx;
		const uint32_t freeNextIdx = _meta.freeNextIdx;

		m_lastCarveIdx = kInvalid;

		if (m_lookup.getIdx(_bin) == _idx)
		{
			BX_ASSERT(kInvalid == freePrevIdx, "");
			m_lookup.setIdx(_bin, freeNextIdx);
		}

		_meta.freePrevIdx = kInvalid;
		_meta.freeNextIdx = kInvalid;

		if (kInvalid != freeNextIdx)
		{
			BX_ASSERT(m_blkFreeList[freeNextIdx].freePrevIdx == _idx, "");
			m_blkFreeList[freeNextIdx].freePrevIdx = freePrevIdx;
		}

		if (kInvalid != freePrevIdx)
		{
			BX_ASSERT(m_blkFreeList[freePrevIdx].freeNextIdx == _idx, "");
			m_blkFreeList[freePrevIdx].freeNextIdx = freeNextIdx;
		}
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline uint32_t TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::findBlock(const uint32_t _sentinelIdx, const Blk& _blk) const
	{
		for (uint32_t idx = _sentinelIdx; idx != kInvalid; )
		{
			const BlkMeta& meta = m_blkFreeList[idx];
			if (_blk.ptr  == meta.blk.ptr
			&&  _blk.size == meta.blk.size)
			{
				return idx;
			}

			idx = meta.prevIdx;
		}

		return kInvalid;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline uint32_t TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::findBlock(const Blk& _blk) const
	{
		internal::RangeTlsf range;
		range.blk = Blk(Copy, _blk);

		const uint32_t idxLb = bx::satSub<uint32_t>(upperBound(range, m_range), 1u);

		if (idxLb >= m_range.getCount()
		||  !contain(m_range[idxLb].blk, _blk) )
		{
			return kInvalid;
		}

		const uint32_t idx = findBlock(m_range[idxLb].sentinelIdx, _blk);

		return idx;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::tryMergeBlocksNext(uint32_t _idx)
	{
		BlkMeta& meta = m_blkFreeList[_idx];
		const bool isFree = meta.isFree();

		size_t   size     = 0;
		uint32_t nextIdx  = m_blkFreeList[_idx].nextIdx;
		uint32_t mergeIdx = nextIdx;

		if (isFree)
		{
			unlinkFreeBlock(_idx);
		}

		for (;;)
		{
			BlkMeta& merge = m_blkFreeList[mergeIdx];
			nextIdx = merge.nextIdx;

			if (isFree != merge.isFree() || merge.isSentinel() )
			{
				break;
			}

			size += merge.blk.size;

			if (isFree)
			{
				unlinkFreeBlock(mergeIdx);
			}

			merge.blk.invalidate();
			m_blkFreeList.remove(mergeIdx);

			mergeIdx = nextIdx;
		}

		if (mergeIdx != nextIdx)
		{
			meta.blk.size += size;
			meta.nextIdx   = mergeIdx;

			m_blkFreeList[mergeIdx].prevIdx = _idx;
		}

		if (isFree)
		{
			linkFreeBlock(_idx);
		}
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline uint32_t TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::findTail(size_t _size) const
	{
		if (kInvalid != m_tailIdx)
		{
			const BlkMeta& meta = m_blkFreeList[m_tailIdx];

			if (meta.isTail()
			&&  _size <= meta.blk.size)
			{
				return m_tailIdx;
			}
		}

		for (uint32_t ii = 0, num = m_range.getCount(); ii < num; ++ii)
		{
			const uint32_t idx = m_blkFreeList[m_range[ii].sentinelIdx].prevIdx;

			if (kInvalid != idx)
			{
				const BlkMeta& meta = m_blkFreeList[idx];

				if (meta.isTail()
				&&  _size <= meta.blk.size)
				{
					return idx;
				}
			}
		}

		return kInvalid;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::allocBlock(size_t _size, size_t _alignment)
	{
		{
			const size_t alignment = max<size_t>(kTraits.minBlockAlignment, _alignment);
			const size_t adjusted  = adjustSize(_size, alignment);
			const size_t size      = internal::TwoLevelSegregatedFitLookup::roundBlockSize(adjusted);

			BX_ASSERT(_size <= size && _alignment == alignment
				, "size %zu > %zu, alignment %zu != %zu"
				, _size,  size
				, _alignment, alignment
				);
			BX_UNUSED(alignment, adjusted, size);
		}

		const size_t alignedSize = _alignment > kTraits.minBlockAlignment
			? _size + _alignment - kTraits.minBlockAlignment
			: _size
			;

		if (kInvalid != m_lastCarveIdx
		&&  m_lastCarveSize <= alignedSize)
		{
			BlkMeta& last = m_blkFreeList[m_lastCarveIdx];

			if (_size <= last.blk.size
			&&  isAligned(last.blk.ptr, _alignment) )
			{
				return carveFreeBlock(m_lastCarveIdx, last, _size, _alignment);
			}
		}

		const size_t searchSize = _alignment > kTraits.minBlockAlignment
			? internal::TwoLevelSegregatedFitLookup::roundBlockSize(alignedSize)
			: _size
			;
		uint32_t idx = m_lookup.find(searchSize);

		if (kInvalid == idx)
		{
			idx = findTail(searchSize);

			if (kInvalid == idx)
			{
				return Blk(Blk::InvalidReason::NoBlockLargeEnough);
			}
		}

		BlkMeta& meta = m_blkFreeList[idx];

		if (isAligned(meta.blk.ptr, _alignment) )
		{
			m_lastCarveSize = searchSize;
			m_lastCarveIdx  = idx;
			return carveFreeBlock(idx, meta, _size, _alignment);
		}

		return carveFreeBlockAligned(idx, meta, _size, _alignment);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::usedBlock(uint32_t _idx, uint64_t _ptr, size_t _size, size_t _alignment)
	{
		BX_ASSERT(isAligned(_ptr, _alignment), "");

		Blk blk(_ptr, _size, _alignment, CpuAccessibleT, AllocatorOp::Allocate);
		blk.bookKeeping = _idx;

		m_used += _size;

		return blk;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::carveFreeBlock(uint32_t _idx, BlkMeta& _meta, size_t _size, size_t _alignment)
	{
		const uint64_t ptr  = _meta.blk.ptr;
		const size_t   size = _meta.blk.size;
		const size_t   rest = size - _size;

		if (rest < kTraits.minBlockSize)
		{
			unlinkFreeBlock(_idx, _meta, internal::TwoLevelSegregatedFitLookup::getBin(size) );

			_meta.flags &= ~BlkMeta::kFlagFree;
			m_blkFreeList[_meta.nextIdx].flags &= ~BlkMeta::kFlagPrevFree;

			return usedBlock(_idx, ptr, size, _alignment);
		}

		const bool relink = !_meta.isTail()
			&& internal::TwoLevelSegregatedFitLookup::getBin(size) != internal::TwoLevelSegregatedFitLookup::getBin(rest)
			;

		if (relink)
		{
			unlinkFreeBlock(_idx, _meta, internal::TwoLevelSegregatedFitLookup::getBin(size) );
		}

		const uint32_t prevIdx = _meta.prevIdx;

		BlkMeta used;
		used.blk     = Blk(ptr, _size, 0, CpuAccessibleT, AllocatorOp::Noop);
		used.prevIdx = prevIdx;
		used.nextIdx = _idx;
		const uint32_t usedIdx = m_blkFreeList.add(move(used) );

		if (kInvalid != prevIdx)
		{
			m_blkFreeList[prevIdx].nextIdx = usedIdx;
		}

		_meta.prevIdx   = usedIdx;
		_meta.blk.ptr   = ptr + _size;
		_meta.blk.size  = rest;
		_meta.flags    &= ~BlkMeta::kFlagPrevFree;

		if (relink)
		{
			linkFreeBlock(_idx, _meta, internal::TwoLevelSegregatedFitLookup::getBin(rest) );
		}

		return usedBlock(usedIdx, ptr, _size, _alignment);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::carveFreeBlockAligned(uint32_t _idx, BlkMeta& _meta, size_t _size, size_t _alignment)
	{
		splitFreeBlock(_idx, _size, _alignment);
		setFree(_idx, false);

		return usedBlock(_idx, _meta.blk.ptr, _meta.blk.size, _alignment);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::freeBlock(Blk&& _blk)
	{
		const uint32_t idx = _blk.bookKeeping;

		BlkMeta& meta = m_blkFreeList[idx];
		BX_ASSERT(!meta.isFree(), "Block is already free (idx: %d).", idx);

		const uint64_t ptr      = meta.blk.ptr;
		const size_t   size     = meta.blk.size;
		const uint32_t prevIdx  = meta.prevIdx;
		const uint32_t nextIdx  = meta.nextIdx;
		const bool     prevFree = meta.isPrevFree();

		BlkMeta&   next     = m_blkFreeList[nextIdx];
		const bool nextFree = next.isFree();

		m_used -= size;

		if (prevFree)
		{
			BlkMeta& prev = m_blkFreeList[prevIdx];
			const uint32_t oldBin = internal::TwoLevelSegregatedFitLookup::getBin(prev.blk.size);

			size_t   mergeSize = size;
			uint32_t lastIdx   = nextIdx;

			if (nextFree)
			{
				unlinkFreeBlock(nextIdx, next, internal::TwoLevelSegregatedFitLookup::getBin(next.blk.size) );

				mergeSize += next.blk.size;
				lastIdx    = next.nextIdx;

				next.blk.invalidate();
				m_blkFreeList.remove(nextIdx);
			}

			const uint32_t newBin = internal::TwoLevelSegregatedFitLookup::getBin(prev.blk.size + mergeSize);

			if (oldBin != newBin)
			{
				unlinkFreeBlock(prevIdx, prev, oldBin);
			}

			prev.blk.size += mergeSize;
			prev.nextIdx   = lastIdx;

			BlkMeta& last = m_blkFreeList[lastIdx];
			last.prevIdx = prevIdx;
			last.flags  |= BlkMeta::kFlagPrevFree;

			meta.blk.invalidate();
			m_blkFreeList.remove(idx);

			if (oldBin != newBin)
			{
				linkFreeBlock(prevIdx, prev, newBin);
			}
		}
		else if (nextFree)
		{
			const bool relink = !next.isTail()
				&& internal::TwoLevelSegregatedFitLookup::getBin(next.blk.size) != internal::TwoLevelSegregatedFitLookup::getBin(next.blk.size + size)
				;

			if (relink)
			{
				unlinkFreeBlock(nextIdx, next, internal::TwoLevelSegregatedFitLookup::getBin(next.blk.size) );
			}

			next.blk.ptr   = ptr;
			next.blk.size += size;
			next.prevIdx   = prevIdx;

			if (kInvalid != prevIdx)
			{
				m_blkFreeList[prevIdx].nextIdx = nextIdx;
			}

			meta.blk.invalidate();
			m_blkFreeList.remove(idx);

			if (relink)
			{
				linkFreeBlock(nextIdx, next, internal::TwoLevelSegregatedFitLookup::getBin(next.blk.size) );
			}
		}
		else
		{
			meta.flags |= BlkMeta::kFlagFree;
			linkFreeBlock(idx, meta, internal::TwoLevelSegregatedFitLookup::getBin(size) );
		}

		_blk.invalidate(Blk::InvalidReason::Freed);
		_blk.allocatorOp = AllocatorOp::Free;

		return move(_blk);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline void TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::splitTail(uint32_t _idx, size_t _size)
	{
		const size_t rest = m_blkFreeList[_idx].blk.size - _size;

		if (rest < kTraits.minBlockSize)
		{
			return;
		}

		const uint32_t nextIdx = m_blkFreeList[_idx].nextIdx;

		BlkMeta free;
		free.blk = Blk(m_blkFreeList[_idx].blk.ptr + _size, rest, 0, CpuAccessibleT, AllocatorOp::Noop);
		free.prevIdx = _idx;
		free.nextIdx = nextIdx;
		free.flags   = BlkMeta::kFlagFree;
		const uint32_t freeIdx = m_blkFreeList.add(move(free) );

		m_blkFreeList[nextIdx].prevIdx = freeIdx;

		BlkMeta& meta = m_blkFreeList[_idx];
		meta.nextIdx  = freeIdx;
		meta.blk.size = _size;

		m_used -= rest;

		linkFreeBlock(freeIdx);
		tryMergeBlocksNext(freeIdx);
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::grow(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const uint32_t idx = _blk.bookKeeping;

		if (isAligned(_blk.ptr, _alignment) )
		{
			uint32_t num     = 0;
			size_t   size    = m_blkFreeList[idx].blk.size;
			uint32_t nextIdx = m_blkFreeList[idx].nextIdx;

			for (; size < _size; nextIdx = m_blkFreeList[nextIdx].nextIdx, ++num)
			{
				const BlkMeta& next = m_blkFreeList[nextIdx];

				if (!next.isFree() )
				{
					break;
				}

				size += next.blk.size;
			}

			if (size >= _size)
			{
				for (uint32_t removeIdx = m_blkFreeList[idx].nextIdx, ii = 0; ii < num; ++ii)
				{
					const uint32_t tmpIdx = m_blkFreeList[removeIdx].nextIdx;

					unlinkFreeBlock(removeIdx);
					m_blkFreeList[removeIdx].blk.invalidate();
					m_blkFreeList.remove(removeIdx);

					removeIdx = tmpIdx;
				}

				BlkMeta& next = m_blkFreeList[nextIdx];
				next.prevIdx = idx;
				next.flags  &= ~BlkMeta::kFlagPrevFree;

				BlkMeta& meta = m_blkFreeList[idx];
				m_used += size - meta.blk.size;
				meta.blk.size = size;
				meta.nextIdx  = nextIdx;

				splitTail(idx, _size);

				_blk.size        = m_blkFreeList[idx].blk.size;
				_blk.allocatorOp = AllocatorOp::Grow;

				return move(_blk);
			}
		}

		Error err;
		Blk blk = internal::tryAllocCopyFreeNoTrack(this, move(_blk), _size, _alignment, &err);

		if (!err.isOk() )
		{
			blk.invalidate();
			return Blk(Blk::InvalidReason::NoBlockLargeEnough);
		}

		blk.allocatorOp = AllocatorOp::Grow;
		return blk;
	}

	template<uint32_t MinBlockSizeT, bool CpuAccessibleT>
	inline Blk TwoLevelSegregatedFitAllocatorT<MinBlockSizeT, CpuAccessibleT>::truncate(Blk&& _blk, size_t _size, size_t _alignment)
	{
		if (!isAligned(_blk.ptr, _alignment) )
		{
			Blk blk = grow(move(_blk), _size, _alignment);

			if (isValid(blk) )
			{
				blk.allocatorOp = AllocatorOp::Realign;
			}

			return blk;
		}

		const uint32_t idx = _blk.bookKeeping;
		BX_ASSERT(!m_blkFreeList[idx].isSentinel(), "");

		splitTail(idx, _size);

		_blk.size        = m_blkFreeList[idx].blk.size;
		_blk.allocatorOp = AllocatorOp::Truncate;

		return move(_blk);
	}

	template<typename AllocatorBlkT>
	inline IntrusiveBlkAllocatorT<AllocatorBlkT>::IntrusiveBlkAllocatorT(AllocatorBlkT& _allocator)
		: m_allocator(&_allocator)
	{
		BX_ASSERT(_allocator.isCpuAccessible()
			, "Allocator passed to IntrusiveBlkAllocatorT must be CPU accessible!"
			);
	}

	template<typename AllocatorBlkT>
	inline IntrusiveBlkAllocatorT<AllocatorBlkT>::~IntrusiveBlkAllocatorT()
	{
	}

	template<typename AllocatorBlkT>
	inline void* IntrusiveBlkAllocatorT<AllocatorBlkT>::realloc(
		  void* _ptr
		, size_t _size
		, size_t _alignment
		, const char* _filePath
		, uint32_t _line
		)
	{
		if (0 == _size)
		{
			if (NULL != _ptr)
			{
				(void)free(m_allocator, move(toBlk(_ptr) ), Location(_filePath, _line) );
			}

			return NULL;
		}

		const uint8_t log2Prefix = calcLog2Prefix(_alignment);
		const size_t  prefix     = size_t(1)<<log2Prefix;

		Error err;

		if (NULL == _ptr)
		{
			Blk blk = tryAlloc(m_allocator, _size + prefix, _alignment, &err, Location(_filePath, _line), 2);

			if (err.isOk() )
			{
				return fromBlk(move(blk), log2Prefix);
			}

			blk.invalidate();

			return NULL;
		}

		Blk blk = toBlk(_ptr);
		const size_t oldPrefix = size_t(uintptr_t(_ptr) - uintptr_t(blk.ptr) );

		if (oldPrefix != prefix)
		{
			void* ptr = realloc(NULL, _size, _alignment, _filePath, _line);

			if (NULL != ptr)
			{
				memCopy(ptr, _ptr, min(_size, size_t(blk.size) - oldPrefix) );
				(void)free(m_allocator, move(blk), Location(_filePath, _line) );
			}
			else
			{
				blk.invalidate();
			}

			return ptr;
		}

		blk = tryRealloc(m_allocator, move(blk), _size + prefix, _alignment, &err, Location(_filePath, _line), 2);

		if (err.isOk() )
		{
			return fromBlk(move(blk), log2Prefix);
		}

		blk.invalidate();

		return NULL;
	}

	template<typename AllocatorBlkT>
	inline size_t IntrusiveBlkAllocatorT<AllocatorBlkT>::usableSize(const void* _ptr) const
	{
		Blk blk = toBlk(_ptr);
		const size_t size = size_t(blk.size) - size_t(uintptr_t(_ptr) - uintptr_t(blk.ptr) );
		blk.invalidate();

		return size;
	}

	template<typename AllocatorBlkT>
	inline uint8_t IntrusiveBlkAllocatorT<AllocatorBlkT>::calcLog2Prefix(size_t _alignment)
	{
		return uint8_t(countTrailingZeros(max(_alignment, size_t(kHeaderSize) ) ) );
	}

	template<typename AllocatorBlkT>
	inline typename IntrusiveBlkAllocatorT<AllocatorBlkT>::IntrusiveBlk*
	IntrusiveBlkAllocatorT<AllocatorBlkT>::getHeader(void* _ptr)
	{
		return (IntrusiveBlk*)( (uint8_t*)_ptr - kHeaderSize);
	}

	template<typename AllocatorBlkT>
	inline Blk IntrusiveBlkAllocatorT<AllocatorBlkT>::toBlk(const void* _ptr)
	{
		const IntrusiveBlk* header = (const IntrusiveBlk*)( (const uint8_t*)_ptr - kHeaderSize);
		const uint64_t      bits   = header->bits;

		const size_t  size          = size_t(bits & IntrusiveBlk::kMaskSize);
		const uint8_t log2Alignment = uint8_t( (bits>>IntrusiveBlk::kShiftLog2Alignment) & IntrusiveBlk::kMaskLog2);
		const uint8_t log2Prefix    = uint8_t( (bits>>IntrusiveBlk::kShiftLog2Prefix)    & IntrusiveBlk::kMaskLog2);

		Blk blk;

		blk.ptr         = uint64_t( (const uint8_t*)_ptr - (size_t(1)<<log2Prefix) );
		blk.bits        = Blk::packBits(size, log2Alignment, true, uint8_t(AllocatorOp::Noop) );
		blk.bookKeeping = header->bookKeeping;
		blk.slotIndex   = header->slotIndex;
#if BX_CONFIG_MEMORY_TRACKING
		blk.locationIdx = (uint32_t(bits>>IntrusiveBlk::kShiftLocationIdxHi)<<IntrusiveBlk::kFieldBitsLocationIdxLo) | header->locationIdxLo;
#endif // BX_CONFIG_MEMORY_TRACKING

		return blk;
	}

	template<typename AllocatorBlkT>
	inline void* IntrusiveBlkAllocatorT<AllocatorBlkT>::fromBlk(Blk&& _blk, uint8_t _log2Prefix)
	{
		void* ptr = (uint8_t*)_blk + (size_t(1)<<_log2Prefix);
		IntrusiveBlk* header = getHeader(ptr);

		uint64_t bits = 0
			| (uint64_t(_blk.size) )
			| (uint64_t(_blk.log2Alignment)<<IntrusiveBlk::kShiftLog2Alignment)
			| (uint64_t(_log2Prefix)<<IntrusiveBlk::kShiftLog2Prefix)
			;

#if BX_CONFIG_MEMORY_TRACKING
		BX_ASSERT(uint64_t(_blk.locationIdx) < (uint64_t(1)<<(IntrusiveBlk::kFieldBitsLocationIdxLo+IntrusiveBlk::kFieldBitsLocationIdxHi) )
			, "Location index %u doesn't fit intrusive block header!"
			, _blk.locationIdx
			);
		bits |= uint64_t(_blk.locationIdx>>IntrusiveBlk::kFieldBitsLocationIdxLo)<<IntrusiveBlk::kShiftLocationIdxHi;
		header->locationIdxLo = uint16_t(_blk.locationIdx);
#else
		header->unused0 = 0;
#endif // BX_CONFIG_MEMORY_TRACKING

		header->bits        = bits;
		header->slotIndex   = _blk.slotIndex;
		header->bookKeeping = _blk.bookKeeping;

		_blk.invalidate();

		return ptr;
	}

	template<typename AllocatorBlkT>
	inline StatsAllocatorT<AllocatorBlkT>::StatsAllocatorT(AllocatorBlkT& _allocator, uint16_t _tag)
		: m_allocator(&_allocator)
		, m_perFrameOpsLast(0)
		, m_tag(_tag)
	{
		memSet(m_shards, 0, sizeof(m_shards) );
	}

	template<typename AllocatorBlkT>
	inline StatsAllocatorT<AllocatorBlkT>::~StatsAllocatorT()
	{
		BX_ASSERT(0 == getUsed()
			, "Leak! Destroying allocator that still has %zu blocks (%zu bytes)!"
			, getNumBlocks()
			, getUsed()
			);
	}

	template<typename AllocatorBlkT>
	inline typename StatsAllocatorT<AllocatorBlkT>::Shard& StatsAllocatorT<AllocatorBlkT>::getShard()
	{
		const uintptr_t stack = uintptr_t(&stack);
		return m_shards[uint32_t( (uint64_t(stack>>16) * UINT64_C(0x9e3779b97f4a7c15) ) >> 60)];
	}

	template<typename AllocatorBlkT>
	inline Blk StatsAllocatorT<AllocatorBlkT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		Shard& shard = getShard();
		atomicFetchAndAdd<uint64_t>(&shard.numOps, 1);

		BX_ASSERT(0 == _size || 0 == (_size & (Blk::kMinBlockSize - 1u) )
			, "StatsAllocatorT received non-%u-byte-aligned _size: %zu. Caller bypasses bx::tryAlloc/bx::tryRealloc rounding."
			, Blk::kMinBlockSize
			, _size
			);

		const bool   wasValid = isValid(_blk);
		const size_t oldSize  = _blk.size;

		Blk blk = m_allocator->realloc(move(_blk), _size, _alignment);

		const bool   isNowValid = isValid(blk);
		const size_t newSize    = blk.size;

		BX_ASSERT(0 == newSize || 0 == (newSize & (Blk::kMinBlockSize - 1u) )
			, "StatsAllocatorT upstream returned non-%u-byte-aligned blk.size: %zu (requested _size: %zu, old _blk.size: %zu)."
			, Blk::kMinBlockSize
			, newSize
			, _size
			, oldSize
			);
		BX_ASSERT(0 == oldSize || 0 == (oldSize & (Blk::kMinBlockSize - 1u) )
			, "StatsAllocatorT received _blk with non-%u-byte-aligned size: %zu (requested _size: %zu)."
			, Blk::kMinBlockSize
			, oldSize
			, _size
			);

		const bool failedResize = wasValid && 0 != _size && !isNowValid;

		if (failedResize)
		{
			return move(blk);
		}

		if (newSize != oldSize)
		{
			atomicFetchAndAdd<int64_t>(&shard.numBytes, int64_t(newSize) - int64_t(oldSize) );
		}

		const int64_t numBlocks = int64_t(isNowValid ? 1 : 0) - int64_t(wasValid ? 1 : 0);

		if (0 != numBlocks)
		{
			atomicFetchAndAdd<int64_t>(&shard.numBlocks, numBlocks);
		}

		return move(blk);
	}

	template<typename AllocatorBlkT>
	inline bool StatsAllocatorT<AllocatorBlkT>::owns(const Blk& _blk)
	{
		return m_allocator->owns(_blk);
	}

	template<typename AllocatorBlkT>
	inline bool StatsAllocatorT<AllocatorBlkT>::isCpuAccessible() const
	{
		return m_allocator->isCpuAccessible();
	}

	template<typename AllocatorBlkT>
	inline size_t StatsAllocatorT<AllocatorBlkT>::getUsed() const
	{
		int64_t numBytes = 0;

		for (uint32_t ii = 0; ii < kNumShards; ++ii)
		{
			numBytes += atomicLoad(&m_shards[ii].numBytes);
		}

		return size_t(numBytes < 0 ? 0 : numBytes);
	}

	template<typename AllocatorBlkT>
	inline size_t StatsAllocatorT<AllocatorBlkT>::getNumBlocks() const
	{
		int64_t numBlocks = 0;

		for (uint32_t ii = 0; ii < kNumShards; ++ii)
		{
			numBlocks += atomicLoad(&m_shards[ii].numBlocks);
		}

		return size_t(numBlocks < 0 ? 0 : numBlocks);
	}

	template<typename AllocatorBlkT>
	inline size_t StatsAllocatorT<AllocatorBlkT>::getNumOps() const
	{
		return size_t(atomicLoad(&m_perFrameOpsLast) );
	}

	template<typename AllocatorBlkT>
	inline void StatsAllocatorT<AllocatorBlkT>::endFrame()
	{
		uint64_t numOps = 0;

		for (uint32_t ii = 0; ii < kNumShards; ++ii)
		{
			numOps += atomicExchange<uint64_t>(&m_shards[ii].numOps, 0);
		}

		atomicStore<uint64_t>(&m_perFrameOpsLast, numOps);
	}

	template<typename AllocatorBlkT>
	inline DebugClearAllocatorT<AllocatorBlkT>::DebugClearAllocatorT(AllocatorBlkT& _allocator)
		: m_allocator(&_allocator)
	{
		BX_ASSERT(_allocator.isCpuAccessible(), "Allocator must be CPU accessible!");
	}

	template<typename AllocatorBlkT>
	inline DebugClearAllocatorT<AllocatorBlkT>::~DebugClearAllocatorT()
	{
	}

	template<typename AllocatorBlkT>
	inline Blk DebugClearAllocatorT<AllocatorBlkT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Free:
			set(_blk, 0, _blk.size, kMemoryDead);
			return m_allocator->realloc(move(_blk), 0, 0);

		case AllocatorOp::Grow:
		case AllocatorOp::Truncate:
		case AllocatorOp::Realign:
			{
				Blk blk = m_allocator->realloc(Blk(), _size, _alignment);

				if (!isValid(blk) )
				{
					return Blk(Blk::InvalidReason::ResizeFailed);
				}

				set(blk, min(_blk.size, blk.size), blk.size, kMemoryInit);
				memCopy( (void*)blk.ptr, (void*)_blk.ptr, min(_blk.size, blk.size) );

				set(_blk, 0, _blk.size, kMemoryDead);
				(void)m_allocator->realloc(move(_blk), 0, 0);

				return blk;
			}
			break;

		case AllocatorOp::Allocate:
			{
				Blk blk = m_allocator->realloc(Blk(), _size, _alignment);

				if (isValid(blk) )
				{
					set(blk, 0, blk.size, kMemoryInit);
				}

				return blk;
			}

		case AllocatorOp::Noop:
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);

		default:
			BX_ASSERT(false, "You should not be here (AllocatorOp: %d)!", allocatorOp);
			break;
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	template<typename AllocatorBlkT>
	inline bool DebugClearAllocatorT<AllocatorBlkT>::owns(const Blk& _blk)
	{
		return m_allocator->owns(_blk);
	}

	template<typename AllocatorBlkT>
	inline bool DebugClearAllocatorT<AllocatorBlkT>::isCpuAccessible() const
	{
		return true;
	}

	template<typename PrimaryAllocatorT, typename SecondaryAllocatorT>
	inline MoreCoreAllocatorT<PrimaryAllocatorT, SecondaryAllocatorT>::MoreCoreAllocatorT(PrimaryAllocatorT& _primary, SecondaryAllocatorT& _secondary, size_t _slabMinSize)
		: m_primary(_primary)
		, m_secondary(_secondary)
		, m_slabMinSize(_slabMinSize)
		, m_cpuAccessible(true
			&&   _primary.isCpuAccessible()
			&& _secondary.isCpuAccessible()
			)
	{
	}

	template<typename PrimaryAllocatorT, typename SecondaryAllocatorT>
	inline MoreCoreAllocatorT<PrimaryAllocatorT, SecondaryAllocatorT>::~MoreCoreAllocatorT()
	{
		for (Blk slab = m_primary.removeUnusedSlab(); isValid(slab); slab = m_primary.removeUnusedSlab() )
		{
			slab = internal::freeNoTrack(&m_secondary, move(slab) );
		}
	}

	template<typename PrimaryAllocatorT, typename SecondaryAllocatorT>
	inline Blk MoreCoreAllocatorT<PrimaryAllocatorT, SecondaryAllocatorT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		if (AllocatorOp::Free == allocatorOp)
		{
			return m_primary.realloc(move(_blk), _size, _alignment);
		}

		for (bool addedSlab = false;; addedSlab = true)
		{
			Error err;
			Blk blk = internal::tryReallocNoTrack(&m_primary, move(_blk), _size, _alignment, &err);

			if (err.isOk() )
			{
				return blk;
			}

			_blk = move(blk);

			if (addedSlab)
			{
				break;
			}

			if (!addSlab(_size, _alignment) )
			{
				break;
			}
		}

		return Blk(Blk::InvalidReason::AllocateFailed);
	}

	template<typename PrimaryAllocatorT, typename SecondaryAllocatorT>
	inline Blk MoreCoreAllocatorT<PrimaryAllocatorT, SecondaryAllocatorT>::alloc(size_t _size, size_t _alignment)
	{
		Blk blk = internal::allocRaw(&m_primary, _size, _alignment);

		if (isValid(blk) ) [[likely]]
		{
			return blk;
		}

		if (!addSlab(_size, _alignment) )
		{
			return Blk(Blk::InvalidReason::AllocateFailed);
		}

		return internal::allocRaw(&m_primary, _size, _alignment);
	}

	template<typename PrimaryAllocatorT, typename SecondaryAllocatorT>
	inline Blk MoreCoreAllocatorT<PrimaryAllocatorT, SecondaryAllocatorT>::free(Blk&& _blk)
	{
		return internal::freeRaw(&m_primary, move(_blk) );
	}

	template<typename PrimaryAllocatorT, typename SecondaryAllocatorT>
	inline bool MoreCoreAllocatorT<PrimaryAllocatorT, SecondaryAllocatorT>::owns(const Blk& _blk)
	{
		return m_primary.owns(_blk);
	}

	template<typename PrimaryAllocatorT, typename SecondaryAllocatorT>
	inline bool MoreCoreAllocatorT<PrimaryAllocatorT, SecondaryAllocatorT>::isCpuAccessible() const
	{
		return m_cpuAccessible;
	}

	template<typename PrimaryAllocatorT, typename SecondaryAllocatorT>
	BX_NO_INLINE bool MoreCoreAllocatorT<PrimaryAllocatorT, SecondaryAllocatorT>::addSlab(size_t _size, size_t _alignment)
	{
		const size_t slabSize = max(m_slabMinSize, nextPow2(_size + _alignment) );

		Blk slab = internal::tryAllocNoTrack(&m_secondary, slabSize, PrimaryAllocatorT::kTraits.minBlockAlignment, ErrorIgnore{});

		if (!isValid(slab) )
		{
			return false;
		}

		m_primary.addSlab(move(slab) );

		return true;
	}

	template<typename AllocatorBlkT>
	inline MutexAllocatorT<AllocatorBlkT>::MutexAllocatorT(AllocatorBlkT& _allocator)
		: m_allocator(_allocator)
	{
	}

	template<typename AllocatorBlkT>
	inline Blk MutexAllocatorT<AllocatorBlkT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		MutexScope scope(m_lock);
		return m_allocator.realloc(move(_blk), _size, _alignment);
	}

	template<typename AllocatorBlkT>
	inline Blk MutexAllocatorT<AllocatorBlkT>::alloc(size_t _size, size_t _alignment)
	{
		MutexScope scope(m_lock);
		return internal::allocRaw(&m_allocator, _size, _alignment);
	}

	template<typename AllocatorBlkT>
	inline Blk MutexAllocatorT<AllocatorBlkT>::free(Blk&& _blk)
	{
		MutexScope scope(m_lock);
		return internal::freeRaw(&m_allocator, move(_blk) );
	}

	template<typename AllocatorBlkT>
	inline bool MutexAllocatorT<AllocatorBlkT>::owns(const Blk& _blk)
	{
		MutexScope scope(m_lock);
		return m_allocator.owns(_blk);
	}

	template<typename AllocatorBlkT>
	inline bool MutexAllocatorT<AllocatorBlkT>::isCpuAccessible() const
	{
		Mutex& lock = *const_cast<Mutex*>(&m_lock);
		MutexScope scope(lock);
		return m_allocator.isCpuAccessible();
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	inline ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::ThreadLocalAllocatorT(const ConfigT& _config, AllocatorBlkI* _metaDataAllocator)
		: m_config(_config)
		, m_metaDataAllocator(_metaDataAllocator)
		, m_slotCount(0u)
	{
		BX_ASSERT(NULL != _metaDataAllocator
			, "ThreadLocalAllocatorT requires a non-null meta-data allocator."
			);

		for (uint32_t ii = 0; ii < MaxThreadsT; ++ii)
		{
			m_slots[ii]        = NULL;
			m_slotThreadId[ii] = 0;
		}
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	inline ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::~ThreadLocalAllocatorT()
	{
		MutexScope scope(m_lock);
		for (uint32_t ii = 0; ii < m_slotCount; ++ii)
		{
			UpstreamT* up = m_slots[ii];
			if (NULL == up)
			{
				continue;
			}

			up->~UpstreamT();
			m_slotBlk[ii] = internal::freeNoTrack(m_metaDataAllocator, move(m_slotBlk[ii]) );
			m_slots[ii] = NULL;
		}
		m_slotCount = 0u;
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	inline uint16_t ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::createSlotLocked()
	{
		BX_ASSERT(m_slotCount < MaxThreadsT
			, "ThreadLocalAllocatorT: exceeded MaxThreadsT (%u)."
			, MaxThreadsT
			);

		if (m_slotCount >= MaxThreadsT)
		{
			return UINT16_MAX;
		}

		constexpr size_t kBlkSize = alignUp(sizeof(UpstreamT), size_t(Blk::kMinBlockSize) );
		Blk slotBlk = m_metaDataAllocator->realloc(Blk(), kBlkSize, alignof(UpstreamT) );

		if (!isValid(slotBlk) )
		{
			return UINT16_MAX;
		}

		UpstreamT* up = BX_PLACEMENT_NEW( (void*)slotBlk.ptr, UpstreamT)(m_config);

		const uint16_t idx = uint16_t(m_slotCount);
		m_slots[idx]        = up;
		m_slotThreadId[idx] = getTid();
		m_slotBlk[idx]      = move(slotBlk);
		atomicStore<uint32_t>(&m_slotCount, idx + 1);

		return idx;
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	BX_FORCE_INLINE uintptr_t ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::getOrCreateSlot()
	{
		const uintptr_t slot = uintptr_t(m_tls.get() );

		if (0 != slot) [[likely]]
		{
			return slot;
		}

		return findOrCreateSlot();
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	BX_NO_INLINE uintptr_t ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::findOrCreateSlot()
	{
		MutexScope scope(m_lock);

		const uint32_t tid = getTid();

		uint16_t idx = UINT16_MAX;

		for (uint16_t ii = 0; ii < m_slotCount; ++ii)
		{
			if (tid == m_slotThreadId[ii]
			&&  NULL != m_slots[ii])
			{
				idx = ii;
				break;
			}
		}

		if (UINT16_MAX == idx)
		{
			idx = createSlotLocked();

			if (UINT16_MAX == idx)
			{
				return 0;
			}
		}

		const uintptr_t slot = uintptr_t(idx) + 1;
		m_tls.set( (void*)slot);

		return slot;
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	inline Blk ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate:
			{
				const uintptr_t slot = getOrCreateSlot();

				if (0 == slot) [[unlikely]]
				{
					return Blk(Blk::InvalidReason::AllocateFailed);
				}

				const uint16_t idx = uint16_t(slot - 1);
				Blk out = m_slots[idx]->realloc(move(_blk), _size, _alignment);
				if (isValid(out) ) [[likely]]
				{
					out.slotIndex = idx;
				}
				return out;
			}

		case AllocatorOp::Free:
		case AllocatorOp::Grow:
		case AllocatorOp::Truncate:
			{
				const uint16_t idx = _blk.slotIndex;
				BX_ASSERT(idx < atomicLoad(&m_slotCount) && NULL != m_slots[idx]
					, "ThreadLocalAllocatorT: invalid slot index %u (count=%u)."
					, idx
					, atomicLoad(&m_slotCount)
					);
				UpstreamT* up = m_slots[idx];
				Blk out = up->realloc(move(_blk), _size, _alignment);
				if (isValid(out) )
				{
					out.slotIndex = idx;
				}
				return out;
			}

		default:
			return move(_blk);
		}
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	inline Blk ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::alloc(size_t _size, size_t _alignment)
	{
		const uintptr_t slot = getOrCreateSlot();

		if (0 == slot) [[unlikely]]
		{
			return Blk(Blk::InvalidReason::AllocateFailed);
		}

		const uint16_t idx = uint16_t(slot - 1);
		Blk out = internal::allocRaw(m_slots[idx], _size, _alignment);

		if (isValid(out) ) [[likely]]
		{
			out.slotIndex = idx;
		}

		return out;
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	inline Blk ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::free(Blk&& _blk)
	{
		const uint16_t idx = _blk.slotIndex;
		BX_ASSERT(idx < atomicLoad(&m_slotCount) && NULL != m_slots[idx]
			, "ThreadLocalAllocatorT: invalid slot index %u (count=%u)."
			, idx
			, atomicLoad(&m_slotCount)
			);

		return internal::freeRaw(m_slots[idx], move(_blk) );
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	inline bool ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::owns(const Blk& _blk)
	{
		if (!isValid(_blk) )
		{
			return false;
		}

		const uint16_t idx = _blk.slotIndex;
		if (idx >= atomicLoad(&m_slotCount)
		||  NULL == m_slots[idx])
		{
			return false;
		}

		return m_slots[idx]->owns(_blk);
	}

	template<typename UpstreamT, uint32_t MaxThreadsT>
	inline bool ThreadLocalAllocatorT<UpstreamT, MaxThreadsT>::isCpuAccessible() const
	{
		return UpstreamT::kTraits.blocksAreCpuAccessible;
	}

	namespace internal
	{
		template<typename AllocatorBlkT
			, uint32_t MaxCachedSizeT
			, uint32_t MaxBlocksPerClassT
			>
		class BonwickMagazineT
		{
			BX_CLASS_NO_COPY(BonwickMagazineT);

		public:
			static_assert(0 == (MaxCachedSizeT & (MaxCachedSizeT - 1u) )
				, "MaxCachedSizeT must be a power of two."
				);
			static_assert(MaxCachedSizeT >= Blk::kMinBlockSize
				, "MaxCachedSizeT must be at least Blk::kMinBlockSize."
				);
			static_assert(MaxBlocksPerClassT >= 2u
				, "MaxBlocksPerClassT must be at least 2 (partial-flush keeps half)."
				);

			static constexpr uint32_t kMinLog2 = Blk::kMinBlockLog2Alignment;
			static constexpr uint32_t kMaxLog2 = ceilLog2(MaxCachedSizeT);
			static constexpr uint32_t kMinSize = uint32_t(1) << kMinLog2;
			static constexpr uint32_t kMaxCachedSize     = MaxCachedSizeT;
			static constexpr uint32_t kMaxBlocksPerClass = MaxBlocksPerClassT;
			static constexpr uint32_t kNumClasses        = (kMaxLog2 >= kMinLog2)
				? (kMaxLog2 - kMinLog2 + 1u)
				: 0u
				;

			explicit BonwickMagazineT(AllocatorBlkT& _allocator);

			~BonwickMagazineT();

			void* pop(uint32_t _classIndex);

			template<typename FlushFnT>
			void push(uint32_t _classIndex, void* _block, FlushFnT&& _flushFn);

			bool tryPush(uint32_t _classIndex, void* _block);

			template<typename FlushFnT>
			void flush(FlushFnT&& _flushFn);

			template<typename FlushFnT>
			void flushAll(FlushFnT&& _flushFn);

			static uint32_t roundClass(size_t _size);

			static uint32_t classIndex(uint32_t _classSize);

			static uint32_t indexToSize(uint32_t _classIndex);

		private:
			struct FreeNode
			{
				FreeNode* next;
			};

			static_assert(sizeof(FreeNode) <= kMinSize
				, "FreeNode overlay must fit into smallest cached class."
				);

			struct Magazine
			{
				FreeNode* heads[kNumClasses];
				uint32_t  counts[kNumClasses];
				Magazine* next;
				Blk       selfBlk;
			};

			Magazine& getMagazine();
			Magazine* createMagazine();
			Magazine* acquireMagazineLocked();

			template<typename FlushFnT>
			void flushClass(Magazine& _mag, uint32_t _idx, uint32_t _keep, FlushFnT&& _flushFn);

			AllocatorBlkT* m_allocator;
			Mutex          m_lock;
			TlsData        m_tls;
			Magazine*      m_registry;
			Magazine       m_fallback;
		};

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		inline BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::BonwickMagazineT(AllocatorBlkT& _allocator)
			: m_allocator(&_allocator)
			, m_registry(NULL)
		{
			BX_ASSERT(_allocator.isCpuAccessible()
				, "BonwickMagazineT requires upstream allocator to be CPU accessible."
				);

			for (uint32_t idx = 0; idx < kNumClasses; ++idx)
			{
				m_fallback.heads[idx]  = NULL;
				m_fallback.counts[idx] = MaxBlocksPerClassT;
			}

			m_fallback.next = NULL;
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		inline BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::~BonwickMagazineT()
		{
			MutexScope scope(m_lock);
			for (Magazine* mag = m_registry; NULL != mag; )
			{
				Magazine* next = mag->next;
				Blk blk = move(mag->selfBlk);
				mag->~Magazine();
				(void)m_allocator->realloc(move(blk), 0, 0);
				mag = next;
			}
			m_registry = NULL;
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		BX_FORCE_INLINE void* BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::pop(uint32_t _classIndex)
		{
			Magazine& mag = getMagazine();
			FreeNode* node = mag.heads[_classIndex];
			if (NULL != node) [[likely]]
			{
				mag.heads[_classIndex] = node->next;
				--mag.counts[_classIndex];
				return node;
			}
			return NULL;
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		template<typename FlushFnT>
		BX_FORCE_INLINE void BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::push(uint32_t _classIndex, void* _block, FlushFnT&& _flushFn)
		{
			Magazine& mag = getMagazine();

			if (&m_fallback == &mag) [[unlikely]]
			{
				_flushFn(_block, indexToSize(_classIndex) );
				return;
			}

			if (mag.counts[_classIndex] >= MaxBlocksPerClassT) [[unlikely]]
			{
				flushClass(mag, _classIndex, MaxBlocksPerClassT / 2u, _flushFn);
			}

			FreeNode* node = static_cast<FreeNode*>(_block);
			node->next = mag.heads[_classIndex];
			mag.heads[_classIndex]   = node;
			++mag.counts[_classIndex];
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		BX_FORCE_INLINE bool BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::tryPush(uint32_t _classIndex, void* _block)
		{
			Magazine& mag = getMagazine();

			if (mag.counts[_classIndex] >= MaxBlocksPerClassT)
			{
				return false;
			}

			FreeNode* node = static_cast<FreeNode*>(_block);
			node->next = mag.heads[_classIndex];
			mag.heads[_classIndex]   = node;
			++mag.counts[_classIndex];

			return true;
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		template<typename FlushFnT>
		inline void BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::flush(FlushFnT&& _flushFn)
		{
			Magazine* mag = (Magazine*)m_tls.get();

			if (NULL == mag)
			{
				return;
			}

			for (uint32_t idx = 0; idx < kNumClasses; ++idx)
			{
				flushClass(*mag, idx, 0u, _flushFn);
			}
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		template<typename FlushFnT>
		inline void BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::flushAll(FlushFnT&& _flushFn)
		{
			MutexScope scope(m_lock);
			for (Magazine* mag = m_registry; NULL != mag; mag = mag->next)
			{
				for (uint32_t idx = 0; idx < kNumClasses; ++idx)
				{
					flushClass(*mag, idx, 0u, _flushFn);
				}
			}
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		template<typename FlushFnT>
		inline void BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::flushClass(Magazine& _mag, uint32_t _idx, uint32_t _keep, FlushFnT&& _flushFn)
		{
			const uint32_t classSize = uint32_t(1) << (_idx + kMinLog2);

			FreeNode* head = _mag.heads[_idx];
			FreeNode* tail = head;
			uint32_t  kept = 0u;
			if (_keep > 0u && NULL != head)
			{
				kept = 1u;
				while (kept < _keep && NULL != tail->next)
				{
					tail = tail->next;
					++kept;
				}
			}

			FreeNode* node;
			if (kept > 0u)
			{
				node = tail->next;
				tail->next = NULL;
				_mag.counts[_idx] = kept;
			}
			else
			{
				node = head;
				_mag.heads[_idx]  = NULL;
				_mag.counts[_idx] = 0;
			}

			while (NULL != node)
			{
				FreeNode* next = node->next;
				_flushFn(static_cast<void*>(node), classSize);
				node = next;
			}
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		BX_FORCE_INLINE typename BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::Magazine&
		BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::getMagazine()
		{
			Magazine* mag = (Magazine*)m_tls.get();

			if (NULL == mag) [[unlikely]]
			{
				mag = createMagazine();
			}

			return *mag;
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		BX_NO_INLINE typename BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::Magazine*
		BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::createMagazine()
		{
			MutexScope scope(m_lock);
			Magazine* mag = acquireMagazineLocked();

			if (NULL == mag)
			{
				return &m_fallback;
			}

			m_tls.set(mag);

			return mag;
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		inline typename BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::Magazine*
		BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::acquireMagazineLocked()
		{
			constexpr size_t kMagBlkSize = alignUp(sizeof(Magazine), size_t(Blk::kMinBlockSize) );
			Blk blk = m_allocator->realloc(Blk(), kMagBlkSize, alignof(Magazine) );

			if (!isValid(blk) )
			{
				return NULL;
			}

			Magazine* mag = BX_PLACEMENT_NEW( (void*)blk.ptr, Magazine);
			for (uint32_t idx = 0; idx < kNumClasses; ++idx)
			{
				mag->heads[idx]  = NULL;
				mag->counts[idx] = 0;
			}
			mag->selfBlk = move(blk);
			mag->next    = m_registry;
			m_registry   = mag;

			return mag;
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		inline uint32_t BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::roundClass(size_t _size)
		{
			if (_size <= size_t(kMinSize) )
			{
				return kMinSize;
			}
			return uint32_t(1) << ceilLog2(uint32_t(_size) );
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		inline uint32_t BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::classIndex(uint32_t _classSize)
		{
			return uint32_t(floorLog2(_classSize) ) - kMinLog2;
		}

		template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
		inline uint32_t BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::indexToSize(uint32_t _classIndex)
		{
			return uint32_t(1) << (_classIndex + kMinLog2);
		}

	} // namespace internal

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::ThreadCacheAllocatorT(AllocatorBlkT& _allocator)
		: m_allocator(&_allocator)
		, m_magazine(_allocator)
	{
		BX_ASSERT(_allocator.isCpuAccessible()
			, "ThreadCacheAllocatorT requires upstream allocator to be CPU accessible."
			);
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline void ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::FlushRealloc::operator()(void* _base, uint32_t _classSize) const
	{
		const CachedNode* cn = static_cast<const CachedNode*>(_base);
		Blk blk(uint64_t(_base), size_t(_classSize), size_t(Blk::kMinBlockAlignment), AllocatorOp::Allocate);
		blk.slotIndex   = cn->slotIndex;
		blk.bookKeeping = cn->bookKeeping;
		(void)m_allocator->realloc(move(blk), 0, 0);
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline void ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::FlushFreeRaw::operator()(void* _base, uint32_t _classSize) const
	{
		const CachedNode* cn = static_cast<const CachedNode*>(_base);
		Blk blk(uint64_t(_base), size_t(_classSize), size_t(Blk::kMinBlockAlignment), AllocatorOp::Allocate);
		blk.slotIndex   = cn->slotIndex;
		blk.bookKeeping = cn->bookKeeping;
		(void)internal::freeRaw(m_allocator, move(blk) );
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::~ThreadCacheAllocatorT()
	{
		m_magazine.flushAll(FlushRealloc{m_allocator});
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline Blk ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate: return alloc(_size, _alignment);
		case AllocatorOp::Free:     return free(move(_blk) );
		default:                    break;
		}

		return reallocUpstream(move(_blk), _size, _alignment);
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline Blk ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::alloc(size_t _size, size_t _alignment)
	{
		if (!isCacheableAlloc(_size, _alignment) )
		{
			return allocUpstream(_size, _alignment);
		}

		const uint32_t idx       = sizeToClass(_size);
		const size_t   classSize = size_t(1) << (idx + kMinLog2);

		if (void* hit = m_magazine.pop(idx) )
		{
			const CachedNode* cn = static_cast<const CachedNode*>(hit);

			Blk blk;
			blk.ptr             = uint64_t(uintptr_t(hit) );
			blk.bits            = Blk::packBits(classSize, Blk::kMinBlockLog2Alignment, true, AllocatorOp::Allocate);
			blk.slotIndex = cn->slotIndex;
			blk.bookKeeping     = cn->bookKeeping;
			return blk;
		}

		Blk blk = allocUpstream(classSize, Blk::kMinBlockAlignment);

		if (isValid(blk) )
		{
			blk.size = classSize;
		}

		return blk;
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline Blk ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::free(Blk&& _blk)
	{
		if (!isCacheableFree(_blk) )
		{
			return freeUpstream(move(_blk) );
		}

		const uint32_t idx = sizeToClass(size_t(_blk.size) );

		CachedNode* cn = reinterpret_cast<CachedNode*>(uintptr_t(_blk.ptr) );
		cn->slotIndex = _blk.slotIndex;
		cn->bookKeeping     = uint32_t(_blk.bookKeeping);

		if (!m_magazine.tryPush(idx, cn) ) [[unlikely]]
		{
			freeOverflow(idx, cn);
		}

		_blk.invalidate();
		return Blk(Blk::InvalidReason::Freed);
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	BX_NO_INLINE Blk ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::reallocUpstream(Blk&& _blk, size_t _size, size_t _alignment)
	{
		return m_allocator->realloc(move(_blk), _size, _alignment);
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	BX_NO_INLINE Blk ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::allocUpstream(size_t _size, size_t _alignment)
	{
		return internal::allocRaw(m_allocator, _size, _alignment);
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	BX_NO_INLINE Blk ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::freeUpstream(Blk&& _blk)
	{
		return internal::freeRaw(m_allocator, move(_blk) );
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	BX_NO_INLINE void ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::freeOverflow(uint32_t _classIndex, void* _block)
	{
		m_magazine.push(_classIndex, _block, FlushFreeRaw{m_allocator});
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline bool ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::owns(const Blk& _blk)
	{
		return m_allocator->owns(_blk);
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline bool ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::isCpuAccessible() const
	{
		return m_allocator->isCpuAccessible();
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	inline void ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::flush()
	{
		m_magazine.flush(FlushRealloc{m_allocator});
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	BX_FORCE_INLINE bool ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::isCacheableAlloc(size_t _size, size_t _alignment)
	{
		if (0 == _size
		||  _size > MaxCachedSizeT)
		{
			return false;
		}

		if (_alignment > size_t(Blk::kMinBlockAlignment) )
		{
			return false;
		}

		return true;
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	BX_FORCE_INLINE bool ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::isCacheableFree(const Blk& _blk)
	{
		if (!isValid(_blk) )
		{
			return false;
		}

		const size_t size = size_t(_blk.size);

		if (size < kMinSize
		||  size > MaxCachedSizeT)
		{
			return false;
		}

		if (0 != (size & (size - 1) ) )
		{
			return false;
		}

		if (_blk.log2Alignment > Blk::kMinBlockLog2Alignment)
		{
			return false;
		}

		return true;
	}

	template<typename AllocatorBlkT, uint32_t MaxCachedSizeT, uint32_t MaxBlocksPerClassT>
	BX_FORCE_INLINE uint32_t ThreadCacheAllocatorT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>::sizeToClass(size_t _size)
	{
		const uint32_t sz    = _size < kMinSize ? kMinSize : uint32_t(_size);
		const uint32_t log2c = ceilLog2(sz);
		return log2c - kMinLog2;
	}

} // namespace bx
