/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/hashmap.h>

namespace bx
{
	static const uint8_t s_hashMapEmptyControl[32] =
	{
		0xff, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	};

	template<typename IndexT, typename GroupT>
	void HashMapControlViewT<IndexT, GroupT>::init(void* _data, uint32_t _keySize, uint32_t _keyAlign, uint32_t _capacityMask, uint32_t _maxFreeSlots, internal::HashKeyFn _hashKeyFn, internal::HashKeyMoveFn _keyMoveFn)
	{
		static_assert(kWidth <= BX_COUNTOF(s_hashMapEmptyControl), "Empty control group is too small for GroupT.");

#if BX_CONFIG_DEBUG
		++m_generation;
#endif // BX_CONFIG_DEBUG

		if (NULL == _data)
		{
			m_control      = const_cast<uint8_t*>(s_hashMapEmptyControl);
			m_keys         = NULL;
			m_slots        = NULL;
			m_freeSlots    = NULL;
			m_hashKeyFn    = _hashKeyFn;
			m_keyMoveFn    = _keyMoveFn;
			m_capacityMask = 0;
			m_count        = 0;
			m_growthLeft   = 0;
			m_slotCapacity = 0;
			m_numFreeSlots = 0;
			m_maxFreeSlots = 0;
			m_keySize      = _keySize;
			m_keyAlign     = _keyAlign;
			return;
		}

		BX_ASSERT(isAligned(_data, max<uint32_t>(_keyAlign, alignof(IndexT) ) ), "Block %p is not aligned to %u.", _data, _keyAlign);

		uint8_t* data = reinterpret_cast<uint8_t*>(_data);

		const uint32_t maxElements = _capacityMask + 1;
		const uint32_t controlSize = _capacityMask + kWidth;
		const uint32_t keysOffset  = alignUp(controlSize, _keyAlign);
		const uint32_t keysSize    = maxElements*_keySize;

		m_control      = data;
		m_keys         = data + keysOffset;
		m_slots        = NULL;
		m_freeSlots    = NULL;
		m_hashKeyFn    = _hashKeyFn;
		m_keyMoveFn    = _keyMoveFn;
		m_capacityMask = _capacityMask;
		m_maxFreeSlots = 0;
		m_keySize      = _keySize;
		m_keyAlign     = _keyAlign;

		if constexpr (kHasSlots)
		{
			const uint32_t slotsOffset = alignUp(keysOffset + keysSize, uint32_t(alignof(IndexT) ) );
			const uint32_t slotsSize   = maxElements*uint32_t(sizeof(IndexT) );

			m_slots        = reinterpret_cast<IndexT*>(data + slotsOffset);
			m_freeSlots    = reinterpret_cast<IndexT*>(data + slotsOffset + slotsSize);
			m_maxFreeSlots = max(maxElements, _maxFreeSlots);
		}

		clear();
	}

	template<typename IndexT, typename GroupT>
	inline void HashMapControlViewT<IndexT, GroupT>::moveKey(uint8_t* _dst, uint8_t* _src)
	{
		if (NULL == m_keyMoveFn)
		{
			memCopy(_dst, _src, m_keySize);
			return;
		}

		m_keyMoveFn(_dst, _src, false);
	}

	template<typename IndexT, typename GroupT>
	inline void HashMapControlViewT<IndexT, GroupT>::swapKey(uint8_t* _lhs, uint8_t* _rhs)
	{
		if (NULL == m_keyMoveFn)
		{
			swap(_lhs, _rhs, m_keySize);
			return;
		}

		m_keyMoveFn(_lhs, _rhs, true);
	}

