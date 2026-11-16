/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HANDLE_ALLOC_H_HEADER_GUARD
#	error "Must be included from bx/handlealloc.h!"
#endif // BX_HANDLE_ALLOC_H_HEADER_GUARD

#include <bx/simd_t.h>

namespace bx
{
	inline HandleAlloc::HandleAlloc(MippedBitArrayBase& _bits, uint16_t _maxHandles)
		: m_bits(_bits)
		, m_maxHandles(_maxHandles)
		, m_numHandles(0)
	{
		BX_ASSERT(_bits.getCount() >= _maxHandles, "Bit array holds %d bits, %d handles requested.", _bits.getCount(), _maxHandles);
		reset();
	}

	inline HandleAlloc::HandleAlloc(InitNoneTag, MippedBitArrayBase& _bits, uint16_t _maxHandles)
		: m_bits(_bits)
		, m_maxHandles(_maxHandles)
		, m_numHandles(0)
	{
	}

	inline uint16_t HandleAlloc::getNumHandles() const
	{
		return m_numHandles;
	}

	inline uint16_t HandleAlloc::getMaxHandles() const
	{
		return m_maxHandles;
	}

	inline uint16_t HandleAlloc::alloc()
	{
		const uint32_t idx = m_bits.findClear(0, m_maxHandles);

		if (kInvalid == idx)
		{
			return kInvalidHandle;
		}

		m_bits.set(idx, true);
		++m_numHandles;

		return uint16_t(idx);
	}

	inline bool HandleAlloc::isValid(uint16_t _handle) const
	{
		return true
			&& _handle < m_maxHandles
			&& m_bits.get(_handle)
			;
	}

	inline void HandleAlloc::free(uint16_t _handle)
	{
		if (isValid(_handle) )
		{
			m_bits.set(_handle, false);
			--m_numHandles;
		}
	}

	inline void HandleAlloc::reset()
	{
		m_bits.set(0, m_maxHandles, false);
		m_numHandles = 0;
	}

	inline uint16_t HandleAlloc::findFirst() const
	{
		return findFrom(0);
	}

	inline uint16_t HandleAlloc::findNext(uint16_t _handle) const
	{
		return findFrom(uint32_t(_handle) + 1);
	}

	inline const BitArrayViewBase& HandleAlloc::getBits() const
	{
		return m_bits;
	}

	inline uint16_t HandleAlloc::findFrom(uint32_t _start) const
	{
		const uint32_t idx = _start < m_maxHandles
			? m_bits.findSet(_start, m_maxHandles)
			: kInvalid
			;

		return kInvalid == idx
			? kInvalidHandle
			: uint16_t(idx)
			;
	}

	template<uint16_t MaxHandlesT>
	inline HandleAllocT<MaxHandlesT>::HandleAllocT()
		: HandleAlloc(InitNone, m_storage, MaxHandlesT)
	{
		m_storage.setCount(MaxHandlesT);
	}

	inline DynamicHandleAlloc::DynamicHandleAlloc(AllocatorHandle _allocator, uint16_t _maxHandles)
		: HandleAlloc(InitNone, m_storage, _maxHandles)
		, m_storage(_allocator)
	{
		m_storage.setCount(_maxHandles);
	}

	template <uint16_t MaxHandlesT>
	inline HandleListT<MaxHandlesT>::HandleListT()
	{
		reset();
	}

