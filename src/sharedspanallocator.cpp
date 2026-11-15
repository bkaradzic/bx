/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/sharedspanallocator.h>
#include <bx/cpu.h>
#include <bx/math.h>
#include <bx/os.h>
#include <bx/timer.h>

#include "spanallocator.h"

namespace bx
{
	struct SharedSpanAllocatorBase::FreeNode
	{
		FreeNode* next;
	};

	struct SharedSpanAllocatorBase::Page
	{
		FreeNode* freeList;
		uint8_t*  bump;
		uint8_t*  bumpEnd;
		Page*     next;
		Page*     prev;
		uint32_t  blockSize;
		uint32_t  used;
		uint16_t  classIdx;
		uint8_t   firstSlice;
		uint8_t   numSlices;
		uint8_t   linked;
		uint8_t   unused0[3];
	};

	struct SharedSpanAllocatorBase::Segment
	{
		Blk       blk;
		uint64_t  freeSlices;
		uint64_t  dirtySlices;
		int64_t   dirtyTime;
		Segment*  next;
		SharedSpanAllocatorBase* allocator;
		uint32_t  numUsedSlices;
		uint32_t  unused0;
		uint8_t   slicePage[kNumSlices];
		uint8_t   sliceClass[kNumSlices];
		Page      pages[kNumSlices];

		static_assert(kNumSlices <= 64, "Slice masks must hold every slice.");
	};

	struct SharedSpanAllocatorBase::ClassPool
	{
		volatile uint32_t lock;
		uint32_t  numBatches;
		Page*     partial;
		Page*     spare;
		FreeNode* batches[kMaxTransfer];
		uint8_t   unused0[2*BX_CACHE_LINE_SIZE - 2*sizeof(uint32_t) - (2 + kMaxTransfer)*sizeof(void*)];
	};

	struct SharedSpanAllocatorBase::Bin
	{
		FreeNode* head;
		uint32_t  count;
		uint32_t  max;
	};

	struct SharedSpanAllocatorBase::Heap
	{
		Heap*     next;
		Heap*     prev;
		SharedSpanAllocatorBase* allocator;
		Bin       bins[kNumClasses];
	};

	SharedSpanAllocatorBase::SegmentMap SharedSpanAllocatorBase::s_segmentMap;

	SharedSpanAllocatorBase::SharedSpanAllocatorBase(AllocatorBlkI* _upstream)
		: m_upstream(_upstream)
		, m_exitKey(createExitKey<internal::sharedSpanAllocatorThreadExit>() )
		, m_pools(NULL)
		, m_segments(NULL)
		, m_heaps(NULL)
		, m_numHeaps(0)
		, m_numSegments(0)
		, m_numEmptySegments(0)
		, m_shuttingDown(false)
		, m_purgeDelay(getHPFrequency()*kPurgeDelayMs/1000)
		, m_purgeAt(INT64_MAX)
	{
		BX_ASSERT(NULL != _upstream, "SharedSpanAllocatorBase requires an upstream allocator.");

		initSegmentMap();
		initClasses();

		m_poolsBlk = m_upstream->realloc(Blk(), sizeof(ClassPool)*kNumClasses, BX_CACHE_LINE_SIZE);
		BX_ASSERT(isValid(m_poolsBlk), "Failed to allocate class pools.");

		m_pools = (ClassPool*)uintptr_t(m_poolsBlk.ptr);
		memSet(m_pools, 0, sizeof(ClassPool)*kNumClasses);
	}

	SharedSpanAllocatorBase::~SharedSpanAllocatorBase()
	{
		m_shuttingDown = true;
		destroyExitKey(m_exitKey);

		MutexScope lock(m_mutex);

		for (Segment* segment = m_segments; NULL != segment;)
		{
			Segment* next = segment->next;

			mapSegment(segment, false);
			Blk blk = move(segment->blk);
			blk = m_upstream->realloc(move(blk), 0, 0);

			segment = next;
		}

		m_segments    = NULL;
		m_heaps       = NULL;
		m_numHeaps    = 0;
		m_numSegments = 0;

		m_poolsBlk = m_upstream->realloc(move(m_poolsBlk), 0, 0);
	}

