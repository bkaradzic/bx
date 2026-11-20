/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/allocators.h>
#include <bx/hash.h>
#include <bx/hashmap.h>
#include <bx/heap.h>
#include <bx/math.h>
#include <bx/mutex.h>

#if BX_PLATFORM_IOS || BX_PLATFORM_OSX || BX_PLATFORM_VISIONOS
#	include <malloc/malloc.h>
#else
#	include <malloc.h>
#endif // BX_PLATFORM_*

namespace bx
{
	static const char* s_allocatorOpName[] =
	{
		"Noop",
		"Alias",
		"Allocate",
		"Free",
		"Grow",
		"Realign",
		"Truncate",
	};
	static_assert(AllocatorOp::Count == BX_COUNTOF(s_allocatorOpName), "");

	const char* getName(AllocatorOp::Enum _op)
	{
		BX_ASSERT(_op < AllocatorOp::Count, "Wrong AllocatorOp::Enum %d?", _op);
		return s_allocatorOpName[_op];
	}

	static const char* s_invalidReasonName[] =
	{
		"Default: Default value set by default constructor (can't be set by constructor).",
		"NotInvalid: Block is not invalid (can't be set by constructor).",
		"Noop: Operation resulted in no-op.",
		"Freed: Block was freed.",
		"AllocateFailed: Allocation failed.",
		"NoBlockLargeEnough: Allocation failed, but there are smaller blocks.",
		"ResizeFailed: Allocator failed to resize block.",
		"RealignFailed: Allocator failed to realign block.",
		"NotSupported: Functionality is not supported.",
		"NotImplemented: Functionality is not implemented.",
	};
	static_assert(Blk::InvalidReason::Count == BX_COUNTOF(s_invalidReasonName), "");

	const char* getName(Blk::InvalidReason::Enum _reason)
	{
		BX_ASSERT(_reason < Blk::InvalidReason::Count, "Wrong Blk::InvalidReason::Enum %d?", _reason);
		return s_invalidReasonName[_reason];
	}

	namespace internal
	{
		AllocatorBlkI* s_allocators[kMaxAllocatorHandles];

	} // namespace internal

	using internal::s_allocators;
	static uint64_t s_allocatorSlots[(kMaxAllocatorHandles+63)/64] = { 1 };
	static uint64_t s_allocatorPermanent[(kMaxAllocatorHandles+63)/64];
	static uint32_t s_allocatorNext = kAllocatorHandleDefault + 1;

#if BX_CONFIG_MEMORY_TRACKING
	static uint64_t s_allocatorUntracked[(kMaxAllocatorHandles+63)/64];
	static volatile int32_t s_numLiveBlocks[kMaxAllocatorHandles];

	namespace internal
	{
		bool isAllocatorTracked(uint8_t _handle)
		{
			return 0 == (s_allocatorUntracked[_handle/64] & (uint64_t(1)<<(_handle%64) ) );
		}

		void trackBlock(uint8_t _handle, int32_t _delta)
		{
			atomicFetchAndAdd<int32_t>(&s_numLiveBlocks[_handle], _delta);
		}

	} // namespace internal
#endif // BX_CONFIG_MEMORY_TRACKING

	static Mutex& getAllocatorSlotsMutex()
	{
		static Mutex s_mutex;
		return s_mutex;
	}

	static bool isAllocatorBitSet(const uint64_t* _bits, uint8_t _handle)
	{
		return 0 != (_bits[_handle/64] & (uint64_t(1)<<(_handle%64) ) );
	}

	static uint32_t allocatorSlotFind(uint32_t _from)
	{
		for (uint32_t ii = _from/64; ii < BX_COUNTOF(s_allocatorSlots); ++ii)
		{
			const uint32_t numSlots = min<uint32_t>(kMaxAllocatorHandles - ii*64, 64);
			const uint64_t valid    = 64 == numSlots ? UINT64_MAX : (uint64_t(1)<<numSlots) - 1;
			const uint64_t from     = ii == _from/64 ? UINT64_MAX<<(_from%64) : UINT64_MAX;
			const uint64_t free     = ~s_allocatorSlots[ii] & valid & from;

			if (0 != free)
			{
				return ii*64 + uint32_t(countTrailingZeros(free) );
			}
		}

		return UINT32_MAX;
	}

	AllocatorBlkI::AllocatorBlkI()
	{
		MutexScope scope(getAllocatorSlotsMutex() );

		uint32_t idx = allocatorSlotFind(s_allocatorNext);

		if (UINT32_MAX == idx)
		{
			idx = allocatorSlotFind(kAllocatorHandleDefault + 1);
		}

		if (UINT32_MAX != idx)
		{
			s_allocatorSlots[idx/64] |= uint64_t(1)<<(idx%64);
			s_allocatorNext = idx + 1;
		}

		BX_ASSERT(kAllocatorHandleDefault != idx
			&&  idx < kMaxAllocatorHandles
			, "Out of allocator handles (%d live or permanent)! Reset allocators instead of recreating them, or raise BX_CONFIG_MAX_ALLOCATOR_HANDLES."
			, kMaxAllocatorHandles-1
			);

		if (idx >= kMaxAllocatorHandles)
		{
			debugBreak();
			m_handle = kAllocatorHandleDefault;
			return;
		}

		m_handle = uint8_t(idx);
		atomicExchangePtr( (void**)&s_allocators[m_handle], this);
	}

