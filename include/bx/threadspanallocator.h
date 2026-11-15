/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_THREADSPANALLOCATOR_H_HEADER_GUARD
#define BX_THREADSPANALLOCATOR_H_HEADER_GUARD

#include <bx/blk.h>
#include <bx/mutex.h>
#include <bx/thread.h>

namespace bx
{
	namespace internal
	{
		void threadSpanAllocatorThreadExit(void* _heap);

	} // namespace internal

	/// Per-thread span heap over an upstream block allocator, serves requests that satisfy
	/// `isSpanRequest`. Compose with `ThresholdAllocatorT` to serve larger requests elsewhere.
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
	class ThreadSpanAllocatorBase : public AllocatorBlkI
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

		/// Constructs span allocator over `_upstream`.
		///
		/// @param[in] _upstream Allocator for segments, see upstream requirements.
		///
		explicit ThreadSpanAllocatorBase(AllocatorBlkI* _upstream);

		/// Frees every heap and segment. Blocks still live are lost.
		~ThreadSpanAllocatorBase();

		/// Returns true when a request is served from span pages rather than the upstream.
		///
		/// @param[in] _size Size in bytes.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns `true` when served from span pages.
		///
		static bool isSpanRequest(size_t _size, size_t _alignment);

		/// Returns true when `_ptr` lies in a segment of any span allocator.
		///
		/// @param[in] _ptr Pointer to test.
		///
		/// @returns `true` when `_ptr` is a span block.
		///
		static bool isSpanBlock(const void* _ptr);

		/// Returns block record for the span block at `_ptr`, sized to its class, alignment not kept.
		///
		/// @param[in] _ptr Pointer to the start of a span block.
		///
		/// @returns Block record for `_ptr`.
		///
		Blk toBlk(void* _ptr);

		/// Takes over a span block and returns its pointer.
		///
		/// @param[in] _blk Span block, invalidated.
		///
		/// @returns Pointer to the block's memory.
		///
		void* fromBlk(Blk&& _blk);

		/// Returns number of bytes usable in the span block starting at `_ptr`.
		///
		/// @param[in] _ptr Pointer to the start of a span block.
		///
		/// @returns Usable size in bytes.
		///
		static size_t usableSize(const void* _ptr);

		/// Grows the span block at `_ptr` in place to at least `_size` bytes by taking the free slices
		/// that follow it, only for blocks larger than `kMaxMediumSize` on the thread that owns them.
		///
		/// @param[in] _ptr Pointer to the start of a span block.
		/// @param[in] _size Size in bytes, at most `kMaxLargeSize`.
		///
		/// @returns `true` when the block now holds `_size` bytes at `_ptr`, `false` when unchanged.
		///
		static bool expandSpan(void* _ptr, size_t _size);

		/// Allocates a span block without a block record, request must satisfy `isSpanRequest`.
		///
		/// @param[in] _size Size in bytes.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		/// @param[in] _location Source location for memory tracking.
		///
		/// @returns Pointer to the block, NULL when out of memory.
		///
		void* allocSpan(size_t _size, size_t _alignment, const Location& _location = Location::current() );

		/// Frees a span block from `allocSpan` or `fromBlk` of any span allocator.
		///
		/// @param[in] _ptr Pointer to the start of a span block.
		/// @param[in] _location Source location for memory tracking.
		///
		static void freeSpan(void* _ptr, const Location& _location = Location::current() );

		/// Allocates block, same as `realloc(Blk(), _size, _alignment)`.
		///
		/// @param[in] _size Size in bytes.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Allocated block, invalid on failure.
		///
		Blk alloc(size_t _size, size_t _alignment);

		/// Frees valid block, same as `realloc(move(_blk), 0, 0)`.
		///
		/// @param[in] _blk Block to free.
		///
		/// @returns Invalid block.
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

		/// Returns the calling thread's spare pages and empty segments to the upstream, and released slices to the OS.
		void threadTrim();

		/// Releases the calling thread's heap before the thread exits.
		void threadRelease();

		/// Returns upstream allocator.
		///
		/// @returns Upstream allocator.
		///
		AllocatorBlkI* getUpstream() const;

		/// Returns number of heaps, live and abandoned.
		///
		/// @returns Number of heaps.
		///
		uint32_t getNumHeaps() const;

		/// Returns number of segments held from the upstream.
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
		friend void internal::threadSpanAllocatorThreadExit(void* _heap);

		struct FreeNode;
		struct Page;
		struct Segment;
		struct Heap;

