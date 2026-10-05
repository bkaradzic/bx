/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_BITARRAY_H_HEADER_GUARD
#	error "Must be included from bx/bitarray.h!"
#endif // BX_BITARRAY_H_HEADER_GUARD

namespace bx
{
	inline BitArrayViewBase::BitArrayViewBase()
		: m_ptr(NULL)
		, m_sizeInBytes(0)
		, m_count(0)
	{
	}

	inline BitArrayViewBase::BitArrayViewBase(void* _ptr, uint32_t _sizeInBytes, uint32_t _count)
	{
		BX_ASSERT(0 == (_sizeInBytes & 7)
			, "_sizeInBytes argument must be in multiples of 8-bytes (_sizeInBytes: %d)."
			, _sizeInBytes
			);

		m_ptr         = (uint64_t*)_ptr;
		m_sizeInBytes = _sizeInBytes;
		m_count = min<uint32_t>(_count, getCapacity() );
	}

	inline bool BitArrayViewBase::get(uint32_t _idx) const
	{
		return 0 != (m_ptr[_idx/64] & (UINT64_C(1) << (_idx & 63) ) );
	}

	inline const void* BitArrayViewBase::getPtr() const
	{
		return m_ptr;
	}

	inline const void* BitArrayViewBase::getTerm() const
	{
		return m_ptr + m_count / 64;
	}

	inline uint32_t BitArrayViewBase::getCount() const
	{
		return m_count;
	}

	inline uint32_t BitArrayViewBase::getCapacity() const
	{
		return m_sizeInBytes * 8;
	}

	inline uint32_t BitArrayViewBase::find(uint32_t _numBits) const
	{
		return find(0, m_count, _numBits);
	}

	inline uint32_t BitArrayViewBase::find(uint32_t _beginBit, uint32_t _endBit, uint32_t _numBits) const
	{
		BX_ASSERT(true
			&& _beginBit <= m_count
			&& _endBit   <= m_count
			&& _beginBit <= _endBit
			&& _endBit-_beginBit  >= _numBits
			&& _beginBit+_numBits <= m_count
			&& 0 < _numBits
			, "Incorrect usage `%s`.\n"
			  "_beginBit %d, _endBit %d, _numBits %d, m_count %d [%d%d%d%d%d%d]"
			, BX_FUNCTION
			, _beginBit
			, _endBit
			, _numBits
			, m_count
			, _beginBit <= m_count
			, _endBit   <= m_count
			, _beginBit <= _endBit
			, _endBit-_beginBit  >= _numBits
			, _beginBit+_numBits <= m_count
			, 0 < _numBits
			);
		return findRange(_beginBit, _endBit, _numBits);
	}

	inline uint32_t BitArrayViewBase::findSet(uint32_t _beginBit, uint32_t _endBit) const
	{
		BX_ASSERT(_beginBit <= _endBit && _endBit <= m_count
			, "Invalid range [%d, %d), count %d."
			, _beginBit
			, _endBit
			, m_count
			);

		uint32_t idx = _beginBit;

		while (idx < _endBit)
		{
			const uint32_t word = idx/64;
			const uint64_t bits = m_ptr[word] & (~UINT64_C(0) << (idx & 63) );

			if (0 != bits)
			{
				const uint32_t found = word*64 + findFirstSet(bits) - 1;

				return found < _endBit
					? found
					: kInvalid
					;
			}

			idx = (word + 1)*64;
		}

		return kInvalid;
	}

	inline uint32_t BitArrayViewBase::findClear(uint32_t _beginBit, uint32_t _endBit) const
	{
		BX_ASSERT(_beginBit <= _endBit && _endBit <= m_count
			, "Invalid range [%d, %d), count %d."
			, _beginBit
			, _endBit
			, m_count
			);

		uint32_t idx = _beginBit;

		while (idx < _endBit)
		{
			const uint32_t word = idx/64;
			const uint64_t bits = ~m_ptr[word] & (~UINT64_C(0) << (idx & 63) );

			if (0 != bits)
			{
				const uint32_t found = word*64 + findFirstSet(bits) - 1;

				return found < _endBit
					? found
					: kInvalid
					;
			}

			idx = (word + 1)*64;
		}

		return kInvalid;
	}

