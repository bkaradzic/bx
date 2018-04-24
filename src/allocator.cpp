/*
 * Copyright 2010-2018 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "bx_p.h"
#include <bx/allocator.h>

#include <stdlib.h>

#ifndef BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT
#	define BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT 8
#endif // BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT

namespace bx
{
	DefaultAllocator::DefaultAllocator()
	{
	}

	DefaultAllocator::~DefaultAllocator()
	{
	}

	void* DefaultAllocator::realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line)
	{
		if (0 == _size)
		{
			if (NULL != _ptr)
			{
				if (BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT >= _align)
				{
					::free(_ptr);
					return NULL;
				}

#	if BX_COMPILER_MSVC
				BX_UNUSED(_file, _line);
				_aligned_free(_ptr);
#	else
				bx::alignedFree(this, _ptr, _align, _file, _line);
#	endif // BX_
			}

			return NULL;
		}
		else if (NULL == _ptr)
		{
			if (BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT >= _align)
			{
				return ::malloc(_size);
			}

#	if BX_COMPILER_MSVC
			BX_UNUSED(_file, _line);
			return _aligned_malloc(_size, _align);
#	else
			return bx::alignedAlloc(this, _size, _align, _file, _line);
#	endif // BX_
		}

		if (BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT >= _align)
		{
			return ::realloc(_ptr, _size);
		}

#	if BX_COMPILER_MSVC
		BX_UNUSED(_file, _line);
		return _aligned_realloc(_ptr, _size, _align);
#	else
		return bx::alignedRealloc(this, _ptr, _size, _align, _file, _line);
#	endif // BX_
	}

} // namespace bx
