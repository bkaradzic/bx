/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/threadspanallocator.h>
#include <bx/cpu.h>
#include <bx/math.h>
#include <bx/os.h>
#include <bx/timer.h>

#include "spanallocator.h"

#if BX_COMPILER_MSVC
#	if BX_CPU_X86
#		include <xmmintrin.h>
#	else
#		include <intrin.h>
#	endif // BX_CPU_X86
#endif // BX_COMPILER_MSVC

namespace bx
{
	struct ThreadSpanAllocatorBase::FreeNode
	{
		FreeNode* next;
	};

	struct ThreadSpanAllocatorBase::Page
	{
		volatile uintptr_t remoteFree;
		FreeNode* freeList;
		uint8_t*  bump;
		uint8_t*  bumpEnd;
		Page*     nextInClass;
		Page*     prevInClass;
		uint32_t  blockSize;
		uint32_t  used;
		uint32_t  numFree;
		uint16_t  classIdx;
		uint8_t   firstSlice;
		uint8_t   numSlices;
#if BX_ARCH_32BIT
		uint32_t  unused0[6];
#endif // BX_ARCH_32BIT
	};

	struct ThreadSpanAllocatorBase::Segment
	{
		static constexpr size_t kRecordSize = sizeof(Blk) + sizeof(uint64_t) + sizeof(int64_t) + sizeof(Heap*) + sizeof(uintptr_t) + sizeof(Segment*) + sizeof(uint32_t) + 2*kNumSlices;

		Blk       blk;
		uint64_t  dirtySlices;
		int64_t   dirtyTime;
		Heap*     owner;
		volatile uintptr_t ownerSelf;
		Segment*  next;
		uint32_t  numUsedSlices;
		uint8_t   slicePage[kNumSlices];
		uint8_t   sliceClass[kNumSlices];
		BX_ALIGN_DECL(BX_CACHE_LINE_SIZE, Page pages[kNumSlices]);

		static_assert(BX_CACHE_LINE_SIZE == sizeof(Page), "Page record must take one cache line, remote frees write it.");
		static_assert(kNumSlices <= 64, "Dirty slice mask must hold every slice.");
	};

	struct ThreadSpanAllocatorBase::Heap
	{
		FreeNode* classFree[kNumClasses];
		Segment*  segments;
		Segment*  home;
		ThreadSpanAllocatorBase* allocator;
		Heap*     next;
		Page*     classPage[kNumClasses];
		Page*     classPages[kNumClasses];
		Page*     classPagesTail[kNumClasses];
		Page*     classEmpty[kNumClasses];
		Page*     classRemote[kNumClasses];
		uintptr_t self;
		uint32_t  numEmptySegments;
		volatile uint32_t abandoned;
		volatile uint32_t lock;
		uint32_t  unused0;
		int64_t   purgeAt;
		int64_t   sweepAt;

		static_assert(alignUp(Segment::kRecordSize, size_t(BX_CACHE_LINE_SIZE) ) + sizeof(Page)*kNumSlices == sizeof(Segment), "Segment record has implicit padding.");
	};

	ThreadSpanAllocatorBase::SegmentMap ThreadSpanAllocatorBase::s_segmentMap;

	namespace
	{
		constexpr uint32_t kHeapOwned     = 0;
		constexpr uint32_t kHeapAbandoned = 1;
		constexpr uint32_t kHeapDying     = 2;

	} // namespace

	namespace
	{
		inline void prefetch(const void* _ptr)
		{
#if BX_COMPILER_MSVC && BX_CPU_X86
			_mm_prefetch( (const char*)_ptr, _MM_HINT_T0);
#elif BX_COMPILER_MSVC
			__prefetch(_ptr);
#else
			__builtin_prefetch(_ptr);
#endif // BX_COMPILER_MSVC && BX_CPU_X86
		}

#if BX_TLS_DATA_TEB
		constexpr uint32_t kTebSelfOffset = BX_ARCH_64BIT ? 0x30 : 0x18;

		uintptr_t threadSelf()
		{
			return uintptr_t(readTeb(kTebSelfOffset) );
		}
#elif BX_PLATFORM_WINDOWS
		uintptr_t threadSelf()
		{
			return uintptr_t(NtCurrentTeb() );
		}
#elif (BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX) && defined(__has_builtin) && __has_builtin(__builtin_thread_pointer)
		uintptr_t threadSelf()
		{
			return uintptr_t(__builtin_thread_pointer() );
		}
#else
		uintptr_t threadSelf()
		{
			return uintptr_t(pthread_self() );
		}
#endif // BX_TLS_DATA_TEB

		uint64_t s_nextTlsId = 0;

#if BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX
		struct TlsHeapCache
		{
			uint64_t id;
			void*    heap;
		};

		BX_THREAD_LOCAL TlsHeapCache t_tlsHeapCache = { 0, NULL };
#endif // BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX

	} // namespace

	inline ThreadSpanAllocatorBase::Heap* ThreadSpanAllocatorBase::getTlsHeap() const
	{
#if BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX
		TlsHeapCache& cache = t_tlsHeapCache;

		if (m_tlsId == cache.id) [[likely]]
		{
			return (Heap*)cache.heap;
		}

		Heap* heap = (Heap*)m_tls.get();
		cache.id   = m_tlsId;
		cache.heap = heap;
		return heap;
#else
		return (Heap*)m_tls.get();
#endif // BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX
	}

	inline ThreadSpanAllocatorBase::Heap* ThreadSpanAllocatorBase::getTlsHeapFast() const
	{
#if BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX
		const TlsHeapCache& cache = t_tlsHeapCache;
		return m_tlsId == cache.id ? (Heap*)cache.heap : NULL;
#else
		return (Heap*)m_tls.get();
#endif // BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX
	}

	inline void ThreadSpanAllocatorBase::setTlsHeap(Heap* _heap)
	{
		m_tls.set(_heap);

#if BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX
		t_tlsHeapCache.id   = m_tlsId;
		t_tlsHeapCache.heap = _heap;
#endif // BX_PLATFORM_ANDROID || BX_PLATFORM_LINUX
	}

	ThreadSpanAllocatorBase::ThreadSpanAllocatorBase(AllocatorBlkI* _upstream)
		: m_upstream(_upstream)
		, m_tlsId(atomicFetchAndAdd<uint64_t>(&s_nextTlsId, 1) + 1)
		, m_exitKey(createExitKey<internal::threadSpanAllocatorThreadExit>() )
		, m_heaps(NULL)
		, m_numHeaps(0)
		, m_numSegments(0)
		, m_shuttingDown(false)
		, m_purgeDelay(getHPFrequency()*kPurgeDelayMs/1000)
		, m_sweepDelay(getHPFrequency()*kSweepDelayMs/1000)
	{
		BX_ASSERT(NULL != _upstream, "ThreadSpanAllocatorBase requires an upstream allocator.");

		initSegmentMap();
		initClasses();
	}

