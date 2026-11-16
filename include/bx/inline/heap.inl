/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HEAP_H_HEADER_GUARD
#	error "Must be included from bx/heap.h!"
#endif // BX_HEAP_H_HEADER_GUARD

#if BX_CONFIG_MEMORY_TRACKING
#	define BX_HEAP_LOCATION(_location) _location
#else
#	define BX_HEAP_LOCATION(_location) kUnknownLocation
#endif // BX_CONFIG_MEMORY_TRACKING

namespace bx
{
	namespace internal
	{
		void* heapAlignedAlloc(size_t _size, size_t _alignment, const Location& _location);
		void* heapRealloc(void* _ptr, size_t _size, const Location& _location);
		void  heapFree(void* _ptr, const Location& _location);

	} // namespace internal

	inline void* malloc(size_t _size, const Location& _location)
	{
		BX_UNUSED(_location);
		return internal::heapAlignedAlloc(_size, 0, BX_HEAP_LOCATION(_location) );
	}

	inline void* alignedAlloc(size_t _size, size_t _alignment, const Location& _location)
	{
		BX_UNUSED(_location);
		return internal::heapAlignedAlloc(_size, _alignment, BX_HEAP_LOCATION(_location) );
	}

	inline void* realloc(void* _ptr, size_t _size, const Location& _location)
	{
		BX_UNUSED(_location);
		return internal::heapRealloc(_ptr, _size, BX_HEAP_LOCATION(_location) );
	}

	inline void free(void* _ptr, const Location& _location)
	{
		BX_UNUSED(_location);
		internal::heapFree(_ptr, BX_HEAP_LOCATION(_location) );
	}

} // namespace bx

#undef BX_HEAP_LOCATION
