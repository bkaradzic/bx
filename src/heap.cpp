/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/heap.h>
#include <bx/allocators.h>
#include <bx/mutex.h>
#include <bx/os.h>
#include <bx/sharedspanallocator.h>
#include <bx/threadspanallocator.h>
#include <bx/timer.h>

namespace bx
{
	namespace
	{
		using LargeT = MutexAllocatorT<SystemArenaAllocatorT<> >;

		/// Large blocks freed at or above `kMinPurgeSize` keep their pages until they are older than
		/// `kPurgeDelayMs`, or more than `kMaxDirtySize` bytes wait, so blocks reallocated soon
		/// after free don't fault pages in again. Smaller large blocks drop their pages when freed.
		/// Arena metadata isn't kept in free memory, pages of free ranges can be dropped at any
		/// time.
		struct LargeAllocatorI final : public AllocatorBlkI
		{
			static constexpr size_t   kMinPurgeSize = BX_CONFIG_HEAP_LARGE_PURGE_SIZE;
			static constexpr size_t   kMaxDirtySize = BX_CONFIG_HEAP_LARGE_DIRTY_SIZE;
			static constexpr uint32_t kPurgeDelayMs = BX_CONFIG_HEAP_LARGE_PURGE_DELAY_MS;
			static constexpr uint32_t kMaxDirty     = 64;

			struct DirtyRange
			{
				uintptr_t begin;
				uintptr_t end;
				int64_t   freedAt;
			};

			LargeAllocatorI(LargeT& _large)
				: m_large(_large)
				, m_pageSize(memoryPageSize() )
				, m_purgeDelay(getHPFrequency()*kPurgeDelayMs/1000)
				, m_dirtySize(0)
				, m_numDirty(0)
			{
			}

			Blk reallocImpl(Blk&& _blk, size_t _size, size_t _alignment) override
			{
				MutexScope lock(m_mutex);

				const int64_t now = getHPCounter();

				if (0 == _size
				&&  isValid(_blk) )
				{
					const uintptr_t begin   = alignUp(uintptr_t(_blk.ptr) + 1, m_pageSize);
					const uintptr_t end     = alignDown(uintptr_t(_blk.ptr) + _blk.size, m_pageSize);
					const bool      delayed = kMinPurgeSize <= _blk.size;

					Blk result = m_large.realloc(move(_blk), 0, 0);

					if (delayed)
					{
						addDirty(begin, end, now);
					}
					else if (begin < end)
					{
						purgeRange(begin, end);
					}

					purgeExpired(now);
					return result;
				}

				Blk result = m_large.realloc(move(_blk), _size, _alignment);

				if (isValid(result) )
				{
					cleanDirty(uintptr_t(result.ptr), uintptr_t(result.ptr) + result.size);
				}

				purgeExpired(now);
				return result;
			}

			bool owns(const Blk& _blk) override
			{
				return m_large.owns(_blk);
			}

			bool isCpuAccessible() const override
			{
				return true;
			}

			void purgeAll()
			{
				MutexScope lock(m_mutex);

				while (0 != m_numDirty)
				{
					purgeOldest();
				}
			}

			void purgeRange(uintptr_t _begin, uintptr_t _end)
			{
				Error err;
				memoryMap( (void*)_begin, _end - _begin, 0, Memory::DontNeed, &err);
			}

			void purgeOldest()
			{
				const DirtyRange& range = m_dirty[0];
				purgeRange(range.begin, range.end);
				m_dirtySize -= range.end - range.begin;

				--m_numDirty;
				memMove(&m_dirty[0], &m_dirty[1], m_numDirty*sizeof(DirtyRange) );
			}

			void purgeExpired(int64_t _now)
			{
				while (0 != m_numDirty
				&&     (_now - m_dirty[0].freedAt >= m_purgeDelay || kMaxDirtySize < m_dirtySize) )
				{
					purgeOldest();
				}
			}

			void addDirty(uintptr_t _begin, uintptr_t _end, int64_t _now)
			{
				if (_begin >= _end)
				{
					return;
				}

				if (kMaxDirty == m_numDirty)
				{
					purgeOldest();
				}

				m_dirty[m_numDirty++] = { _begin, _end, _now };
				m_dirtySize += _end - _begin;
			}