	ThreadSpanAllocatorBase::~ThreadSpanAllocatorBase()
	{
		m_shuttingDown = true;
		destroyExitKey(m_exitKey);

		MutexScope lock(m_mutex);

		for (Heap* heap = m_heaps; NULL != heap;)
		{
			Heap* next = heap->next;
			Segment* home = heap->home;

			for (Segment* segment = heap->segments; NULL != segment;)
			{
				Segment* nextSegment = segment->next;

				if (segment != home)
				{
					mapSegment(segment, false);
					Blk blk = move(segment->blk);
					blk = m_upstream->realloc(move(blk), 0, 0);
					--m_numSegments;
				}

				segment = nextSegment;
			}

			mapSegment(home, false);
			Blk blk = move(home->blk);
			blk = m_upstream->realloc(move(blk), 0, 0);
			--m_numSegments;

			heap = next;
		}

		m_heaps    = NULL;
		m_numHeaps = 0;
	}

	void ThreadSpanAllocatorBase::initClasses()
	{
		uint32_t idx = 0;

		for (uint32_t size = Blk::kMinBlockSize; size <= kMaxSmallSize; size += Blk::kMinBlockSize)
		{
			m_classSize[idx++] = size;
		}

		for (uint32_t base = kMaxSmallSize; base < kMaxMediumSize; base *= 2)
		{
			for (uint32_t ii = 1; ii <= 4; ++ii)
			{
				m_classSize[idx++] = base + ii*base/4;
			}
		}

		BX_ASSERT(kFirstLargeClass == idx, "Unexpected number of small and medium size classes %d.", idx);

		for (uint32_t numSlices = 2; numSlices <= kMaxLargeSize/kSliceSize; ++numSlices)
		{
			m_classSize[idx++] = numSlices*uint32_t(kSliceSize);
		}

		BX_ASSERT(kNumClasses == idx, "Unexpected number of size classes %d.", idx);

		uint32_t cls = 0;

		for (uint32_t ii = 0; ii < BX_COUNTOF(m_classLut); ++ii)
		{
			const uint32_t size = ii*Blk::kMinBlockSize;

			while (m_classSize[cls] < size)
			{
				++cls;
			}

			m_classLut[ii] = uint8_t(cls);
		}
	}

	inline uint32_t ThreadSpanAllocatorBase::sizeToClass(size_t _size) const
	{
		return m_classLut[(uint32_t(_size) + Blk::kMinBlockSize-1)/Blk::kMinBlockSize];
	}

	inline uint32_t ThreadSpanAllocatorBase::largeClass(size_t _size)
	{
		return kFirstLargeClass + uint32_t( (_size + kSliceSize-1)>>kSliceShift) - 2;
	}

	void ThreadSpanAllocatorBase::initSegmentMap()
	{
		MutexScope lock(getSegmentMapMutex<ThreadSpanAllocatorBase>() );

		if (0 != s_segmentMap.limit)
		{
			return;
		}

		Error err;
		s_segmentMap.bits = (uint64_t*)memoryMap(NULL, kMapBits/8, 0, Memory::ReadWrite, &err);
		BX_ASSERT(err.isOk(), "Failed to map segment map of %zu bytes.", size_t(kMapBits/8) );

		atomicStore(&s_segmentMap.limit, kMapBits);
	}

	void ThreadSpanAllocatorBase::mapSegment(const Segment* _segment, bool _present)
	{
		const uintptr_t idx = uintptr_t(_segment)>>kSegmentShift;
		BX_ASSERT(idx < kMapBits, "Segment %p is outside the mapped address range.", _segment);

		MutexScope lock(getSegmentMapMutex<ThreadSpanAllocatorBase>() );

		if (_present)
		{
			atomicStore(&s_segmentMap.bits[idx>>6], s_segmentMap.bits[idx>>6] | (uint64_t(1)<<(idx&63) ) );
		}
		else
		{
			atomicStore(&s_segmentMap.bits[idx>>6], s_segmentMap.bits[idx>>6] & ~(uint64_t(1)<<(idx&63) ) );
		}
	}

	inline ThreadSpanAllocatorBase::Segment* ThreadSpanAllocatorBase::toSegment(const void* _ptr)
	{
		return (Segment*)(uintptr_t(_ptr) & ~(kSegmentSize-1) );
	}

	inline ThreadSpanAllocatorBase::Page* ThreadSpanAllocatorBase::toPage(const void* _ptr)
	{
		Segment* segment = toSegment(_ptr);
		const uint32_t slice = uint32_t( (uintptr_t(_ptr) - uintptr_t(segment) )>>kSliceShift);
		const uint8_t  first = segment->slicePage[slice];

		BX_ASSERT(first < kNumSlices, "Pointer %p doesn't point into a block.", _ptr);

		return &segment->pages[first];
	}

	inline ThreadSpanAllocatorBase::Segment* ThreadSpanAllocatorBase::segmentOf(const Page* _page)
	{
		return toSegment(_page);
	}

	inline uint8_t* ThreadSpanAllocatorBase::pageData(const Page* _page)
	{
		return (uint8_t*)segmentOf(_page) + (size_t(_page->firstSlice)<<kSliceShift);
	}

	inline size_t ThreadSpanAllocatorBase::pageBytes(const Page* _page)
	{
		return size_t(_page->numSlices)<<kSliceShift;
	}

	inline bool ThreadSpanAllocatorBase::pageHasSpace(const Page* _page)
	{
		return NULL != _page->freeList
			|| _page->bump < _page->bumpEnd
			;
	}

	inline void ThreadSpanAllocatorBase::linkPageHead(Heap* _heap, Page* _page)
	{
		Page*& head = _heap->classPages[_page->classIdx];
		Page*& tail = _heap->classPagesTail[_page->classIdx];

		_page->prevInClass = NULL;
		_page->nextInClass = head;

		if (NULL != head)
		{
			head->prevInClass = _page;
		}
		else
		{
			tail = _page;
		}

		head = _page;
	}

	inline void ThreadSpanAllocatorBase::linkPageTail(Heap* _heap, Page* _page)
	{
		Page*& head = _heap->classPages[_page->classIdx];
		Page*& tail = _heap->classPagesTail[_page->classIdx];

		_page->nextInClass = NULL;
		_page->prevInClass = tail;

		if (NULL != tail)
		{
			tail->nextInClass = _page;
		}
		else
		{
			head = _page;
		}

		tail = _page;
	}