	AllocatorBlkI::~AllocatorBlkI()
	{
		if (this == s_allocators[kAllocatorHandleDefault])
		{
			atomicExchangePtr( (void**)&s_allocators[kAllocatorHandleDefault], NULL);
		}

		if (kAllocatorHandleDefault == m_handle)
		{
			return;
		}

#if BX_CONFIG_MEMORY_TRACKING
		BX_ASSERT(0 == s_numLiveBlocks[m_handle]
			, "Allocator %d destroyed with %d live blocks!"
			, m_handle
			, s_numLiveBlocks[m_handle]
			);
		s_numLiveBlocks[m_handle] = 0;
#endif // BX_CONFIG_MEMORY_TRACKING

		atomicExchangePtr( (void**)&s_allocators[m_handle], NULL);

		MutexScope scope(getAllocatorSlotsMutex() );

		if (isAllocatorBitSet(s_allocatorPermanent, m_handle) )
		{
			return;
		}

		s_allocatorSlots[m_handle/64] &= ~(uint64_t(1)<<(m_handle%64) );
#if BX_CONFIG_MEMORY_TRACKING
		s_allocatorUntracked[m_handle/64] &= ~(uint64_t(1)<<(m_handle%64) );
#endif // BX_CONFIG_MEMORY_TRACKING
	}

	AllocatorBlkI* setAllocatorTracking(AllocatorBlkI* _allocator, bool _enabled)
	{
#if BX_CONFIG_MEMORY_TRACKING
		const uint8_t  handle = _allocator->getHandle().idx;
		const uint64_t bit    = uint64_t(1)<<(handle%64);

		MutexScope scope(getAllocatorSlotsMutex() );

		if (_enabled)
		{
			s_allocatorUntracked[handle/64] &= ~bit;
		}
		else
		{
			s_allocatorUntracked[handle/64] |= bit;
		}
#endif // BX_CONFIG_MEMORY_TRACKING

		BX_UNUSED(_enabled);
		return _allocator;
	}

	AllocatorBlkI* setAllocatorPermanent(AllocatorBlkI* _allocator)
	{
		const uint8_t handle = _allocator->getHandle().idx;
		BX_ASSERT(kAllocatorHandleDefault != handle, "Allocator without a handle can't be permanent!");

		MutexScope scope(getAllocatorSlotsMutex() );
		s_allocatorPermanent[handle/64] |= uint64_t(1)<<(handle%64);

		return _allocator;
	}

	bool isAllocatorPermanent(AllocatorHandle _handle)
	{
		return isAllocatorBitSet(s_allocatorPermanent, _handle.idx);
	}

	AllocatorBlkI* getDefaultAllocatorBlk()
	{
		AllocatorBlkI* allocator = s_allocators[kAllocatorHandleDefault];

		if (NULL == allocator)
		{
			allocator = getHeapAllocator();
			atomicExchangePtr( (void**)&s_allocators[kAllocatorHandleDefault], allocator);
		}

		return allocator;
	}

	void setDefaultAllocatorBlk(AllocatorBlkI* _allocator)
	{
		atomicExchangePtr( (void**)&s_allocators[kAllocatorHandleDefault], _allocator);
	}

	namespace internal
	{
		AllocatorBlkI* getAllocatorSlow(AllocatorHandle _handle)
		{
			BX_ASSERT(kAllocatorHandleDefault == _handle.idx, "Allocator %d was destroyed, or was never created!", _handle.idx);

			return kAllocatorHandleDefault == _handle.idx
				? getDefaultAllocatorBlk()
				: NULL
				;
		}

	} // namespace internal

	CrtAllocator::CrtAllocator()
	{
	}

	CrtAllocator::~CrtAllocator()
	{
	}

	static constexpr size_t   kCrtNaturalAlignment = BX_ARCH_64BIT ? 16 : 8;
	static constexpr uint32_t kCrtBookKeepingAligned = 1;

	static void* crtAlloc(size_t _size, bool _aligned, size_t _alignment)
	{
		if (!_aligned)
		{
			return ::malloc(_size);
		}

#if BX_PLATFORM_WINDOWS || BX_PLATFORM_WINRT || BX_PLATFORM_XBOXONE
		return ::_aligned_malloc(_size, _alignment);
#else
		void* ptr = NULL;
		return 0 == ::posix_memalign(&ptr, _alignment, _size) ? ptr : NULL;
#endif // BX_PLATFORM_WINDOWS || BX_PLATFORM_WINRT || BX_PLATFORM_XBOXONE
	}

	static size_t crtSize(void* _ptr, size_t _size)
	{
#if BX_PLATFORM_IOS || BX_PLATFORM_OSX || BX_PLATFORM_VISIONOS
		const size_t size = ::malloc_size(_ptr);
#elif BX_PLATFORM_ANDROID || BX_PLATFORM_EMSCRIPTEN || BX_PLATFORM_LINUX
		const size_t size = ::malloc_usable_size(_ptr);
#else
		BX_UNUSED(_ptr);
		const size_t size = _size;
#endif // BX_PLATFORM_*

		BX_ASSERT(size >= _size, "CRT reports %zu usable bytes for block of %zu bytes!", size, _size);
		return max(_size, alignDown(size, size_t(Blk::kMinBlockSize) ) );
	}