	void SharedSpanAllocatorBase::initClasses()
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

		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			const uint32_t batch = ii < kFirstLargeClass
				? clamp(kBatchBytes/m_classSize[ii], kMinBatch, kMaxBatch)
				: 1u
				;

			m_classBatch[ii] = uint16_t(batch);
			m_classMax[ii]   = uint16_t(ii < kFirstLargeClass ? 2*batch : 0);
		}

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

	inline uint32_t SharedSpanAllocatorBase::sizeToClass(size_t _size) const
	{
		return m_classLut[(uint32_t(_size) + Blk::kMinBlockSize-1)/Blk::kMinBlockSize];
	}

	inline uint32_t SharedSpanAllocatorBase::largeClass(size_t _size)
	{
		return kFirstLargeClass + uint32_t( (_size + kSliceSize-1)>>kSliceShift) - 2;
	}

	void SharedSpanAllocatorBase::initSegmentMap()
	{
		MutexScope lock(getSegmentMapMutex<SharedSpanAllocatorBase>() );

		if (0 != s_segmentMap.limit)
		{
			return;
		}

		Error err;
		s_segmentMap.bits = (uint64_t*)memoryMap(NULL, kMapBits/8, 0, Memory::ReadWrite, &err);
		BX_ASSERT(err.isOk(), "Failed to map segment map of %zu bytes.", size_t(kMapBits/8) );

		atomicStore(&s_segmentMap.limit, kMapBits);
	}

	void SharedSpanAllocatorBase::mapSegment(const Segment* _segment, bool _present)
	{
		const uintptr_t idx = uintptr_t(_segment)>>kSegmentShift;
		BX_ASSERT(idx < kMapBits, "Segment %p is outside the mapped address range.", _segment);

		MutexScope lock(getSegmentMapMutex<SharedSpanAllocatorBase>() );

		if (_present)
		{
			atomicStore(&s_segmentMap.bits[idx>>6], s_segmentMap.bits[idx>>6] | (uint64_t(1)<<(idx&63) ) );
		}
		else
		{
			atomicStore(&s_segmentMap.bits[idx>>6], s_segmentMap.bits[idx>>6] & ~(uint64_t(1)<<(idx&63) ) );
		}
	}

	inline SharedSpanAllocatorBase::Segment* SharedSpanAllocatorBase::toSegment(const void* _ptr)
	{
		return (Segment*)(uintptr_t(_ptr) & ~(kSegmentSize-1) );
	}

	inline SharedSpanAllocatorBase::Page* SharedSpanAllocatorBase::toPage(const void* _ptr)
	{
		Segment* segment = toSegment(_ptr);
		const uint32_t slice = uint32_t( (uintptr_t(_ptr) - uintptr_t(segment) )>>kSliceShift);
		const uint8_t  first = segment->slicePage[slice];

		BX_ASSERT(first < kNumSlices, "Pointer %p doesn't point into a block.", _ptr);

		return &segment->pages[first];
	}

	inline uint8_t* SharedSpanAllocatorBase::pageData(const Page* _page)
	{
		return (uint8_t*)toSegment(_page) + (size_t(_page->firstSlice)<<kSliceShift);
	}

	inline size_t SharedSpanAllocatorBase::pageBytes(const Page* _page)
	{
		return size_t(_page->numSlices)<<kSliceShift;
	}

	inline bool SharedSpanAllocatorBase::pageHasSpace(const Page* _page)
	{
		return NULL != _page->freeList
			|| _page->bump < _page->bumpEnd
			;
	}

	void SharedSpanAllocatorBase::lockPool(ClassPool* _pool)
	{
		while (0 != atomicCompareAndSwap<uint32_t>(&_pool->lock, 0, 1) )
		{
			while (0 != atomicLoad(&_pool->lock) )
			{
				yield();
			}
		}
	}

	void SharedSpanAllocatorBase::unlockPool(ClassPool* _pool)
	{
		atomicStore<uint32_t>(&_pool->lock, 0);
	}

	void SharedSpanAllocatorBase::linkPage(ClassPool* _pool, Page* _page)
	{
		_page->prev = NULL;
		_page->next = _pool->partial;

		if (NULL != _pool->partial)
		{
			_pool->partial->prev = _page;
		}

		_pool->partial = _page;
		_page->linked  = 1;
	}

	void SharedSpanAllocatorBase::unlinkPage(ClassPool* _pool, Page* _page)
	{
		if (NULL != _page->prev)
		{
			_page->prev->next = _page->next;
		}
		else
		{
			_pool->partial = _page->next;
		}

		if (NULL != _page->next)
		{
			_page->next->prev = _page->prev;
		}

		_page->next   = NULL;
		_page->prev   = NULL;
		_page->linked = 0;
	}

