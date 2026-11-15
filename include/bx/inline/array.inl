/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_ARRAY_H_HEADER_GUARD
#	error "Must be included from bx/array.h!"
#endif // BX_ARRAY_H_HEADER_GUARD

namespace bx
{
	namespace internal
	{
		template<typename Ty>
		inline bool isEqual(const Ty* _lhs, const Ty* _rhs, uint32_t _count)
		{
			if (_lhs == _rhs
			||     0 == _count)
			{
				return true;
			}

			if constexpr (isPod<Ty>() )
			{
				return 0 == memCmp(_lhs, _rhs, sizeof(Ty)*_count);
			}
			else
			{
				for (uint32_t ii = 0; ii < _count; ++ii)
				{
					if (!(_lhs[ii] == _rhs[ii]) )
					{
						return false;
					}
				}

				return true;
			}
		}

	} // namespace internal

	template<typename Ty>
	inline bool isEqual(const ArrayViewBaseT<Ty>& _lhs, const ArrayViewBaseT<Ty>& _rhs)
	{
		const uint32_t count = _lhs.getCount();
		return count == _rhs.getCount()
			&& internal::isEqual(_lhs.getPtr(), _rhs.getPtr(), count)
			;
	}

	//---

	template<typename Ty>
	inline ArrayViewBaseT<Ty>::ArrayViewBaseT()
		: m_storageView(NULL, 0)
	{
	}

	template<typename Ty>
	inline const Ty* ArrayViewBaseT<Ty>::begin() const
	{
		return getPtr();
	}

	template<typename Ty>
	inline const Ty* ArrayViewBaseT<Ty>::end() const
	{
		return getTerm();
	}

	template<typename Ty>
	inline uint32_t ArrayViewBaseT<Ty>::getCount() const
	{
		return m_storageView.getCount();
	}

	template<typename Ty>
	inline bool ArrayViewBaseT<Ty>::isEmpty() const
	{
		return 0 == getCount();
	}

	template<typename Ty>
	inline const Ty* ArrayViewBaseT<Ty>::getPtr(uint32_t _idx) const
	{
		BX_ASSERT(_idx <= getCount(), "Out-of-bounds (_idx: %d, max: %d)!", _idx, getCount() );
		return m_storageView.m_ptr + _idx;
	}

	template<typename Ty>
	inline const Ty* ArrayViewBaseT<Ty>::getTerm() const
	{
		return m_storageView.m_term;
	}

	template<typename Ty>
	uint32_t ArrayViewBaseT<Ty>::getIndex(const Ty* _ptr) const
	{
		const uint32_t idx = uint32_t(_ptr - m_storageView.m_ptr);
		BX_ASSERT(idx < getCount(), "Pointer %p doesn't belong to the view (idx: %d, count: %d)!", _ptr, idx, getCount() );
		return idx;
	}

	template<typename Ty>
	inline const Ty& ArrayViewBaseT<Ty>::operator[](uint32_t _idx) const
	{
		BX_ASSERT(_idx < getCount(), "Out-of-bounds (_idx: %d, max: %d)!", _idx, getCount() );
		return *getPtr(_idx);
	}

	template<typename Ty>
	bool ArrayViewBaseT<Ty>::operator==(const ArrayViewBaseT<Ty>& _rhs) const
	{
		return isEqual(*this, _rhs);
	}

	template<typename Ty>
	inline bool isSorted(const ArrayViewBaseT<Ty>& _src, const ComparisonFn _fn)
	{
		return isSorted(_src.getPtr(), _src.getCount(), _fn);
	}

	template<typename Ty>
	inline uint32_t lowerBound(const Ty& _key, const ArrayViewBaseT<Ty>& _src, const ComparisonFn _fn)
	{
		return lowerBound(_key, _src.getPtr(), _src.getCount(), _fn);
	}

	template<typename Ty>
	inline uint32_t upperBound(const Ty& _key, const ArrayViewBaseT<Ty>& _src, const ComparisonFn _fn)
	{
		return upperBound(_key, _src.getPtr(), _src.getCount(), _fn);
	}

	template<typename Ty>
	inline int32_t binarySearch(const Ty& _key, const ArrayViewBaseT<Ty>& _src, const ComparisonFn _fn)
	{
		return binarySearch(_key, _src.getPtr(), _src.getCount(), _fn);
	}

	//---

	template<typename Ty>
	inline MutableArrayViewT<Ty>::MutableArrayViewT()
		: ArrayViewBaseT<Ty>()
	{
		set(NULL, 0u);
	}

	template<typename Ty>
	inline MutableArrayViewT<Ty>::MutableArrayViewT(Ty* _src, uint32_t _count)
	{
		set(_src, _count);
	}

	template<typename Ty>
	inline void MutableArrayViewT<Ty>::set(Ty* _src, uint32_t _count)
	{
		this->m_storageView = { _src, _count };
	}

	template<typename Ty>
	inline MutableArrayViewT<Ty>::MutableArrayViewT(Ty* _src, Ty* _term)
	{
		set(_src, _term);
	}

	template<typename Ty>
	inline void MutableArrayViewT<Ty>::set(Ty* _src, Ty* _term)
	{
		BX_ASSERT(_src <= _term, "Invalid argument. _src %p must be less or equal than _term %p.", _src, _term);
		set(_src, uint32_t(_term - _src) );
	}

	template<typename Ty>
	template<typename Uy>
	inline MutableArrayViewT<Ty>::MutableArrayViewT(ReinterpretTag, MutableArrayViewT<Uy>& _src)
	{
		set(Reinterpret, _src);
	}

	template<typename Ty>
	template<typename Uy>
	void inline MutableArrayViewT<Ty>::set(ReinterpretTag, MutableArrayViewT<Uy>& _src)
	{
		set(Reinterpret, _src.getPtr(), _src.getTerm() );
	}

	template<typename Ty>
	inline MutableArrayViewT<Ty>::MutableArrayViewT(ReinterpretTag, void* _src, uint32_t _count)
	{
		set(Reinterpret, _src, _count);
	}

	template<typename Ty>
	inline void MutableArrayViewT<Ty>::set(ReinterpretTag, void* _src, uint32_t _count)
	{
		set( (Ty*)_src, _count);
	}

	template<typename Ty>
	inline MutableArrayViewT<Ty>::MutableArrayViewT(ReinterpretTag, void* _src, void* _term)
	{
		set(Reinterpret, _src, _term);
	}

	template<typename Ty>
	inline void MutableArrayViewT<Ty>::set(ReinterpretTag, void* _src, void* _term)
	{
		BX_ASSERT(_src <= _term, "Invalid argument. _src %p must be less or equal than _term %p.", _src, _term);
		set(Reinterpret, _src, uint32_t( (const Ty*)_term -  (const Ty*)_src) );
	}

	template<typename Ty>
	template<size_t SizeT>
	inline MutableArrayViewT<Ty>::MutableArrayViewT(Ty (&_src)[SizeT])
	{
		set(_src, uint32_t(SizeT) );
	}

	template<typename Ty>
	template<size_t SizeT>
	inline void MutableArrayViewT<Ty>::set(Ty (&_src)[SizeT])
	{
		set(_src, uint32_t(SizeT) );
	}