	static void crtFree(void* _ptr, bool _aligned)
	{
		if (!_aligned)
		{
			::free(_ptr);
			return;
		}

#if BX_PLATFORM_WINDOWS || BX_PLATFORM_WINRT || BX_PLATFORM_XBOXONE
		::_aligned_free(_ptr);
#else
		::free(_ptr);
#endif // BX_PLATFORM_WINDOWS || BX_PLATFORM_WINRT || BX_PLATFORM_XBOXONE
	}

	static Blk crtAllocBlk(uint8_t _handle, size_t _size, size_t _alignment)
	{
		const bool aligned = _alignment > kCrtNaturalAlignment;
		void* ptr = crtAlloc(_size, aligned, _alignment);

		if (NULL == ptr)
		{
			return Blk(Blk::InvalidReason::AllocateFailed);
		}

		Blk blk(uint64_t(uintptr_t(ptr) ), crtSize(ptr, _size), _alignment, AllocatorOp::Allocate);
		blk.bookKeeping     = aligned ? kCrtBookKeepingAligned : 0;
		internal::stampAllocatorHandle(blk, _handle);
		return blk;
	}

	static Blk crtFreeBlk(Blk&& _blk)
	{
		crtFree( (void*)uintptr_t(_blk.ptr), 0 != (_blk.bookKeeping & kCrtBookKeepingAligned) );
		_blk.invalidate();
		return Blk(Blk::InvalidReason::Freed);
	}

	Blk CrtAllocator::alloc(size_t _size, size_t _alignment)
	{
		Blk blk = crtAllocBlk(getHandle().idx, _size, _alignment);
#if BX_CONFIG_MEMORY_TRACKING
		if (isValid(blk) )
		{
			internal::trackBlock(getHandle().idx, 1);
		}
#endif // BX_CONFIG_MEMORY_TRACKING
		return blk;
	}

	Blk CrtAllocator::free(Blk&& _blk)
	{
#if BX_CONFIG_MEMORY_TRACKING
		internal::trackBlock(getHandle().idx, -1);
#endif // BX_CONFIG_MEMORY_TRACKING
		return crtFreeBlk(move(_blk) );
	}

