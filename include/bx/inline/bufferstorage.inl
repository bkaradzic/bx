/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_BUFFERSTORAGE_H_HEADER_GUARD
#	error "Must be included from bx/bufferstorage.h!"
#endif // BX_BUFFERSTORAGE_H_HEADER_GUARD

namespace bx
{
	inline void dbgGraph(uint32_t _total, uint32_t _begin, uint32_t _count, char _ch)
	{
		const char* empty = "................................";
		char rep[32];
		for (uint32_t ii = 0; ii < BX_COUNTOF(rep); ++ii) { rep[ii] = _ch; }

		BX_UNUSED("\t|%.*s%.*s%.*s|\n"
			, _begin,               empty
			, _count,               rep
			, _total-_begin-_count, empty
			);
	}

	inline void sanitize(uint32_t _total, uint32_t& _idx, uint32_t& _count)
	{
		_idx   = min(_total, _idx);
		_count = min(_total - _idx, _count);
	}

	inline bool overlap(const void* _a, size_t _sizeInBytesA, const void* _b, size_t _sizeInBytesB)
	{
		const uintptr_t aa = uintptr_t(_a);
		const uintptr_t bb = uintptr_t(_b);
		return aa + _sizeInBytesA > bb
			&& bb + _sizeInBytesB > aa
			;
	}

	template<typename Ty>
	inline constexpr bool overlap(const Ty* _a, const Ty* _b, uint32_t _count)
	{
		const size_t sizeInBytes = sizeof(Ty)*_count;
		return overlap(_a, sizeInBytes, _b, sizeInBytes);
	}

	template<typename Ty>
	inline uint32_t StorageTraits::copy(Ty* _dst, const Ty* _src, uint32_t _count)
	{
		if (BX_ENABLED(isTriviallyCopyAssignable<Ty>() ) )
		{
			memMove(_dst, _src, _count * sizeof(Ty) );
		}
		else
		{
			uint32_t from = 0;
			uint32_t mask = 0;

			if (_src + _count > _dst
			&&  _src < _dst)
			{
				from = _count;
				mask = UINT32_MAX;
			}

			for (uint32_t ii = 0; ii < _count; ++ii)
			{
				const uint32_t idx = ( (~from + 1) + ii) ^ mask;
				_dst[idx] = _src[idx];
			}
		}

		return _count;
	}

	template<typename Ty>
	inline void StorageTraits::move(Ty* _dst, Ty* _src, uint32_t _count)
	{
		if (BX_ENABLED(isTriviallyMoveAssignable<Ty>() ) )
		{
			memMove(_dst, _src, _count * sizeof(Ty) );
		}
		else
		{
			uint32_t from = 0;
			uint32_t mask = 0;

			if (_src + _count > _dst
			&&  _src < _dst)
			{
				from = _count;
				mask = UINT32_MAX;
			}

			for (uint32_t ii = 0; ii < _count; ++ii)
			{
				const uint32_t idx = ( (~from + 1) + ii) ^ mask;
				_dst[idx] = bx::move(_src[idx]);
			}
		}
	}

	template<typename Ty>
	inline EnableIfType<true
		&& !IsTriviallyConstructibleT<Ty>::value
		&& !IsConstructibleT<Ty, InitNoneTag>::value
		, void>
	StorageTraits::constructDefault(Ty* _ptr, uint32_t _count)
	{
		for (Ty *ptr = _ptr, *term = _ptr + _count; ptr < term; ++ptr)
		{
			BX_PLACEMENT_NEW(ptr, Ty);
		}
	}

	template<typename Ty>
	inline EnableIfType<true
		&& !IsTriviallyConstructibleT<Ty>::value
		&&  IsConstructibleT<Ty, InitNoneTag>::value
		, void>
	StorageTraits::constructDefault(Ty* _ptr, uint32_t _count)
	{
		for (Ty *ptr = _ptr, *term = _ptr + _count; ptr < term; ++ptr)
		{
			BX_PLACEMENT_NEW(ptr, Ty)(InitNone);
		}
	}

	template<typename Ty>
	inline EnableIfType<true
		&& IsTriviallyConstructibleT<Ty>::value
		, void>
	StorageTraits::constructDefault(Ty* _ptr, uint32_t _count)
	{
		BX_UNUSED(_ptr, _count);
	}

	template<typename Ty, typename... ArgsT>
	inline uint32_t StorageTraits::constructEmplace(Ty* _ptr, uint32_t _count, ArgsT&&... _args)
	{
		for (Ty *ptr = _ptr, *term = _ptr + _count; ptr < term; ++ptr)
		{
			BX_PLACEMENT_NEW(ptr, Ty)(forward<ArgsT>(_args)...);
		}

		return _count;
	}

