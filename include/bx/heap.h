/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HEAP_H_HEADER_GUARD
#define BX_HEAP_H_HEADER_GUARD

#include <bx/allocator.h>
#include <bx/blk.h>

namespace bx
{
	/// Allocates block aligned to `Blk::kMinBlockAlignment`.
	///
	/// @param[in] _size Size in bytes.
	/// @param[in] _location Source location for memory tracking.
	///
	/// @returns Pointer to block of at least `_size` bytes, NULL when out of memory.
	///
	void* malloc(size_t _size, const Location& _location = Location::current() );

	/// Allocates block aligned to `_alignment`.
	///
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	/// @param[in] _location Source location for memory tracking.
	///
	/// @returns Pointer to block of at least `_size` bytes, NULL when out of memory.
	///
	void* alignedAlloc(size_t _size, size_t _alignment, const Location& _location = Location::current() );

	/// Resizes `_ptr` keeping its contents, the result is aligned to `Blk::kMinBlockAlignment`.
	///
	/// @param[in] _ptr Pointer to block, NULL allocates.
	/// @param[in] _size Size in bytes, zero frees.
	/// @param[in] _location Source location for memory tracking.
	///
	/// @returns Pointer to resized block, NULL when out of memory, `_ptr` stays valid.
	///
	void* realloc(void* _ptr, size_t _size, const Location& _location = Location::current() );

	/// Frees block from `malloc`, `alignedAlloc` or `realloc`, NULL is ignored.
	///
	/// @param[in] _ptr Pointer to block.
	/// @param[in] _location Source location for memory tracking.
	///
	void free(void* _ptr, const Location& _location = Location::current() );

	/// Returns number of bytes usable at `_ptr`.
	///
	/// @param[in] _ptr Pointer to block.
	///
	/// @returns Size in bytes, 0 for NULL.
	///
	size_t usableSize(const void* _ptr);

	/// Heap behind `malloc`, `alignedAlloc`, `realloc` and `free`, all heap functions are thread safe.
	struct Heap
	{
		/// Heap:
		enum Enum
		{
			Balanced,   //!< New blocks up to `BX_CONFIG_HEAP_BALANCED_SPLIT` bytes from `ThreadSpanAllocatorT`, larger from `SharedSpanAllocatorT`; `getHeapAllocator` resizes a block within its heap, `realloc` picks by the new size.
			Throughput, //!< Thread owned span pages, unbounded thread free lists, `ThreadSpanAllocatorT`.
			Footprint,  //!< Shared span pages, bounded thread caches, `SharedSpanAllocatorT`.

			Count
		};
	};

	/// Selects the heap, call before the first allocation. Default is `Heap::Balanced`.
	///
	/// @param[in] _heap Heap.
	///
	void setHeap(Heap::Enum _heap);

	/// Sets the upstream allocator the heap takes segments and large blocks from, call before the first
	/// allocation.
	///
	/// @param[in] _allocator Thread safe, CPU accessible, honoring `ThreadSpanAllocatorBase::kSegmentSize`
	///   alignment. NULL uses system memory.
	///
	void setHeapUpstream(AllocatorBlkI* _allocator);

	/// Returns the heap as a block allocator.
	///
	/// @returns Block allocator.
	///
	AllocatorBlkI* getHeapAllocator();

	/// Returns handle of the heap's block allocator, it's permanent.
	///
	/// @returns Allocator handle.
	///
	AllocatorHandle getHeapAllocatorHandle();

	/// Returns `AllocatorI` over the heap.
	///
	/// @returns Allocator.
	///
	AllocatorI* getHeapAllocatorI();

	/// What the heap holds from its upstream (system memory, or `setHeapUpstream`'s): span
	/// segments blocks are carved from with the span allocators' bookkeeping, and large blocks
	/// it passes through.
	struct HeapStats
	{
		size_t segmentBytes = 0; //!< Bytes of span segments and their bookkeeping.
		size_t numSegments  = 0; //!< Number of span segments and bookkeeping blocks.
		size_t largeBytes   = 0; //!< Bytes of large blocks.
		size_t numLarge     = 0; //!< Number of large blocks.
	};

	/// Returns what the heap holds from its upstream, zeros before its first allocation.
	///
	/// @returns Heap statistics.
	///
	HeapStats getHeapStats();

	/// Returns the calling thread's unused cached memory to the block allocator.
	void heapThreadTrim();

	/// Releases the calling thread's heap, called automatically when the thread exits.
	void heapThreadRelease();

} // namespace bx

#include "inline/heap.inl"

#endif // BX_HEAP_H_HEADER_GUARD
