/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_THREADSPANALLOCATOR_H_HEADER_GUARD
#	error "Must be included from bx/threadspanallocator.h!"
#endif // BX_THREADSPANALLOCATOR_H_HEADER_GUARD

namespace bx
{
	inline bool ThreadSpanAllocatorBase::isSpanBlock(const void* _ptr)
	{
		const uintptr_t idx   = uintptr_t(_ptr)>>kSegmentShift;
		const size_t    limit = atomicLoad(&s_segmentMap.limit);

		return idx < limit
			&& 0 != ( (atomicLoad(&s_segmentMap.bits[idx>>6]) >> (idx&63) ) & 1)
			;
	}

	inline bool ThreadSpanAllocatorBase::isSpanRequest(size_t _size, size_t _alignment)
	{
		if (_alignment <= Blk::kMinBlockAlignment) [[likely]]
		{
			return _size <= kMaxLargeSize;
		}

		return _alignment <= kMaxAlignment
			&& alignUp(max<size_t>(_size, 1), _alignment) <= kMaxLargeSize
			;
	}

	inline bool ThreadSpanAllocatorBase::isCpuAccessible() const
	{
		return true;
	}

#if !BX_CONFIG_MEMORY_TRACKING
	inline void* ThreadSpanAllocatorBase::allocSpan(size_t _size, size_t _alignment, const Location& _location)
	{
		BX_UNUSED(_location);
		return allocSpanBlock(_size, _alignment);
	}

	inline void ThreadSpanAllocatorBase::freeSpan(void* _ptr, const Location& _location)
	{
		BX_UNUSED(_location);
		freeSpanBlock(_ptr);
	}
#endif // !BX_CONFIG_MEMORY_TRACKING

	template<typename AllocatorBlkT>
	inline ThreadSpanAllocatorT<AllocatorBlkT>::ThreadSpanAllocatorT(AllocatorBlkT& _upstream)
		: AllocatorBlkInterfaceHolderT<AllocatorBlkT>(_upstream)
		, ThreadSpanAllocatorBase(&this->m_interface)
	{
	}

} // namespace bx