	template<typename Ty>
	inline void StorageTraits::constructMove(Ty* _dst, Ty* _src, uint32_t _count)
	{
		if (BX_ENABLED(isTriviallyMoveConstructible<Ty>() ) )
		{
			memMove(_dst, _src, _count * sizeof(Ty) );
		}
		else if (BX_ENABLED(isMoveConstructible<Ty>() ) )
		{
			for (uint32_t ii = 0; ii < _count; ++ii)
			{
				BX_PLACEMENT_NEW(&_dst[ii], Ty)(bx::move(_src[ii]) );
			}
		}
		else
		{
			constructDefault(_dst, _count);
			move(_dst, _src, _count);
		}
	}

	template<typename Ty>
	inline void StorageTraits::destruct(Ty* _ptr, uint32_t _count)
	{
		if (!BX_ENABLED(isTriviallyDestructible<Ty>() ) )
		{
			for (Ty *ptr = _ptr, *term = _ptr + _count; ptr < term; ++ptr)
			{
				ptr->~Ty();
			}
		}
	}

	//---

	template<typename Ty>
	inline StorageViewT<Ty>::StorageViewT(const Ty* _ptr, uint32_t _count)
		: m_ptr(_ptr)
		, m_count(_count)
	{
	}

	template<typename Ty>
	inline StorageViewT<Ty>::StorageViewT(const StorageViewT<Ty>& _other)
		: m_ptr(_other.m_ptr)
		, m_count(_other.m_count)
	{
	}

	template<typename Ty>
	inline StorageViewT<Ty>::~StorageViewT()
	{
	}

	template<typename Ty>
	inline StorageViewT<Ty>& StorageViewT<Ty>::operator=(const StorageViewT<Ty>& _other)
	{
		m_ptr   = _other.m_ptr;
		m_count = _other.m_count;

		return *this;
	}

	template<typename Ty>
	inline StorageViewT<Ty>& StorageViewT<Ty>::operator=(StorageViewT<Ty>&& _other)
	{
		m_ptr   = _other.m_ptr;
		m_count = _other.m_count;

		_other.m_ptr   = NULL;
		_other.m_count = 0;

		return *this;
	}

	template<typename Ty>
	template<typename StorageT>
	inline void StorageViewT<Ty>::setCapacity(StorageT& _storage, uint32_t _capacity)
	{
		const uint32_t count = m_count;
		_storage.setCapacity(max(count, _capacity), count);
		m_ptr = _storage.getPtr();
	}

	template<typename Ty>
	inline uint32_t StorageViewT<Ty>::getCount() const
	{
		return m_count;
	}

	template<typename Ty>
	template<typename StorageT>
	inline void StorageViewT<Ty>::setCount(StorageT& _storage, uint32_t _count)
	{
		if (_count > _storage.getCapacity() )
		{
			setCapacity(_storage, _count);
		}

		{
			uint32_t idx = 0;
			sanitize(_storage.getCapacity(), idx, _count);
		}

		const uint32_t oldCount = m_count;

		if (oldCount > _count)
		{
			return remove(_storage, _count, kUnordered, UINT32_MAX);
		}

		if (oldCount < _count)
		{
			const uint32_t diff = _count - oldCount;

			dbgGraph(_count, oldCount, diff, 'C');
			StorageTraits::constructDefault(_storage.getPtr(oldCount), diff);
		}

		m_count = _count;
	}

	template<typename Ty>
	template<typename StorageT>
	inline void StorageViewT<Ty>::add(StorageT& _storage, uint32_t _pos, const Ty* _ptr, uint32_t _count)
	{
		if (0 == _count)
		{
			return;
		}

		const uint32_t dstPos = carve(_storage, _pos, _count, true);

		BX_ASSERT(!overlap(_storage.getPtr(dstPos), _ptr, _count), "");

		dbgGraph(m_count+_count, dstPos, _count, 'c');

		m_count += StorageTraits::copy(_storage.getPtr(dstPos), _ptr, _count);
	}