	inline uint32_t BitArrayViewBase::countBits() const
	{
		return countBits(0, m_count);
	}

	inline bool BitArrayViewBase::testAny() const
	{
		return testAny(0, m_count);
	}

	inline bool BitArrayViewBase::testAll() const
	{
		return testAll(0, m_count);
	}

	inline bool BitArrayViewBase::testNone() const
	{
		return testNone(0, m_count);
	}

	inline MutableBitArrayView::MutableBitArrayView()
	{
		set(nullptr, 0);
	}

	inline MutableBitArrayView::MutableBitArrayView(void* _ptr, uint32_t _sizeInBytes, uint32_t _count)
		: BitArrayViewBase(_ptr, _sizeInBytes, _count)
	{
	}

	inline void MutableBitArrayView::set(void* _src, uint32_t _count)
	{
		this->m_ptr   = (uint64_t*)_src;
		this->m_count = _count;
	}

	inline void MutableBitArrayView::set(uint32_t _idx, bool _value)
	{
		BX_ASSERT(_idx <= m_count, "Out of bounds (idx %d, max %d)", _idx, m_count);

		const uint64_t mask = UINT64_C(1) << (_idx & 63);

		if (_value)
		{
			m_ptr[_idx/64] |= mask;
		}
		else
		{
			m_ptr[_idx/64] &= ~mask;
		}
	}

	inline void MutableBitArrayView::clear(bool _all)
	{
		const uint32_t sizeInBytes = _all
			? m_sizeInBytes
			: alignUp(m_count, 8) / 8
			;
		memSet(m_ptr, 0, sizeInBytes);
	}

	inline void* MutableBitArrayView::getPtr()
	{
		return this->m_ptr;
	}

	inline const void* MutableBitArrayView::getPtr() const
	{
		return BitArrayViewBase::getPtr();
	}

	inline void* MutableBitArrayView::getTerm()
	{
		return this->m_ptr + m_count / 64;
	}

	inline const void* MutableBitArrayView::getTerm() const
	{
		return BitArrayViewBase::getTerm();
	}

	template<typename StorageT>
	inline BitArrayBaseT<StorageT>::BitArrayBaseT()
	{
		this->m_ptr         = m_storage.getPtr();
		this->m_sizeInBytes = m_storage.getCapacity() * sizeof(uint64_t);
		this->m_count       = 0;
	}

	template<typename StorageT>
	BX_NO_INLINE inline BitArrayBaseT<StorageT>::BitArrayBaseT(const BitArrayViewBase& _src)
		: MutableBitArrayView::MutableBitArrayView()
	{
		this->m_ptr         = m_storage.getPtr();
		this->m_sizeInBytes = m_storage.getCapacity() * sizeof(uint64_t);
		this->m_count       = 0;

		set(_src);
	}

	template<typename StorageT>
	inline void BitArrayBaseT<StorageT>::set(const BitArrayViewBase& _src)
	{
		if (this != &_src)
		{
			this->setCount(_src.getCount() );

			const uint32_t sizeInBytes = min(this->getCapacity(), _src.getCapacity() ) / 8;
			bx::memCopy(this->m_ptr, _src.getPtr(), sizeInBytes);
		}
	}

	template<typename StorageT>
	inline void BitArrayBaseT<StorageT>::set(uint32_t _idx, bool _value)
	{
		MutableBitArrayView::set(_idx, _value);
	}

	template<typename StorageT>
	inline void BitArrayBaseT<StorageT>::set(uint32_t _beginBit, uint32_t _endBit, bool _value)
	{
		MutableBitArrayView::set(_beginBit, _endBit, _value);
	}

	template<typename StorageT>
	inline void BitArrayBaseT<StorageT>::set(uint32_t _beginBit, uint32_t _endBit, uint64_t _value)
	{
		MutableBitArrayView::set(_beginBit, _endBit, _value);
	}

	template<typename StorageT>
	inline BitArrayBaseT<StorageT>::~BitArrayBaseT()
	{
		this->clear();
	}