	Blk CrtAllocator::reallocImpl(Blk&& _blk, size_t _size, size_t _alignment)
	{
		void* ptr;

		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate:
			return crtAllocBlk(getHandle().idx, _size, _alignment);

		case AllocatorOp::Free:
			return crtFreeBlk(move(_blk) );

		case AllocatorOp::Grow:
		case AllocatorOp::Truncate:
		case AllocatorOp::Realign:
			{
				const bool   oldAligned = 0 != (_blk.bookKeeping & kCrtBookKeepingAligned);
				const size_t alignment  = max(_alignment, _blk.getAlignment() );
				const bool   aligned    = alignment > kCrtNaturalAlignment;

				if (!oldAligned
				&&  !aligned)
				{
					ptr = ::realloc( (void*)_blk.ptr, _size);
				}
				else
				{
					ptr = crtAlloc(_size, aligned, alignment);

					if (NULL != ptr)
					{
						memCopy(ptr, (const void*)_blk.ptr, min<size_t>(_blk.size, _size) );
						crtFree( (void*)_blk.ptr, oldAligned);
					}
				}

				if (NULL == ptr)
				{
					return Blk(Blk::InvalidReason::ResizeFailed);
				}

				Blk blk(uint64_t(ptr), crtSize(ptr, _size), alignment, allocatorOp);
				blk.bookKeeping = aligned ? kCrtBookKeepingAligned : 0;
				return blk;
			}

		case AllocatorOp::Noop:
			return move(_blk);

		default:
			BX_ASSERT(false, "You should not be here!");
			break;
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	bool CrtAllocator::owns(const Blk& _blk)
	{
		BX_UNUSED(_blk);
		return true;
	}

	bool CrtAllocator::isCpuAccessible() const
	{
		return true;
	}

	PointerAllocator::PointerAllocator(AllocatorI* _allocator, size_t _naturalAlignment)
		: m_allocator(_allocator)
		, m_naturalAlignment(_naturalAlignment)
	{
	}

	PointerAllocator::~PointerAllocator()
	{
	}

	struct PtrHeader
	{
		Blk    blk;
		size_t offset;
	};

	static PtrHeader* getPtrHeader(void* _ptr)
	{
		return (PtrHeader*)( (uint8_t*)_ptr - sizeof(PtrHeader) );
	}

	static void* setPtrHeader(Blk&& _blk, size_t _offset)
	{
		uint8_t* ptr = (uint8_t*)_blk + _offset;
		PtrHeader* header = BX_PLACEMENT_NEW(getPtrHeader(ptr), PtrHeader);
		header->blk    = move(_blk);
		header->offset = _offset;

		return ptr;
	}

	void* allocPtr(AllocatorBlkI* _allocator, size_t _size, size_t _alignment, const Location& _location)
	{
		const size_t alignment = max<size_t>(_alignment, Blk::kMinBlockAlignment);
		const size_t offset    = alignUp(sizeof(PtrHeader), alignment);

		return setPtrHeader(alloc(_allocator, offset + _size, alignment, _location), offset);
	}

	void* reallocPtr(AllocatorBlkI* _allocator, void* _ptr, size_t _size, size_t _alignment, const Location& _location)
	{
		if (NULL == _ptr)
		{
			return 0 == _size ? NULL : allocPtr(_allocator, _size, _alignment, _location);
		}

		if (0 == _size)
		{
			freePtr(_ptr, _location);
			return NULL;
		}

		PtrHeader* header = getPtrHeader(_ptr);
		const size_t offset = header->offset;

		Blk blk(move(header->blk) );
		header->~PtrHeader();

		return setPtrHeader(realloc(move(blk), offset + _size, 0, _location), offset);
	}

	void freePtr(void* _ptr, const Location& _location)
	{
		if (NULL == _ptr)
		{
			return;
		}

		PtrHeader* header = getPtrHeader(_ptr);

		Blk blk(move(header->blk) );
		header->~PtrHeader();

		(void)free(move(blk), _location);
	}

	static constexpr uint32_t kMaxSharedPointerAllocators = 32;

	struct SharedPointerAllocator
	{
		alignas(PointerAllocator) uint8_t storage[sizeof(PointerAllocator)];
		AllocatorI* allocator;
		uint32_t    numRefs;
	};

	static SharedPointerAllocator s_sharedPointerAllocators[kMaxSharedPointerAllocators];

	static Mutex& getSharedPointerAllocatorsMutex()
	{
		static Mutex s_mutex;
		return s_mutex;
	}

	namespace internal
	{
		AllocatorHandle pointerAllocatorAcquire(AllocatorI* _allocator)
		{
			if (getHeapAllocatorI() == _allocator)
			{
				return getHeapAllocatorHandle();
			}

			MutexScope scope(getSharedPointerAllocatorsMutex() );

			SharedPointerAllocator* unused = NULL;

			for (uint32_t ii = 0; ii < kMaxSharedPointerAllocators; ++ii)
			{
				SharedPointerAllocator& shared = s_sharedPointerAllocators[ii];

				if (0 == shared.numRefs)
				{
					unused = NULL == unused ? &shared : unused;
				}
				else if (_allocator == shared.allocator)
				{
					++shared.numRefs;
					return ( (PointerAllocator*)shared.storage)->getHandle();
				}
			}

			BX_ASSERT(NULL != unused
				, "Out of shared pointer allocators (%d live)! Pass AllocatorHandle instead of AllocatorI."
				, kMaxSharedPointerAllocators
				);

			if (NULL == unused)
			{
				return getDefaultAllocatorBlk()->getHandle();
			}

			unused->allocator = _allocator;
			unused->numRefs   = 1;

			return (BX_PLACEMENT_NEW(unused->storage, PointerAllocator)(_allocator) )->getHandle();
		}

		void pointerAllocatorRelease(AllocatorHandle _allocator)
		{
			const uintptr_t offset = uintptr_t(getAllocator(_allocator) ) - uintptr_t(s_sharedPointerAllocators);

			if (offset >= sizeof(s_sharedPointerAllocators) )
			{
				return;
			}

			MutexScope scope(getSharedPointerAllocatorsMutex() );

			SharedPointerAllocator& shared = s_sharedPointerAllocators[offset / sizeof(SharedPointerAllocator)];

			if (0 == --shared.numRefs)
			{
				( (PointerAllocator*)shared.storage)->~PointerAllocator();
			}
		}

	} // namespace internal

	Blk PointerAllocator::reallocImpl(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate:
			{
				const size_t alignment = _alignment > m_naturalAlignment ? _alignment : 0;
				void* ptr = bx::alloc(m_allocator, _size, alignment);

				if (NULL == ptr)
				{
					return Blk(Blk::InvalidReason::AllocateFailed);
				}

				Blk blk(uint64_t(uintptr_t(ptr) ), _size, _alignment, allocatorOp);
				blk.bookKeeping = uint32_t(alignment);
				return blk;
			}

		case AllocatorOp::Free:
			bx::free(m_allocator, (void*)uintptr_t(_blk.ptr), _blk.bookKeeping);
			_blk.invalidate();
			return Blk(Blk::InvalidReason::Freed);

		case AllocatorOp::Grow:
		case AllocatorOp::Truncate:
		case AllocatorOp::Realign:
			{
				const size_t blkAlignment = max(_alignment, _blk.getAlignment() );
				const size_t oldAlignment = _blk.bookKeeping;
				const size_t alignment    = blkAlignment > m_naturalAlignment ? blkAlignment : 0;

				void* ptr;

				if (alignment == oldAlignment)
				{
					ptr = bx::realloc(m_allocator, (void*)uintptr_t(_blk.ptr), _size, alignment);
				}
				else
				{
					ptr = bx::alloc(m_allocator, _size, alignment);

					if (NULL != ptr)
					{
						memCopy(ptr, (const void*)uintptr_t(_blk.ptr), min<size_t>(_blk.size, _size) );
						bx::free(m_allocator, (void*)uintptr_t(_blk.ptr), oldAlignment);
					}
				}

				if (NULL == ptr)
				{
					return Blk(Blk::InvalidReason::ResizeFailed);
				}

				Blk blk(uint64_t(uintptr_t(ptr) ), _size, blkAlignment, allocatorOp);
				blk.bookKeeping = uint32_t(alignment);
				return blk;
			}

		case AllocatorOp::Noop:
			return move(_blk);

		default:
			BX_ASSERT(false, "You should not be here!");
			break;
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	bool PointerAllocator::owns(const Blk& _blk)
	{
		BX_UNUSED(_blk);
		return true;
	}

	bool PointerAllocator::isCpuAccessible() const
	{
		return true;
	}

	SystemMemoryAllocator::SystemMemoryAllocator()
		: m_pageSize(memoryPageSize() )
	{
		BX_ASSERT(isPowerOf2(m_pageSize), "m_pageSize is not power of two! %zu", m_pageSize);
	}

	SystemMemoryAllocator::~SystemMemoryAllocator()
	{
	}

	Blk SystemMemoryAllocator::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		void* ptr;
		Error err;

		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate:
			{
				const size_t alignment = max(_alignment, m_pageSize);
				const size_t slack     = alignment > m_pageSize ? alignment : 0;

				_size = alignUp(max(_size, kTraits.minBlockSize), m_pageSize);
				ptr = memoryMap(NULL, _size + slack, 0, Memory::ReadWrite, &err);

				if (!err.isOk() )
				{
					return Blk(Blk::InvalidReason::AllocateFailed);
				}

				const uintptr_t aligned = alignUp(uintptr_t(ptr), alignment);

				Blk blk(uint64_t(aligned), _size, alignment, allocatorOp);
				blk.bookKeeping = uint32_t( (aligned - uintptr_t(ptr) )/m_pageSize);
				return blk;
			}

		case AllocatorOp::Free:
			{
				const size_t alignment = _blk.getAlignment();
				const size_t slack     = alignment > m_pageSize ? alignment : 0;
				const size_t offset    = size_t(_blk.bookKeeping)*m_pageSize;

				memoryUnmap( (uint8_t*)uintptr_t(_blk.ptr) - offset, _blk.size + slack, &err);
				_blk.invalidate();
				return Blk(Blk::InvalidReason::Freed);
			}

		case AllocatorOp::Noop:
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);

		default:
			break;
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	bool SystemMemoryAllocator::owns(const Blk& _blk)
	{
		BX_UNUSED(_blk);
		return true;
	}

	bool SystemMemoryAllocator::isCpuAccessible() const
	{
		return kTraits.blocksAreCpuAccessible;
	}

	PageReserveAllocator::PageReserveAllocator(size_t _reserveSize)
		: m_pageSize(memoryPageSize() )
		, m_granularity(memoryAllocationGranularity() )
		, m_reserveSize(alignUp(max<size_t>(_reserveSize, 1), memoryAllocationGranularity() ) )
	{
		BX_ASSERT(isPowerOf2(m_pageSize), "m_pageSize is not power of two! %zu", m_pageSize);
		BX_ASSERT(isPowerOf2(m_granularity), "m_granularity is not power of two! %zu", m_granularity);
	}

	PageReserveAllocator::~PageReserveAllocator()
	{
	}

	Blk PageReserveAllocator::allocBlock(size_t _size, size_t _alignment, AllocatorOp::Enum _allocatorOp)
	{
		if (_alignment > m_granularity)
		{
			return Blk(Blk::InvalidReason::NotSupported);
		}

		const size_t reserve = max(m_reserveSize, alignUp(_size, m_granularity) );
		const size_t commit  = alignUp(max(_size, size_t(kTraits.minBlockSize) ), m_pageSize);

		Error err;
		void* ptr = memoryMap(NULL, reserve, 0, Memory::Reserve, &err);

		if (!err.isOk() )
		{
			return Blk(Blk::InvalidReason::AllocateFailed);
		}

		memoryMap(ptr, commit, 0, Memory::ReadWrite, &err);

		if (!err.isOk() )
		{
			memoryUnmap(ptr, reserve, &err);
			return Blk(Blk::InvalidReason::AllocateFailed);
		}

		Blk blk(uint64_t(uintptr_t(ptr) ), _size, max(_alignment, m_pageSize), _allocatorOp);
		blk.bookKeeping = uint32_t(reserve/m_granularity);
		return blk;
	}

	Blk PageReserveAllocator::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate:
			return allocBlock(_size, _alignment, allocatorOp);

		case AllocatorOp::Free:
			{
				Error err;
				memoryUnmap( (void*)uintptr_t(_blk.ptr), size_t(_blk.bookKeeping)*m_granularity, &err);
				_blk.invalidate(Blk::InvalidReason::Freed);
				_blk.allocatorOp = AllocatorOp::Free;
				return move(_blk);
			}

		case AllocatorOp::Grow:
		case AllocatorOp::Truncate:
		case AllocatorOp::Realign:
			{
				uint8_t*     ptr     = (uint8_t*)uintptr_t(_blk.ptr);
				const size_t reserve = size_t(_blk.bookKeeping)*m_granularity;
				const size_t used    = alignUp(max(size_t(_blk.size), size_t(kTraits.minBlockSize) ), m_pageSize);
				const size_t commit  = alignUp(max(_size, size_t(kTraits.minBlockSize) ), m_pageSize);

				if (commit <= reserve
				&&  isAligned(ptr, max(_alignment, _blk.getAlignment() ) ) )
				{
					Error err;

					if (commit > used)
					{
						memoryMap(ptr + used, commit - used, 0, Memory::ReadWrite, &err);
					}
					else if (commit < used)
					{
						memoryMap(ptr + commit, used - commit, 0, Memory::Decommit, &err);
					}

					if (!err.isOk() )
					{
						return Blk(Blk::InvalidReason::ResizeFailed);
					}

					_blk.size        = _size;
					_blk.allocatorOp = allocatorOp;
					return move(_blk);
				}

				Blk blk = allocBlock(_size, max(_alignment, _blk.getAlignment() ), allocatorOp);

				if (!isValid(blk) )
				{
					return Blk(Blk::InvalidReason::ResizeFailed);
				}

				memCopy( (void*)uintptr_t(blk.ptr), ptr, min<size_t>(_blk.size, _size) );
#if BX_CONFIG_MEMORY_TRACKING
				blk.locationIdx = _blk.locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING

				Error err;
				memoryUnmap(ptr, reserve, &err);
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

	bool PageReserveAllocator::owns(const Blk& _blk)
	{
		BX_UNUSED(_blk);
		return true;
	}

	bool PageReserveAllocator::isCpuAccessible() const
	{
		return kTraits.blocksAreCpuAccessible;
	}

	size_t PageReserveAllocator::getReserveSize() const
	{
		return m_reserveSize;
	}

	DebugStompAllocator::DebugStompAllocator(Mode::Enum _mode, size_t _reserveSize)
		: m_mode(_mode)
		, m_pageSize(memoryPageSize() )
		, m_reserveSize(alignUp(_reserveSize, m_pageSize) )
		, m_cursor(0)
		, m_end(0)
		, m_reservations(NULL)
	{
		BX_ASSERT(isPowerOf2(m_pageSize), "m_pageSize is not power of two! %zu", m_pageSize);
	}

	DebugStompAllocator::~DebugStompAllocator()
	{
		Reservation* r = m_reservations;
		while (NULL != r)
		{
			Reservation* next = r->next;
			const uintptr_t base = r->base;
			const size_t    size = r->size;
			Error err;
			memoryUnmap( (void*)base, size, &err);
			r = next;
		}
	}

	Blk DebugStompAllocator::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
		const AllocatorOp::Enum allocatorOp = getAllocatorOp(_blk, _size, _alignment);

		switch (allocatorOp)
		{
		case AllocatorOp::Allocate:
			return doAlloc(_size, _alignment);

		case AllocatorOp::Free:
			doFree(_blk);
			_blk.invalidate();
			return Blk(Blk::InvalidReason::Freed);

		case AllocatorOp::Noop:
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);

		case AllocatorOp::Grow:
		case AllocatorOp::Truncate:
		case AllocatorOp::Realign:
		case AllocatorOp::Alias:
			{
				Blk newBlk = doAlloc(_size, _alignment);

				if (Blk::kInvalid == newBlk.ptr)
				{
					return move(newBlk);
				}

				const size_t copySize = min<size_t>(_blk.size, _size);
				memCopy( (void*)newBlk.ptr, (const void*)_blk.ptr, copySize);

				doFree(_blk);
				_blk.invalidate();

				newBlk.allocatorOp = allocatorOp;
				return move(newBlk);
			}

		default:
			break;
		}

		return Blk(Blk::InvalidReason::NotImplemented);
	}