	inline void ThreadSpanAllocatorBase::unlinkPage(Heap* _heap, Page* _page)
	{
		if (NULL != _page->prevInClass)
		{
			_page->prevInClass->nextInClass = _page->nextInClass;
		}
		else
		{
			_heap->classPages[_page->classIdx] = _page->nextInClass;
		}

		if (NULL != _page->nextInClass)
		{
			_page->nextInClass->prevInClass = _page->prevInClass;
		}
		else
		{
			_heap->classPagesTail[_page->classIdx] = _page->prevInClass;
		}
	}

#if BX_CONFIG_MEMORY_TRACKING
	inline uint32_t* ThreadSpanAllocatorBase::locationSlot(Page* _page, const void* _ptr)
	{
		if (_page->classIdx >= kFirstLargeClass)
		{
			return (uint32_t*)(pageData(_page) + pageBytes(_page) ) - 1;
		}

		const size_t idx = (uintptr_t(_ptr) - uintptr_t(pageData(_page) ) )/_page->blockSize;

		return (uint32_t*)pageData(_page) + idx;
	}
#endif // BX_CONFIG_MEMORY_TRACKING

	ThreadSpanAllocatorBase::Heap* ThreadSpanAllocatorBase::getHeap()
	{
		Heap* heap = getTlsHeap();

		if (NULL == heap) [[unlikely]]
		{
			heap = createHeap();
		}

		return heap;
	}

	ThreadSpanAllocatorBase::Segment* ThreadSpanAllocatorBase::newSegment(Heap* _heap)
	{
		Blk blk = m_upstream->realloc(Blk(), kSegmentSize, kSegmentSize);

		if (!isValid(blk) )
		{
			return NULL;
		}

		if ( (uintptr_t(blk.ptr)>>kSegmentShift) >= kMapBits)
		{
			blk = m_upstream->realloc(move(blk), 0, 0);
			return NULL;
		}

		BX_ASSERT(isAligned(blk.ptr, kSegmentSize), "Upstream returned segment 0x%llx not aligned to %zu.", blk.ptr, kSegmentSize);

		Segment* segment = (Segment*)uintptr_t(blk.ptr);
		segment->blk           = move(blk);
		segment->owner         = _heap;
		segment->ownerSelf     = NULL != _heap ? _heap->self : 0;
		segment->next          = NULL;
		segment->numUsedSlices = 0;
		segment->dirtySlices   = 0;
		segment->dirtyTime     = 0;

		memSet(segment->slicePage, kFreeSlice, sizeof(segment->slicePage) );
		segment->slicePage[0] = kRecordSlice;

		mapSegment(segment, true);

		if (NULL != _heap)
		{
			MutexScope lock(m_mutex);
			segment->next   = _heap->segments;
			_heap->segments = segment;
			++_heap->numEmptySegments;
			++m_numSegments;
		}

		return segment;
	}

	void ThreadSpanAllocatorBase::freeSegment(Heap* _heap, Segment* _segment)
	{
		{
			MutexScope lock(m_mutex);

			for (Segment** link = &_heap->segments; NULL != *link; link = &(*link)->next)
			{
				if (*link == _segment)
				{
					*link = _segment->next;
					break;
				}
			}

			--m_numSegments;
		}

		mapSegment(_segment, false);

		Blk blk = move(_segment->blk);
		blk = m_upstream->realloc(move(blk), 0, 0);
	}

	ThreadSpanAllocatorBase::Heap* ThreadSpanAllocatorBase::createHeap()
	{
		constexpr size_t kSegmentRecordSize = alignUp(sizeof(Segment), size_t(Blk::kMinBlockAlignment) );
		constexpr size_t kHeapRecordSize    = alignUp(sizeof(Heap),    size_t(Blk::kMinBlockAlignment) );
		static_assert(kSegmentRecordSize + kHeapRecordSize <= kSliceSize, "Segment and heap records must fit the first slice.");

		Heap* adopted = adoptHeap();

		if (NULL != adopted)
		{
			return adopted;
		}

		Segment* home = newSegment(NULL);

		if (NULL == home)
		{
			return NULL;
		}

		Heap* heap = (Heap*)( (uint8_t*)home + kSegmentRecordSize);
		heap->segments         = home;
		heap->home             = home;
		heap->allocator        = this;
		heap->next             = NULL;
		heap->self             = threadSelf();
		heap->numEmptySegments = 0;
		heap->abandoned        = kHeapOwned;
		heap->lock             = 0;
		heap->unused0          = 0;
		heap->purgeAt          = INT64_MAX;
		heap->sweepAt          = 0;

		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			heap->classFree[ii]      = NULL;
			heap->classPage[ii]      = NULL;
			heap->classPages[ii]     = NULL;
			heap->classPagesTail[ii] = NULL;
			heap->classEmpty[ii]     = NULL;
			heap->classRemote[ii]    = NULL;
		}

		home->owner     = heap;
		home->ownerSelf = heap->self;

		{
			MutexScope lock(m_mutex);
			heap->next = m_heaps;
			m_heaps    = heap;
			++m_numHeaps;
			++m_numSegments;
		}

		setTlsHeap(heap);
		setExitValue(m_exitKey, heap);