	template<typename Ty>
	inline MutableArrayViewT<Ty>::MutableArrayViewT(MutableArrayViewT<Ty>& _src)
	{
		set(_src);
	}

	template<typename Ty>
	inline void MutableArrayViewT<Ty>::set(MutableArrayViewT<Ty>& _src)
	{
		set(_src.getPtr(), _src.getTerm() );
	}

	template<typename Ty>
	inline Ty* MutableArrayViewT<Ty>::getPtr(uint32_t _idx)
	{
		BX_ASSERT(_idx <= this->getCount(), "Out-of-bounds (_idx: %d, max: %d)!", _idx, this->getCount() );
		return const_cast<Ty*>(&this->m_storageView.m_ptr[_idx]);
	}

	template<typename Ty>
	inline const Ty* MutableArrayViewT<Ty>::getPtr(uint32_t _idx) const
	{
		BX_ASSERT(_idx <= this->getCount(), "Out-of-bounds (_idx: %d, max: %d)!", _idx, this->getCount() );
		return this->m_storageView.m_ptr + _idx;
	}

	template<typename Ty>
	inline Ty* MutableArrayViewT<Ty>::getTerm()
	{
		return getPtr(this->getCount() );
	}

	template<typename Ty>
	inline const Ty* MutableArrayViewT<Ty>::getTerm() const
	{
		return getPtr(this->getCount() );
	}

	template<typename Ty>
	inline MutableArrayViewT<Ty> MutableArrayViewT<Ty>::get(uint32_t _idx, uint32_t _count)
	{
		sanitize(this->getCount(), _idx, _count);
		return MutableArrayViewT<Ty>(getPtr(_idx), _count);
	}

	template<typename Ty>
	inline MutableArrayViewT<Ty> MutableArrayViewT<Ty>::get(Element _element, uint32_t _count)
	{
		const uint32_t oldCount = this->getCount();
		_count = min(oldCount, _count);

		const uint32_t idx = _element == kLast ? oldCount - _count : 0;
		return MutableArrayViewT<Ty>(this->getPtr(idx), _count);
	}

	template<typename Ty>
	inline Ty& MutableArrayViewT<Ty>::operator[](uint32_t _idx)
	{
		BX_ASSERT(_idx < this->getCount(), "Out-of-bounds (_idx: %d, max: %d)!", _idx, this->getCount() );
		return *getPtr(_idx);
	}

	template<typename Ty>
	inline const Ty& MutableArrayViewT<Ty>::operator[](uint32_t _idx) const
	{
		BX_ASSERT(_idx < this->getCount(), "Out-of-bounds (_idx: %d, max: %d)!", _idx, this->getCount() );
		return *ArrayViewBaseT<Ty>::getPtr(_idx);
	}

	template<typename Ty>
	inline void sort(MutableArrayViewT<Ty>& _inoutSrc, const ComparisonFn _fn)
	{
		quickSort<Ty>(_inoutSrc.getPtr(), _inoutSrc.getCount(), _fn);
	}

	template<typename Ty, typename LessFn>
	inline void stableSort(MutableArrayViewT<Ty>& _inoutSrc, LessFn _less)
	{
		stableSort(getDefaultAllocatorBlk()->getHandle(), _inoutSrc.getPtr(), _inoutSrc.getCount(), _less);
	}

	template<typename Ty>
	inline uint32_t unique(MutableArrayViewT<Ty>& _inoutSrc, const ComparisonFn _fn)
	{
		return unique<Ty>(_inoutSrc.getPtr(), _inoutSrc.getCount(), _fn);
	}

	//---

	template<typename Ty>
	inline ArrayViewT<Ty>::ArrayViewT()
	{
		set(NULL, 0u);
	}

	template<typename Ty>
	inline ArrayViewT<Ty>::ArrayViewT(const Ty* _src, uint32_t _count)
	{
		set(_src, _count);
	}

	template<typename Ty>
	inline void ArrayViewT<Ty>::set(const Ty* _src, uint32_t _count)
	{
		this->m_storageView = { _src, _count };
	}

	template<typename Ty>
	inline ArrayViewT<Ty>::ArrayViewT(const Ty* _src, const Ty* _term)
	{
		set(_src, _term);
	}

	template<typename Ty>
	inline void ArrayViewT<Ty>::set(const Ty* _src, const Ty* _term)
	{
		BX_ASSERT(_src <= _term, "Invalid argument. _src %p must be smaller than _term %p.", _src, _term);
		set(_src, uint32_t(_term - _src) );
	}

	template<typename Ty>
	template<typename Uy>
	inline ArrayViewT<Ty>::ArrayViewT(ReinterpretTag, const ArrayViewT<Uy>& _src)
	{
		set(Reinterpret, _src);
	}

	template<typename Ty>
	template<typename Uy>
	inline void ArrayViewT<Ty>::set(ReinterpretTag, const ArrayViewT<Uy>& _src)
	{
		set(Reinterpret, _src.getPtr(), _src.getTerm() );
	}

	template<typename Ty>
	inline ArrayViewT<Ty>::ArrayViewT(ReinterpretTag, const void* _src, uint32_t _count)
	{
		set(Reinterpret, _src, _count);
	}

	template<typename Ty>
	inline void ArrayViewT<Ty>::set(ReinterpretTag, const void* _src, uint32_t _count)
	{
		set( (const Ty*)_src, _count);
	}

	template<typename Ty>
	inline ArrayViewT<Ty>::ArrayViewT(ReinterpretTag, const void* _src, const void* _term)
	{
		set(Reinterpret, _src, _term);
	}

	template<typename Ty>
	inline void ArrayViewT<Ty>::set(ReinterpretTag, const void* _src, const void* _term)
	{
		BX_ASSERT(_src <= _term, "Invalid argument. _src %p must be smaller than _term %p.", _src, _term);
		set(Reinterpret, _src, uint32_t( (const Ty*)_term - (const Ty*)_src) );
	}

	template<typename Ty>
	template<size_t SizeT>
	inline ArrayViewT<Ty>::ArrayViewT(const Ty (&_src)[SizeT])
	{
		set(_src, uint32_t(SizeT) );
	}

	template<typename Ty>
	template<size_t SizeT>
	inline void ArrayViewT<Ty>::set(const Ty (&_src)[SizeT])
	{
		set(_src, uint32_t(SizeT) );
	}

	template<typename Ty>
	inline ArrayViewT<Ty>::ArrayViewT(const ArrayViewBaseT<Ty>& _src)
	{
		set(_src);
	}

	template<typename Ty>
	inline void ArrayViewT<Ty>::set(const ArrayViewBaseT<Ty>& _src)
	{
		set(_src.getPtr(), _src.getTerm() );
	}

	template<typename Ty>
	inline ArrayViewT<Ty> ArrayViewT<Ty>::get(uint32_t _idx, uint32_t _count) const
	{
		sanitize(this->getCount(), _idx, _count);
		return ArrayViewT<Ty>(this->getPtr(_idx), _count);
	}

	template<typename Ty>
	inline ArrayViewT<Ty> ArrayViewT<Ty>::get(Element _element, uint32_t _count) const
	{
		const uint32_t oldCount = this->getCount();
		_count = min(oldCount, _count);

		const uint32_t idx = _element == kLast ? oldCount - _count : 0;
		return ArrayViewT<Ty>(this->getPtr(idx), _count);
	}