	bool DebugStompAllocator::owns(const Blk& _blk)
	{
		for (Reservation* r = m_reservations; NULL != r; r = r->next)
		{
			if (contain(Blk(uint64_t(r->base), r->size, 0, AllocatorOp::Alias), _blk) )
			{
				return true;
			}
		}
		return false;
	}

	bool DebugStompAllocator::isCpuAccessible() const
	{
		return kTraits.blocksAreCpuAccessible;
	}

	Blk DebugStompAllocator::doAlloc(size_t _size, size_t _alignment)
	{
		const size_t alignment   = max<size_t>(_alignment, Blk::kMinBlockAlignment);
		const size_t userSize    = max<size_t>(_size, size_t(1) );
		const size_t alignedSize = alignUp(userSize, alignment);
		const size_t headerSize  = alignUp(sizeof(Header), alignment);
		const size_t payloadSize = alignUp(alignedSize + headerSize, m_pageSize);
		const size_t totalSize   = payloadSize + m_pageSize;
		const size_t padSize     = alignment > m_pageSize ? alignment - m_pageSize : 0;

		const size_t alignOffset = Mode::Overrun == m_mode ? payloadSize : m_pageSize;

		uintptr_t fullPtr = alignUp(m_cursor + alignOffset, alignment) - alignOffset;

		if (fullPtr + totalSize > m_end)
		{
			const size_t reserveSize = max<size_t>(m_reserveSize, totalSize + padSize + m_pageSize);
			Error err;
			void* reserved = memoryMap(NULL, reserveSize, 0, Memory::Reserve | Memory::ProtectNone, &err);

			if (!err.isOk()
			||  NULL == reserved)
			{
				return Blk(Blk::InvalidReason::AllocateFailed);
			}

			memoryMap(reserved, m_pageSize, 0, Memory::Commit | Memory::ProtectReadWrite, &err);

			if (!err.isOk() )
			{
				Error releaseErr;
				memoryUnmap(reserved, reserveSize, &releaseErr);
				return Blk(Blk::InvalidReason::AllocateFailed);
			}

			Reservation* r = (Reservation*)reserved;
			r->next        = m_reservations;
			r->base        = uintptr_t(reserved);
			r->size        = reserveSize;
			m_reservations = r;

			m_cursor = uintptr_t(reserved) + m_pageSize;
			m_end    = uintptr_t(reserved) + reserveSize;

			fullPtr = alignUp(m_cursor + alignOffset, alignment) - alignOffset;

			if (fullPtr + totalSize > m_end)
			{
				return Blk(Blk::InvalidReason::AllocateFailed);
			}
		}

		m_cursor = fullPtr + totalSize;

		uintptr_t userPtr;
		Error err;

		if (Mode::Overrun == m_mode)
		{
			memoryMap( (void*)fullPtr, payloadSize, 0, Memory::Commit | Memory::ProtectReadWrite, &err);

			if (!err.isOk() )
			{
				return Blk(Blk::InvalidReason::AllocateFailed);
			}

			userPtr = fullPtr + payloadSize - alignedSize;
		}
		else // Underrun
		{
			memoryMap( (void*)(fullPtr + m_pageSize), payloadSize, 0, Memory::Commit | Memory::ProtectReadWrite, &err);

			if (!err.isOk() )
			{
				return Blk(Blk::InvalidReason::AllocateFailed);
			}

			userPtr = fullPtr + m_pageSize + headerSize;
		}

		Header* hdr = (Header*)(userPtr - sizeof(Header) );
		hdr->base     = fullPtr;
		hdr->fullSize = totalSize;
		hdr->userSize = userSize;
		hdr->sentinel = kSentinel;

		return Blk(uint64_t(userPtr), userSize, alignment, AllocatorOp::Allocate);
	}