		return heap;
	}

	ThreadSpanAllocatorBase::Heap* ThreadSpanAllocatorBase::adoptHeap()
	{
		Heap* heap = NULL;

		for (;;)
		{
			bool busy = false;

			{
				MutexScope lock(m_mutex);

				for (Heap* candidate = m_heaps; NULL != candidate; candidate = candidate->next)
				{
					if (kHeapAbandoned != atomicLoad(&candidate->abandoned) )
					{
						continue;
					}

					if (0 != atomicCompareAndSwap<uint32_t>(&candidate->lock, 0, 1) )
					{
						busy = true;
						continue;
					}

					if (kHeapAbandoned == candidate->abandoned)
					{
						heap = candidate;
						break;
					}

					unlockHeap(candidate);
				}
			}

			if (NULL != heap)
			{
				break;
			}

			if (!busy)
			{
				return NULL;
			}

			yield();
		}

		const uintptr_t self = threadSelf();
		heap->self = self;

		for (Segment* segment = heap->segments; NULL != segment; segment = segment->next)
		{
			atomicStore<uintptr_t>(&segment->ownerSelf, self);

			for (uint32_t ii = 0; ii < kNumSlices; ++ii)
			{
				Page* page = &segment->pages[ii];

				for (FreeNode* node = (FreeNode*)(atomicExchange<uintptr_t>(&page->remoteFree, 0) & ~kRemoteAbandoned); NULL != node;)
				{
					FreeNode* next = node->next;
					freeOwned(heap, page, node);
					node = next;
				}
			}
		}

		heap->purgeAt = INT64_MAX;
		atomicStore<uint32_t>(&heap->abandoned, kHeapOwned);
		unlockHeap(heap);

		setTlsHeap(heap);
		setExitValue(m_exitKey, heap);

		return heap;
	}

	void ThreadSpanAllocatorBase::destroyHeap(Heap* _heap)
	{
		Segment* home = _heap->home;

		for (Segment* segment = _heap->segments; NULL != segment;)
		{
			Segment* next = segment->next;

			if (segment != home)
			{
				freeSegment(_heap, segment);
			}

			segment = next;
		}

		{
			MutexScope lock(m_mutex);

			for (Heap** link = &m_heaps; NULL != *link; link = &(*link)->next)
			{
				if (*link == _heap)
				{
					*link = _heap->next;
					break;
				}
			}

			--m_numHeaps;
			--m_numSegments;
		}

		mapSegment(home, false);

		Blk blk = move(home->blk);
		blk = m_upstream->realloc(move(blk), 0, 0);
	}

	ThreadSpanAllocatorBase::Page* ThreadSpanAllocatorBase::newPage(Heap* _heap, uint32_t _numSlices)
	{
		const int64_t now = getHPCounter();

		if (now >= _heap->sweepAt)
		{
			_heap->sweepAt = now + m_sweepDelay;
			sweepLarge(_heap);
		}

		if (now >= _heap->purgeAt)
		{
			purgeHeap(_heap, now);
		}

		for (uint32_t pass = 0; pass < 2; ++pass)
		{
			for (Segment* segment = _heap->segments; NULL != segment; segment = segment->next)
			{
				uint32_t run = 0;

				for (uint32_t ii = 1; ii < kNumSlices; ++ii)
				{
					if (kFreeSlice != segment->slicePage[ii])
					{
						run = 0;
						continue;
					}

					++run;

					if (run == _numSlices)
					{
						const uint32_t first = ii + 1 - _numSlices;

						for (uint32_t jj = first; jj <= ii; ++jj)
						{
							segment->slicePage[jj] = uint8_t(first);
						}

						if (0 == segment->numUsedSlices
						&&  segment != _heap->home)
						{
							--_heap->numEmptySegments;
						}

						segment->numUsedSlices += _numSlices;
						segment->dirtySlices   &= ~sliceMask(first, _numSlices);

						Page* page = &segment->pages[first];
						page->freeList    = NULL;
						page->bump        = NULL;
						page->bumpEnd     = NULL;
						page->nextInClass = NULL;
						page->prevInClass = NULL;
						page->remoteFree  = 0;
						page->blockSize   = 0;
						page->used        = 0;
						page->numFree     = 0;
						page->classIdx    = 0;
						page->firstSlice  = uint8_t(first);
						page->numSlices   = uint8_t(_numSlices);

						return page;
					}
				}
			}

			if (0 == pass
			&&  NULL == newSegment(_heap) )
			{
				return NULL;
			}
		}

		return NULL;
	}

	ThreadSpanAllocatorBase::Page* ThreadSpanAllocatorBase::newClassPage(Heap* _heap, uint32_t _classIdx)
	{
		const uint32_t blockSize = m_classSize[_classIdx];
		const uint32_t numSlices = blockSize <= kMaxSmallSize   ? 1
			: _classIdx < kFirstLargeClass ? 8
			: blockSize/uint32_t(kSliceSize) + BX_CONFIG_MEMORY_TRACKING
			;

		Page* page = newPage(_heap, numSlices);

		if (NULL == page)
		{
			return NULL;
		}

		uint8_t*     data  = pageData(page);
		const size_t bytes = pageBytes(page);
		size_t       first = 0;

#if BX_CONFIG_MEMORY_TRACKING
		if (_classIdx < kFirstLargeClass)
		{
			first = alignUp( (bytes/blockSize)*sizeof(uint32_t), size_t(blockSize) );
			memSet(data, 0, first);
		}
#endif // BX_CONFIG_MEMORY_TRACKING

		page->bump      = data + first;
		page->bumpEnd   = data + bytes - (bytes - first) % blockSize;
		page->blockSize = blockSize;
		page->classIdx  = uint16_t(_classIdx);

		Segment* segment = segmentOf(page);

		for (uint32_t ii = page->firstSlice, end = ii + page->numSlices; ii < end; ++ii)
		{
			segment->sliceClass[ii] = uint8_t(_classIdx);
		}

		linkPageHead(_heap, page);

		return page;
	}

	void ThreadSpanAllocatorBase::releasePage(Heap* _heap, Page* _page)
	{
		Segment* segment = segmentOf(_page);

		if (_page == _heap->classRemote[_page->classIdx])
		{
			_heap->classRemote[_page->classIdx] = _page->nextInClass;
		}

		unlinkPage(_heap, _page);

		if (_page == _heap->classEmpty[_page->classIdx])
		{
			_heap->classEmpty[_page->classIdx] = NULL;
		}

		for (uint32_t ii = _page->firstSlice, end = ii + _page->numSlices; ii < end; ++ii)
		{
			segment->slicePage[ii] = kFreeSlice;
		}

		const int64_t now = getHPCounter();
		segment->dirtySlices |= sliceMask(_page->firstSlice, _page->numSlices);
		segment->dirtyTime    = now;
		_heap->purgeAt        = min(_heap->purgeAt, now + m_purgeDelay);

		if (now >= _heap->purgeAt
		||  0 != _heap->abandoned)
		{
			purgeHeap(_heap, 0 != _heap->abandoned ? INT64_MAX : now);
		}

		segment->numUsedSlices -= _page->numSlices;

		if (0 == segment->numUsedSlices
		&&  segment != _heap->home)
		{
			if (_heap->numEmptySegments < kMaxEmptySegments
			&&  0 == _heap->abandoned)
			{
				++_heap->numEmptySegments;
			}
			else
			{
				freeSegment(_heap, segment);
			}
		}
	}

	BX_FORCE_INLINE void* ThreadSpanAllocatorBase::allocClass(Heap* _heap, uint32_t _classIdx)
	{
		FreeNode* node = _heap->classFree[_classIdx];

		if (NULL != node) [[likely]]
		{
			FreeNode* next = node->next;
			_heap->classFree[_classIdx] = next;
			prefetch(next);
			return node;
		}

		return allocClassSlow(_heap, _classIdx);
	}

	void* ThreadSpanAllocatorBase::takeFromPage(Heap* _heap, uint32_t _classIdx, Page* _page)
	{
		FreeNode* node = _page->freeList;

		if (NULL != node)
		{
			_heap->classFree[_classIdx] = node->next;
			_page->freeList = NULL;
			_page->used    += _page->numFree;
			_page->numFree  = 0;
			return node;
		}

		uint8_t* ptr = _page->bump;
		_page->bump += _page->blockSize;
		++_page->used;

		return ptr;
	}

	inline void* ThreadSpanAllocatorBase::takeRemote(Heap* _heap, uint32_t _classIdx, Page* _page)
	{
		FreeNode* node = (FreeNode*)atomicExchange<uintptr_t>(&_page->remoteFree, 0);
		_heap->classFree[_classIdx]   = node->next;
		_heap->classRemote[_classIdx] = _page->nextInClass;

		return node;
	}

	BX_NO_INLINE void* ThreadSpanAllocatorBase::allocClassSlow(Heap* _heap, uint32_t _classIdx)
	{
		Page* cursor = _heap->classRemote[_classIdx];

		if (NULL == cursor)
		{
			cursor = _heap->classPages[_classIdx];
		}

		for (Page* page = cursor; NULL != page; page = page->nextInClass)
		{
			if (0 != atomicLoad(&page->remoteFree) )
			{
				return takeRemote(_heap, _classIdx, page);
			}
		}

		for (Page* page = _heap->classPages[_classIdx]; cursor != page; page = page->nextInClass)
		{
			if (0 != atomicLoad(&page->remoteFree) )
			{
				return takeRemote(_heap, _classIdx, page);
			}
		}

		for (uint32_t pass = 0; pass < 2; ++pass)
		{
			Page* page = _heap->classPage[_classIdx];

			if (NULL != page)
			{
				if (pageHasSpace(page) )
				{
					return takeFromPage(_heap, _classIdx, page);
				}

				_heap->classPage[_classIdx] = NULL;
				unlinkPage(_heap, page);
				linkPageTail(_heap, page);
			}

			page = _heap->classPages[_classIdx];

			if (NULL != page
			&&  pageHasSpace(page) )
			{
				if (page == _heap->classEmpty[_classIdx])
				{
					_heap->classEmpty[_classIdx] = NULL;
				}

				_heap->classPage[_classIdx] = page;
				return takeFromPage(_heap, _classIdx, page);
			}

			if (0 == pass
			&&  _classIdx < kFirstLargeClass)
			{
				flushHeap(_heap);
			}
		}

		if (_classIdx >= kFirstLargeClass)
		{
			for (uint32_t ii = _classIdx + 1; ii < kNumClasses; ++ii)
			{
				FreeNode* node = _heap->classFree[ii];

				if (NULL != node)
				{
					_heap->classFree[ii] = node->next;
					resizeLargePage(_heap, toPage(node), _classIdx);
					return node;
				}
			}
		}

		Page* page = newClassPage(_heap, _classIdx);

		if (NULL == page)
		{
			return NULL;
		}

		_heap->classPage[_classIdx] = page;

		return takeFromPage(_heap, _classIdx, page);
	}

	void ThreadSpanAllocatorBase::resizeLargePage(Heap* _heap, Page* _page, uint32_t _classIdx)
	{
		Segment*       segment     = segmentOf(_page);
		const uint32_t oldClassIdx = _page->classIdx;
		const uint32_t blockSize   = m_classSize[_classIdx];
		const uint32_t first       = _page->firstSlice;
		const uint32_t end         = first + _page->numSlices;
		const uint32_t newEnd      = first + blockSize/uint32_t(kSliceSize) + BX_CONFIG_MEMORY_TRACKING;

#if BX_CONFIG_MEMORY_TRACKING
		const uint32_t locationIdx = *locationSlot(_page, pageData(_page) );
#endif // BX_CONFIG_MEMORY_TRACKING

		if (_page == _heap->classPage[oldClassIdx])
		{
			_heap->classPage[oldClassIdx] = NULL;
		}

		if (_page == _heap->classRemote[oldClassIdx])
		{
			_heap->classRemote[oldClassIdx] = _page->nextInClass;
		}

		unlinkPage(_heap, _page);

		if (newEnd > end)
		{
			for (uint32_t ii = end; ii < newEnd; ++ii)
			{
				segment->slicePage[ii] = uint8_t(first);
			}

			segment->numUsedSlices += newEnd - end;
			segment->dirtySlices   &= ~sliceMask(end, newEnd - end);
		}
		else
		{
			for (uint32_t ii = newEnd; ii < end; ++ii)
			{
				segment->slicePage[ii] = kFreeSlice;
			}

			const int64_t now = getHPCounter();
			segment->numUsedSlices -= end - newEnd;
			segment->dirtySlices   |= sliceMask(newEnd, end - newEnd);
			segment->dirtyTime      = now;
			_heap->purgeAt          = min(_heap->purgeAt, now + m_purgeDelay);
		}

		for (uint32_t ii = first; ii < newEnd; ++ii)
		{
			segment->sliceClass[ii] = uint8_t(_classIdx);
		}

		uint8_t* data = pageData(_page);
		_page->numSlices = uint8_t(newEnd - first);
		_page->blockSize = blockSize;
		_page->classIdx  = uint16_t(_classIdx);
		_page->bump      = data + blockSize;
		_page->bumpEnd   = data + blockSize;

#if BX_CONFIG_MEMORY_TRACKING
		*locationSlot(_page, data) = locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING

		linkPageTail(_heap, _page);
	}

	bool ThreadSpanAllocatorBase::expandSpan(void* _ptr, size_t _size)
	{
		BX_ASSERT(isSpanBlock(_ptr), "Pointer %p is not a span block.", _ptr);

		if (_size <= kMaxMediumSize
		||  _size >  kMaxLargeSize)
		{
			return false;
		}

		Segment* segment = toSegment(_ptr);
		Heap*    heap    = segment->owner;
		Page*    page    = toPage(_ptr);

		if (page->classIdx < kFirstLargeClass
		||  pageData(page) != _ptr
		||  atomicLoad(&segment->ownerSelf) != threadSelf()
		||  0 != heap->abandoned)
		{
			return false;
		}

		if (_size <= page->blockSize)
		{
			return true;
		}

		const uint32_t classIdx = largeClass(_size);
		const uint32_t end      = page->firstSlice + page->numSlices;
		const uint32_t newEnd   = page->firstSlice + uint32_t(_size + kSliceSize - 1)/uint32_t(kSliceSize) + BX_CONFIG_MEMORY_TRACKING;

		if (newEnd > kNumSlices)
		{
			return false;
		}

		for (uint32_t ii = end; ii < newEnd; ++ii)
		{
			if (kFreeSlice != segment->slicePage[ii])
			{
				return false;
			}
		}

		heap->allocator->resizeLargePage(heap, page, classIdx);

		return true;
	}

	void ThreadSpanAllocatorBase::flushHeap(Heap* _heap)
	{
		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			for (FreeNode* node = _heap->classFree[ii]; NULL != node;)
			{
				FreeNode* next = node->next;
				freeOwned(_heap, toPage(node), node);
				node = next;
			}

			_heap->classFree[ii] = NULL;
		}
	}

	BX_NO_INLINE void* ThreadSpanAllocatorBase::allocSpanBlockFirst(size_t _size, size_t _alignment)
	{
		if (NULL == getTlsHeap()
		&&  NULL == createHeap() )
		{
			return NULL;
		}

		return allocSpanBlock(_size, _alignment);
	}

	void* ThreadSpanAllocatorBase::allocSpanBlock(size_t _size, size_t _alignment)
	{
		Heap* heap = getTlsHeapFast();

		if (NULL == heap) [[unlikely]]
		{
			return allocSpanBlockFirst(_size, _alignment);
		}

		if (_alignment <= Blk::kMinBlockAlignment) [[likely]]
		{
			if (_size <= kMaxMediumSize) [[likely]]
			{
				return allocClass(heap, sizeToClass(_size) );
			}

			return allocClass(heap, largeClass(_size) );
		}

		const size_t size = alignUp(max<size_t>(_size, 1), _alignment);

		if (size > kMaxMediumSize)
		{
			return allocClass(heap, largeClass(size) );
		}

		const uint32_t classIdx = sizeToClass(size);
		BX_ASSERT(0 == (m_classSize[classIdx] % _alignment), "Class %u of %u bytes doesn't hold alignment %zu.", classIdx, m_classSize[classIdx], _alignment);

		return allocClass(heap, classIdx);
	}

	void ThreadSpanAllocatorBase::freeOwned(Heap* _heap, Page* _page, void* _ptr)
	{
		FreeNode* node  = (FreeNode*)_ptr;
		FreeNode* prev  = _page->freeList;
		node->next      = prev;
		_page->freeList = node;
		++_page->numFree;
		--_page->used;

		if (_page == _heap->classPage[_page->classIdx]) [[likely]]
		{
			return;
		}

		if (0 == _page->used
		|| (NULL == prev && _page->bump >= _page->bumpEnd) )
		{
			freeOwnedSlow(_heap, _page);
		}
	}

	void ThreadSpanAllocatorBase::sweepLarge(Heap* _heap)
	{
		for (uint32_t ii = kFirstLargeClass; ii < kNumClasses; ++ii)
		{
			Page* page = _heap->classPage[ii];
			_heap->classPage[ii] = NULL;

			if (NULL != page
			&&  0 == page->used)
			{
				releasePage(_heap, page);
			}

			for (FreeNode* node = _heap->classFree[ii]; NULL != node;)
			{
				FreeNode* next = node->next;
				freeOwned(_heap, toPage(node), node);
				node = next;
			}

			_heap->classFree[ii] = NULL;

			Page* spare = _heap->classEmpty[ii];

			if (NULL != spare)
			{
				releasePage(_heap, spare);
			}
		}
	}

	BX_NO_INLINE void ThreadSpanAllocatorBase::freeOwnedSlow(Heap* _heap, Page* _page)
	{
		if (0 == _page->used)
		{
			if (NULL != _heap->classEmpty[_page->classIdx]
			||  0    != _heap->abandoned)
			{
				releasePage(_heap, _page);
				return;
			}

			_heap->classEmpty[_page->classIdx] = _page;
		}

		unlinkPage(_heap, _page);
		linkPageHead(_heap, _page);
	}

	BX_NO_INLINE void ThreadSpanAllocatorBase::drainRemote(Heap* _heap, uintptr_t _tag)
	{
		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			for (Page* page = _heap->classPages[ii]; NULL != page;)
			{
				Page* next = page->nextInClass;

				if (_tag != atomicLoad(&page->remoteFree) )
				{
					for (FreeNode* node = (FreeNode*)(atomicExchange<uintptr_t>(&page->remoteFree, _tag) & ~kRemoteAbandoned); NULL != node;)
					{
						FreeNode* nextNode = node->next;
						freeOwned(_heap, page, node);
						node = nextNode;
					}
				}

				page = next;
			}
		}
	}

	void ThreadSpanAllocatorBase::freeSpanBlock(void* _ptr)
	{
		Segment*       segment  = toSegment(_ptr);
		Heap*          heap     = segment->owner;
		const uint32_t slice    = uint32_t( (uintptr_t(_ptr) - uintptr_t(segment) )>>kSliceShift);
		const uint32_t classIdx = segment->sliceClass[slice];
		FreeNode*      node     = (FreeNode*)_ptr;

		if (atomicLoad(&segment->ownerSelf) == threadSelf() ) [[likely]]
		{
			node->next = heap->classFree[classIdx];
			heap->classFree[classIdx] = node;
			return;
		}

		freeRemote(segment, slice, node);
	}

	BX_NO_INLINE void ThreadSpanAllocatorBase::freeRemote(Segment* _segment, uint32_t _slice, FreeNode* _node)
	{
		Heap*     heap = _segment->owner;
		FreeNode* node = _node;
		Page*     page = &_segment->pages[_segment->slicePage[_slice] ];
		uintptr_t head = atomicLoad(&page->remoteFree);

		for (;;)
		{
			if (0 != (head & kRemoteAbandoned) ) [[unlikely]]
			{
				return heap->allocator->freeAbandoned(heap, page, node);
			}

			node->next = (FreeNode*)head;

			const uintptr_t prev = atomicCompareAndSwapPtr(&page->remoteFree, head, uintptr_t(node) );

			if (prev == head)
			{
				return;
			}

			head = prev;
		}
	}

	BX_NO_INLINE void ThreadSpanAllocatorBase::freeAbandoned(Heap* _heap, Page* _page, FreeNode* _node)
	{
		lockHeap(_heap);

		if (kHeapOwned == _heap->abandoned)
		{
			unlockHeap(_heap);

			Segment* segment = segmentOf(_page);
			freeRemote(segment, _page->firstSlice, _node);
			return;
		}

		freeOwned(_heap, _page, _node);
		const bool live = hasLiveSlices(_heap);

		if (!live)
		{
			atomicStore<uint32_t>(&_heap->abandoned, kHeapDying);
		}

		unlockHeap(_heap);

		if (!live)
		{
			destroyHeap(_heap);
		}
	}

	void ThreadSpanAllocatorBase::lockHeap(Heap* _heap)
	{
		while (0 != atomicCompareAndSwap<uint32_t>(&_heap->lock, 0, 1) )
		{
			yield();
		}
	}

	void ThreadSpanAllocatorBase::unlockHeap(Heap* _heap)
	{
		atomicStore<uint32_t>(&_heap->lock, 0);
	}

	bool ThreadSpanAllocatorBase::hasLiveSlices(const Heap* _heap)
	{
		for (const Segment* segment = _heap->segments; NULL != segment; segment = segment->next)
		{
			if (0 != segment->numUsedSlices)
			{
				return true;
			}
		}

		return false;
	}

	Blk ThreadSpanAllocatorBase::blkOf(void* _ptr, uint8_t _allocatorOp, size_t _alignment)
	{
		Page* page = toPage(_ptr);

		Blk blk(uint64_t(uintptr_t(_ptr) ), page->blockSize, _alignment, AllocatorOp::Enum(_allocatorOp) );
		internal::stampAllocatorHandle(blk, getHandle().idx);
#if BX_CONFIG_MEMORY_TRACKING
		blk.locationIdx = AllocatorOp::Allocate == _allocatorOp
			? 0
			: *locationSlot(page, _ptr)
			;
#endif // BX_CONFIG_MEMORY_TRACKING

		return blk;
	}

	Blk ThreadSpanAllocatorBase::toBlk(void* _ptr)
	{
		BX_ASSERT(isSpanBlock(_ptr), "Pointer %p is not a span block.", _ptr);
		return blkOf(_ptr, AllocatorOp::Noop, Blk::kMinBlockAlignment);
	}

	void* ThreadSpanAllocatorBase::fromBlk(Blk&& _blk)
	{
		void* ptr = (void*)uintptr_t(_blk.ptr);
		BX_ASSERT(isSpanBlock(ptr), "Block 0x%llx is not a span block.", _blk.ptr);

#if BX_CONFIG_MEMORY_TRACKING
		*locationSlot(toPage(ptr), ptr) = _blk.locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING

		_blk.invalidate();

		return ptr;
	}

	size_t ThreadSpanAllocatorBase::usableSize(const void* _ptr)
	{
		BX_ASSERT(isSpanBlock(_ptr), "Pointer %p is not a span block.", _ptr);
		return toPage(_ptr)->blockSize;
	}