			/// Drops `[_begin, _end)` from dirty ranges, it's live again.
			void cleanDirty(uintptr_t _begin, uintptr_t _end)
			{
				for (uint32_t ii = 0; ii < m_numDirty;)
				{
					DirtyRange& range = m_dirty[ii];

					if (_end <= range.begin
					||  _begin >= range.end)
					{
						++ii;
						continue;
					}

					const bool keepHead = range.begin < _begin;
					const bool keepTail = _end < range.end;

					m_dirtySize -= min(range.end, _end) - max(range.begin, _begin);

					if (keepHead && keepTail)
					{
						const uintptr_t tailBegin = _end;
						const uintptr_t tailEnd   = range.end;
						range.end = _begin;

						if (kMaxDirty == m_numDirty)
						{
							purgeRange(tailBegin, tailEnd);
							m_dirtySize -= tailEnd - tailBegin;
							return;
						}

						memMove(&m_dirty[ii+2], &m_dirty[ii+1], (m_numDirty - ii - 1)*sizeof(DirtyRange) );
						m_dirty[ii+1] = { tailBegin, tailEnd, range.freedAt };
						++m_numDirty;
						return;
					}

					if (keepHead)
					{
						range.end = _begin;
						++ii;
					}
					else if (keepTail)
					{
						range.begin = _end;
						++ii;
					}
					else
					{
						--m_numDirty;
						memMove(&m_dirty[ii], &m_dirty[ii+1], (m_numDirty - ii)*sizeof(DirtyRange) );
					}
				}
			}

			LargeT&    m_large;
			Mutex      m_mutex;
			size_t     m_pageSize;
			int64_t    m_purgeDelay;
			size_t     m_dirtySize;
			uint32_t   m_numDirty;
			DirtyRange m_dirty[kMaxDirty];
		};

		struct DefaultChain
		{
			using ArenaT = SystemArenaAllocatorT<>;

			DefaultChain()
				: systemI(system)
				, large(arena)
				, largeI(large)
			{
				setAllocatorPermanent(&systemI);
				setAllocatorPermanent(&largeI);
			}

			SystemMemoryAllocator                                 system;
			ImplementAllocatorBlkInterfaceT<SystemMemoryAllocator> systemI;
			ArenaT                                                 arena;
			LargeT                                                 large;
			LargeAllocatorI                                        largeI;
		};

		struct HeapAllocatorI final : public AllocatorI
		{
			void* realloc(void* _ptr, size_t _size, size_t _align, const char* _filePath, uint32_t _line) override
			{
				const Location location(_filePath, _line);

				if (0 == _size)
				{
					bx::free(_ptr, location);
					return NULL;
				}

				if (NULL == _ptr)
				{
					return bx::alignedAlloc(_size, _align, location);
				}

				if (_align <= Blk::kMinBlockAlignment)
				{
					return bx::realloc(_ptr, _size, location);
				}

				void* ptr = bx::alignedAlloc(_size, _align, location);

				if (NULL != ptr)
				{
					memCopy(ptr, _ptr, min(_size, bx::usableSize(_ptr) ) );
					bx::free(_ptr, location);
				}

				return ptr;
			}
		};

		static_assert(ThreadSpanAllocatorBase::kMaxLargeSize == SharedSpanAllocatorBase::kMaxLargeSize, "Span request limits differ.");
		static_assert(ThreadSpanAllocatorBase::kMaxAlignment == SharedSpanAllocatorBase::kMaxAlignment, "Span request limits differ.");

		struct HeapState
		{
			HeapState(AllocatorBlkI* _large)
				: large(*_large)
				, blkI(NULL)
			{
			}

			IntrusiveBlkAllocatorT<AllocatorBlkI> large;
			AllocatorBlkI*                        blkI;
			HeapAllocatorI                        allocatorI;
		};

		template<typename SpanT>
		struct HeapStateT : public HeapState
		{
			using BlkT = ThresholdAllocatorT<SpanT::kMaxLargeSize, SpanT&, AllocatorBlkI&, SpanT::kMaxAlignment>;

			HeapStateT(AllocatorBlkI* _segments, AllocatorBlkI* _large)
				: HeapState(_large)
				, span(_segments)
				, blk(span, *_large)
				, blkImpl(blk)
			{
				blkI = &blkImpl;
			}

			SpanT                                 span;
			BlkT                                  blk;
			ImplementAllocatorBlkInterfaceT<BlkT> blkImpl;
		};

		using ThroughputState = HeapStateT<ThreadSpanAllocatorBase>;
		using FootprintState  = HeapStateT<SharedSpanAllocatorBase>;