	void DebugStompAllocator::doFree(const Blk& _blk)
	{
		const uintptr_t userPtr = uintptr_t(_blk.ptr);
		const Header*   hdr     = (const Header*)(userPtr - sizeof(Header) );

		BX_ASSERT(kSentinel == hdr->sentinel
			, "DebugStompAllocator: header sentinel corrupt - buffer underrun?"
			);

		Error err;
		if (Mode::Overrun == m_mode)
		{
			memoryMap( (void*)hdr->base, hdr->fullSize - m_pageSize, 0, Memory::Decommit | Memory::ProtectNone, &err);
		}
		else
		{
			memoryMap( (void*)(hdr->base + m_pageSize), hdr->fullSize - m_pageSize, 0, Memory::Decommit | Memory::ProtectNone, &err);
		}
	}

	struct MemoryTracker
	{
		static constexpr size_t kReserveSize = 1_MiB;

		PageReserveAllocator m_pageAllocator;
		ImplementAllocatorBlkInterfaceT<PageReserveAllocator> m_allocator;

		HashMapT<uint64_t, uint32_t> m_locationMap;
		HandleTableT<MemoryLocation> m_memLocation;

		MemoryLocation m_global;

		Mutex m_mutex;
		bool m_init = false;

		MemoryTracker()
			: m_pageAllocator(kReserveSize)
			, m_allocator(m_pageAllocator)
			, m_locationMap(m_allocator.getHandle(), 0)
			, m_memLocation(m_allocator.getHandle(), 0)
			, m_init(true)
		{
		}

