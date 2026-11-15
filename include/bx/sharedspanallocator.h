/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_SHAREDSPANALLOCATOR_H_HEADER_GUARD
#define BX_SHAREDSPANALLOCATOR_H_HEADER_GUARD

#include <bx/blk.h>
#include <bx/mutex.h>
#include <bx/thread.h>

namespace bx
{
	namespace internal
	{
		void sharedSpanAllocatorThreadExit(void* _heap);

	} // namespace internal

	/// Span heap with pages shared by all threads and bounded per-thread caches, serves requests
	/// that satisfy `isSpanRequest`. Compose with `ThresholdAllocatorT` to serve larger requests
	/// elsewhere.
	///
	/// Blocks move between per-thread caches and shared pages in batches, under a lock per size
	/// class. Uses less memory than `ThreadSpanAllocatorBase` with many threads, at the cost of a lock
	/// when a thread cache runs dry or overflows.
	///
	/// Thread safe.
	///
	/// Upstream requirements:
	///  - Blocks are CPU accessible.
	///  - Honors alignment of `kSegmentSize`.
	///  - Thread safe, or wrapped in `MutexAllocatorT`.
	///
	/// Pages of released slices are returned to the OS with `Memory::DontNeed` after a short delay.
	///
	class SharedSpanAllocatorBase : public AllocatorBlkI
	{
	public:
		static constexpr uint32_t kSegmentShift  = 22;                                //!< Log2 of `kSegmentSize`.
		static constexpr uint32_t kSliceShift    = 16;                                //!< Log2 of `kSliceSize`.
		static constexpr size_t   kSegmentSize   = size_t(1)<<kSegmentShift;          //!< Size and alignment of segments taken from the upstream.
		static constexpr size_t   kSliceSize     = size_t(1)<<kSliceShift;            //!< Size of a segment slice.
		static constexpr uint32_t kNumSlices     = uint32_t(kSegmentSize/kSliceSize); //!< Number of slices per segment.
		static constexpr uint32_t kMaxSmallSize  = 1024;                              //!< Largest small size class.
		static constexpr uint32_t kMaxMediumSize = 65536;                             //!< Largest medium size class.
		static constexpr uint32_t kMaxLargeSize  = 1<<20;                             //!< Largest request, rounded up to its alignment, served from span pages.
		static constexpr uint32_t kMaxAlignment  = kMaxMediumSize;                    //!< Largest alignment served from span pages.
		static constexpr uint32_t kNumClasses    = 88 + kMaxLargeSize/kSliceSize - 1; //!< Number of size classes.

		/// Constructs allocator over `_upstream`.
		///
		/// @param[in] _upstream Allocator for segments, see upstream requirements.
		///
		explicit SharedSpanAllocatorBase(AllocatorBlkI* _upstream);

		/// Destroys allocator, returns all segments to the upstream.
		~SharedSpanAllocatorBase();

		/// Returns true when request is served from span pages.
		///
		/// @param[in] _size Size in bytes.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns True when request is served from span pages.
		///
		static bool isSpanRequest(size_t _size, size_t _alignment);

		/// Returns true when `_ptr` points into a segment of any shared span allocator.
		///
		/// @param[in] _ptr Pointer.
		///
		/// @returns True when `_ptr` is a span block.
		///
		static bool isSpanBlock(const void* _ptr);

		/// Returns block of span pointer `_ptr`, without changing ownership.
		///
		/// @param[in] _ptr Span pointer.
		///
		/// @returns Block.
		///
		Blk toBlk(void* _ptr);

		/// Returns pointer of span block `_blk`, and invalidates `_blk`.
		///
		/// @param[in] _blk Span block.
		///
		/// @returns Pointer.
		///
		void* fromBlk(Blk&& _blk);

		/// Returns usable size of span pointer `_ptr`.
		///
		/// @param[in] _ptr Span pointer.
		///
		/// @returns Size in bytes.
		///
		static size_t usableSize(const void* _ptr);

