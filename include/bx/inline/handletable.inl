/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HANDLETABLE_H_HEADER_GUARD
#	error "Must be included from bx/handletable.h!"
#endif // BX_HANDLETABLE_H_HEADER_GUARD

namespace bx
{
	inline AllocatorHandle HandleTableBase::getAllocatorHandle() const
	{
		return m_usedSlots.getAllocatorHandle();
	}

	inline uint32_t HandleTableBase::getCapacity() const
	{
		return m_numBlocks<<m_blockShift;
	}

	inline uint32_t HandleTableBase::getCount() const
	{
		return m_count;
	}

	inline bool HandleTableBase::isEmpty() const
	{
		return 0 == m_count;
	}

	inline bool HandleTableBase::isUsed(uint32_t _idx) const
	{
		return 0 != _idx
			&& _idx < m_usedSlots.getCount()
			&& m_usedSlots.get(_idx)
			;
	}

	inline uint32_t HandleTableBase::findUsed(uint32_t _idx) const
	{
		const uint32_t count = m_usedSlots.getCount();

		_idx = max(_idx, 1u);

		if (_idx >= count)
		{
			return kInvalid;
		}

		if (m_usedSlots.get(_idx) )
		{
			return _idx;
		}

		return m_usedSlots.findSet(_idx, count);
	}

	inline void* HandleTableBase::getPtr(uint32_t _idx)
	{
		BX_ASSERT(_idx > 0, "Index can't be 0!");
		return (void*)(m_base[_idx>>m_blockShift] + size_t(_idx)*m_elementSize);
	}

	inline const void* HandleTableBase::getPtr(uint32_t _idx) const
	{
		return const_cast<HandleTableBase*>(this)->getPtr(_idx);
	}

	inline void* HandleTableBase::getBlockPtr(uint32_t _blockIdx)
	{
		BX_ASSERT(_blockIdx < m_numBlocks && m_allocatedBlocks.get(_blockIdx), "Accessing released block %u.", _blockIdx);
		return (void*)(m_base[_blockIdx] + (size_t(_blockIdx)<<m_blockShift)*m_elementSize);
	}

	inline const void* HandleTableBase::getBlockPtr(uint32_t _blockIdx) const
	{
		return const_cast<HandleTableBase*>(this)->getBlockPtr(_blockIdx);
	}

	inline uint32_t HandleTableBase::acquire()
	{
		uint32_t blockIdx = m_curBlock;
		uint32_t idx      = m_freeHead[blockIdx];

		if (kInvalid == idx) [[unlikely]]
		{
			selectBlock();

			blockIdx = m_curBlock;
			idx      = m_freeHead[blockIdx];
		}

		m_freeHead[blockIdx] = *(const uint32_t*)(m_base[blockIdx] + size_t(idx)*m_elementSize);
		++m_blockCount[blockIdx];

		m_usedSlots.set(idx, true);
		++m_count;

		return idx;
	}

	inline void HandleTableBase::release(uint32_t _idx)
	{
		BX_ASSERT(0 < _idx && isUsed(_idx), "Releasing non-existing element %u.", _idx);

		const uint32_t blockIdx = _idx>>m_blockShift;

		*(uint32_t*)(m_base[blockIdx] + size_t(_idx)*m_elementSize) = m_freeHead[blockIdx];
		m_freeHead[blockIdx] = _idx;
		m_nonFullBlocks.set(blockIdx, true);

		m_usedSlots.set(_idx, false);
		--m_count;

		if (0 == --m_blockCount[blockIdx]) [[unlikely]]
		{
			releaseBlock(blockIdx);
		}
	}

	inline uintptr_t HandleTableBase::getBase(uint32_t _blockIdx) const
	{
		return m_base[_blockIdx];
	}

	template<typename ContainerT, typename Ty>
	inline HandleTableIteratorT<ContainerT, Ty>::HandleTableIteratorT(ContainerT& _container, uint32_t _idx)
		: m_container(_container)
		, m_idx(_idx)
	{
	}

