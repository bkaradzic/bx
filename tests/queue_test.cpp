/*
 * Copyright 2010-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/spscqueue.h>
#include <bx/mpscqueue.h>

void* bitsToPtr(uintptr_t _ui)
{
	return reinterpret_cast<void*>(_ui);
}

uintptr_t ptrToBits(void* _ptr)
{
	return reinterpret_cast<uintptr_t>(_ptr);
}

TEST_CASE("SpSc", "")
{
	bx::DefaultAllocator allocator;
	bx::SpScUnboundedQueue queue(&allocator);
	queue.push(bitsToPtr(0xdeadbeef) );
	REQUIRE(0xdeadbeef == ptrToBits(queue.pop() ) );
}

TEST_CASE("MpSc", "")
{
	bx::DefaultAllocator allocator;
	bx::MpScUnboundedQueueT<void> queue(&allocator);
	queue.push(bitsToPtr(0xdeadbeef) );
	REQUIRE(0xdeadbeef == ptrToBits(queue.pop() ) );
}