#if BX_CONFIG_MEMORY_TRACKING
	inline uint32_t* SharedSpanAllocatorBase::locationSlot(Page* _page, const void* _ptr)
	{
		if (_page->classIdx >= kFirstLargeClass)
		{
			return (uint32_t*)(pageData(_page) + pageBytes(_page) ) - 1;
		}

		const size_t idx = (uintptr_t(_ptr) - uintptr_t(pageData(_page) ) )/_page->blockSize;

		return (uint32_t*)pageData(_page) + idx;
	}
#endif // BX_CONFIG_MEMORY_TRACKING

	SharedSpanAllocatorBase::Segment* SharedSpanAllocatorBase::newSegment()
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
		static_assert(sizeof(Segment) <= kSliceSize, "Segment record must fit the first slice.");

		Segment* segment = (Segment*)uintptr_t(blk.ptr);
		segment->blk           = move(blk);
		segment->freeSlices    = ~UINT64_C(1);
		segment->dirtySlices   = 0;
		segment->dirtyTime     = 0;
		segment->allocator     = this;
		segment->numUsedSlices = 0;
		segment->unused0       = 0;
		segment->next          = m_segments;

		memSet(segment->slicePage, kFreeSlice, sizeof(segment->slicePage) );
		segment->slicePage[0] = kRecordSlice;

		mapSegment(segment, true);

		m_segments = segment;
		++m_numSegments;
		++m_numEmptySegments;

		return segment;
	}

	void SharedSpanAllocatorBase::freeSegment(Segment* _segment)
	{
		for (Segment** link = &m_segments; NULL != *link; link = &(*link)->next)
		{
			if (*link == _segment)
			{
				*link = _segment->next;
				break;
			}
		}

		--m_numSegments;

		mapSegment(_segment, false);

		Blk blk = move(_segment->blk);
		blk = m_upstream->realloc(move(blk), 0, 0);
	}

	void SharedSpanAllocatorBase::purge(int64_t _now)
	{
		int64_t purgeAt = INT64_MAX;

		for (Segment* segment = m_segments; NULL != segment; segment = segment->next)
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

		m_purgeAt = purgeAt;
	}

	SharedSpanAllocatorBase::Page* SharedSpanAllocatorBase::newPage(uint32_t _numSlices)
	{
		MutexScope lock(m_mutex);

		const int64_t now = getHPCounter();

		if (now >= m_purgeAt)
		{
			purge(now);
		}

		for (uint32_t pass = 0; pass < 2; ++pass)
		{
			for (Segment* segment = m_segments; NULL != segment; segment = segment->next)
			{
				uint64_t starts = segment->freeSlices;

				for (uint32_t ii = 1; ii < _numSlices && 0 != starts; ++ii)
				{
					starts &= segment->freeSlices >> ii;
				}

				if (0 == starts)
				{
					continue;
				}

				const uint32_t first = countTrailingZeros(starts);
				const uint64_t mask  = sliceMask(first, _numSlices);

				for (uint32_t jj = first, end = first + _numSlices; jj < end; ++jj)
				{
					segment->slicePage[jj] = uint8_t(first);
				}

				if (0 == segment->numUsedSlices)
				{
					--m_numEmptySegments;
				}

				segment->numUsedSlices += _numSlices;
				segment->freeSlices    &= ~mask;
				segment->dirtySlices   &= ~mask;

				Page* page = &segment->pages[first];
				page->freeList   = NULL;
				page->bump       = NULL;
				page->bumpEnd    = NULL;
				page->next       = NULL;
				page->prev       = NULL;
				page->blockSize  = 0;
				page->used       = 0;
				page->classIdx   = 0;
				page->firstSlice = uint8_t(first);
				page->numSlices  = uint8_t(_numSlices);
				page->linked     = 0;

				return page;
			}

			if (0 == pass
			&&  NULL == newSegment() )
			{
				return NULL;
			}
		}

		return NULL;
	}

	void SharedSpanAllocatorBase::releasePage(Page* _page)
	{
		Segment* segment = toSegment(_page);

		const uint64_t mask = sliceMask(_page->firstSlice, _page->numSlices);

		for (uint32_t ii = _page->firstSlice, end = ii + _page->numSlices; ii < end; ++ii)
		{
			segment->slicePage[ii] = kFreeSlice;
		}

		const int64_t now = getHPCounter();
		segment->freeSlices  |= mask;
		segment->dirtySlices |= mask;
		segment->dirtyTime    = now;
		m_purgeAt             = min(m_purgeAt, now + m_purgeDelay);

		segment->numUsedSlices -= _page->numSlices;

		if (0 == segment->numUsedSlices)
		{
			if (m_numEmptySegments < kMaxEmptySegments)
			{
				++m_numEmptySegments;
			}
			else
			{
				freeSegment(segment);
			}
		}

		if (now >= m_purgeAt)
		{
			purge(now);
		}
	}

	SharedSpanAllocatorBase::Page* SharedSpanAllocatorBase::newClassPage(uint32_t _classIdx)
	{
		const uint32_t blockSize = m_classSize[_classIdx];
		const uint32_t numSlices = blockSize <= kMaxSmallSize   ? 1
			: _classIdx < kFirstLargeClass ? 8
			: blockSize/uint32_t(kSliceSize) + BX_CONFIG_MEMORY_TRACKING
			;

		Page* page = newPage(numSlices);

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

		Segment* segment = toSegment(page);

		for (uint32_t ii = page->firstSlice, end = ii + page->numSlices; ii < end; ++ii)
		{
			segment->sliceClass[ii] = uint8_t(_classIdx);
		}

		return page;
	}

	bool SharedSpanAllocatorBase::expandPage(Page* _page, uint32_t _classIdx)
	{
		Segment*       segment   = toSegment(_page);
		const uint32_t blockSize = m_classSize[_classIdx];
		const uint32_t numSlices = blockSize/uint32_t(kSliceSize) + BX_CONFIG_MEMORY_TRACKING;
		const uint32_t first     = _page->firstSlice;
		const uint32_t end       = first + _page->numSlices;
		const uint32_t newEnd    = first + numSlices;

		if (newEnd > kNumSlices)
		{
			return false;
		}

		const uint64_t mask = sliceMask(end, newEnd - end);

		MutexScope lock(m_mutex);

		if (mask != (segment->freeSlices & mask) )
		{
			return false;
		}

#if BX_CONFIG_MEMORY_TRACKING
		const uint32_t locationIdx = *locationSlot(_page, pageData(_page) );
#endif // BX_CONFIG_MEMORY_TRACKING

		for (uint32_t ii = end; ii < newEnd; ++ii)
		{
			segment->slicePage[ii] = uint8_t(first);
		}

		for (uint32_t ii = first; ii < newEnd; ++ii)
		{
			segment->sliceClass[ii] = uint8_t(_classIdx);
		}

		segment->numUsedSlices += newEnd - end;
		segment->freeSlices    &= ~mask;
		segment->dirtySlices   &= ~mask;

		uint8_t* data = pageData(_page);
		_page->numSlices = uint8_t(numSlices);
		_page->blockSize = blockSize;
		_page->classIdx  = uint16_t(_classIdx);
		_page->bump      = data + blockSize;
		_page->bumpEnd   = data + blockSize;

#if BX_CONFIG_MEMORY_TRACKING
		*locationSlot(_page, data) = locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING

		return true;
	}

	bool SharedSpanAllocatorBase::expandSpan(void* _ptr, size_t _size)
	{
		BX_ASSERT(isSpanBlock(_ptr), "Pointer %p is not a span block.", _ptr);

		if (_size <= kMaxMediumSize
		||  _size >  kMaxLargeSize)
		{
			return false;
		}

		Page* page = toPage(_ptr);

		if (page->classIdx < kFirstLargeClass
		||  pageData(page) != _ptr)
		{
			return false;
		}

		if (_size <= page->blockSize)
		{
			return true;
		}

		return toSegment(_ptr)->allocator->expandPage(page, largeClass(_size) );
	}

	uint32_t SharedSpanAllocatorBase::fillBatch(uint32_t _classIdx, uint32_t _num, FreeNode** _outHead)
	{
		ClassPool* pool = &m_pools[_classIdx];
		FreeNode*  head = NULL;
		uint32_t   num  = 0;

		lockPool(pool);

		if (0 != pool->numBatches
		&&  _num == m_classBatch[_classIdx])
		{
			*_outHead = pool->batches[--pool->numBatches];
			unlockPool(pool);
			return _num;
		}

		while (num < _num)
		{
			Page* page = pool->partial;

			if (NULL == page)
			{
				page = pool->spare;
				pool->spare = NULL;

				if (NULL == page)
				{
					page = newClassPage(_classIdx);

					if (NULL == page)
					{
						break;
					}
				}

				linkPage(pool, page);
			}

			while (num < _num
			&&     NULL != page->freeList)
			{
				FreeNode* node = page->freeList;
				page->freeList = node->next;
				node->next = head;
				head       = node;
				++page->used;
				++num;
			}

			while (num < _num
			&&     page->bump < page->bumpEnd)
			{
				FreeNode* node = (FreeNode*)page->bump;
				page->bump += page->blockSize;
				node->next = head;
				head       = node;
				++page->used;
				++num;
			}

			if (!pageHasSpace(page) )
			{
				unlinkPage(pool, page);
			}
		}

		unlockPool(pool);

		*_outHead = head;

		return num;
	}

	void SharedSpanAllocatorBase::returnBatch(uint32_t _classIdx, FreeNode* _head, bool _full)
	{
		ClassPool* pool     = &m_pools[_classIdx];
		Page*      released = NULL;

		lockPool(pool);

		if (_full
		&&  pool->numBatches < kMaxTransfer)
		{
			pool->batches[pool->numBatches++] = _head;
			unlockPool(pool);
			return;
		}

		for (FreeNode* node = _head; NULL != node;)
		{
			FreeNode* next = node->next;
			Page*     page = toPage(node);

			node->next     = page->freeList;
			page->freeList = node;
			--page->used;

			if (0 == page->used)
			{
				if (0 != page->linked)
				{
					unlinkPage(pool, page);
				}

				if (NULL == pool->spare
				&&  1 == page->numSlices)
				{
					pool->spare = page;
				}
				else
				{
					page->next = released;
					released   = page;
				}
			}
			else if (0 == page->linked)
			{
				linkPage(pool, page);
			}

			node = next;
		}

		unlockPool(pool);

		if (NULL != released)
		{
			MutexScope lock(m_mutex);

			for (Page* page = released; NULL != page;)
			{
				Page* next = page->next;
				releasePage(page);
				page = next;
			}
		}
	}

	SharedSpanAllocatorBase::Heap* SharedSpanAllocatorBase::createHeap()
	{
		FreeNode* node;

		if (0 == fillBatch(sizeToClass(sizeof(Heap) ), 1, &node) )
		{
			return NULL;
		}

		Heap* heap = (Heap*)node;
		memSet(heap, 0, sizeof(Heap) );
		heap->allocator = this;

		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			heap->bins[ii].max = m_classMax[ii];
		}

		{
			MutexScope lock(m_mutex);
			heap->next = m_heaps;

			if (NULL != m_heaps)
			{
				m_heaps->prev = heap;
			}

			m_heaps = heap;
			++m_numHeaps;
		}

		m_tls.set(heap);
		setExitValue(m_exitKey, heap);

		return heap;
	}

	void SharedSpanAllocatorBase::destroyHeap(Heap* _heap)
	{
		flushHeap(_heap);

		{
			MutexScope lock(m_mutex);

			if (NULL != _heap->prev)
			{
				_heap->prev->next = _heap->next;
			}
			else
			{
				m_heaps = _heap->next;
			}

			if (NULL != _heap->next)
			{
				_heap->next->prev = _heap->prev;
			}

			--m_numHeaps;
		}

		FreeNode* node = (FreeNode*)_heap;
		node->next = NULL;
		returnBatch(sizeToClass(sizeof(Heap) ), node, false);
	}

	void SharedSpanAllocatorBase::flushHeap(Heap* _heap)
	{
		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			if (NULL != _heap->bins[ii].head)
			{
				returnBatch(ii, _heap->bins[ii].head, false);
				_heap->bins[ii].head  = NULL;
				_heap->bins[ii].count = 0;
			}
		}
	}

	BX_FORCE_INLINE void* SharedSpanAllocatorBase::allocClass(Heap* _heap, uint32_t _classIdx)
	{
		FreeNode* node = _heap->bins[_classIdx].head;

		if (NULL != node) [[likely]]
		{
			_heap->bins[_classIdx].head = node->next;
			--_heap->bins[_classIdx].count;
			return node;
		}

		return allocClassSlow(_heap, _classIdx);
	}

	BX_NO_INLINE void* SharedSpanAllocatorBase::allocClassSlow(Heap* _heap, uint32_t _classIdx)
	{
		FreeNode* head;
		const uint32_t num = fillBatch(_classIdx, m_classBatch[_classIdx], &head);

		if (0 == num)
		{
			return NULL;
		}

		_heap->bins[_classIdx].head  = head->next;
		_heap->bins[_classIdx].count = num - 1;

		return head;
	}

	BX_NO_INLINE void* SharedSpanAllocatorBase::allocSpanBlockFirst(size_t _size, size_t _alignment)
	{
		if (NULL == createHeap() )
		{
			return NULL;
		}

		return allocSpanBlock(_size, _alignment);
	}

	void* SharedSpanAllocatorBase::allocSpanBlock(size_t _size, size_t _alignment)
	{
		Heap* heap = (Heap*)m_tls.get();

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

	void SharedSpanAllocatorBase::freeSpanBlock(void* _ptr)
	{
		Segment*                 segment   = toSegment(_ptr);
		SharedSpanAllocatorBase* allocator = segment->allocator;
		const uint32_t           slice     = uint32_t( (uintptr_t(_ptr) - uintptr_t(segment) )>>kSliceShift);
		const uint32_t           classIdx  = segment->sliceClass[slice];
		Heap*                    heap      = (Heap*)allocator->m_tls.get();
		FreeNode*                node      = (FreeNode*)_ptr;

		if (NULL == heap) [[unlikely]]
		{
			freeNoHeap(_ptr, classIdx);
			return;
		}

		node->next = heap->bins[classIdx].head;
		heap->bins[classIdx].head = node;

		if (++heap->bins[classIdx].count > heap->bins[classIdx].max) [[unlikely]]
		{
			freeClassSlow(heap, classIdx);
		}
	}

	BX_NO_INLINE void SharedSpanAllocatorBase::freeClassSlow(Heap* _heap, uint32_t _classIdx)
	{
		SharedSpanAllocatorBase* allocator = _heap->allocator;
		FreeNode* head = _heap->bins[_classIdx].head;

		if (0 == allocator->m_classMax[_classIdx])
		{
			_heap->bins[_classIdx].head  = NULL;
			_heap->bins[_classIdx].count = 0;
			allocator->returnBatch(_classIdx, head, false);
			return;
		}

		const uint32_t batch = allocator->m_classBatch[_classIdx];

		FreeNode* tail = head;

		for (uint32_t ii = 1; ii < batch; ++ii)
		{
			tail = tail->next;
		}

		_heap->bins[_classIdx].head   = tail->next;
		_heap->bins[_classIdx].count -= batch;
		tail->next = NULL;

		allocator->returnBatch(_classIdx, head, true);
	}

	BX_NO_INLINE void SharedSpanAllocatorBase::freeNoHeap(void* _ptr, uint32_t _classIdx)
	{
		SharedSpanAllocatorBase* allocator = toSegment(_ptr)->allocator;

		if (allocator->m_shuttingDown)
		{
			return;
		}

		FreeNode* node = (FreeNode*)_ptr;
		Heap*     heap = allocator->createHeap();

		if (NULL == heap)
		{
			node->next = NULL;
			allocator->returnBatch(_classIdx, node, false);
			return;
		}

		node->next = heap->bins[_classIdx].head;
		heap->bins[_classIdx].head = node;

		if (++heap->bins[_classIdx].count > heap->bins[_classIdx].max)
		{
			freeClassSlow(heap, _classIdx);
		}
	}

	Blk SharedSpanAllocatorBase::blkOf(void* _ptr, uint8_t _allocatorOp, size_t _alignment)
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

	Blk SharedSpanAllocatorBase::toBlk(void* _ptr)
	{
		BX_ASSERT(isSpanBlock(_ptr), "Pointer %p is not a span block.", _ptr);
		return blkOf(_ptr, AllocatorOp::Noop, Blk::kMinBlockAlignment);
	}

	void* SharedSpanAllocatorBase::fromBlk(Blk&& _blk)
	{
		void* ptr = (void*)uintptr_t(_blk.ptr);
		BX_ASSERT(isSpanBlock(ptr), "Block 0x%llx is not a span block.", _blk.ptr);

#if BX_CONFIG_MEMORY_TRACKING
		*locationSlot(toPage(ptr), ptr) = _blk.locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING

		_blk.invalidate();

		return ptr;
	}

	size_t SharedSpanAllocatorBase::usableSize(const void* _ptr)
	{
		BX_ASSERT(isSpanBlock(_ptr), "Pointer %p is not a span block.", _ptr);
		return toPage(_ptr)->blockSize;
	}

#if BX_CONFIG_MEMORY_TRACKING
	void* SharedSpanAllocatorBase::allocSpan(size_t _size, size_t _alignment, const Location& _location)
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

	void SharedSpanAllocatorBase::freeSpan(void* _ptr, const Location& _location)
	{
		BX_ASSERT(isSpanBlock(_ptr), "Pointer %p is not a span block.", _ptr);

		SharedSpanAllocatorBase* allocator = toSegment(_ptr)->allocator;
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

	Blk SharedSpanAllocatorBase::allocBlk(size_t _size, size_t _alignment)
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

	Blk SharedSpanAllocatorBase::freeBlk(Blk&& _blk)
	{
		void* ptr = (void*)uintptr_t(_blk.ptr);
		BX_ASSERT(isSpanBlock(ptr), "Block 0x%llx is not a span block.", _blk.ptr);

		freeSpanBlock(ptr);
		_blk.invalidate(Blk::InvalidReason::Freed);
		_blk.allocatorOp = AllocatorOp::Free;

		return move(_blk);
	}

	Blk SharedSpanAllocatorBase::alloc(size_t _size, size_t _alignment)
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

	Blk SharedSpanAllocatorBase::free(Blk&& _blk)
	{
#if BX_CONFIG_MEMORY_TRACKING
		internal::trackBlock(getHandle().idx, -1);
#endif // BX_CONFIG_MEMORY_TRACKING

		return freeBlk(move(_blk) );
	}

	Blk SharedSpanAllocatorBase::reallocImpl(Blk&& _blk, size_t _size, size_t _alignment)
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

	bool SharedSpanAllocatorBase::owns(const Blk& _blk)
	{
		const void* ptr = (const void*)uintptr_t(_blk.ptr);

		if (!isSpanBlock(ptr) )
		{
			return false;
		}

		return this == toSegment(ptr)->allocator;
	}

	void SharedSpanAllocatorBase::threadTrim()
	{
		Heap* heap = (Heap*)m_tls.get();

		if (NULL != heap)
		{
			flushHeap(heap);
		}

		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			ClassPool* pool = &m_pools[ii];
			FreeNode*  batches[kMaxTransfer];

			lockPool(pool);
			const uint32_t numBatches = pool->numBatches;
			memCopy(batches, pool->batches, sizeof(batches) );
			pool->numBatches = 0;
			unlockPool(pool);

			for (uint32_t jj = 0; jj < numBatches; ++jj)
			{
				returnBatch(ii, batches[jj], false);
			}
		}

		Page* spares = NULL;

		for (uint32_t ii = 0; ii < kNumClasses; ++ii)
		{
			ClassPool* pool = &m_pools[ii];

			lockPool(pool);
			Page* spare = pool->spare;
			pool->spare = NULL;
			unlockPool(pool);

			if (NULL != spare)
			{
				spare->next = spares;
				spares      = spare;
			}
		}

		MutexScope lock(m_mutex);

		for (Page* page = spares; NULL != page;)
		{
			Page* next = page->next;
			releasePage(page);
			page = next;
		}

		for (Segment* segment = m_segments; NULL != segment;)
		{
			Segment* next = segment->next;

			if (0 == segment->numUsedSlices)
			{
				freeSegment(segment);
				--m_numEmptySegments;
			}

			segment = next;
		}

		purge(INT64_MAX);
	}

	void SharedSpanAllocatorBase::threadRelease()
	{
		releaseThreadHeap( (Heap*)m_tls.get() );
	}

	namespace internal
	{
		void sharedSpanAllocatorThreadExit(void* _heap)
		{
			SharedSpanAllocatorBase::Heap* heap = (SharedSpanAllocatorBase::Heap*)_heap;

			if (NULL == heap
			||  heap->allocator->m_shuttingDown)
			{
				return;
			}

			heap->allocator->releaseThreadHeap(heap);
		}

	} // namespace internal

	void SharedSpanAllocatorBase::releaseThreadHeap(Heap* _heap)
	{
		if (NULL == _heap)
		{
			return;
		}

		m_tls.set(NULL);
		setExitValue(m_exitKey, NULL);

		destroyHeap(_heap);
	}

	AllocatorBlkI* SharedSpanAllocatorBase::getUpstream() const
	{
		return m_upstream;
	}

	uint32_t SharedSpanAllocatorBase::getNumHeaps() const
	{
		return m_numHeaps;
	}

	uint32_t SharedSpanAllocatorBase::getNumSegments() const
	{
		return m_numSegments;
	}

} // namespace bx