	template<typename StorageT>
	template<typename StorageU>
	inline BitArrayBaseT<StorageT>& BitArrayBaseT<StorageT>::operator=(const BitArrayBaseT<StorageU>& _rhs)
	{
		if (this != &_rhs)
		{
			clear();
		}

		return *this;
	}

	template<typename StorageT>
	inline uint32_t BitArrayBaseT<StorageT>::getCapacity() const
	{
		return m_storage.getCapacity() * 64;
	}

	template<typename StorageT>
	inline void BitArrayBaseT<StorageT>::setCapacity(uint32_t _capacityInBits)
	{
		const uint32_t capacity = this->getCapacity();
		m_storage.setCapacity(
			  alignUp(max(capacity, _capacityInBits), 64)/64
			, capacity/64
			);
		this->m_ptr         = m_storage.getPtr();
		this->m_sizeInBytes = m_storage.getCapacity() * sizeof(uint64_t);
	}

	template<typename StorageT>
	inline void BitArrayBaseT<StorageT>::setCount(uint32_t _countInBits)
	{
		if (_countInBits > getCapacity() )
		{
			setCapacity(_countInBits);
		}

		const uint32_t oldCountInBits = this->m_count;
		this->m_count = _countInBits;

		if (oldCountInBits < _countInBits)
		{
			set(oldCountInBits, _countInBits, false);
		}
	}

	inline BitArray::BitArray(const BitArray& _other)
		: BitArray(_other.m_storage.getAllocator(), _other)
	{
	}

	inline BitArray::BitArray(AllocatorI* _allocator)
		: BitArrayBaseT<DynamicStorageT<uint64_t>>()
	{
		this->m_storage.setAllocator(_allocator);
	}

	inline BitArray::BitArray(AllocatorI* _allocator, const BitArrayViewBase& _src)
		: BitArray(_allocator)
	{
		set(_src);
	}

	inline AllocatorI* BitArray::getAllocator() const
	{
		return this->m_storage.getAllocator();
	}

	inline BitArray& BitArray::operator=(const BitArray& _other)
	{
		if (this != &_other)
		{
			release();

			this->m_storage.setAllocator(_other.m_storage.getAllocator() );
			this->set(_other);
		}

		return *this;
	}

	inline BitArray& BitArray::operator=(BitArray&& _other)
	{
		set(move(_other) );

		return *this;
	}

	inline void BitArray::set(const BitArrayViewBase& _src)
	{
		BitArrayBaseT<DynamicStorageT<uint64_t>>::set(_src);
	}

	inline void BitArray::set(BitArray&& _other)
	{
		if (this != &_other)
		{
			release();

			this->m_storage = move(_other.m_storage);

			this->m_ptr         = _other.m_ptr;
			this->m_sizeInBytes = _other.m_sizeInBytes;
			this->m_count       = _other.m_count;

			_other.m_ptr         = NULL;
			_other.m_sizeInBytes = 0;
			_other.m_count       = 0;
		}
	}

	inline void BitArray::release()
	{
		if (NULL != this->m_storage.getAllocator() )
		{
			this->clear();
			this->m_storage.setCapacity(0, 0);
		}

		this->m_ptr         = this->m_storage.getPtr();
		this->m_sizeInBytes = 0;
		this->m_count       = 0;
	}

	inline void BitArray::set(uint32_t _idx, bool _value)
	{
		BitArrayBaseT<DynamicStorageT<uint64_t>>::set(_idx, _value);
	}

	inline void BitArray::set(uint32_t _beginBit, uint32_t _endBit, bool _value)
	{
		BitArrayBaseT<DynamicStorageT<uint64_t>>::set(_beginBit, _endBit, _value);
	}

	inline void BitArray::set(uint32_t _beginBit, uint32_t _endBit, uint64_t _value)
	{
		BitArrayBaseT<DynamicStorageT<uint64_t>>::set(_beginBit, _endBit, _value);
	}

	inline MippedBitArrayBase::MippedBitArrayBase()
		: BitArrayViewBase()
		, m_mip(NULL)
	{
	}