		/// Grows the span block at `_ptr` in place to at least `_size` bytes by taking the free slices
		/// that follow it, only for blocks larger than `kMaxMediumSize`.
		///
		/// @param[in] _ptr Pointer to the start of a span block.
		/// @param[in] _size Size in bytes, at most `kMaxLargeSize`.
		///
		/// @returns `true` when the block now holds `_size` bytes at `_ptr`, `false` when unchanged.
		///
		static bool expandSpan(void* _ptr, size_t _size);

		/// Allocates span block, request must satisfy `isSpanRequest`.
		///
		/// @param[in] _size Size in bytes.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		/// @param[in] _location Source location.
		///
		/// @returns Pointer, NULL when out of memory.
		///
		void* allocSpan(size_t _size, size_t _alignment, const Location& _location = Location::current() );

		/// Frees span pointer `_ptr`.
		///
		/// @param[in] _ptr Span pointer.
		/// @param[in] _location Source location.
		///
		static void freeSpan(void* _ptr, const Location& _location = Location::current() );

		/// Allocates block.
		///
		/// @param[in] _size Size in bytes.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Block, invalid when out of memory or not a span request.
		///
		Blk alloc(size_t _size, size_t _alignment);

		/// Frees block.
		///
		/// @param[in] _blk Block.
		///
		/// @returns Invalidated block.
		///
		Blk free(Blk&& _blk);

		/// Returns `true` when `_blk` was allocated by this allocator.
		///
		/// @param[in] _blk Live block.
		///
		/// @returns `true` when owned by this allocator.
		///
		bool owns(const Blk& _blk) override;

		/// Returns `true`, span blocks are CPU accessible.
		///
		/// @returns `true`.
		///
		bool isCpuAccessible() const override;

		/// Returns calling thread's cached blocks to shared pages, and returns released pages to
		/// the OS.
		void threadTrim();

		/// Releases calling thread's cache.
		void threadRelease();

		/// Returns upstream allocator.
		///
		/// @returns Upstream allocator.
		///
		AllocatorBlkI* getUpstream() const;

		/// Returns number of thread caches.
		///
		/// @returns Number of thread caches.
		///
		uint32_t getNumHeaps() const;

		/// Returns number of segments.
		///
		/// @returns Number of segments.
		///
		uint32_t getNumSegments() const;

	protected:
		/// Allocates, resizes or frees block.
		///
		/// @param[in] _blk Block to resize, invalid to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resized block, invalid on failure or when freed.
		///
		Blk reallocImpl(Blk&& _blk, size_t _size, size_t _alignment) override;

	private:
		friend void internal::sharedSpanAllocatorThreadExit(void* _heap);

		struct FreeNode;
		struct Page;
		struct Segment;
		struct ClassPool;
		struct Bin;
		struct Heap;

		static constexpr uint8_t  kFreeSlice        = UINT8_MAX;
		static constexpr uint8_t  kRecordSlice      = UINT8_MAX-1;
		static constexpr uint32_t kMaxEmptySegments = BX_CONFIG_HEAP_SHARED_SPAN_EMPTY_SEGMENTS;
		static constexpr uint32_t kPurgeDelayMs     = BX_CONFIG_HEAP_SHARED_SPAN_PURGE_DELAY_MS;
		static constexpr uint32_t kBatchBytes       = BX_CONFIG_HEAP_SHARED_SPAN_BATCH_SIZE;
		static constexpr uint32_t kMaxBatch         = 1024;
		static constexpr uint32_t kMinBatch         = 8;
		static constexpr uint32_t kMaxTransfer      = 12;
		static constexpr uint32_t kFirstLargeClass  = kNumClasses - (kMaxLargeSize/kSliceSize - 1);
		static constexpr uint32_t kMapAddressBits   = BX_ARCH_64BIT ? 48 : 32;
		static constexpr size_t   kMapBits          = size_t(1)<<(kMapAddressBits - kSegmentShift);

		struct SegmentMap
		{
			uint64_t* bits;
			size_t    limit;
		};

