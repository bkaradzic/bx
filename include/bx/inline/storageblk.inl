/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_STORAGEBLK_H_HEADER_GUARD
#	error "Must be included from bx/storageblk.h!"
#endif // BX_STORAGEBLK_H_HEADER_GUARD

namespace bx
{
	namespace internal
	{
		void* storageSetCapacity(Blk& _blk, uint32_t _capacity, uint32_t _count, uint32_t _elementSize);

		const void* storageGrow(Blk& _blk, void*& _capacityTerm, const void*& _ptr, uint32_t _count, uint32_t _num, uint32_t _elementSize);

		void* storageCarve(Blk& _blk, void*& _capacityTerm, const void*& _ptr, uint32_t _count, uint32_t _pos, uint32_t _num, uint32_t _elementSize);

		const void* storageInsert(Blk& _blk, void*& _capacityTerm, const void*& _ptr, uint32_t _count, uint32_t _pos, const void* _src, uint32_t _num, uint32_t _elementSize);

	} // namespace internal

	template<typename Ty>
	inline DynamicStorageT<Ty>::DynamicStorageT()
		: m_blk()
		, m_term(NULL)
	{
	}

	template<typename Ty>
	inline void DynamicStorageT<Ty>::setAllocatorHandle(AllocatorHandle _handle)
	{
		BX_ASSERT(!isValid(m_blk), "Allocator can only change while storage holds no block!");
		m_blk.allocatorHandle = _handle.idx;
	}

	template<typename Ty>
	inline AllocatorHandle DynamicStorageT<Ty>::getAllocatorHandle() const
	{
		return bx::getAllocatorHandle(m_blk);
	}

	template<typename Ty>
	inline DynamicStorageT<Ty>::~DynamicStorageT()
	{
		m_blk = free(Unchecked, bx::move(m_blk) );
	}

	template<typename Ty>
	inline uint32_t DynamicStorageT<Ty>::getCapacity() const
	{
		return uint32_t(m_blk.size/sizeof(Ty) );
	}

	template<typename Ty>
	inline void DynamicStorageT<Ty>::setCapacity(uint32_t _capacity, uint32_t _count)
	{
		if constexpr (isTriviallyMoveConstructible<Ty>() || isTriviallyRelocatable<Ty>())
		{
			m_term = (Ty*)internal::storageSetCapacity(m_blk, _capacity, _count, sizeof(Ty) );
		}
		else
		{
			setCapacityNonTrivial(_capacity, _count);
		}
	}

	template<typename Ty>
	inline void DynamicStorageT<Ty>::setCapacityNonTrivial(uint32_t _capacity, uint32_t _count)
	{
		const uint32_t minCapacity = max(_capacity, _count);
		const size_t   sizeInBytes = minCapacity == 0 ? 0 : sizeof(Ty)*size_t(alignUp(minCapacity, 16) );

		if (0 == sizeInBytes)
		{
			const uint8_t handle = uint8_t(m_blk.allocatorHandle);
			m_blk = free(Unchecked, bx::move(m_blk) );
			m_blk.allocatorHandle = handle;
		}
		else
		{
			Blk blk;
			blk.allocatorHandle = m_blk.allocatorHandle;
			blk = realloc(Unchecked, bx::move(blk), sizeInBytes);

			if (isValid(blk)
			&&  0 < _count)
			{
				Ty* dst = (Ty*)blk;
				Ty* src = (Ty*)m_blk;

				if constexpr (isMoveConstructible<Ty>() )
				{
					for (uint32_t ii = 0; ii < _count; ++ii)
					{
						BX_PLACEMENT_NEW(dst + ii, Ty)(bx::move(src[ii]) );
					}
				}
				else
				{
					StorageTraits::constructMove(dst, src, _count);
				}

				StorageTraits::destruct(src, _count);
			}

			(void)free(Unchecked, bx::move(m_blk) );
			m_blk = bx::move(blk);
		}

		updateTerm();
	}

	template<typename Ty>
	inline void DynamicStorageT<Ty>::updateTerm()
	{
		m_term = isValid(m_blk) ? (Ty*)m_blk + getCapacity() : NULL;
	}

	template<typename Ty>
	inline Ty* DynamicStorageT<Ty>::getPtr(uint32_t _idx)
	{
		if (!isValid(m_blk) )
		{
			return NULL;
		}

		return (Ty*)m_blk + _idx;
	}

	template<typename Ty>
	inline const Ty* DynamicStorageT<Ty>::getPtr(uint32_t _idx) const
	{
		return const_cast<DynamicStorageT<Ty>*>(this)->getPtr(_idx);
	}

	template<typename Ty>
	inline Ty* DynamicStorageT<Ty>::getTerm()
	{
		return m_term;
	}

	template<typename Ty>
	inline const Ty* DynamicStorageT<Ty>::getTerm() const
	{
		return m_term;
	}

	template<typename Ty>
	inline DynamicStorageT<Ty>& DynamicStorageT<Ty>::operator=(DynamicStorageT<Ty>&& _other)
	{
		if (this != &_other)
		{
			const uint8_t otherHandle = uint8_t(_other.m_blk.allocatorHandle);

			m_blk = free(Unchecked, bx::move(m_blk) );
			m_blk = bx::move(_other.m_blk);

			_other.m_blk.allocatorHandle = otherHandle;

			m_term        = _other.m_term;
			_other.m_term = NULL;
		}

		return *this;
	}

	template<typename Ty>
	inline const Ty* DynamicStorageT<Ty>::grow(const Ty*& _ptr, uint32_t _count, uint32_t _num)
	{
		static_assert(isTriviallyRelocatable<Ty>(), "Element type must be trivially relocatable.");

		return (const Ty*)internal::storageGrow(
			  m_blk
			, reinterpret_cast<void*&>(m_term)
			, reinterpret_cast<const void*&>(_ptr)
			, _count
			, _num
			, sizeof(Ty)
			);
	}

	template<typename Ty>
	inline Ty* DynamicStorageT<Ty>::carve(const Ty*& _ptr, uint32_t _count, uint32_t _pos, uint32_t _num)
	{
		static_assert(isTriviallyRelocatable<Ty>(), "Element type must be trivially relocatable.");

		return (Ty*)internal::storageCarve(
			  m_blk
			, reinterpret_cast<void*&>(m_term)
			, reinterpret_cast<const void*&>(_ptr)
			, _count
			, _pos
			, _num
			, sizeof(Ty)
			);
	}

	template<typename Ty>
	inline const Ty* DynamicStorageT<Ty>::insert(const Ty*& _ptr, uint32_t _count, uint32_t _pos, const Ty* _src, uint32_t _num)
	{
		static_assert(isTriviallyCopyable<Ty>(), "Element type must be trivially copyable.");

		return (const Ty*)internal::storageInsert(
			  m_blk
			, reinterpret_cast<void*&>(m_term)
			, reinterpret_cast<const void*&>(_ptr)
			, _count
			, _pos
			, _src
			, _num
			, sizeof(Ty)
			);
	}

} // namespace bx