	template <uint16_t MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::pushBack(uint16_t _handle)
	{
		insertAfter(m_back, _handle);
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleListT<MaxHandlesT>::popBack()
	{
		uint16_t last = kInvalidHandle != m_back
			? m_back
			: m_front
			;

		if (kInvalidHandle != last)
		{
			remove(last);
		}

		return last;
	}

	template <uint16_t MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::pushFront(uint16_t _handle)
	{
		insertBefore(m_front, _handle);
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleListT<MaxHandlesT>::popFront()
	{
		uint16_t front = m_front;

		if (kInvalidHandle != front)
		{
			remove(front);
		}

		return front;
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleListT<MaxHandlesT>::getFront() const
	{
		return m_front;
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleListT<MaxHandlesT>::getBack() const
	{
		return m_back;
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleListT<MaxHandlesT>::getNext(uint16_t _handle) const
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		const Link& curr = m_links[_handle];
		return curr.m_next;
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleListT<MaxHandlesT>::getPrev(uint16_t _handle) const
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		const Link& curr = m_links[_handle];
		return curr.m_prev;
	}

	template <uint16_t MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::remove(uint16_t _handle)
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		Link& curr = m_links[_handle];

		if (kInvalidHandle != curr.m_prev)
		{
			Link& prev  = m_links[curr.m_prev];
			prev.m_next = curr.m_next;
		}
		else
		{
			m_front = curr.m_next;
		}

		if (kInvalidHandle != curr.m_next)
		{
			Link& next  = m_links[curr.m_next];
			next.m_prev = curr.m_prev;
		}
		else
		{
			m_back = curr.m_prev;
		}

		curr.m_prev = kInvalidHandle;
		curr.m_next = kInvalidHandle;
	}

	template <uint16_t MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::reset()
	{
		memSet(m_links, 0xff, sizeof(m_links) );
		m_front = kInvalidHandle;
		m_back  = kInvalidHandle;
	}

	template <uint16_t MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::insertBefore(uint16_t _before, uint16_t _handle)
	{
		Link& curr = m_links[_handle];
		curr.m_next = _before;

		if (kInvalidHandle != _before)
		{
			Link& link = m_links[_before];
			if (kInvalidHandle != link.m_prev)
			{
				Link& prev = m_links[link.m_prev];
				prev.m_next = _handle;
			}

			curr.m_prev = link.m_prev;
			link.m_prev = _handle;
		}

		updateFrontBack(_handle);
	}

	template <uint16_t MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::insertAfter(uint16_t _after, uint16_t _handle)
	{
		Link& curr = m_links[_handle];
		curr.m_prev = _after;

		if (kInvalidHandle != _after)
		{
			Link& link = m_links[_after];
			if (kInvalidHandle != link.m_next)
			{
				Link& next = m_links[link.m_next];
				next.m_prev = _handle;
			}

			curr.m_next = link.m_next;
			link.m_next = _handle;
		}

		updateFrontBack(_handle);
	}

	template <uint16_t MaxHandlesT>
	inline bool HandleListT<MaxHandlesT>::isValid(uint16_t _handle) const
	{
		return _handle < MaxHandlesT;
	}

	template <uint16_t MaxHandlesT>
	inline void HandleListT<MaxHandlesT>::updateFrontBack(uint16_t _handle)
	{
		Link& curr = m_links[_handle];

		if (kInvalidHandle == curr.m_prev)
		{
			m_front = _handle;
		}

		if (kInvalidHandle == curr.m_next)
		{
			m_back = _handle;
		}
	}

	template <uint16_t MaxHandlesT>
	inline HandleAllocLruT<MaxHandlesT>::HandleAllocLruT()
	{
		reset();
	}

	template <uint16_t MaxHandlesT>
	inline HandleAllocLruT<MaxHandlesT>::~HandleAllocLruT()
	{
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleAllocLruT<MaxHandlesT>::getNumHandles() const
	{
		return m_alloc.getNumHandles();
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleAllocLruT<MaxHandlesT>::getMaxHandles() const
	{
		return m_alloc.getMaxHandles();
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleAllocLruT<MaxHandlesT>::alloc()
	{
		uint16_t handle = m_alloc.alloc();
		if (kInvalidHandle != handle)
		{
			m_list.pushFront(handle);
		}
		return handle;
	}

	template <uint16_t MaxHandlesT>
	inline bool HandleAllocLruT<MaxHandlesT>::isValid(uint16_t _handle) const
	{
		return m_alloc.isValid(_handle);
	}

	template <uint16_t MaxHandlesT>
	inline void HandleAllocLruT<MaxHandlesT>::free(uint16_t _handle)
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		m_list.remove(_handle);
		m_alloc.free(_handle);
	}

	template <uint16_t MaxHandlesT>
	inline void HandleAllocLruT<MaxHandlesT>::touch(uint16_t _handle)
	{
		BX_ASSERT(isValid(_handle), "Invalid handle %d!", _handle);
		m_list.remove(_handle);
		m_list.pushFront(_handle);
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleAllocLruT<MaxHandlesT>::getFront() const
	{
		return m_list.getFront();
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleAllocLruT<MaxHandlesT>::getBack() const
	{
		return m_list.getBack();
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleAllocLruT<MaxHandlesT>::getNext(uint16_t _handle) const
	{
		return m_list.getNext(_handle);
	}

	template <uint16_t MaxHandlesT>
	inline uint16_t HandleAllocLruT<MaxHandlesT>::getPrev(uint16_t _handle) const
	{
		return m_list.getPrev(_handle);
	}

	template <uint16_t MaxHandlesT>
	inline void HandleAllocLruT<MaxHandlesT>::reset()
	{
		m_list.reset();
		m_alloc.reset();
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline HandleHashMapT<MaxCapacityT, KeyT>::HandleHashMapT()
		: m_maxCapacity(MaxCapacityT)
	{
		reset();
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline HandleHashMapT<MaxCapacityT, KeyT>::~HandleHashMapT()
	{
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::insert(KeyT _key, uint16_t _handle)
	{
		if (kInvalidHandle == _handle)
		{
			return false;
		}

		const KeyT hash = mix(_key);
		const uint32_t firstIdx = hash % MaxCapacityT;
		uint32_t idx = firstIdx;
		do
		{
			if (m_handle[idx] == kInvalidHandle)
			{
				m_key[idx]    = _key;
				m_handle[idx] = _handle;
				++m_numElements;
				return true;
			}

			if (m_key[idx] == _key)
			{
				return false;
			}

			idx = (idx + 1) % MaxCapacityT;

		} while (idx != firstIdx);

		return false;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::removeByKey(KeyT _key)
	{
		uint32_t idx = findIndex(_key);
		if (UINT32_MAX != idx)
		{
			removeIndex(idx);
			return true;
		}

		return false;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::removeByHandle(uint16_t _handle)
	{
		if (kInvalidHandle != _handle)
		{
			for (uint32_t idx = 0; idx < MaxCapacityT; ++idx)
			{
				if (m_handle[idx] == _handle)
				{
					removeIndex(idx);
				}
			}
		}

		return false;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint16_t HandleHashMapT<MaxCapacityT, KeyT>::find(KeyT _key) const
	{
		uint32_t idx = findIndex(_key);
		if (UINT32_MAX != idx)
		{
			return m_handle[idx];
		}

		return kInvalidHandle;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline void HandleHashMapT<MaxCapacityT, KeyT>::reset()
	{
		memSet(m_handle, 0xff, sizeof(m_handle) );
		m_numElements = 0;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint32_t HandleHashMapT<MaxCapacityT, KeyT>::getNumElements() const
	{
		return m_numElements;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint32_t HandleHashMapT<MaxCapacityT, KeyT>::getMaxCapacity() const
	{
		return m_maxCapacity;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline typename HandleHashMapT<MaxCapacityT, KeyT>::Iterator HandleHashMapT<MaxCapacityT, KeyT>::first() const
	{
		Iterator it;
		it.handle = kInvalidHandle;
		it.pos    = 0;
		it.num    = m_numElements;

		if (0 == it.num)
		{
			return it;
		}

		++it.num;
		next(it);
		return it;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline bool HandleHashMapT<MaxCapacityT, KeyT>::next(Iterator& _it) const
	{
		if (0 == _it.num)
		{
			return false;
		}

		for (
			;_it.pos < MaxCapacityT && kInvalidHandle == m_handle[_it.pos]
			; ++_it.pos
			);
		_it.handle = m_handle[_it.pos];
		++_it.pos;
		--_it.num;
		return true;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint32_t HandleHashMapT<MaxCapacityT, KeyT>::findIndex(KeyT _key) const
	{
		const KeyT hash = mix(_key);

		const uint32_t firstIdx = hash % MaxCapacityT;
		uint32_t idx = firstIdx;
		do
		{
			if (m_handle[idx] == kInvalidHandle)
			{
				return UINT32_MAX;
			}

			if (m_key[idx] == _key)
			{
				return idx;
			}

			idx = (idx + 1) % MaxCapacityT;

		} while (idx != firstIdx);

		return UINT32_MAX;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline void HandleHashMapT<MaxCapacityT, KeyT>::removeIndex(uint32_t _idx)
	{
		m_handle[_idx] = kInvalidHandle;
		--m_numElements;

		for (uint32_t idx = (_idx + 1) % MaxCapacityT
				; m_handle[idx] != kInvalidHandle
				; idx = (idx + 1) % MaxCapacityT)
		{
			if (m_handle[idx] != kInvalidHandle)
			{
				const KeyT key = m_key[idx];
				if (idx != findIndex(key) )
				{
					const uint16_t handle = m_handle[idx];
					m_handle[idx] = kInvalidHandle;
					--m_numElements;
					insert(key, handle);
				}
			}
		}
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint32_t HandleHashMapT<MaxCapacityT, KeyT>::mix(uint32_t _x) const
	{
		const simd32_t x      = simd32_splat(_x);
		const simd32_t c0     = simd32_splat(2246822519u);
		const simd32_t tmp0   = simd32_u32_mul(x, c0);
		const simd32_t tmp1   = simd32_x32_rol(tmp0, 13);
		const simd32_t c1     = simd32_splat(2654435761u);
		const simd32_t result = simd32_u32_mul(tmp1, c1);
		return result.u32;
	}

	template <uint32_t MaxCapacityT, typename KeyT>
	inline uint64_t HandleHashMapT<MaxCapacityT, KeyT>::mix(uint64_t _x) const
	{
		const simd64_t x      = simd64_splat(_x);
		const simd64_t c0     = simd64_splat(uint64_t(14029467366897019727ull) );
		const simd64_t tmp0   = simd64_u64_mul(x, c0);
		const simd64_t tmp1   = simd64_x64_rol(tmp0, 31);
		const simd64_t c1     = simd64_splat(uint64_t(11400714785074694791ull) );
		const simd64_t result = simd64_u64_mul(tmp1, c1);
		return result.u64;
	}

} // namespace bx