	//---

	template<typename ContainerT, typename Ty>
	inline ArrayIteratorT<ContainerT, Ty>::ArrayIteratorT(ContainerT& _container, uint32_t _idx)
		: m_container(_container)
		, m_idx(_idx)
		, m_count(_container.getCount() )
	{
	}

	template<typename ContainerT, typename Ty>
	inline Ty& ArrayIteratorT<ContainerT, Ty>::operator*() const
	{
		return *m_container.getPtr(m_idx);
	}

	template<typename ContainerT, typename Ty>
	inline bool ArrayIteratorT<ContainerT, Ty>::operator!=(const ArrayIteratorT<ContainerT, Ty>& _rhs) const
	{
		BX_UNUSED(_rhs);
		return m_idx < m_container.getCount();
	}

	template<typename ContainerT, typename Ty>
	inline const ArrayIteratorT<ContainerT, Ty>& ArrayIteratorT<ContainerT, Ty>::operator++()
	{
		const uint32_t count = m_container.getCount();
		BX_ASSERT(count <= m_count, "Element added during iteration (count: %u, was: %u)!", count, m_count);

		m_idx  += count == m_count;
		m_count = count;

		return *this;
	}

	//---

	template<typename Ty, typename StorageT>
	inline ArrayBaseT<Ty, StorageT>::ArrayBaseT()
		: MutableArrayViewT<Ty>()
	{
		this->m_storageView = { m_storage.getPtr(), 0 };
	}