	inline void MippedBitArrayBase::set(uint32_t _idx, bool _value)
	{
		BX_ASSERT(_idx <= m_count, "Out of bounds (idx %d, max %d)", _idx, m_count);

		const uint32_t word = _idx/64;
		const uint64_t mask = UINT64_C(1) << (_idx & 63);

		if (_value)
		{
			m_ptr[word] |= mask;
		}
		else
		{
			m_ptr[word] &= ~mask;
		}

		const uint64_t mipMask = UINT64_C(1) << (word & 63);

		if (~UINT64_C(0) == m_ptr[word])
		{
			m_mip[word/64] |= mipMask;
		}
		else
		{
			m_mip[word/64] &= ~mipMask;
		}
	}

	inline void MippedBitArrayBase::set(uint32_t _beginBit, uint32_t _endBit, bool _value)
	{
		if (_beginBit < _endBit)
		{
			MutableBitArrayView(m_ptr, m_sizeInBytes, m_count).set(_beginBit, _endBit, _value);
			updateMip(_beginBit/64, alignUp(_endBit, 64)/64);
		}
	}

	inline void MippedBitArrayBase::set(uint32_t _beginBit, uint32_t _endBit, uint64_t _value)
	{
		if (_beginBit < _endBit)
		{
			MutableBitArrayView(m_ptr, m_sizeInBytes, m_count).set(_beginBit, _endBit, _value);
			updateMip(_beginBit/64, alignUp(_endBit, 64)/64);
		}
	}

	inline void MippedBitArrayBase::blit(uint32_t _dstBit, uint32_t _srcBit, uint32_t _numBits)
	{
		if (0 < _numBits)
		{
			MutableBitArrayView(m_ptr, m_sizeInBytes, m_count).blit(_dstBit, _srcBit, _numBits);
			updateMip(_dstBit/64, alignUp(_dstBit+_numBits, 64)/64);
		}
	}

	inline void MippedBitArrayBase::clear()
	{
		if (NULL != m_ptr)
		{
			MutableBitArrayView(m_ptr, m_sizeInBytes, m_count).clear();
			updateMip(0, m_sizeInBytes/8);
		}
	}

	inline uint32_t MippedBitArrayBase::findClear(uint32_t _beginBit, uint32_t _endBit) const
	{
		BX_ASSERT(_beginBit <= _endBit && _endBit <= m_count
			, "Invalid range [%d, %d), count %d."
			, _beginBit
			, _endBit
			, m_count
			);

		const uint32_t numWords    = m_sizeInBytes/8;
		const uint32_t numMipWords = getNumMipWords(numWords);
		const uint32_t beginWord   = _beginBit/64;

		for (uint32_t word = beginWord; word < numWords;)
		{
			uint32_t mipWord = word/64;
			uint64_t avail   = ~m_mip[mipWord] & (~UINT64_C(0) << (word & 63) );

			while (0 == avail)
			{
				if (++mipWord >= numMipWords)
				{
					return kInvalid;
				}

				avail = ~m_mip[mipWord];
			}

			word = mipWord*64 + findFirstSet(avail) - 1;

			if (word >= numWords)
			{
				return kInvalid;
			}

			const uint64_t mask = word == beginWord
				? ~UINT64_C(0) << (_beginBit & 63)
				: ~UINT64_C(0)
				;
			const uint64_t bits = ~m_ptr[word] & mask;

			if (0 != bits)
			{
				const uint32_t found = word*64 + findFirstSet(bits) - 1;

				return found < _endBit
					? found
					: kInvalid
					;
			}

			++word;
		}

		return kInvalid;
	}

	inline const uint64_t* MippedBitArrayBase::getMip() const
	{
		return m_mip;
	}

	inline void MippedBitArrayBase::rebind(uint64_t* _ptr, uint32_t _numWords)
	{
		m_ptr         = _ptr;
		m_sizeInBytes = _numWords * sizeof(uint64_t);
		m_mip         = NULL;

		if (NULL != _ptr)
		{
			m_mip = _ptr + _numWords;
			memSet(m_mip, 0, getNumMipWords(_numWords) * sizeof(uint64_t) );
			setMipTail();
		}
	}