#if BX_CONFIG_MEMORY_TRACKING
	void* ThreadSpanAllocatorBase::allocSpan(size_t _size, size_t _alignment, const Location& _location)
	{
		BX_ASSERT(isSpanRequest(_size, _alignment), "Request of %zu bytes aligned to %zu is not a span request.", _size, _alignment);

		void* ptr = allocSpanBlock(_size, _alignment);

		if (NULL == ptr) [[unlikely]]
		{
			return NULL;
		}

#if BX_CONFIG_MEMORY_TRACKING
		internal::trackBlock(getHandle().idx, 1);
#endif // BX_CONFIG_MEMORY_TRACKING

#if BX_CONFIG_MEMORY_TRACKING
		Blk blk = blkOf(ptr, AllocatorOp::Allocate, max<size_t>(_alignment, Blk::kMinBlockAlignment) );
		internal::addLocation(blk, _location, 2);
		*locationSlot(toPage(ptr), ptr) = blk.locationIdx;
		blk.invalidate();
#endif // BX_CONFIG_MEMORY_TRACKING

		BX_UNUSED(_location);

		return ptr;
	}

	void ThreadSpanAllocatorBase::freeSpan(void* _ptr, const Location& _location)
	{
		BX_ASSERT(isSpanBlock(_ptr), "Pointer %p is not a span block.", _ptr);

		ThreadSpanAllocatorBase* allocator = toSegment(_ptr)->owner->allocator;
		BX_UNUSED(allocator);

#if BX_CONFIG_MEMORY_TRACKING
		Blk blk = allocator->blkOf(_ptr, AllocatorOp::Noop, Blk::kMinBlockAlignment);
		internal::freeLocation(blk);
		blk.invalidate();
#endif // BX_CONFIG_MEMORY_TRACKING

#if BX_CONFIG_MEMORY_TRACKING
		internal::trackBlock(allocator->getHandle().idx, -1);
#endif // BX_CONFIG_MEMORY_TRACKING

		BX_UNUSED(_location);

		freeSpanBlock(_ptr);
	}
