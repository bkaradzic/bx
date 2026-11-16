/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_SHAREDSPANALLOCATOR_H_HEADER_GUARD
#	error "Must be included from bx/sharedspanallocator.h!"
#endif // BX_SHAREDSPANALLOCATOR_H_HEADER_GUARD

namespace bx
{
	inline bool SharedSpanAllocatorBase::isSpanBlock(const void* _ptr)
	{
		const uintptr_t idx   = uintptr_t(_ptr)>>kSegmentShift;
		const size_t    limit = atomicLoad(&s_segmentMap.limit);

		return idx < limit
			&& 0 != ( (atomicLoad(&s_segmentMap.bits[idx>>6]) >> (idx&63) ) & 1)
			;
	}

	inline bool SharedSpanAllocatorBase::isSpanRequest(size_t _size, size_t _alignment)
	{
		if (_alignment <= Blk::kMinBlockAlignment) [[likely]]
		{
			return _size <= kMaxLargeSize;
		}

		return _alignment <= kMaxAlignment
			&& alignUp(max<size_t>(_size, 1), _alignment) <= kMaxLargeSize
			;
	}

	inline bool SharedSpanAllocatorBase::isCpuAccessible() const
	{
		return true;
	}

#if !BX_CONFIG_MEMORY_TRACKING
	inline void* SharedSpanAllocatorBase::allocSpan(size_t _size, size_t _alignment, const Location& _location)
	{
		BX_UNUSED(_location);
		return allocSpanBlock(_size, _alignment);
	}

	inline void SharedSpanAllocatorBase::freeSpan(void* _ptr, const Location& _location)
	{
		BX_UNUSED(_location);
		freeSpanBlock(_ptr);
	}
#endif // !BX_CONFIG_MEMORY_TRACKING

	template<typename AllocatorBlkT>
	inline SharedSpanAllocatorT<AllocatorBlkT>::SharedSpanAllocatorT(AllocatorBlkT& _upstream)
		: AllocatorBlkInterfaceHolderT<AllocatorBlkT>(_upstream)
		, SharedSpanAllocatorBase(&this->m_interface)
	{
	}

} // namespace bx