		static constexpr size_t kBalancedSplit = BX_CONFIG_HEAP_BALANCED_SPLIT;

		struct BalancedBlk
		{
			using ThreadBlkT = ThresholdAllocatorT<ThreadSpanAllocatorBase::kMaxLargeSize, ThreadSpanAllocatorBase&, AllocatorBlkI&, ThreadSpanAllocatorBase::kMaxAlignment>;
			using SharedBlkT = ThresholdAllocatorT<SharedSpanAllocatorBase::kMaxLargeSize, SharedSpanAllocatorBase&, AllocatorBlkI&, SharedSpanAllocatorBase::kMaxAlignment>;

			BalancedBlk(ThreadSpanAllocatorBase& _thread, SharedSpanAllocatorBase& _shared, AllocatorBlkI& _large)
				: threadBlk(_thread, _large)
				, sharedBlk(_shared, _large)
			{
			}

			Blk realloc(Blk&& _blk, size_t _size, size_t _alignment)
			{
				const bool thread = isValid(_blk)
					? ThreadSpanAllocatorBase::isSpanBlock( (const void*)uintptr_t(_blk.ptr) )
					: _size <= kBalancedSplit
					;

				return thread
					? threadBlk.realloc(move(_blk), _size, _alignment)
					: sharedBlk.realloc(move(_blk), _size, _alignment)
					;
			}

			bool owns(const Blk& _blk)
			{
				return threadBlk.owns(_blk) || sharedBlk.owns(_blk);
			}

			bool isCpuAccessible() const
			{
				return true;
			}

			ThreadBlkT threadBlk;
			SharedBlkT sharedBlk;
		};

		struct BalancedState : public HeapState
		{
			BalancedState(AllocatorBlkI* _segments, AllocatorBlkI* _large)
				: HeapState(_large)
				, thread(_segments)
				, shared(_segments)
				, blk(thread, shared, *_large)
				, blkImpl(blk)
			{
				blkI = &blkImpl;
			}

			ThreadSpanAllocatorBase                      thread;
			SharedSpanAllocatorBase                      shared;
			BalancedBlk                                  blk;
			ImplementAllocatorBlkInterfaceT<BalancedBlk> blkImpl;
		};

		struct alignas(64) SpanFront
		{
			size_t                            threadLimit;
			ThreadSpanAllocatorBase* volatile thread;
			SharedSpanAllocatorBase* volatile shared;
		};

		AllocatorBlkI*      s_heapUpstream = NULL;
		Heap::Enum          s_heapSelected = Heap::Balanced;
		HeapState* volatile s_heapState    = NULL;
		SpanFront           s_front        = { 0, NULL, NULL };

		bool isSpanBlock(const void* _ptr)
		{
			return false
				|| ThreadSpanAllocatorBase::isSpanBlock(_ptr)
				|| SharedSpanAllocatorBase::isSpanBlock(_ptr)
				;
		}

		bool expandSpan(void* _ptr, size_t _size)
		{
			return ThreadSpanAllocatorBase::isSpanBlock(_ptr)
				? ThreadSpanAllocatorBase::expandSpan(_ptr, _size)
				: SharedSpanAllocatorBase::expandSpan(_ptr, _size)
				;
		}

		size_t spanUsableSize(const void* _ptr)
		{
			return ThreadSpanAllocatorBase::isSpanBlock(_ptr)
				? ThreadSpanAllocatorBase::usableSize(_ptr)
				: SharedSpanAllocatorBase::usableSize(_ptr)
				;
		}

		DefaultChain& getDefaultChain()
		{
			alignas(DefaultChain) static uint8_t s_chainStorage[sizeof(DefaultChain)];
			static DefaultChain* s_chain = BX_PLACEMENT_NEW(s_chainStorage, DefaultChain);
			return *s_chain;
		}

		/// What the heap takes from its upstream, counted: span segments, and large blocks.
		struct UpstreamStats
		{
			using StatsT = StatsAllocatorT<AllocatorBlkI>;

			UpstreamStats(AllocatorBlkI* _segments, AllocatorBlkI* _large)
				: segments(*_segments)
				, segmentsI(segments)
				, large(*_large)
				, largeI(large)
			{
				setAllocatorPermanent(&segmentsI);
				setAllocatorPermanent(&largeI);
			}

			StatsT                                 segments;
			ImplementAllocatorBlkInterfaceT<StatsT> segmentsI;
			StatsT                                  large;
			ImplementAllocatorBlkInterfaceT<StatsT> largeI;
		};