	inline void MippedBitArrayBase::moveMip(uint64_t* _ptr, uint32_t _oldNumWords, uint32_t _numWords)
	{
		if (NULL == _ptr
		||  0 == _oldNumWords)
		{
			rebind(_ptr, _numWords);
			return;
		}

		const uint32_t oldNumMipWords = getNumMipWords(_oldNumWords);
		const uint32_t numMipWords    = getNumMipWords(_numWords);
		uint64_t* mip = _ptr + _numWords;

		memMove(mip, _ptr + _oldNumWords, min(oldNumMipWords, numMipWords) * sizeof(uint64_t) );

		if (numMipWords > oldNumMipWords)
		{
			memSet(mip + oldNumMipWords, 0, (numMipWords - oldNumMipWords) * sizeof(uint64_t) );
		}

		if (0 != (_oldNumWords & 63) )
		{
			mip[_oldNumWords/64] &= (UINT64_C(1) << (_oldNumWords & 63) ) - 1;
		}

		m_ptr         = _ptr;
		m_sizeInBytes = _numWords * sizeof(uint64_t);
		m_mip         = mip;
		setMipTail();
	}

	inline void MippedBitArrayBase::updateMip(uint32_t _beginWord, uint32_t _endWord)
	{
		for (uint32_t ii = _beginWord; ii < _endWord; ++ii)
		{
			const uint64_t mipMask = UINT64_C(1) << (ii & 63);

			if (~UINT64_C(0) == m_ptr[ii])
			{
				m_mip[ii/64] |= mipMask;
			}
			else
			{
				m_mip[ii/64] &= ~mipMask;
			}
		}
	}

	inline void MippedBitArrayBase::setMipTail()
	{
		const uint32_t numWords = m_sizeInBytes/8;
		const uint32_t tail     = numWords & 63;

		if (0 != tail)
		{
			m_mip[numWords/64] |= ~UINT64_C(0) << tail;
		}
	}

	template<typename StorageT>
	inline MippedBitArrayT<StorageT>::MippedBitArrayT()
	{
		const uint32_t total = m_storage.getCapacity();

		m_numWords    = total - (total + 64)/65;
		this->m_count = 0;
		rebind(m_storage.getPtr(), m_numWords);
	}

	template<typename StorageT>
	inline uint32_t MippedBitArrayT<StorageT>::getCapacity() const
	{
		return m_numWords * 64;
	}

	template<typename StorageT>
	inline void MippedBitArrayT<StorageT>::setCapacity(uint32_t _capacityInBits)
	{
		const uint32_t numWords = alignUp(_capacityInBits, 64)/64;

		if (numWords == m_numWords)
		{
			return;
		}

		const uint32_t oldNumWords = m_numWords;
		const uint32_t total       = numWords + getNumMipWords(numWords);

		if (numWords > oldNumWords)
		{
			m_storage.setCapacity(total, total);
			m_numWords = numWords;
			moveMip(m_storage.getPtr(), oldNumWords, numWords);
		}
		else
		{
			moveMip(m_storage.getPtr(), oldNumWords, numWords);
			m_storage.setCapacity(total, total);
			m_numWords = numWords;
			this->m_ptr = m_storage.getPtr();
			this->m_mip = this->m_ptr + numWords;
		}
	}

	template<typename StorageT>
	inline void MippedBitArrayT<StorageT>::setCount(uint32_t _countInBits)
	{
		if (_countInBits > getCapacity() )
		{
			setCapacity(_countInBits);
		}

		const uint32_t oldCountInBits = this->m_count;
		this->m_count = _countInBits;

		if (oldCountInBits < _countInBits)
		{
			set(oldCountInBits, _countInBits, false);
		}
	}

	inline MippedBitArray::MippedBitArray(AllocatorI* _allocator)
		: MippedBitArrayT<DynamicStorageT<uint64_t>>()
	{
		this->m_storage.setAllocator(_allocator);
	}

	inline AllocatorI* MippedBitArray::getAllocator() const
	{
		return this->m_storage.getAllocator();
	}


} // namespace bx