	template<typename IndexT, typename GroupT>
	void HashMapControlViewT<IndexT, GroupT>::rehashInto(void* _data, uint32_t _capacityMask)
	{
		const uint8_t* oldControl      = m_control;
		const uint8_t* oldKeys         = m_keys;
		const IndexT*  oldSlots        = m_slots;
		const IndexT*  oldFreeSlots    = m_freeSlots;
		const uint32_t oldCapacityMask = m_capacityMask;
		const uint32_t oldCount        = m_count;
		const uint32_t oldSlotCapacity = m_slotCapacity;
		const uint32_t oldNumFreeSlots = m_numFreeSlots;

		BX_ASSERT(oldCount <= calcMaxCount(_capacityMask), "Capacity mask %u can't hold %u elements.", _capacityMask, oldCount);

		init(_data, m_keySize, m_keyAlign, _capacityMask, oldNumFreeSlots, m_hashKeyFn, m_keyMoveFn);

		for (uint32_t ii = 0; ii < oldCapacityMask; ++ii)
		{
			if (0 != (oldControl[ii] & kEmpty) )
			{
				continue;
			}

			const uint8_t* key  = oldKeys + ii*m_keySize;
			const uint32_t hash = m_hashKeyFn(key);
			const uint32_t idx  = findFirstEmpty(hash);

			setControl(idx, uint8_t(hash) & kNotEmptyMask);
			moveKey(m_keys + idx*m_keySize, const_cast<uint8_t*>(key) );

			if constexpr (kHasSlots)
			{
				m_slots[idx] = oldSlots[ii];
			}
		}

		if constexpr (kHasSlots)
		{
			memCopy(m_freeSlots, oldFreeSlots, oldNumFreeSlots*sizeof(IndexT) );
		}

		m_count        = oldCount;
		m_slotCapacity = oldSlotCapacity;
		m_numFreeSlots = oldNumFreeSlots;
		m_growthLeft   = calcMaxCount(_capacityMask) - oldCount;
	}

	template<typename IndexT, typename GroupT>
	void HashMapControlViewT<IndexT, GroupT>::dropDeletesWithoutResize()
	{
		convertDeletedToEmptyAndFullToDeleted();

		for (uint32_t ii = 0; ii < m_capacityMask; ++ii)
		{
			if (m_control[ii] != kDeleted)
			{
				continue;
			}

			uint8_t* key = m_keys + ii*m_keySize;

			const uint32_t hash = m_hashKeyFn(key);
			const uint32_t idx  = findFirstEmpty(hash);

			if (isInSameGroup(hash, m_capacityMask, ii, idx) )
			{
				setControl(ii, uint8_t(hash) & kNotEmptyMask);
			}
			else if (m_control[idx] == kEmpty)
			{
				setControl(idx, uint8_t(hash) & kNotEmptyMask);
				setControl(ii, kEmpty);

				moveKey(m_keys + idx*m_keySize, key);

				if constexpr (kHasSlots)
				{
					m_slots[idx] = m_slots[ii];
				}
			}
			else
			{
				setControl(idx, uint8_t(hash) & kNotEmptyMask);

				swapKey(m_keys + idx*m_keySize, key);

				if constexpr (kHasSlots)
				{
					swap(m_slots[idx], m_slots[ii]);
				}

				--ii;
			}
		}

		m_growthLeft = calcMaxCount(m_capacityMask) - m_count;
	}

	template<typename IndexT, typename GroupT>
	void HashMapControlViewT<IndexT, GroupT>::convertDeletedToEmptyAndFullToDeleted()
	{
		for (uint8_t *ctrl = m_control, *term = m_control + m_capacityMask; ctrl < term; ctrl += kWidth)
		{
			GroupT::convertSpecialToEmptyAndFullToDeleted(ctrl);
		}

		memCopy(m_control + m_capacityMask + 1, m_control, kWidth-1);
		m_control[m_capacityMask] = kSentinel;
	}

	template<typename IndexT, typename GroupT>
	bool HashMapControlViewT<IndexT, GroupT>::isInSameGroup(uint32_t _hash, uint32_t _capacityMask, uint32_t _idxA, uint32_t _idxB)
	{
		const uint32_t probeOffset = (_hash >> 7) & _capacityMask;
		const uint32_t idxA        = (_idxA - probeOffset) & _capacityMask;
		const uint32_t idxB        = (_idxB - probeOffset) & _capacityMask;
		return idxA / kWidth == idxB / kWidth;
	}

	template class HashMapControlViewT<uint16_t,       HashMapGroupRef>;
	template class HashMapControlViewT<uint16_t,       HashMapGroupSimd>;
	template class HashMapControlViewT<uint16_t,       HashMapGroupSimd256>;
	template class HashMapControlViewT<uint32_t,       HashMapGroupRef>;
	template class HashMapControlViewT<uint32_t,       HashMapGroupSimd>;
	template class HashMapControlViewT<uint32_t,       HashMapGroupSimd256>;
	template class HashMapControlViewT<HashMapNoSlots, HashMapGroupRef>;
	template class HashMapControlViewT<HashMapNoSlots, HashMapGroupSimd>;
	template class HashMapControlViewT<HashMapNoSlots, HashMapGroupSimd256>;

} // namespace bx