		UpstreamStats& getUpstreamStats()
		{
			alignas(UpstreamStats) static uint8_t s_statsStorage[sizeof(UpstreamStats)];
			static UpstreamStats* s_stats = BX_PLACEMENT_NEW(s_statsStorage, UpstreamStats)(
				  NULL != s_heapUpstream ? s_heapUpstream : &getDefaultChain().systemI
				, NULL != s_heapUpstream ? s_heapUpstream : &getDefaultChain().largeI
				);
			return *s_stats;
		}

		AllocatorBlkI* getSegmentUpstream()
		{
			return &getUpstreamStats().segmentsI;
		}

		AllocatorBlkI* getLargeUpstream()
		{
			return &getUpstreamStats().largeI;
		}

		HeapState* constructHeapState(uint8_t* _storage)
		{
			HeapState* state;
			size_t     threadLimit = 0;

			if (Heap::Balanced == s_heapSelected)
			{
				BalancedState* balanced = BX_PLACEMENT_NEW(_storage, BalancedState)(getSegmentUpstream(), getLargeUpstream() );
				atomicStore<ThreadSpanAllocatorBase*>(&s_front.thread, &balanced->thread);
				atomicStore<SharedSpanAllocatorBase*>(&s_front.shared, &balanced->shared);
				threadLimit = kBalancedSplit + 1;
				state       = balanced;
			}
			else if (Heap::Footprint == s_heapSelected)
			{
				FootprintState* footprint = BX_PLACEMENT_NEW(_storage, FootprintState)(getSegmentUpstream(), getLargeUpstream() );
				atomicStore<SharedSpanAllocatorBase*>(&s_front.shared, &footprint->span);
				state = footprint;
			}
			else
			{
				ThroughputState* throughput = BX_PLACEMENT_NEW(_storage, ThroughputState)(getSegmentUpstream(), getLargeUpstream() );
				atomicStore<ThreadSpanAllocatorBase*>(&s_front.thread, &throughput->span);
				threadLimit = ThreadSpanAllocatorBase::kMaxLargeSize + 1;
				state       = throughput;
			}

			setAllocatorPermanent(state->blkI);
			atomicStore<size_t>(&s_front.threadLimit, threadLimit);
			atomicStore<HeapState*>(&s_heapState, state);
			return state;
		}

		BX_NO_INLINE HeapState& createHeapState()
		{
			alignas(ThroughputState)
			alignas(FootprintState)
			alignas(BalancedState) static uint8_t s_stateStorage[max(sizeof(ThroughputState), sizeof(FootprintState), sizeof(BalancedState) )];
			static HeapState* s_state = constructHeapState(s_stateStorage);
			return *s_state;
		}

		HeapState& getHeapState()
		{
			HeapState* state = atomicLoad(&s_heapState);

			if (NULL == state) [[unlikely]]
			{
				return createHeapState();
			}

			return *state;
		}

		BX_NO_INLINE void* allocLarge(HeapState& _state, size_t _size, size_t _alignment, const Location& _location)
		{
			return _state.large.realloc(NULL, _size, _alignment, _location.filePath, _location.line);
		}

		BX_NO_INLINE void freeLarge(HeapState& _state, void* _ptr, const Location& _location)
		{
			_state.large.realloc(_ptr, 0, 0, _location.filePath, _location.line);
		}

		BX_NO_INLINE void* alignedAllocFirst(size_t _size, size_t _alignment, const Location& _location)
		{
			createHeapState();
			return internal::heapAlignedAlloc(_size, _alignment, _location);
		}

		BX_NO_INLINE void freeFirst(void* _ptr, const Location& _location)
		{
			createHeapState();
			internal::heapFree(_ptr, _location);
		}

	} // namespace

	void* internal::heapAlignedAlloc(size_t _size, size_t _alignment, const Location& _location)
	{
		if (ThreadSpanAllocatorBase::isSpanRequest(_size, _alignment) ) [[likely]]
		{
			if (_size < atomicLoad(&s_front.threadLimit) ) [[likely]]
			{
				return s_front.thread->allocSpan(_size, _alignment, _location);
			}

			SharedSpanAllocatorBase* shared = atomicLoad(&s_front.shared);

			if (NULL != shared)
			{
				return shared->allocSpan(_size, _alignment, _location);
			}

			return alignedAllocFirst(_size, _alignment, _location);
		}

		HeapState* state = atomicLoad(&s_heapState);

		if (NULL == state) [[unlikely]]
		{
			return alignedAllocFirst(_size, _alignment, _location);
		}

		return allocLarge(*state, _size, _alignment, _location);
	}