	template<typename Ty, typename StorageT>
	inline ArrayBaseT<Ty, StorageT>::ArrayBaseT(const ArrayViewBaseT<Ty>& _src)
		: ArrayBaseT<Ty, StorageT>::ArrayBaseT()
	{
		add(_src);
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::set(const ArrayViewBaseT<Ty>& _src)
	{
		const uint32_t num = _src.getCount();

		if (0 != num
		&&  isAliased(_src.getPtr() ) )
		{
			const uint32_t pos = this->getIndex(_src.getPtr() );
			remove(pos + num, kOrdered, this->getCount() - pos - num);
			remove(0, kOrdered, pos);
			return;
		}

		this->clear();
		add(_src);
	}

	template<typename Ty, typename StorageT>
	inline ArrayBaseT<Ty, StorageT>::ArrayBaseT(const ArrayBaseT<Ty, StorageT>& _src)
		: ArrayBaseT<Ty, StorageT>::ArrayBaseT()
	{
		add(_src);
	}

	template<typename Ty, typename StorageT>
	template<typename... ArgsT>
	inline ArrayBaseT<Ty, StorageT>::ArrayBaseT(EmplaceTag, ArgsT&&... _args)
		: ArrayBaseT<Ty, StorageT>::ArrayBaseT()
	{
		this->m_storageView =
		{
			m_storage.getPtr(),
			StorageTraits::constructEmplace(m_storage.getPtr(), m_storage.getCapacity(), bx::forward<ArgsT>(_args)...),
		};
	}

	template<typename Ty, typename StorageT>
	template<typename... ArgsT>
	inline ArrayBaseT<Ty, StorageT>::ArrayBaseT(uint32_t _count, EmplaceTag, ArgsT&&... _args)
		: ArrayBaseT<Ty, StorageT>::ArrayBaseT()
	{
		grow(_count);

		this->m_storageView =
		{
			m_storage.getPtr(),
			StorageTraits::constructEmplace(m_storage.getPtr(), _count, bx::forward<ArgsT>(_args)...),
		};
	}

	template<typename Ty, typename StorageT>
	inline ArrayBaseT<Ty, StorageT>::~ArrayBaseT()
	{
		this->clear();
	}

	template<typename Ty, typename StorageT>
	inline ArrayBaseT<Ty, StorageT>& ArrayBaseT<Ty, StorageT>::operator=(const ArrayViewBaseT<Ty>& _src)
	{
		if (this != &_src)
		{
			set(_src);
		}

		return *this;
	}

	template<typename Ty, typename StorageT>
	inline ArrayBaseT<Ty, StorageT>& ArrayBaseT<Ty, StorageT>::operator=(const ArrayBaseT<Ty, StorageT>& _src)
	{
		if (this != &_src)
		{
			this->set(_src);
		}

		return *this;
	}

	template<typename Ty, typename StorageT>
	inline typename ArrayBaseT<Ty, StorageT>::Iterator ArrayBaseT<Ty, StorageT>::begin()
	{
		return Iterator(*this, 0);
	}

	template<typename Ty, typename StorageT>
	inline typename ArrayBaseT<Ty, StorageT>::Iterator ArrayBaseT<Ty, StorageT>::end()
	{
		return Iterator(*this, this->getCount() );
	}

	template<typename Ty, typename StorageT>
	inline typename ArrayBaseT<Ty, StorageT>::ConstIterator ArrayBaseT<Ty, StorageT>::begin() const
	{
		return ConstIterator(*this, 0);
	}

	template<typename Ty, typename StorageT>
	inline typename ArrayBaseT<Ty, StorageT>::ConstIterator ArrayBaseT<Ty, StorageT>::end() const
	{
		return ConstIterator(*this, this->getCount() );
	}

	template<typename Ty, typename StorageT>
	inline uint32_t ArrayBaseT<Ty, StorageT>::getCapacity() const
	{
		return m_storage.getCapacity();
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::setCapacity(uint32_t _capacity)
	{
		this->m_storageView.setCapacity(m_storage, _capacity);
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::setCount(uint32_t _count, InitNoneTag)
	{
		if constexpr (isTriviallyConstructible<Ty>() && isTriviallyDestructible<Ty>() )
		{
			if (_count <= m_storage.getCapacity() )
			{
				this->m_storageView.m_term = this->m_storageView.m_ptr + _count;
				return;
			}
		}

		const uint32_t oldCount = this->getCount();

		if (_count > oldCount)
		{
			grow(_count - oldCount);
		}

		if constexpr (isTypeless() && isTriviallyConstructible<Ty>() )
		{
			this->m_storageView.m_term = this->m_storageView.m_ptr + _count;
		}
		else
		{
			this->m_storageView.setCount(m_storage, _count);
		}
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::setCount(uint32_t _count)
	{
		const uint32_t oldCount = this->getCount();

		setCount(_count, InitNone);

		if constexpr (isTriviallyConstructible<Ty>() )
		{
			if (_count > oldCount)
			{
				memSet(this->getPtr(oldCount), 0, (_count - oldCount)*sizeof(Ty) );
			}
		}
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::setCount(uint32_t _count, const Ty& _value)
	{
		const uint32_t oldCount = this->getCount();

		if (_count <= oldCount)
		{
			remove(_count, kOrdered, oldCount - _count);
			return;
		}

		if (isAliased(addressOf(_value) ) )
		{
			const Ty value(_value);
			setCount(_count, value);
			return;
		}

		grow(_count - oldCount);

		Ty* ptr  = const_cast<Ty*>(this->m_storageView.m_ptr);
		Ty* term = ptr + _count;

		for (Ty* dst = ptr + oldCount; dst != term; ++dst)
		{
			if constexpr (isTriviallyCopyable<Ty>() )
			{
				*dst = _value;
			}
			else
			{
				BX_PLACEMENT_NEW(dst, Ty)(_value);
			}
		}

		this->m_storageView.m_term = term;
	}

	template<typename Ty, typename StorageT>
	inline constexpr bool ArrayBaseT<Ty, StorageT>::isTypeless()
	{
		return IsDynamicStorageT<StorageT>::value
			&& isTriviallyCopyable<Ty>()
			;
	}

	template<typename Ty, typename StorageT>
	inline constexpr bool ArrayBaseT<Ty, StorageT>::isRelocatable()
	{
		return IsDynamicStorageT<StorageT>::value
			&& isTriviallyRelocatable<Ty>()
			;
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::grow(uint32_t _num)
	{
		constexpr uint32_t kMinGrowCapacity = max<uint32_t>(1, 64/sizeof(Ty) );

		const uint32_t count    = this->getCount();
		const uint32_t capacity = m_storage.getCapacity();

		if (count + _num > capacity)
		{
			if constexpr (isRelocatable() )
			{
				this->m_storageView.m_term = m_storage.grow(this->m_storageView.m_ptr, count, _num);
			}
			else
			{
				setCapacity(max(count + _num, capacity + capacity/2, kMinGrowCapacity) );
			}
		}
	}

	template<typename Ty, typename StorageT>
	inline const Ty* ArrayBaseT<Ty, StorageT>::insertTypeless(uint32_t _pos, const Ty* _src, uint32_t _num)
	{
		return m_storage.insert(this->m_storageView.m_ptr, this->getCount(), _pos, _src, _num);
	}

	template<typename Ty, typename StorageT>
	inline bool ArrayBaseT<Ty, StorageT>::isAliased(const Ty* _ptr) const
	{
		const Ty* ptr = this->getPtr();
		return NULL != ptr
			&& _ptr >= ptr
			&& _ptr <  ptr + this->getCount()
			;
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::clear()
	{
		StorageTraits::destruct(const_cast<Ty*>(this->m_storageView.m_ptr), this->getCount() );
		this->m_storageView.m_term = this->m_storageView.m_ptr;
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::trim()
	{
		setCapacity(0);
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::add(const Ty& _src)
	{
		add(UINT32_MAX, _src);
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::add(uint32_t _pos, const Ty& _src)
	{
		Ty* ptr = const_cast<Ty*>(this->m_storageView.m_term);

		if (_pos >= this->getCount()
		&&  ptr < m_storage.getTerm() )
		{
			if constexpr (isTriviallyCopyable<Ty>() )
			{
				*ptr = _src;
			}
			else
			{
				BX_PLACEMENT_NEW(ptr, Ty)(_src);
			}

			this->m_storageView.m_term = ptr + 1;
			return;
		}

		if constexpr (isTypeless() )
		{
			const Ty value = _src;
			this->m_storageView.m_term = insertTypeless(_pos, &value, 1);
		}
		else if constexpr (isTriviallyCopyable<Ty>() )
		{
			const Ty value = _src;
			this->m_storageView.m_term = addSlow(_pos, value);
		}
		else
		{
			this->m_storageView.m_term = addSlow(_pos, _src);
		}
	}

	template<typename Ty, typename StorageT>
	const Ty* ArrayBaseT<Ty, StorageT>::addSlow(uint32_t _pos, const Ty& _src)
	{
		if (isAliased(addressOf(_src) ) )
		{
			Ty value(_src);
			add(_pos, Emplace, bx::move(value) );
			return this->m_storageView.m_term;
		}

		if constexpr (isRelocatable() )
		{
			const uint32_t count = this->getCount();
			Ty* room = m_storage.carve(this->m_storageView.m_ptr, count, _pos, 1);
			BX_PLACEMENT_NEW(room, Ty)(_src);

			return this->m_storageView.m_ptr + count + 1;
		}
		else
		{
			grow(1);
			this->m_storageView.add(m_storage, _pos, &_src, 1);

			return this->m_storageView.m_term;
		}
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::add(Ty&& _src)
	{
		add(Emplace, bx::move(_src) );
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::add(uint32_t _pos, Ty&& _src)
	{
		add(_pos, Emplace, bx::move(_src) );
	}

	template<typename Ty, typename StorageT>
	template<size_t SizeT>
	inline void ArrayBaseT<Ty, StorageT>::add(const Ty (&_src)[SizeT])
	{
		add(UINT32_MAX, ArrayViewT<Ty>(_src) );
	}

	template<typename Ty, typename StorageT>
	template<size_t SizeT>
	inline void ArrayBaseT<Ty, StorageT>::add(uint32_t _pos, const Ty (&_src)[SizeT])
	{
		add(_pos, ArrayViewT<Ty>(_src) );
	}

	template<typename Ty, typename StorageT>
	template<typename... ArgsT>
	inline void ArrayBaseT<Ty, StorageT>::add(EmplaceTag, ArgsT&&... _args)
	{
		add(UINT32_MAX, Emplace, bx::forward<ArgsT>(_args)...);
	}

	template<typename Ty, typename StorageT>
	template<typename... ArgsT>
	inline void ArrayBaseT<Ty, StorageT>::add(uint32_t _pos, EmplaceTag, ArgsT&&... _args)
	{
		Ty* ptr = const_cast<Ty*>(this->m_storageView.m_term);

		if (_pos >= this->getCount()
		&&  ptr < m_storage.getTerm() )
		{
			BX_PLACEMENT_NEW(ptr, Ty)(bx::forward<ArgsT>(_args)...);
			this->m_storageView.m_term = ptr + 1;
			return;
		}

		if constexpr (isTypeless() )
		{
			const Ty value(bx::forward<ArgsT>(_args)...);
			this->m_storageView.m_term = insertTypeless(_pos, &value, 1);
		}
		else
		{
			this->m_storageView.m_term = addSlow(_pos, Emplace, bx::forward<ArgsT>(_args)...);
		}
	}

	template<typename Ty, typename StorageT>
	template<typename... ArgsT>
	const Ty* ArrayBaseT<Ty, StorageT>::addSlow(uint32_t _pos, EmplaceTag, ArgsT&&... _args)
	{
		if constexpr (isRelocatable() )
		{
			Ty value(bx::forward<ArgsT>(_args)...);

			const uint32_t count = this->getCount();
			Ty* room = m_storage.carve(this->m_storageView.m_ptr, count, _pos, 1);
			BX_PLACEMENT_NEW(room, Ty)(bx::move(value) );

			return this->m_storageView.m_ptr + count + 1;
		}
		else
		{
			grow(1);
			this->m_storageView.add(m_storage, _pos, Emplace, bx::forward<ArgsT>(_args)...);

			return this->m_storageView.m_term;
		}
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::add(const ArrayViewBaseT<Ty>& _src)
	{
		add(UINT32_MAX, _src);
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::add(uint32_t _pos, const ArrayViewBaseT<Ty>& _src)
	{
		if (0 != _src.getCount()
		&&  isAliased(_src.getPtr() ) )
		{
			ArrayT<Ty> src;
			src.add(_src);
			add(_pos, src);
			return;
		}

		if constexpr (isTriviallyCopyable<Ty>() )
		{
			Ty*            ptr = const_cast<Ty*>(this->m_storageView.m_term);
			const uint32_t num = _src.getCount();

			if (_pos >= this->getCount()
			&&  num <= uint32_t(m_storage.getTerm() - ptr) )
			{
				memCopy(ptr, _src.getPtr(), num*sizeof(Ty) );
				this->m_storageView.m_term = ptr + num;
				return;
			}
		}

		if constexpr (isTypeless() )
		{
			this->m_storageView.m_term = insertTypeless(_pos, _src.getPtr(), _src.getCount() );
		}
		else
		{
			this->m_storageView.m_term = addSlow(_pos, _src);
		}
	}

	template<typename Ty, typename StorageT>
	const Ty* ArrayBaseT<Ty, StorageT>::addSlow(uint32_t _pos, const ArrayViewBaseT<Ty>& _src)
	{
		if constexpr (isRelocatable() )
		{
			const uint32_t count = this->getCount();
			const uint32_t num   = _src.getCount();
			const Ty*      src   = _src.getPtr();
			Ty* room = m_storage.carve(this->m_storageView.m_ptr, count, _pos, num);

			for (uint32_t ii = 0; ii < num; ++ii)
			{
				BX_PLACEMENT_NEW(room + ii, Ty)(src[ii]);
			}

			return this->m_storageView.m_ptr + count + num;
		}
		else
		{
			grow(_src.getCount() );
			this->m_storageView.add(m_storage, _pos, _src.getPtr(), _src.getCount() );

			return this->m_storageView.m_term;
		}
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::remove(uint32_t _pos, uint32_t _count)
	{
		remove(_pos, kUnordered, _count);
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::remove(uint32_t _pos, Ordering _ordering, uint32_t _count)
	{
		if constexpr (isRelocatable() && !isTriviallyCopyable<Ty>() )
		{
			const uint32_t oldCount = this->getCount();
			uint32_t pos   = _pos;
			uint32_t count = _count;
			sanitize(oldCount, pos, count);

			if (0 == count)
			{
				return;
			}

			const uint32_t end  = pos + count;
			const uint32_t move = kOrdered == _ordering ? oldCount - end : min(oldCount - end, count);
			const uint32_t from = kOrdered == _ordering ? end : oldCount - move;

			Ty* ptr = const_cast<Ty*>(this->m_storageView.m_ptr);
			StorageTraits::destruct(ptr + pos, count);
			memMove(ptr + pos, ptr + from, move*sizeof(Ty) );

			this->m_storageView.m_term = ptr + oldCount - count;
		}
		else
		{
			this->m_storageView.remove(m_storage, _pos, _ordering, _count);
		}
	}

	template<typename Ty, typename StorageT>
	template<typename Uy>
	inline void ArrayBaseT<Ty, StorageT>::remove(const Uy* _element, Ordering _ordering)
	{
		static_assert(isSame<Ty, Uy>(), "Element pointer must point to an element of this array type.");
		remove(this->getIndex(_element), _ordering, 1);
	}

	template<typename Ty, typename StorageT>
	inline void ArrayBaseT<Ty, StorageT>::remove(Element _element, uint32_t _count)
	{
		const uint32_t oldCount = this->getCount();
		_count = min(oldCount, _count);

		const uint32_t idx = _element == kLast ? oldCount - _count : 0;
		remove(idx, kOrdered, _count);
	}

	template<typename Ty, typename StorageT>
	inline Ty ArrayBaseT<Ty, StorageT>::pop(uint32_t _idx, Ordering _ordering)
	{
		const uint32_t count = this->getCount();
		BX_ASSERT(0 < count, "Cannot pop from empty array.");

		const uint32_t idx  = min(_idx, count-1);
		const bool     last = idx == count-1;
		Ty* ptr = const_cast<Ty*>(last ? this->m_storageView.m_term - 1 : this->m_storageView.m_ptr + idx);
		Ty result = bx::move(*ptr);

		if (last)
		{
			StorageTraits::destruct(ptr, 1);
			this->m_storageView.m_term = ptr;
		}
		else
		{
			remove(idx, _ordering);
		}

		return bx::move(result);
	}

	template<typename Ty, typename StorageT>
	inline Ty ArrayBaseT<Ty, StorageT>::pop(Element _element)
	{
		const uint32_t idx = kLast == _element ? this->getCount()-1 : 0;
		return bx::move(pop(idx, kOrdered) );
	}

	template<typename Ty, uint32_t CapacityT>
	StaticArrayT<Ty, CapacityT>::StaticArrayT()
	{
		this->setCount(this->getCapacity() );
	}

	template<typename Ty>
	inline ArrayT<Ty>::ArrayT()
		: ArrayBaseT<Ty, DynamicStorageT<Ty>>()
	{
	}

	template<typename Ty>
	inline ArrayT<Ty>::ArrayT(const ArrayT<Ty>& _src)
		: ArrayBaseT<Ty, DynamicStorageT<Ty>>()
	{
		this->m_storage.setAllocatorHandle(_src.m_storage.getAllocatorHandle() );
		set(_src);
	}

	template<typename Ty>
	inline ArrayT<Ty>::ArrayT(ArrayT<Ty>&& _src)
	{
		set(bx::move(_src) );
	}

	template<typename Ty>
	inline ArrayT<Ty>::ArrayT(AllocatorHandle _allocator)
		: ArrayBaseT<Ty, DynamicStorageT<Ty>>()
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");
		this->m_storage.setAllocatorHandle(_allocator);
	}

	template<typename Ty>
	inline ArrayT<Ty>::ArrayT(AllocatorHandle _allocator, uint32_t _capacity)
		: ArrayT(_allocator)
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");

		if (0 != _capacity)
		{
			this->setCapacity(_capacity);
		}
	}

	template<typename Ty>
	template<typename... ArgsT>
	inline ArrayT<Ty>::ArrayT(AllocatorHandle _allocator, uint32_t _count, EmplaceTag, ArgsT&&... _args)
		: ArrayT(_allocator, _count)
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");
		for (uint32_t ii = 0; ii < _count; ++ii)
		{
			this->add(Emplace, _args...);
		}
	}

	template<typename Ty>
	template<size_t SizeT>
	inline ArrayT<Ty>::ArrayT(AllocatorHandle _allocator, const Ty (&_src)[SizeT])
		: ArrayT(_allocator, SizeT)
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");
		set(_src);
	}

	template<typename Ty>
	inline ArrayT<Ty>::ArrayT(AllocatorHandle _allocator, const ArrayViewBaseT<Ty>& _src)
		: ArrayT(_allocator)
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");
		set(_src);
	}

	template<typename Ty>
	inline void ArrayT<Ty>::setAllocatorHandle(AllocatorHandle _allocator)
	{
		this->m_storage.setAllocatorHandle(_allocator);
	}

	template<typename Ty>
	inline AllocatorHandle ArrayT<Ty>::getAllocatorHandle() const
	{
		return this->m_storage.getAllocatorHandle();
	}

	template<typename Ty>
	inline ArrayT<Ty>& ArrayT<Ty>::operator=(const ArrayViewBaseT<Ty>& _src)
	{
		if (this != &_src)
		{
			this->set(_src);
		}

		return *this;
	}

	template<typename Ty>
	inline ArrayT<Ty>& ArrayT<Ty>::operator=(const ArrayT<Ty>& _src)
	{
		if (this != &_src)
		{
			if (0 == this->getCapacity()
			&&  kAllocatorHandleDefault == this->m_storage.getAllocatorHandle().idx)
			{
				this->m_storage.setAllocatorHandle(_src.m_storage.getAllocatorHandle() );
			}

			this->set(_src);
		}

		return *this;
	}

	template<typename Ty>
	inline ArrayT<Ty>& ArrayT<Ty>::operator=(ArrayT<Ty>&& _src)
	{
		set(bx::move(_src) );

		return *this;
	}

	template<typename Ty>
	template<size_t SizeT>
	inline void ArrayT<Ty>::set(const Ty (&_src)[SizeT])
	{
		set(ArrayViewT<Ty>(_src) );
	}

	template<typename Ty>
	inline void ArrayT<Ty>::set(const ArrayViewBaseT<Ty>& _src)
	{
		ArrayBaseT<Ty, DynamicStorageT<Ty>>::set(_src);
	}

	template<typename Ty>
	inline void ArrayT<Ty>::set(ArrayT<Ty>&& _src)
	{
		if (this != &_src)
		{
			this->clear();
			this->trim();

			this->m_storage = bx::move(_src.m_storage);
			this->m_storageView = bx::move(_src.m_storageView);
		}
	}

	template<typename Ty>
	template<typename... ArgsT>
	inline void ArrayT<Ty>::set(EmplaceTag, ArgsT&&... _args)
	{
		this->clear();
		this->add(Emplace, bx::forward<ArgsT>(_args)...);
	}

	//---

	template<typename Ty>
	inline ArrayWriterT<Ty>::ArrayWriterT(ArrayT<Ty>* _array)
		: m_array(_array)
	{
	}

	template<typename Ty>
	inline ArrayWriterT<Ty>::~ArrayWriterT()
	{
	}

	template<typename Ty>
	inline int32_t ArrayWriterT<Ty>::write(const void* _data, int32_t _size, Error* _err)
	{
		BX_UNUSED(_err);
		BX_ASSERT(0 <= _size, "Size must not be negative!");

		m_array->add(ArrayViewT<Ty>(static_cast<const Ty*>(_data), uint32_t(_size) ) );

		return _size;
	}

	//---

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline SortedArrayBaseT<Ty, StorageT, CmpFnT>::SortedArrayBaseT()
		: ArrayViewBaseT<Ty>()
	{
		this->m_storageView = { m_storage.getPtr(), 0 };
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline Ty* SortedArrayBaseT<Ty, StorageT, CmpFnT>::getPtr(UncheckedTag, uint32_t _idx)
	{
		return const_cast<Ty*>(ArrayViewBaseT<Ty>::getPtr(_idx) );
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline Ty* SortedArrayBaseT<Ty, StorageT, CmpFnT>::getTerm(UncheckedTag)
	{
		return const_cast<Ty*>(ArrayViewBaseT<Ty>::getTerm() );
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	template<typename StorageU, ComparisonFn CmpFnU>
	inline SortedArrayBaseT<Ty, StorageT, CmpFnT>::SortedArrayBaseT(const SortedArrayBaseT<Ty, StorageU, CmpFnU>& _src)
		: SortedArrayBaseT<Ty, StorageT, CmpFnT>::SortedArrayBaseT()
	{
		add(_src);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline SortedArrayBaseT<Ty, StorageT, CmpFnT>::SortedArrayBaseT(const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src)
		: SortedArrayBaseT<Ty, StorageT, CmpFnT>::SortedArrayBaseT()
	{
		this->m_storageView.add(m_storage, 0, _src.getPtr(), _src.getCount() );
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline SortedArrayBaseT<Ty, StorageT, CmpFnT>::SortedArrayBaseT(const ArrayViewBaseT<Ty>& _src)
		: SortedArrayBaseT<Ty, StorageT, CmpFnT>::SortedArrayBaseT()
	{
		add(_src);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::set(const ArrayViewBaseT<Ty>& _src)
	{
		const uint32_t num = _src.getCount();

		if (0 != num
		&&  isAliased(_src.getPtr() ) )
		{
			const uint32_t pos = this->getIndex(_src.getPtr() );
			remove(pos + num, this->getCount() - pos - num);
			remove(0, pos);
			return;
		}

		this->clear();
		add(_src);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline SortedArrayBaseT<Ty, StorageT, CmpFnT>& SortedArrayBaseT<Ty, StorageT, CmpFnT>::operator=(const ArrayViewBaseT<Ty>& _src)
	{
		if (this != &_src)
		{
			set(_src);
		}

		return *this;
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline SortedArrayBaseT<Ty, StorageT, CmpFnT>& SortedArrayBaseT<Ty, StorageT, CmpFnT>::operator=(const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src)
	{
		if (this != &_src)
		{
			set(_src);
		}

		return *this;
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline SortedArrayBaseT<Ty, StorageT, CmpFnT>::~SortedArrayBaseT()
	{
		this->clear();
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline typename SortedArrayBaseT<Ty, StorageT, CmpFnT>::ConstIterator SortedArrayBaseT<Ty, StorageT, CmpFnT>::begin() const
	{
		return ConstIterator(*this, 0);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline typename SortedArrayBaseT<Ty, StorageT, CmpFnT>::ConstIterator SortedArrayBaseT<Ty, StorageT, CmpFnT>::end() const
	{
		return ConstIterator(*this, this->getCount() );
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline uint32_t SortedArrayBaseT<Ty, StorageT, CmpFnT>::getCapacity() const
	{
		return m_storage.getCapacity();
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::setCapacity(uint32_t _capacity)
	{
		this->m_storageView.setCapacity(m_storage, _capacity);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::setCount(uint32_t _count)
	{
		this->m_storageView.setCount(m_storage, _count);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline constexpr bool SortedArrayBaseT<Ty, StorageT, CmpFnT>::isTypeless()
	{
		return IsDynamicStorageT<StorageT>::value
			&& isTriviallyCopyable<Ty>()
			;
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline constexpr bool SortedArrayBaseT<Ty, StorageT, CmpFnT>::isRelocatable()
	{
		return IsDynamicStorageT<StorageT>::value
			&& isTriviallyRelocatable<Ty>()
			;
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::grow(uint32_t _num)
	{
		constexpr uint32_t kMinGrowCapacity = max<uint32_t>(1, 64/sizeof(Ty) );

		const uint32_t count    = this->getCount();
		const uint32_t capacity = m_storage.getCapacity();

		if (count + _num > capacity)
		{
			if constexpr (isRelocatable() )
			{
				this->m_storageView.m_term = m_storage.grow(this->m_storageView.m_ptr, count, _num);
			}
			else
			{
				setCapacity(max(count + _num, capacity + capacity/2, kMinGrowCapacity) );
			}
		}
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::addTypeless(uint32_t _pos, bool _search, const Ty& _src)
	{
		const Ty       value = _src;
		const uint32_t count = this->getCount();
		const uint32_t pos   = min(_pos, count);
		const uint32_t idx   = _search
			? pos + upperBound(value, this->m_storageView.m_ptr + pos, count - pos, CmpFnT)
			: pos
			;

		this->m_storageView.m_term = m_storage.insert(this->m_storageView.m_ptr, count, idx, &value, 1);

		BX_ASSERT(isSorted(*this), "Not sorted!");
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline bool SortedArrayBaseT<Ty, StorageT, CmpFnT>::isAliased(const Ty* _ptr) const
	{
		const Ty* ptr = this->getPtr();
		return NULL != ptr
			&& _ptr >= ptr
			&& _ptr <  ptr + this->getCount()
			;
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::clear()
	{
		StorageTraits::destruct(const_cast<Ty*>(this->m_storageView.m_ptr), this->getCount() );
		this->m_storageView.m_term = this->m_storageView.m_ptr;
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::trim()
	{
		setCapacity(0);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline bool SortedArrayBaseT<Ty, StorageT, CmpFnT>::isSortedAt(uint32_t _pos, const Ty& _value) const
	{
		const uint32_t count = this->getCount();
		const Ty*      ptr   = this->m_storageView.m_ptr;

		return true
			&& _pos <= count
			&& (0     == _pos || 0 >= CmpFnT(ptr + _pos-1, addressOf(_value) ) )
			&& (count == _pos || 0 >= CmpFnT(addressOf(_value), ptr + _pos) )
			;
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::add(const Ty& _src)
	{
		insertAt(0, true, _src);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::add(uint32_t _pos, const Ty& _src)
	{
		insertAt(_pos, true, _src);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::add(UncheckedTag, uint32_t _pos, const Ty& _src)
	{
		BX_ASSERT(isSortedAt(_pos, _src), "Position %u is not the sorted position of the element!", _pos);
		insertAt(_pos, false, _src);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::insertAt(uint32_t _pos, bool _search, const Ty& _src)
	{
		if constexpr (isTypeless() )
		{
			addTypeless(_pos, _search, _src);
		}
		else if (isAliased(addressOf(_src) ) )
		{
			Ty value(_src);
			insertAt(_pos, _search, bx::move(value) );
		}
		else
		{
			grow(1);

			const uint32_t count = this->getCount();
			int32_t idx = 0;
			_pos = min(_pos, count);

			if (0 < count)
			{
				const Ty* ptr = m_storage.getPtr(_pos);
				idx = _search
					? upperBound(_src, ptr, count - _pos, CmpFnT)
					: 0
					;
			}

			if constexpr (isRelocatable() )
			{
				Ty* room = m_storage.carve(this->m_storageView.m_ptr, count, idx + _pos, 1);
				BX_PLACEMENT_NEW(room, Ty)(_src);
				this->m_storageView.m_term = this->m_storageView.m_ptr + count + 1;
			}
			else
			{
				this->m_storageView.add(m_storage, idx + _pos, &_src, 1);
			}

			BX_ASSERT(isSorted(*this), "Not sorted!");
		}
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::add(Ty&& _src)
	{
		insertAt(0, true, bx::move(_src) );
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::add(uint32_t _pos, Ty&& _src)
	{
		insertAt(_pos, true, bx::move(_src) );
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::add(UncheckedTag, uint32_t _pos, Ty&& _src)
	{
		BX_ASSERT(isSortedAt(_pos, _src), "Position %u is not the sorted position of the element!", _pos);
		insertAt(_pos, false, bx::move(_src) );
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::insertAt(uint32_t _pos, bool _search, Ty&& _src)
	{
		if constexpr (isTypeless() )
		{
			addTypeless(_pos, _search, _src);
		}
		else
		{
			grow(1);

			const uint32_t count = this->getCount();
			int32_t idx = 0;
			_pos = min(_pos, count);

			if (0 < count)
			{
				const Ty* ptr = m_storage.getPtr(_pos);
				idx = _search
					? upperBound(_src, ptr, count - _pos, CmpFnT)
					: 0
					;
			}

			if constexpr (isRelocatable() )
			{
				if (isAliased(addressOf(_src) ) )
				{
					Ty value(bx::move(_src) );
					Ty* room = m_storage.carve(this->m_storageView.m_ptr, count, idx + _pos, 1);
					BX_PLACEMENT_NEW(room, Ty)(bx::move(value) );
				}
				else
				{
					Ty* room = m_storage.carve(this->m_storageView.m_ptr, count, idx + _pos, 1);
					BX_PLACEMENT_NEW(room, Ty)(bx::move(_src) );
				}

				this->m_storageView.m_term = this->m_storageView.m_ptr + count + 1;
			}
			else
			{
				this->m_storageView.add(m_storage, idx + _pos, Emplace, bx::move(_src) );
			}

			BX_ASSERT(isSorted(*this), "Not sorted!");
		}
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	template<size_t SizeT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::add(const Ty (&_src)[SizeT])
	{
		add(ArrayViewT<Ty>(_src) );
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::add(const ArrayViewBaseT<Ty>& _src)
	{
		if (0 != _src.getCount()
		&&  isAliased(_src.getPtr() ) )
		{
			ArrayT<Ty> src;
			src.add(_src);
			add(src);
			return;
		}

		for (auto&& value : _src)
		{
			add(value);
		}
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::remove(uint32_t _pos, uint32_t _count)
	{
		if constexpr (isRelocatable() && !isTriviallyCopyable<Ty>() )
		{
			const uint32_t oldCount = this->getCount();
			uint32_t pos   = _pos;
			uint32_t count = _count;
			sanitize(oldCount, pos, count);

			if (0 == count)
			{
				return;
			}

			Ty* ptr = const_cast<Ty*>(this->m_storageView.m_ptr);
			StorageTraits::destruct(ptr + pos, count);
			memMove(ptr + pos, ptr + pos + count, (oldCount - pos - count)*sizeof(Ty) );

			this->m_storageView.m_term = ptr + oldCount - count;
		}
		else
		{
			this->m_storageView.remove(m_storage, _pos, kOrdered, _count);
		}
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	template<typename Uy>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::remove(const Uy* _element)
	{
		static_assert(isSame<Ty, Uy>(), "Element pointer must point to an element of this array type.");
		remove(this->getIndex(_element), 1);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline void SortedArrayBaseT<Ty, StorageT, CmpFnT>::remove(Element _element, uint32_t _count)
	{
		const uint32_t oldCount = this->getCount();
		_count = min(oldCount, _count);

		const uint32_t idx = _element == kLast ? oldCount - _count : 0;
		remove(idx, _count);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline Ty SortedArrayBaseT<Ty, StorageT, CmpFnT>::pop(uint32_t _idx)
	{
		BX_ASSERT(0 < this->getCount(), "Cannot pop from empty array.");

		const uint32_t idx = min(_idx, this->getCount()-1);
		Ty result = bx::move(*m_storage.getPtr(idx) );
		remove(idx);

		return bx::move(result);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline Ty SortedArrayBaseT<Ty, StorageT, CmpFnT>::pop(Element _element)
	{
		const uint32_t idx = kLast == _element ? this->getCount()-1 : 0;
		return bx::move(pop(idx) );
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline bool isSorted(const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src)
	{
		return isSorted(_src.getPtr(), _src.getCount(), CmpFnT);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline uint32_t lowerBound(const Ty& _key, const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src)
	{
		return lowerBound(_key, _src.getPtr(), _src.getCount(), CmpFnT);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline uint32_t upperBound(const Ty& _key, const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src)
	{
		return upperBound(_key, _src.getPtr(), _src.getCount(), CmpFnT);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline int32_t binarySearch(const Ty& _key, const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src)
	{
		return binarySearch(_key, _src.getPtr(), _src.getCount(), CmpFnT);
	}

	//---

	template<typename Ty, ComparisonFn CmpFnT>
	inline SortedArrayT<Ty, CmpFnT>::SortedArrayT()
		: SortedArrayBaseT<Ty, DynamicStorageT<Ty>, CmpFnT>()
	{
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline SortedArrayT<Ty, CmpFnT>::SortedArrayT(AllocatorHandle _allocator)
		: SortedArrayBaseT<Ty, DynamicStorageT<Ty>, CmpFnT>()
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");
		this->m_storage.setAllocatorHandle(_allocator);
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline SortedArrayT<Ty, CmpFnT>::SortedArrayT(AllocatorHandle _allocator, uint32_t _capacity)
		: SortedArrayT(_allocator)
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");

		if (0 != _capacity)
		{
			this->setCapacity(_capacity);
		}
	}

	template<typename Ty, ComparisonFn CmpFnT>
	template<typename... ArgsT>
	inline SortedArrayT<Ty, CmpFnT>::SortedArrayT(AllocatorHandle _allocator, uint32_t _count, EmplaceTag, ArgsT&&... _args)
		: SortedArrayT(_allocator, _count)
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");

		for (uint32_t ii = 0; ii < _count; ++ii)
		{
			this->add(Unchecked, this->getCount(), Ty(_args...) );
		}
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline SortedArrayT<Ty, CmpFnT>::SortedArrayT(const SortedArrayT<Ty, CmpFnT>& _src)
		: SortedArrayBaseT<Ty, DynamicStorageT<Ty>, CmpFnT>()
	{
		this->m_storage.setAllocatorHandle(_src.m_storage.getAllocatorHandle() );
		this->setCapacity(_src.getCount() );
		this->m_storageView.add(this->m_storage, 0, _src.getPtr(), _src.getCount() );
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline SortedArrayT<Ty, CmpFnT>::SortedArrayT(SortedArrayT<Ty, CmpFnT>&& _src)
		: SortedArrayBaseT<Ty, DynamicStorageT<Ty>, CmpFnT>()
	{
		set(bx::move(_src) );
	}

	template<typename Ty, ComparisonFn CmpFnT>
	template<size_t SizeT>
	inline SortedArrayT<Ty, CmpFnT>::SortedArrayT(AllocatorHandle _allocator, const Ty (&_src)[SizeT])
		: SortedArrayT<Ty, CmpFnT>(_allocator)
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");
		set(_src);
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline SortedArrayT<Ty, CmpFnT>::SortedArrayT(AllocatorHandle _allocator, const ArrayViewBaseT<Ty>& _src)
		: SortedArrayT(_allocator)
	{
		BX_ASSERT(getAllocator(_allocator)->isCpuAccessible(), "Allocator must be CPU accessible!");
		set(_src);
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline AllocatorHandle SortedArrayT<Ty, CmpFnT>::getAllocatorHandle() const
	{
		return this->m_storage.getAllocatorHandle();
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline SortedArrayT<Ty, CmpFnT>& SortedArrayT<Ty, CmpFnT>::operator=(const ArrayViewBaseT<Ty>& _src)
	{
		if (this != &_src)
		{
			this->set(_src);
		}

		return *this;
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline SortedArrayT<Ty, CmpFnT>& SortedArrayT<Ty, CmpFnT>::operator=(const SortedArrayT<Ty, CmpFnT>& _src)
	{
		if (this != &_src)
		{
			if (0 == this->getCapacity()
			&&  kAllocatorHandleDefault == this->m_storage.getAllocatorHandle().idx)
			{
				this->m_storage.setAllocatorHandle(_src.m_storage.getAllocatorHandle() );
			}

			this->set(_src);
		}

		return *this;
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline SortedArrayT<Ty, CmpFnT>& SortedArrayT<Ty, CmpFnT>::operator=(SortedArrayT<Ty, CmpFnT>&& _src)
	{
		set(bx::move(_src) );

		return *this;
	}

	template<typename Ty, ComparisonFn CmpFnT>
	template<size_t SizeT>
	inline void SortedArrayT<Ty, CmpFnT>::set(const Ty (&_src)[SizeT])
	{
		set(ArrayViewT<Ty>(_src) );
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline void SortedArrayT<Ty, CmpFnT>::set(const ArrayViewBaseT<Ty>& _src)
	{
		SortedArrayBaseT<Ty, DynamicStorageT<Ty>, CmpFnT>::set(_src);
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline void SortedArrayT<Ty, CmpFnT>::set(SortedArrayT<Ty, CmpFnT>&& _src)
	{
		if (this != &_src)
		{
			this->clear();
			this->trim();

			this->m_storage = bx::move(_src.m_storage);
			this->m_storageView = bx::move(_src.m_storageView);
		}
	}

	template<typename Ty, ComparisonFn CmpFnT>
	template<typename... ArgsT>
	inline void SortedArrayT<Ty, CmpFnT>::set(EmplaceTag, ArgsT&&... _args)
	{
		this->clear();
		this->grow(1);
		this->m_storageView.add(this->m_storage, 0, Emplace, bx::forward<ArgsT>(_args)...);
	}

	//---

	namespace internal
	{
		template<typename Ty>
		inline int32_t writeArray(WriterI* _writer, const ArrayViewBaseT<Ty>& _array, Error* _err)
		{
			static_assert(isTriviallyCopyable<Ty>(), "Element type must be trivially copyable.");
			return write(_writer, _array.getPtr(), int32_t(_array.getCount()*sizeof(Ty) ), _err);
		}

	} // namespace internal

	template<typename Ty>
	inline int32_t write(WriterI* _writer, const ArrayViewT<Ty>& _array, Error* _err)
	{
		return internal::writeArray(_writer, _array, _err);
	}

	template<typename Ty>
	inline int32_t write(WriterI* _writer, const MutableArrayViewT<Ty>& _array, Error* _err)
	{
		return internal::writeArray(_writer, _array, _err);
	}

	template<typename Ty, typename StorageT>
	inline int32_t write(WriterI* _writer, const ArrayBaseT<Ty, StorageT>& _array, Error* _err)
	{
		return internal::writeArray(_writer, _array, _err);
	}

	template<typename Ty, uint32_t CapacityT>
	inline int32_t write(WriterI* _writer, const StaticArrayT<Ty, CapacityT>& _array, Error* _err)
	{
		return internal::writeArray(_writer, _array, _err);
	}

	template<typename Ty>
	inline int32_t write(WriterI* _writer, const ArrayT<Ty>& _array, Error* _err)
	{
		return internal::writeArray(_writer, _array, _err);
	}

	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	inline int32_t write(WriterI* _writer, const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _array, Error* _err)
	{
		return internal::writeArray(_writer, _array, _err);
	}

	template<typename Ty, ComparisonFn CmpFnT>
	inline int32_t write(WriterI* _writer, const SortedArrayT<Ty, CmpFnT>& _array, Error* _err)
	{
		return internal::writeArray(_writer, _array, _err);
	}

} // namespace bx