		~MemoryTracker()
		{
			dump();
			m_init = false;
		}

		void touch(MemoryLocation& _memLoc, int8_t _numBlocks, ptrdiff_t _numBytes)
		{
			if (0 > _numBlocks)
			{
				BX_ASSERT(
					  _memLoc.numBlocks >= size_t(-_numBlocks)
					, "%zu, %d -> %s %d"
					, _memLoc.numBlocks
					, -_numBlocks
					, _memLoc.location.filePath
					, _memLoc.location.line
					);
			}

			_memLoc.numBlocks += _numBlocks;
			_memLoc.maxBlocks  = max(_memLoc.maxBlocks, _memLoc.numBlocks);

			_memLoc.bytesAllocated   += size_t(_numBytes);
			_memLoc.maxBytesAllocated = max(_memLoc.maxBytesAllocated, _memLoc.bytesAllocated);

			++_memLoc.numOps;
		}

		struct ReentryGuard
		{
			ReentryGuard()
			{
				s_insideTracker = true;
			}

			~ReentryGuard()
			{
				s_insideTracker = false;
			}
		};

		static BX_THREAD_LOCAL bool s_insideTracker;

		uint32_t addLocationTracking(const Location& _location, uint32_t _skip, size_t _newSize)
		{
			if (s_insideTracker)
			{
				return 0;
			}

			ReentryGuard guard;

			_skip += 1;

			MemoryLocation memLoc =
			{
				.location  = _location,
				.numBlocks = 1,
				.maxBlocks = 1,
				.bytesAllocated    = _newSize,
				.maxBytesAllocated = _newSize,
				.numOps = 1,
				.numCallstackFrames = 0,
				.callstack = {}
			};

			memLoc.numCallstackFrames = getCallStackFast(_skip, BX_COUNTOF(memLoc.callstack), memLoc.callstack);

			bx::HashMurmur3_64 murmur;
			murmur.begin(_location.line);
			murmur.add(uintptr_t(_location.filePath) );
			murmur.add(memLoc.callstack, sizeof(uintptr_t)*memLoc.numCallstackFrames);
			const uint64_t hash = murmur.end();

			bx::MutexScope scope(m_mutex);

			const uint32_t* memLocIdx = m_locationMap.find(hash);

			uint32_t idx = 0;

			if (NULL != memLocIdx)
			{
				idx = *memLocIdx;

				updateLocationTracking(idx, 0, _newSize);
			}
			else
			{
				idx = m_memLocation.add(memLoc);

				m_locationMap.add(hash, idx);

				touch(m_global, 1, ptrdiff_t(_newSize) );
			}

			return idx;
		}