		static constexpr uint8_t  kFreeSlice        = UINT8_MAX;
		static constexpr uint8_t  kRecordSlice      = UINT8_MAX-1;
		static constexpr uint32_t kMaxEmptySegments = BX_CONFIG_HEAP_THREAD_SPAN_EMPTY_SEGMENTS;
		static constexpr uint32_t kPurgeDelayMs     = BX_CONFIG_HEAP_THREAD_SPAN_PURGE_DELAY_MS;
		static constexpr uint32_t kSweepDelayMs     = BX_CONFIG_HEAP_THREAD_SPAN_SWEEP_DELAY_MS;
		static constexpr uint32_t kFirstLargeClass  = kNumClasses - (kMaxLargeSize/kSliceSize - 1);
		static constexpr uintptr_t kRemoteAbandoned = 1;
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
		static Segment* segmentOf(const Page* _page);
		static uint8_t* pageData(const Page* _page);
		static size_t   pageBytes(const Page* _page);
		static bool     pageHasSpace(const Page* _page);
		static bool     hasLiveSlices(const Heap* _heap);
		static void     linkPageHead(Heap* _heap, Page* _page);
		static void     linkPageTail(Heap* _heap, Page* _page);
		static void     unlinkPage(Heap* _heap, Page* _page);
		static void     initSegmentMap();
		static void     mapSegment(const Segment* _segment, bool _present);
		static void     lockHeap(Heap* _heap);
		static void     unlockHeap(Heap* _heap);

		void     initClasses();
		uint32_t sizeToClass(size_t _size) const;
		static uint32_t largeClass(size_t _size);

		Heap*    getHeap();
		Heap*    getTlsHeap() const;
		Heap*    getTlsHeapFast() const;
		void     setTlsHeap(Heap* _heap);
		Heap*    createHeap();
		Heap*    adoptHeap();
		void     destroyHeap(Heap* _heap);
		void     purgeHeap(Heap* _heap, int64_t _now);
		void     sweepLarge(Heap* _heap);
		void     trimHeap(Heap* _heap);
		void     releaseThreadHeap(Heap* _heap);

		Segment* newSegment(Heap* _heap);
		void     freeSegment(Heap* _heap, Segment* _segment);
		Page*    newPage(Heap* _heap, uint32_t _numSlices);
		Page*    newClassPage(Heap* _heap, uint32_t _classIdx);
		void     releasePage(Heap* _heap, Page* _page);
		void     resizeLargePage(Heap* _heap, Page* _page, uint32_t _classIdx);

		void*    allocSpanBlock(size_t _size, size_t _alignment);
		void*    allocSpanBlockFirst(size_t _size, size_t _alignment);
		void*    allocClass(Heap* _heap, uint32_t _classIdx);
		void*    allocClassSlow(Heap* _heap, uint32_t _classIdx);
		void*    takeFromPage(Heap* _heap, uint32_t _classIdx, Page* _page);
		void*    takeRemote(Heap* _heap, uint32_t _classIdx, Page* _page);
		void     flushHeap(Heap* _heap);
		static void freeSpanBlock(void* _ptr);
		static void freeRemote(Segment* _segment, uint32_t _slice, FreeNode* _node);
		void     freeOwned(Heap* _heap, Page* _page, void* _ptr);
		void     freeOwnedSlow(Heap* _heap, Page* _page);
		void     drainRemote(Heap* _heap, uintptr_t _tag);
		void     freeAbandoned(Heap* _heap, Page* _page, FreeNode* _node);
		Blk      blkOf(void* _ptr, uint8_t _allocatorOp, size_t _alignment);
		Blk      allocBlk(size_t _size, size_t _alignment);
		Blk      freeBlk(Blk&& _blk);

#if BX_CONFIG_MEMORY_TRACKING
		static uint32_t* locationSlot(Page* _page, const void* _ptr);
#endif // BX_CONFIG_MEMORY_TRACKING

		AllocatorBlkI* m_upstream;
		TlsData        m_tls;
		uint64_t       m_tlsId;
		uintptr_t      m_exitKey;
		Mutex          m_mutex;
		Heap*          m_heaps;
		uint32_t       m_numHeaps;
		uint32_t       m_numSegments;
		bool           m_shuttingDown;
		int64_t        m_purgeDelay;
		int64_t        m_sweepDelay;
		uint32_t       m_classSize[kNumClasses];
		uint8_t        m_classLut[kMaxMediumSize/Blk::kMinBlockSize + 1];
	};

	/// Span heap over a duck typed upstream allocator, see `ThreadSpanAllocatorBase`.
	///
	/// @tparam AllocatorBlkT Upstream allocator type, duck typed `AllocatorBlkI`.
	///
	template<typename AllocatorBlkT>
	class ThreadSpanAllocatorT final : private AllocatorBlkInterfaceHolderT<AllocatorBlkT>, public ThreadSpanAllocatorBase
	{
	public:
		/// Constructs span allocator over `_upstream`.
		///
		/// @param[in] _upstream Allocator for segments, see upstream requirements.
		///
		explicit ThreadSpanAllocatorT(AllocatorBlkT& _upstream);
	};

} // namespace bx

#include "inline/threadspanallocator.inl"

#endif // BX_THREADSPANALLOCATOR_H_HEADER_GUARD