	template<typename Ty>
	template<typename StorageT, typename... ArgsT>
	inline void StorageViewT<Ty>::add(StorageT& _storage, uint32_t _pos, EmplaceTag, ArgsT&&... _args)
	{
		const uint32_t oldCount = m_count;
		const uint32_t dstPos   = carve(_storage, _pos, 1, false);
		Ty* dst = _storage.getPtr(dstPos);

		if (dstPos < oldCount)
		{
			dbgGraph(oldCount+1, dstPos, 1, 'D');

			StorageTraits::destruct(dst, 1);
		}

		dbgGraph(oldCount+1, dstPos, 1, 'E');

		m_count += StorageTraits::constructEmplace(dst, 1, forward<ArgsT>(_args)...);
	}

	template<typename Ty>
	template<typename StorageT>
	inline void StorageViewT<Ty>::remove(StorageT& _storage, uint32_t _pos, Ordering _ordering, uint32_t _count)
	{
		if (0 == _count)
		{
			return;
		}

		const uint32_t oldCount = m_count;
		sanitize(oldCount, _pos, _count);

		const uint32_t end      = _pos + _count;
		const uint32_t newCount = oldCount - _count;

		uint32_t move, from;

		if (_ordering == kOrdered)
		{
			move = oldCount - end;
			from = end;
		}
		else
		{
			move = min(oldCount - end, _count);
			from = oldCount - move;
		}

		if (0 == _count)
		{
			return;
		}

		if (0 != move)
		{
			dbgGraph(oldCount, from, move, 'F');
			dbgGraph(oldCount, _pos, move, 'M');

			StorageTraits::move(_storage.getPtr(_pos), _storage.getPtr(from), move);
		}

		dbgGraph(oldCount, newCount, _count, 'D');
		StorageTraits::destruct(_storage.getPtr(newCount), _count);

		m_count = newCount;
	}

	template<typename Ty>
	template<typename StorageT>
	inline uint32_t StorageViewT<Ty>::carve(StorageT& _storage, uint32_t _pos, uint32_t _count, bool _init)
	{
		const uint32_t dstCount = m_count;
		const uint32_t newCount = _count + dstCount;

		if (newCount > _storage.getCapacity() )
		{
			_storage.setCapacity(max(dstCount, newCount), dstCount);
			m_ptr = _storage.getPtr();
		}

		BX_ASSERT(newCount <= _storage.getCapacity()
			, "Out-of-bounds (count: %d, capacity: %d)!"
			, newCount, _storage.getCapacity()
			);

		const uint32_t dstPos    = min(_pos, dstCount);
		const uint32_t moveCount = dstCount - dstPos;
		const uint32_t newPos    = dstPos + _count;

		const bool needCtor = _init || dstPos != dstCount;

		dbgGraph(newCount, dstCount, _count, needCtor ? 'C' : '?');

		if (needCtor)
		{
			StorageTraits::constructDefault(_storage.getPtr(dstCount), _count);
		}

		if (0 != moveCount)
		{
			dbgGraph(newCount, dstPos, moveCount, 'F');
			dbgGraph(newCount, newPos, moveCount, 'M');

			StorageTraits::move(_storage.getPtr(newPos), _storage.getPtr(dstPos), moveCount);
		}

		return dstPos;
	}

	//---

	template<typename Ty, uint32_t CapacityT>
	inline FixedStorageT<Ty, CapacityT>::FixedStorageT()
	{
	}

	template<typename Ty, uint32_t CapacityT>
	inline FixedStorageT<Ty, CapacityT>::~FixedStorageT()
	{
	}

	template<typename Ty, uint32_t CapacityT>
	inline constexpr uint32_t FixedStorageT<Ty, CapacityT>::getCapacity() const
	{
		return CapacityT;
	}

	template<typename Ty, uint32_t CapacityT>
	inline void FixedStorageT<Ty, CapacityT>::setCapacity(uint32_t /*_capacity*/, uint32_t /*_count*/)
	{
	}

	template<typename Ty, uint32_t CapacityT>
	inline Ty* FixedStorageT<Ty, CapacityT>::getPtr(uint32_t _idx)
	{
		BX_ASSERT(_idx <= CapacityT, "Out-of-bounds (_idx: %d, max %d)!", _idx, CapacityT);
		return addressOf<Ty>(m_data) + _idx;
	}

	template<typename Ty, uint32_t CapacityT>
	inline const Ty* FixedStorageT<Ty, CapacityT>::getPtr(uint32_t _idx) const
	{
		BX_ASSERT(_idx <= CapacityT, "Out-of-bounds (_idx: %d, max %d)!", _idx, CapacityT);
		return addressOf<Ty>(m_data) + _idx;
	}

	template<typename Ty, uint32_t CapacityT>
	inline Ty* FixedStorageT<Ty, CapacityT>::getTerm()
	{
		return getPtr(CapacityT);
	}

