/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/storageblk.h>

namespace bx
{
	namespace internal
	{
		static uint8_t* storageRealloc(Blk& _blk, void*& _capacityTerm, const void*& _ptr, uint32_t _count, uint32_t _num, uint32_t _elementSize)
		{
			const uint8_t* ptr = (const uint8_t*)_ptr;

			const uint32_t oldCapacity = uint32_t( ( (const uint8_t*)_capacityTerm - ptr)/_elementSize);
			const uint32_t minCapacity = max<uint32_t>(1, 64/_elementSize);
			const uint32_t capacity    = max(_count + _num, oldCapacity + oldCapacity/2, minCapacity);

			_blk = realloc(Unchecked, move(_blk), size_t(_elementSize)*alignUp(capacity, 16) );

			uint8_t* result = (uint8_t*)_blk;
			_ptr          = result;
			_capacityTerm = result + size_t(_blk.size/_elementSize)*_elementSize;

			return result;
		}

		void* storageSetCapacity(Blk& _blk, uint32_t _capacity, uint32_t _count, uint32_t _elementSize)
		{
			const uint32_t minCapacity = max(_capacity, _count);
			const size_t   sizeInBytes = minCapacity == 0 ? 0 : size_t(_elementSize)*size_t(alignUp(minCapacity, 16) );

			if (0 == sizeInBytes)
			{
				const uint8_t handle = uint8_t(_blk.allocatorHandle);
				_blk = free(Unchecked, move(_blk) );
				_blk.allocatorHandle = handle;

				return NULL;
			}

			_blk = realloc(Unchecked, move(_blk), sizeInBytes);

			if (!isValid(_blk) )
			{
				return NULL;
			}

			return (uint8_t*)_blk + size_t(_blk.size/_elementSize)*_elementSize;
		}

		const void* storageGrow(Blk& _blk, void*& _capacityTerm, const void*& _ptr, uint32_t _count, uint32_t _num, uint32_t _elementSize)
		{
			uint8_t* ptr = storageRealloc(_blk, _capacityTerm, _ptr, _count, _num, _elementSize);
			return ptr + size_t(_count)*_elementSize;
		}

		void* storageCarve(Blk& _blk, void*& _capacityTerm, const void*& _ptr, uint32_t _count, uint32_t _pos, uint32_t _num, uint32_t _elementSize)
		{
			const size_t count = size_t(_count)*_elementSize;
			const size_t num   = size_t(_num)*_elementSize;

			uint8_t* ptr = (uint8_t*)_ptr;

			if (count + num > size_t( (uint8_t*)_capacityTerm - ptr) )
			{
				ptr = storageRealloc(_blk, _capacityTerm, _ptr, _count, _num, _elementSize);
			}

			const size_t pos = size_t(min(_pos, _count) )*_elementSize;

			memMove(ptr + pos + num, ptr + pos, count - pos);

			return ptr + pos;
		}

		const void* storageInsert(Blk& _blk, void*& _capacityTerm, const void*& _ptr, uint32_t _count, uint32_t _pos, const void* _src, uint32_t _num, uint32_t _elementSize)
		{
			const size_t num = size_t(_num)*_elementSize;

			uint8_t* room = (uint8_t*)storageCarve(_blk, _capacityTerm, _ptr, _count, _pos, _num, _elementSize);
			memCopy(room, _src, num);

			return (const uint8_t*)_ptr + size_t(_count)*_elementSize + num;
		}

	} // namespace internal

} // namespace bx