		static SegmentMap s_segmentMap;

		static Segment* toSegment(const void* _ptr);
		static Page*    toPage(const void* _ptr);
		static uint8_t* pageData(const Page* _page);
		static size_t   pageBytes(const Page* _page);
		static bool     pageHasSpace(const Page* _page);
		static void     initSegmentMap();
		static void     mapSegment(const Segment* _segment, bool _present);
		static void     lockPool(ClassPool* _pool);
		static void     unlockPool(ClassPool* _pool);
		static void     linkPage(ClassPool* _pool, Page* _page);
		static void     unlinkPage(ClassPool* _pool, Page* _page);

		void     initClasses();
		uint32_t sizeToClass(size_t _size) const;
		static uint32_t largeClass(size_t _size);

		Heap*    createHeap();
		void     destroyHeap(Heap* _heap);
		void     flushHeap(Heap* _heap);
		void     releaseThreadHeap(Heap* _heap);

		Segment* newSegment();
		void     freeSegment(Segment* _segment);
		void     purge(int64_t _now);
		Page*    newPage(uint32_t _numSlices);
		void     releasePage(Page* _page);
		Page*    newClassPage(uint32_t _classIdx);
		bool     expandPage(Page* _page, uint32_t _classIdx);

		uint32_t fillBatch(uint32_t _classIdx, uint32_t _num, FreeNode** _outHead);
		void     returnBatch(uint32_t _classIdx, FreeNode* _head, bool _full);

		void*    allocSpanBlock(size_t _size, size_t _alignment);
		void*    allocClass(Heap* _heap, uint32_t _classIdx);
		void*    allocClassSlow(Heap* _heap, uint32_t _classIdx);
		void*    allocSpanBlockFirst(size_t _size, size_t _alignment);
		static void freeSpanBlock(void* _ptr);
		static void freeClassSlow(Heap* _heap, uint32_t _classIdx);
		static void freeNoHeap(void* _ptr, uint32_t _classIdx);
		Blk      blkOf(void* _ptr, uint8_t _allocatorOp, size_t _alignment);
		Blk      allocBlk(size_t _size, size_t _alignment);
		Blk      freeBlk(Blk&& _blk);

#if BX_CONFIG_MEMORY_TRACKING
		static uint32_t* locationSlot(Page* _page, const void* _ptr);
#endif // BX_CONFIG_MEMORY_TRACKING

		AllocatorBlkI* m_upstream;
		TlsData        m_tls;
		uintptr_t      m_exitKey;
		ClassPool*     m_pools;
		Blk            m_poolsBlk;
		Mutex          m_mutex;
		Segment*       m_segments;
		Heap*          m_heaps;
		uint32_t       m_numHeaps;
		uint32_t       m_numSegments;
		uint32_t       m_numEmptySegments;
		bool           m_shuttingDown;
		int64_t        m_purgeDelay;
		int64_t        m_purgeAt;
		uint32_t       m_classSize[kNumClasses];
		uint16_t       m_classBatch[kNumClasses];
		uint16_t       m_classMax[kNumClasses];
		uint8_t        m_classLut[kMaxMediumSize/Blk::kMinBlockSize + 1];
	};

	/// Shared span heap over a duck typed upstream allocator, see `SharedSpanAllocatorBase`.
	///
	/// @tparam AllocatorBlkT Upstream allocator type, duck typed `AllocatorBlkI`.
	///
	template<typename AllocatorBlkT>
	class SharedSpanAllocatorT final : private AllocatorBlkInterfaceHolderT<AllocatorBlkT>, public SharedSpanAllocatorBase
	{
	public:
		/// Constructs shared span allocator over `_upstream`.
		///
		/// @param[in] _upstream Allocator for segments, see upstream requirements.
		///
		explicit SharedSpanAllocatorT(AllocatorBlkT& _upstream);
	};

} // namespace bx

#include "inline/sharedspanallocator.inl"

#endif // BX_SHAREDSPANALLOCATOR_H_HEADER_GUARD