	template<typename Ty, uint32_t CapacityT>
	inline const Ty* FixedStorageT<Ty, CapacityT>::getTerm() const
	{
		return getPtr(CapacityT);
	}

	template<typename Ty, uint32_t CapacityT>
	inline FixedStorageT<Ty, CapacityT>& FixedStorageT<Ty, CapacityT>::operator=(const FixedStorageT<Ty, CapacityT>& _other)
	{
		memCopy(m_data, _other.m_data, sizeof(m_data) );
		return *this;
	}

	template<typename Ty, uint32_t CapacityT>
	inline FixedStorageT<Ty, CapacityT>& FixedStorageT<Ty, CapacityT>::operator=(FixedStorageT<Ty, CapacityT>&& _other)
	{
		memCopy(m_data, _other.m_data, sizeof(m_data) );
		return *this;
	}

	template<typename Ty>
	inline DynamicStorageT<Ty>::DynamicStorageT()
		: m_ptr(NULL)
		, m_capacity(0)
		, m_allocator(NULL)
	{
	}

	template<typename Ty>
	inline void DynamicStorageT<Ty>::setAllocator(AllocatorI* _allocator)
	{
		BX_ASSERT(false
			|| NULL == m_ptr
			|| NULL == m_allocator
			, "Allocator is already set, or storage is not empty!"
			);

		m_allocator = _allocator;
	}

	template<typename Ty>
	inline AllocatorI* DynamicStorageT<Ty>::getAllocator() const
	{
		return m_allocator;
	}

	template<typename Ty>
	inline DynamicStorageT<Ty>::~DynamicStorageT()
	{
		if (NULL != m_ptr)
		{
			bx::free(m_allocator, m_ptr, BX_ALIGNOF(Ty) );
		}
	}

	template<typename Ty>
	inline uint32_t DynamicStorageT<Ty>::getCapacity() const
	{
		return m_capacity;
	}

	template<typename Ty>
	inline void DynamicStorageT<Ty>::setCapacity(uint32_t _capacity, uint32_t _count)
	{
		const uint32_t minCapacity = max(_capacity, _count);
		const uint32_t capacity    = minCapacity == 0 ? 0 : alignUp(minCapacity, 16);

		if (capacity == m_capacity)
		{
			return;
		}

		if (0 == capacity)
		{
			bx::free(m_allocator, m_ptr, BX_ALIGNOF(Ty) );
			m_ptr = NULL;
		}
		else if (BX_ENABLED(isTriviallyMoveConstructible<Ty>() ) )
		{
			m_ptr = (Ty*)bx::realloc(m_allocator, m_ptr, sizeof(Ty)*capacity, BX_ALIGNOF(Ty) );
		}
		else
		{
			Ty* ptr = (Ty*)bx::alloc(m_allocator, sizeof(Ty)*capacity, BX_ALIGNOF(Ty) );

			if (0 < _count)
			{
				StorageTraits::constructMove(ptr, m_ptr, _count);
				StorageTraits::destruct(m_ptr, _count);
			}

			if (NULL != m_ptr)
			{
				bx::free(m_allocator, m_ptr, BX_ALIGNOF(Ty) );
			}

			m_ptr = ptr;
		}

		m_capacity = capacity;
	}

	template<typename Ty>
	inline Ty* DynamicStorageT<Ty>::getPtr(uint32_t _idx)
	{
		if (NULL == m_ptr)
		{
			return NULL;
		}

		return m_ptr + _idx;
	}

	template<typename Ty>
	inline const Ty* DynamicStorageT<Ty>::getPtr(uint32_t _idx) const
	{
		return const_cast<DynamicStorageT<Ty>*>(this)->getPtr(_idx);
	}

	template<typename Ty>
	inline Ty* DynamicStorageT<Ty>::getTerm()
	{
		return getPtr(getCapacity() );
	}

	template<typename Ty>
	inline const Ty* DynamicStorageT<Ty>::getTerm() const
	{
		return getPtr(getCapacity() );
	}

	template<typename Ty>
	inline DynamicStorageT<Ty>& DynamicStorageT<Ty>::operator=(DynamicStorageT<Ty>&& _other)
	{
		if (NULL != m_ptr)
		{
			bx::free(m_allocator, m_ptr, BX_ALIGNOF(Ty) );
		}

		m_ptr       = _other.m_ptr;
		m_capacity  = _other.m_capacity;
		m_allocator = _other.m_allocator;

		_other.m_ptr      = NULL;
		_other.m_capacity = 0;

		return *this;
	}

} // namespace bx