	void* internal::heapRealloc(void* _ptr, size_t _size, const Location& _location)
	{
		if (NULL == _ptr)
		{
			return malloc(_size, _location);
		}

		if (0 == _size)
		{
			free(_ptr, _location);
			return NULL;
		}

		HeapState& state = getHeapState();

		if (isSpanBlock(_ptr) )
		{
			const size_t usable = spanUsableSize(_ptr);

			if (_size <= usable
			||  expandSpan(_ptr, _size) )
			{
				return _ptr;
			}

			void* ptr = malloc(_size, _location);

			if (NULL == ptr)
			{
				return NULL;
			}

			memCopy(ptr, _ptr, usable);
			free(_ptr, _location);

			return ptr;
		}

		if (!ThreadSpanAllocatorBase::isSpanRequest(_size, 0) )
		{
			return state.large.realloc(_ptr, _size, 0, _location.filePath, _location.line);
		}

		void* ptr = malloc(_size, _location);

		if (NULL == ptr)
		{
			return NULL;
		}

		memCopy(ptr, _ptr, min(_size, state.large.usableSize(_ptr) ) );
		free(_ptr, _location);

		return ptr;
	}

	void internal::heapFree(void* _ptr, const Location& _location)
	{
		if (NULL == _ptr)
		{
			return;
		}

		if (ThreadSpanAllocatorBase::isSpanBlock(_ptr) ) [[likely]]
		{
			ThreadSpanAllocatorBase::freeSpan(_ptr, _location);
			return;
		}

		if (SharedSpanAllocatorBase::isSpanBlock(_ptr) )
		{
			SharedSpanAllocatorBase::freeSpan(_ptr, _location);
			return;
		}

		HeapState* state = atomicLoad(&s_heapState);

		if (NULL == state) [[unlikely]]
		{
			freeFirst(_ptr, _location);
			return;
		}

		freeLarge(*state, _ptr, _location);
	}

	size_t usableSize(const void* _ptr)
	{
		if (NULL == _ptr)
		{
			return 0;
		}

		HeapState& state = getHeapState();

		return isSpanBlock(_ptr)
			? spanUsableSize(_ptr)
			: state.large.usableSize(const_cast<void*>(_ptr) )
			;
	}

	void setHeap(Heap::Enum _heap)
	{
		BX_ASSERT(NULL == atomicLoad(&s_heapState), "setHeap must be called before the first allocation.");
		BX_ASSERT(_heap < Heap::Count, "Invalid heap %d.", _heap);
		s_heapSelected = _heap;
	}

	void setHeapUpstream(AllocatorBlkI* _allocator)
	{
		BX_ASSERT(NULL == atomicLoad(&s_heapState), "setHeapUpstream must be called before the first allocation.");
		s_heapUpstream = _allocator;
	}

	AllocatorBlkI* getHeapAllocator()
	{
		return getHeapState().blkI;
	}

	AllocatorHandle getHeapAllocatorHandle()
	{
		return getHeapState().blkI->getHandle();
	}

	AllocatorI* getHeapAllocatorI()
	{
		return &getHeapState().allocatorI;
	}

	HeapStats getHeapStats()
	{
		HeapStats stats;

		if (NULL != atomicLoad(&s_heapState) )
		{
			const UpstreamStats& upstream = getUpstreamStats();
			stats.segmentBytes = upstream.segments.getUsed();
			stats.numSegments  = upstream.segments.getNumBlocks();
			stats.largeBytes   = upstream.large.getUsed();
			stats.numLarge     = upstream.large.getNumBlocks();
		}

		return stats;
	}

	void heapThreadTrim()
	{
		getHeapState();

		if (NULL == s_heapUpstream)
		{
			getDefaultChain().largeI.purgeAll();
		}

		if (NULL != s_front.thread)
		{
			s_front.thread->threadTrim();
		}

		if (NULL != s_front.shared)
		{
			s_front.shared->threadTrim();
		}
	}

	void heapThreadRelease()
	{
		getHeapState();

		if (NULL != s_front.thread)
		{
			s_front.thread->threadRelease();
		}

		if (NULL != s_front.shared)
		{
			s_front.shared->threadRelease();
		}
	}

} // namespace bx