		void updateLocationTracking(uint32_t _locationIdx, size_t _oldSize, size_t _newSize)
		{
			const ptrdiff_t numBytes = ptrdiff_t(_newSize - _oldSize);
			const int8_t numBlocks = int8_t( (_newSize > 0) - (_oldSize > 0) );

			BX_ASSERT(0 != _locationIdx, "");

			bx::MutexScope scope(m_mutex);

			MemoryLocation& memLoc = m_memLocation[_locationIdx];
			touch(memLoc, numBlocks, numBytes);
			touch(m_global, numBlocks, numBytes);
		}

		void dump(bool _all = false)
		{
			MutexScope scope(m_mutex);

			if (true
			&&  0 == m_global.numBlocks
			&&  !_all)
			{
				return;
			}

			debugPrintf("\n--- MEMORY TRACKER LEAKS ---\n\n");

			uint32_t numLines        = 0;
			size_t   numLeaks        = 0;
			size_t   numBlocksLeaked = 0;
			size_t   bytesLeaked     = 0;

			for (const auto iter : m_locationMap)
			{
				const MemoryLocation& memLoc = m_memLocation[iter.getValue()];

				const bool leak  = 0 < memLoc.numBlocks;
				const bool error = !leak && (0 < memLoc.bytesAllocated);

				if (leak
				||  error
				||  _all)
				{
					++numLines;

					debugPrintf("%s(%d): %zu blocks (max %zu), %zu bytes (max %zu), num ops: %zu%s\n"
						, memLoc.location.filePath
						, memLoc.location.line
						, memLoc.numBlocks
						, memLoc.maxBlocks
						, memLoc.bytesAllocated
						, memLoc.maxBytesAllocated
						, memLoc.numOps
						, error ? "   <--- ERROR!" : (leak ? "   <--- LEAK" : "")
						);

					if (leak)
					{
						writeCallstack(getDebugOut(), memLoc.callstack, memLoc.numCallstackFrames, ErrorIgnore{});
						debugPrintf("\n");

						++numLeaks;
						numBlocksLeaked += memLoc.numBlocks;
						bytesLeaked     += memLoc.bytesAllocated;
					}
				}
			}

			if (0 != numLines)
			{
				debugPrintf("\n");
			}

			debugPrintf("\t     Location leaks: %zu / %u\n"
				, numLeaks
				, m_locationMap.getCount()
				);

			debugPrintf("\tTotal blocks leaked: %zu / %zu\n", numBlocksLeaked, m_global.maxBlocks);
			debugPrintf("\t Total bytes leaked: %zu / %zu\n", bytesLeaked, m_global.maxBytesAllocated);

			debugPrintf("\n--- END ---\n\n");

			BX_ASSERT(true
				&& numBlocksLeaked == m_global.numBlocks
				&& bytesLeaked     == m_global.bytesAllocated
				, "%zu / %zu, %zu / %zu\n"
				, m_global.numBlocks
				, m_global.maxBlocks
				, m_global.bytesAllocated
				, m_global.maxBytesAllocated
				);
		}
	};

	BX_THREAD_LOCAL bool MemoryTracker::s_insideTracker = false;

	static MemoryTracker& getMemoryTracker()
	{
		static MemoryTracker s_memoryTracker;
		return s_memoryTracker;
	}

#if BX_CONFIG_MEMORY_TRACKING
#	if BX_COMPILER_MSVC
	static void memoryTrackerInit()
	{
		getMemoryTracker();
	}

#	pragma section(".CRT$XCL", read)
	__declspec(allocate(".CRT$XCL") ) static void (*s_memoryTrackerInit)() = memoryTrackerInit;
#	else
	__attribute__( (constructor(101) ) ) static void memoryTrackerInit()
	{
		getMemoryTracker();
	}
#	endif // BX_COMPILER_MSVC
#endif // BX_CONFIG_MEMORY_TRACKING

	namespace internal
	{
		uint32_t addLocationTracking(const Location& _location, uint32_t _skip, size_t _newSize)
		{
			MemoryTracker& tracker = getMemoryTracker();

			if (!tracker.m_init)
			{
				return 0;
			}

			return tracker.addLocationTracking(_location, _skip+1, _newSize);
		}

		void updateLocationTracking(uint32_t _locationIdx, size_t _oldSize, size_t _newSize)
		{
			MemoryTracker& tracker = getMemoryTracker();

			if (!tracker.m_init)
			{
				return;
			}

			tracker.updateLocationTracking(_locationIdx, _oldSize, _newSize);
		}

	} // namespace internal

	uint32_t getNumMemoryLocations()
	{
		return getMemoryTracker().m_memLocation.getCount();
	}

	const MemoryLocation& getMemoryLocation(uint32_t _locationIdx)
	{
		if (0 == _locationIdx)
		{
			return getMemoryTracker().m_global;
		}

		return getMemoryTracker().m_memLocation[_locationIdx];
	}

} // namespace bx