#endif // BX_CONFIG_MEMORY_TRACKING

	Blk ThreadSpanAllocatorBase::allocBlk(size_t _size, size_t _alignment)
	{
		BX_ASSERT(isSpanRequest(_size, _alignment), "Request of %zu bytes aligned to %zu is not a span request.", _size, _alignment);

		if (!isSpanRequest(_size, _alignment) )
		{
			return Blk(Blk::InvalidReason::NotSupported);
		}

		void* ptr = allocSpanBlock(max<size_t>(_size, 1), max<size_t>(_alignment, Blk::kMinBlockAlignment) );

		if (NULL == ptr)
		{
			return Blk(Blk::InvalidReason::AllocateFailed);
		}

		return blkOf(ptr, AllocatorOp::Allocate, max<size_t>(_alignment, Blk::kMinBlockAlignment) );
	}

	Blk ThreadSpanAllocatorBase::freeBlk(Blk&& _blk)
	{
		void* ptr = (void*)uintptr_t(_blk.ptr);
		BX_ASSERT(isSpanBlock(ptr), "Block 0x%llx is not a span block.", _blk.ptr);

		freeSpanBlock(ptr);
		_blk.invalidate(Blk::InvalidReason::Freed);
		_blk.allocatorOp = AllocatorOp::Free;

		return move(_blk);
	}

	Blk ThreadSpanAllocatorBase::alloc(size_t _size, size_t _alignment)
	{
		Blk blk = allocBlk(_size, _alignment);

#if BX_CONFIG_MEMORY_TRACKING
		if (isValid(blk) )
		{
			internal::trackBlock(getHandle().idx, 1);
		}
#endif // BX_CONFIG_MEMORY_TRACKING

		internal::stampAllocatorHandle(blk, getHandle().idx);

		return blk;
	}

	Blk ThreadSpanAllocatorBase::free(Blk&& _blk)
	{
#if BX_CONFIG_MEMORY_TRACKING
		internal::trackBlock(getHandle().idx, -1);
#endif // BX_CONFIG_MEMORY_TRACKING

		return freeBlk(move(_blk) );
	}

	Blk ThreadSpanAllocatorBase::reallocImpl(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate:
			return allocBlk(_size, _alignment);

		case AllocatorOp::Free:
			return freeBlk(move(_blk) );

		case AllocatorOp::Grow:
		case AllocatorOp::Truncate:
		case AllocatorOp::Realign:
			{
				void* oldPtr = (void*)uintptr_t(_blk.ptr);
				BX_ASSERT(isSpanBlock(oldPtr), "Block 0x%llx is not a span block.", _blk.ptr);

				Page* page = toPage(oldPtr);
				const size_t alignment = max<size_t>(_alignment, _blk.getAlignment() );

				if (isAligned(oldPtr, alignment)
				&& (_size <= page->blockSize || expandSpan(oldPtr, _size) ) )
				{
					Blk blk  = blkOf(oldPtr, allocatorOp, alignment);
					blk.size = _size;
#if BX_CONFIG_MEMORY_TRACKING
					blk.locationIdx = _blk.locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING
					_blk.invalidate();
					return blk;
				}

				if (!isSpanRequest(_size, alignment) )
				{
					return Blk(Blk::InvalidReason::ResizeFailed);
				}

				Blk blk = allocBlk(_size, alignment);

				if (!isValid(blk) )
				{
					return Blk(Blk::InvalidReason::ResizeFailed);
				}

				memCopy( (void*)uintptr_t(blk.ptr), oldPtr, min<size_t>(_blk.size, _size) );
				freeSpanBlock(oldPtr);

				blk.allocatorOp = allocatorOp;
#if BX_CONFIG_MEMORY_TRACKING
				blk.locationIdx = _blk.locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING
				_blk.invalidate();
				return blk;
			}

		case AllocatorOp::Noop:
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);

		default:
			break;
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	bool ThreadSpanAllocatorBase::owns(const Blk& _blk)
	{
		const void* ptr = (const void*)uintptr_t(_blk.ptr);

		if (!isSpanBlock(ptr) )
		{
			return false;
		}

		return this == toSegment(ptr)->owner->allocator;
	}

	void ThreadSpanAllocatorBase::threadTrim()
	{
		Heap* heap = getTlsHeap();

		if (NULL != heap)
		{
			trimHeap(heap);
		}
	}

	void ThreadSpanAllocatorBase::purgeHeap(Heap* _heap, int64_t _now)
	{
		int64_t purgeAt = INT64_MAX;

		for (Segment* segment = _heap->segments; NULL != segment; segment = segment->next)
		{
			if (0 == segment->dirtySlices)
			{
				continue;
			}

			if (_now - segment->dirtyTime < m_purgeDelay)
			{
				purgeAt = min(purgeAt, segment->dirtyTime + m_purgeDelay);
				continue;
			}

			for (uint64_t dirty = segment->dirtySlices; 0 != dirty;)
			{
				const uint32_t first = countTrailingZeros(dirty);
				const uint32_t num   = countTrailingZeros(~(dirty>>first) );

				Error err;
				memoryMap( (uint8_t*)segment + (size_t(first)<<kSliceShift), size_t(num)<<kSliceShift, 0, Memory::DontNeed, &err);

				dirty &= ~sliceMask(first, num);
			}

			segment->dirtySlices = 0;
		}

		_heap->purgeAt = purgeAt;
	}

	void ThreadSpanAllocatorBase::trimHeap(Heap* _heap)
	{
		drainRemote(_heap, 0);
		flushHeap(_heap);

		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			Page* page = _heap->classPage[ii];

			if (NULL != page
			&&  0 == page->used)
			{
				_heap->classPage[ii] = NULL;
				releasePage(_heap, page);
			}

			Page* spare = _heap->classEmpty[ii];

			if (NULL != spare)
			{
				releasePage(_heap, spare);
			}
		}

		for (Segment* segment = _heap->segments; NULL != segment;)
		{
			Segment* next = segment->next;

			if (0 == segment->numUsedSlices
			&&  segment != _heap->home)
			{
				freeSegment(_heap, segment);
				--_heap->numEmptySegments;
			}

			segment = next;
		}

		purgeHeap(_heap, INT64_MAX);
	}

	void ThreadSpanAllocatorBase::threadRelease()
	{
		releaseThreadHeap(getTlsHeap() );
	}

	namespace internal
	{
		void threadSpanAllocatorThreadExit(void* _heap)
		{
			ThreadSpanAllocatorBase::Heap* heap = (ThreadSpanAllocatorBase::Heap*)_heap;

			if (NULL == heap
			||  heap->allocator->m_shuttingDown)
			{
				return;
			}

			heap->allocator->releaseThreadHeap(heap);
		}

	} // namespace internal

	void ThreadSpanAllocatorBase::releaseThreadHeap(Heap* _heap)
	{
		if (NULL == _heap)
		{
			return;
		}

		setTlsHeap(NULL);
		setExitValue(m_exitKey, NULL);

		trimHeap(_heap);

		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			_heap->classPage[ii] = NULL;
		}

		if (!hasLiveSlices(_heap) )
		{
			destroyHeap(_heap);
			return;
		}

		lockHeap(_heap);
		atomicStore<uint32_t>(&_heap->abandoned, kHeapAbandoned);

		for (Segment* segment = _heap->segments; NULL != segment; segment = segment->next)
		{
			atomicStore<uintptr_t>(&segment->ownerSelf, 0);

			for (uint32_t ii = 0; ii < kNumSlices; ++ii)
			{
				volatile uintptr_t* remoteHead = &segment->pages[ii].remoteFree;

				for (;;)
				{
					const uintptr_t head = atomicLoad(remoteHead);

					if (head == atomicCompareAndSwapPtr(remoteHead, head, head | kRemoteAbandoned) )
					{
						break;
					}
				}
			}
		}

		drainRemote(_heap, kRemoteAbandoned);

		const bool live = hasLiveSlices(_heap);

		if (!live)
		{
			atomicStore<uint32_t>(&_heap->abandoned, kHeapDying);
		}

		unlockHeap(_heap);

		if (!live)
		{
			destroyHeap(_heap);
		}
	}

	AllocatorBlkI* ThreadSpanAllocatorBase::getUpstream() const
	{
		return m_upstream;
	}

	uint32_t ThreadSpanAllocatorBase::getNumHeaps() const
	{
		return m_numHeaps;
	}

	uint32_t ThreadSpanAllocatorBase::getNumSegments() const
	{
		return m_numSegments;
	}

} // namespace bx