	template<typename ContainerT, typename Ty>
	inline const HandleTableIteratorT<ContainerT, Ty>& HandleTableIteratorT<ContainerT, Ty>::operator*() const
	{
		return *this;
	}

	template<typename ContainerT, typename Ty>
	inline bool HandleTableIteratorT<ContainerT, Ty>::operator!=(const HandleTableIteratorT<ContainerT, Ty>& _rhs) const
	{
		return m_idx != _rhs.m_idx;
	}

	template<typename ContainerT, typename Ty>
	inline const HandleTableIteratorT<ContainerT, Ty>& HandleTableIteratorT<ContainerT, Ty>::operator++()
	{
		next();
		return *this;
	}

	template<typename ContainerT, typename Ty>
	inline bool HandleTableIteratorT<ContainerT, Ty>::next()
	{
		if (kInvalid == m_idx)
		{
			return false;
		}

		m_idx = m_container.findUsed(m_idx + 1);

		return kInvalid != m_idx;
	}

	template<typename ContainerT, typename Ty>
	inline uint32_t HandleTableIteratorT<ContainerT, Ty>::getIndex() const
	{
		return m_idx;
	}

	template<typename ContainerT, typename Ty>
	inline Ty& HandleTableIteratorT<ContainerT, Ty>::getValue() const
	{
		return *m_container.getPtr(m_idx);
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline HandleTableT<Ty, BlockCapacityT>::HandleTableT()
		: HandleTableBase(sizeof(Ty), alignof(Ty), BlockCapacityT)
	{
		debugPtrUpdate();
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline HandleTableT<Ty, BlockCapacityT>::HandleTableT(HandleTableT<Ty, BlockCapacityT>&& _src)
		: HandleTableBase(bx::move(_src) )
	{
		debugPtrUpdate();
		_src.debugPtrUpdate();
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline HandleTableT<Ty, BlockCapacityT>::HandleTableT(AllocatorHandle _allocator)
		: HandleTableBase(_allocator, sizeof(Ty), alignof(Ty), BlockCapacityT, 0)
	{
		debugPtrUpdate();
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline HandleTableT<Ty, BlockCapacityT>::HandleTableT(AllocatorHandle _allocator, uint32_t _capacity)
		: HandleTableBase(_allocator, sizeof(Ty), alignof(Ty), BlockCapacityT, _capacity)
	{
		debugPtrUpdate();
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline HandleTableT<Ty, BlockCapacityT>& HandleTableT<Ty, BlockCapacityT>::operator=(HandleTableT<Ty, BlockCapacityT>&& _src)
	{
		set(bx::move(_src) );
		return *this;
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline void HandleTableT<Ty, BlockCapacityT>::set(HandleTableT<Ty, BlockCapacityT>&& _src)
	{
		if (this != &_src)
		{
			destroyElements();
			HandleTableBase::set(bx::move(_src) );
			debugPtrUpdate();
			_src.debugPtrUpdate();
		}
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline void HandleTableT<Ty, BlockCapacityT>::clear()
	{
		destroyElements();
		HandleTableBase::clear();
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline void HandleTableT<Ty, BlockCapacityT>::destroyElements()
	{
		if constexpr (!isTriviallyDestructible<Ty>() )
		{
			for (uint32_t idx = findUsed(1); kInvalid != idx; idx = findUsed(idx + 1) )
			{
				getPtr(idx)->~Ty();
			}
		}
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline uint32_t HandleTableT<Ty, BlockCapacityT>::acquire()
	{
		const uint32_t idx = HandleTableBase::acquire();
		debugPtrUpdate();
		return idx;
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline void HandleTableT<Ty, BlockCapacityT>::debugPtrUpdate()
	{
#if BX_CONFIG_DEBUG
		m_debugBase = reinterpret_cast<Ty**>(m_base);
#endif // BX_CONFIG_DEBUG
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline typename HandleTableT<Ty, BlockCapacityT>::Iterator HandleTableT<Ty, BlockCapacityT>::begin()
	{
		return Iterator(*this, findUsed(1) );
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline typename HandleTableT<Ty, BlockCapacityT>::Iterator HandleTableT<Ty, BlockCapacityT>::end()
	{
		return Iterator(*this, kInvalid);
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline typename HandleTableT<Ty, BlockCapacityT>::ConstIterator HandleTableT<Ty, BlockCapacityT>::begin() const
	{
		return ConstIterator(*this, findUsed(1) );
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline typename HandleTableT<Ty, BlockCapacityT>::ConstIterator HandleTableT<Ty, BlockCapacityT>::end() const
	{
		return ConstIterator(*this, kInvalid);
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline HandleTableT<Ty, BlockCapacityT>::~HandleTableT()
	{
		destroyElements();
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline Ty* HandleTableT<Ty, BlockCapacityT>::getPtr(uint32_t _idx)
	{
		BX_ASSERT(_idx > 0, "Index can't be 0!");
		return (Ty*)getBase(_idx>>kBlockShift) + _idx;
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline const Ty* HandleTableT<Ty, BlockCapacityT>::getPtr(uint32_t _idx) const
	{
		BX_ASSERT(_idx > 0, "Index can't be 0!");
		return (const Ty*)getBase(_idx>>kBlockShift) + _idx;
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline Ty& HandleTableT<Ty, BlockCapacityT>::operator[](uint32_t _idx)
	{
		return *getPtr(_idx);
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline const Ty& HandleTableT<Ty, BlockCapacityT>::operator[](uint32_t _idx) const
	{
		return *getPtr(_idx);
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline uint32_t HandleTableT<Ty, BlockCapacityT>::add(const Ty& _src)
	{
		const uint32_t idx = acquire();
		BX_PLACEMENT_NEW(getPtr(idx), Ty)(_src);
		return idx;
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline uint32_t HandleTableT<Ty, BlockCapacityT>::add(Ty&& _src)
	{
		const uint32_t idx = acquire();
		BX_PLACEMENT_NEW(getPtr(idx), Ty)(bx::move(_src) );
		return idx;
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline void HandleTableT<Ty, BlockCapacityT>::remove(uint32_t _idx)
	{
		getPtr(_idx)->~Ty();
		release(_idx);
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline void HandleTableT<Ty, BlockCapacityT>::remove(const Iterator& _it)
	{
		remove(_it.getIndex() );
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline Ty HandleTableT<Ty, BlockCapacityT>::pop(uint32_t _idx)
	{
		BX_ASSERT(isUsed(_idx), "Removing non-existing element %d.", _idx);

		Ty* ptr = getPtr(_idx);
		Ty value = bx::move(*ptr);
		ptr->~Ty();

		release(_idx);

		return value;
	}

	template<typename Ty, uint32_t BlockCapacityT>
	inline int32_t write(WriterI* _writer, const HandleTableT<Ty, BlockCapacityT>& _table, Error* _err)
	{
		static_assert(isTriviallyCopyable<Ty>(), "Element type must be trivially copyable.");

		BX_ERROR_SCOPE(_err);

		int32_t  size = 0;
		const Ty* run = NULL;
		uint32_t num  = 0;

		for (const auto it : _table)
		{
			const Ty* ptr = &it.getValue();

			if (run + num != ptr)
			{
				if (0 != num)
				{
					size += write(_writer, run, int32_t(num*sizeof(Ty) ), _err);

					if (!_err->isOk() )
					{
						return size;
					}
				}

				run = ptr;
				num = 0;
			}

			++num;
		}

		if (0 != num)
		{
			size += write(_writer, run, int32_t(num*sizeof(Ty) ), _err);
		}

		return size;
	}

} // namespace bx
