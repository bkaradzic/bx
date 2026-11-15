/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_BLK_H_HEADER_GUARD
#	error "Must be included from bx/blk.h!"
#endif // BX_BLK_H_HEADER_GUARD

namespace bx
{
	namespace internal
	{
		extern AllocatorBlkI* s_allocators[kMaxAllocatorHandles];

		AllocatorBlkI* getAllocatorSlow(AllocatorHandle _handle);

		void stampAllocatorHandle(Blk& _blk, uint8_t _handle);

		void addLocation(Blk& _inoutBlk, const Location& _location, uint32_t _skip = 1);

		void updateLocation(Blk& _inoutBlk, size_t _oldSize);

		void freeLocation(Blk& _inoutBlk);

		AllocatorHandle pointerAllocatorAcquire(AllocatorI* _allocator);

		void pointerAllocatorRelease(AllocatorHandle _allocator);

#if BX_CONFIG_MEMORY_TRACKING
		bool isAllocatorTracked(uint8_t _handle);

		uint32_t addLocationTracking(const Location& _location, uint32_t _skip, size_t _newSize);

		void updateLocationTracking(uint32_t _locationIdx, size_t _oldSize, size_t _newSize);

		void trackBlock(uint8_t _handle, int32_t _delta);
#endif // BX_CONFIG_MEMORY_TRACKING

	} // namespace internal

	inline constexpr uint64_t Blk::packBits(size_t _size, uint8_t _log2Alignment, bool _cpuAccessible, uint8_t _allocatorOp)
	{
		return 0
			| (uint64_t(_size)          )
			| (uint64_t(_log2Alignment) << kBitsShiftLog2Alignment)
			| (uint64_t(_cpuAccessible) << kBitsShiftCpuAccessible)
			| (uint64_t(_allocatorOp)   << kBitsShiftAllocatorOp)
			;
	}

	inline constexpr Blk::Blk()
		: ptr(kInvalid)
		, bits(0)
		, bookKeeping(0)
		, slotIndex(0)
		, unused0(0)
#if BX_CONFIG_MEMORY_TRACKING
		, locationIdx(0)
		, unused1(0)
#endif // BX_CONFIG_MEMORY_TRACKING
	{
	}

	inline constexpr Blk::Blk(
		  InvalidReason::Enum _reason
		)
		: ptr(kInvalid)
		, bits(packBits(0, uint8_t(_reason), false, 0) )
		, bookKeeping(0)
		, slotIndex(0)
		, unused0(0)
#if BX_CONFIG_MEMORY_TRACKING
		, locationIdx(0)
		, unused1(0)
#endif // BX_CONFIG_MEMORY_TRACKING
	{
		BX_ASSERT(true
			&& Blk::InvalidReason::Default    != _reason
			&& Blk::InvalidReason::NotInvalid != _reason
			, "Blk invalid reason can't be Default or NotInvalid!"
			);
	}

	inline constexpr Blk::Blk(
		  uint64_t _ptr
		, size_t _size
		, size_t _alignment
		, AllocatorOp::Enum _allocatorOp
		)
		: Blk(
			  _ptr
			, _size
			, _alignment
			, true
			, _allocatorOp
			)
	{
	}

	inline constexpr Blk::Blk(
		  uint64_t _ptr
		, size_t _size
		, size_t _alignment
		, bool _cpuAccessible
		, AllocatorOp::Enum _allocatorOp
		)
		: ptr(_ptr)
		, bits(packBits(_size, ceilLog2(_alignment), _cpuAccessible, uint8_t(_allocatorOp) ) )
		, bookKeeping(0)
		, slotIndex(0)
		, unused0(0)
#if BX_CONFIG_MEMORY_TRACKING
		, locationIdx(0)
		, unused1(0)
#endif // BX_CONFIG_MEMORY_TRACKING
	{
		BX_ASSERT(_size < kMaxBlockSize, "Blk _size %zu, but maximum can be %zu!"
			, _size
			, kMaxBlockSize
			);
		BX_ASSERT(nextPow2(_alignment) == size_t(1)<<ceilLog2(_alignment)
			, "_alignment must be power-of-tow (nextpow2 %zu, alignment %zu)!"
			, nextPow2(_alignment)
			, size_t(1)<<ceilLog2(_alignment)
			);
		BX_ASSERT(isAligned(_ptr, size_t(1)<<ceilLog2(_alignment) )
			, "Blk pointer is not aligned to required alignment (_ptr 0x%llx, alignment %zu)!"
			, _ptr
			, size_t(1)<<ceilLog2(_alignment)
			);
	}

	inline Blk::Blk(Blk&& _rhs)
		: ptr(_rhs.ptr)
		, bits(_rhs.bits)
		, bookKeeping(_rhs.bookKeeping)
		, slotIndex(_rhs.slotIndex)
		, unused0(0)
#if BX_CONFIG_MEMORY_TRACKING
		, locationIdx(_rhs.locationIdx)
		, unused1(0)
#endif // BX_CONFIG_MEMORY_TRACKING
	{
		_rhs.invalidate();
	}

	inline Blk::Blk(Blk&& _rhs, bool _cpuAccessible)
		: ptr(_rhs.ptr)
		, bits( (_rhs.bits & ~packBits(0, 0, true, 0) ) | packBits(0, 0, _cpuAccessible, 0) )
		, bookKeeping(_rhs.bookKeeping)
		, slotIndex(_rhs.slotIndex)
		, unused0(0)
#if BX_CONFIG_MEMORY_TRACKING
		, locationIdx(_rhs.locationIdx)
		, unused1(0)
#endif // BX_CONFIG_MEMORY_TRACKING
	{
		_rhs.invalidate();
	}

	inline Blk::Blk(CopyTag, const Blk& _rhs)
		: ptr(_rhs.ptr)
		, bits( (_rhs.bits & ~packBits(0, 0, false, (1<<kFieldBitsAllocatorOp)-1) ) | packBits(0, 0, false, AllocatorOp::Alias) )
		, bookKeeping(_rhs.bookKeeping)
		, slotIndex(_rhs.slotIndex)
		, unused0(0)
#if BX_CONFIG_MEMORY_TRACKING
		, locationIdx(_rhs.locationIdx)
		, unused1(0)
#endif // BX_CONFIG_MEMORY_TRACKING
	{
	}

	inline Blk& Blk::operator=(Blk&& _rhs)
	{
		if (this != &_rhs)
		{
			ptr          = _rhs.ptr;
			bits         = _rhs.bits;
			bookKeeping  = _rhs.bookKeeping;
			slotIndex    = _rhs.slotIndex;
#if BX_CONFIG_MEMORY_TRACKING
			locationIdx  = _rhs.locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING

			_rhs.invalidate();
		}

		return *this;
	}

	inline void Blk::invalidate()
	{
		ptr         = kInvalid;
		bits        = 0;
		bookKeeping = 0;
		slotIndex   = 0;
		unused0     = 0;
#if BX_CONFIG_MEMORY_TRACKING
		locationIdx = 0;
		unused1     = 0;
#endif // BX_CONFIG_MEMORY_TRACKING
	}

	inline void Blk::invalidate(InvalidReason::Enum _reason)
	{
		BX_ASSERT(true
			&& Blk::InvalidReason::Default    != _reason
			&& Blk::InvalidReason::NotInvalid != _reason
			, "Blk invalid reason can't be Default or NotInvalid!"
			);

		invalidate();
		bits = packBits(0, uint8_t(_reason), false, 0);
	}

	inline constexpr bool Blk::isLeaking() const
	{
#if BX_CONFIG_MEMORY_TRACKING
		return isValid(*this)
			&& AllocatorOp::Alias != allocatorOp
			&& 0 != locationIdx
			;
#else
		return false;
#endif // BX_CONFIG_MEMORY_TRACKING
	}

#if BX_CONFIG_MEMORY_TRACKING
	inline Blk::~Blk()
	{
		BX_ASSERT_LOC(getLocation()
			, !isLeaking()
			, "Memory leak! %s -> ptr 0x%llx, size %llu, alignment %zu"
			, getName(AllocatorOp::Enum(allocatorOp) )
			, ptr
			, uint64_t(size)
			, getAlignment()
			);
	}
#endif // BX_CONFIG_MEMORY_TRACKING

	inline Blk::InvalidReason::Enum Blk::getReason() const
	{
		if (kInvalid == ptr)
		{
			return InvalidReason::Enum(log2Alignment);
		}

		return InvalidReason::NotInvalid;
	}

	inline const Location& Blk::getLocation() const
	{
#if BX_CONFIG_MEMORY_TRACKING
		if (0 == locationIdx)
		{
			return kUnknownLocation;
		}

		return getMemoryLocation(locationIdx).location;
#else
		return kUnknownLocation;
#endif // BX_CONFIG_MEMORY_TRACKING
	}

	inline size_t Blk::getAlignment() const
	{
		return size_t(1)<<log2Alignment;
	}

	inline constexpr bool Blk::operator==(const Blk& _rhs) const
	{
		return ptr  == _rhs.ptr
			&& size == _rhs.size
			;
	}

	template<typename Ty>
	inline constexpr Blk::operator Ty*() const
	{
		BX_ASSERT(cpuAccessible, "Blk pointer is not CPU accesible!");
		return (Ty*)ptr;
	}

	inline constexpr bool isValid(const Blk& _blk)
	{
		return Blk::kInvalid != _blk.ptr;
	}

	namespace internal
	{
		inline void stampAllocatorHandle(Blk& _blk, uint8_t _handle)
		{
			constexpr uint64_t kMask = uint64_t( (1<<Blk::kFieldBitsAllocatorHandle)-1) << Blk::kBitsShiftAllocatorHandle;
			_blk.bits = (_blk.bits & ~kMask) | (uint64_t(_handle) << Blk::kBitsShiftAllocatorHandle);
		}

	} // namespace internal

	inline constexpr bool overlap(const Blk& _a, const Blk& _b)
	{
		return _a.ptr + _a.size > _b.ptr
			&& _b.ptr + _b.size > _a.ptr
			;
	}

	inline constexpr bool contain(const Blk& _a, const Blk& _b)
	{
		return _a.ptr           <= _b.ptr
			&& _a.ptr + _a.size >= _b.ptr + _b.size
			;
	}

	constexpr AllocatorHandle getAllocatorHandle(const Blk& _blk)
	{
		return { uint8_t(_blk.allocatorHandle) };
	}

	inline AllocatorHandle AllocatorBlkI::getHandle() const
	{
		return { m_handle };
	}

	inline AllocatorBlkI* getAllocator(AllocatorHandle _handle)
	{
		AllocatorBlkI* allocator = internal::s_allocators[_handle.idx];

		if (NULL == allocator) [[unlikely]]
		{
			return internal::getAllocatorSlow(_handle);
		}

		return allocator;
	}

	inline Blk AllocatorBlkI::realloc(Blk&& _blk, size_t _size, size_t _alignment)
	{
#if BX_CONFIG_MEMORY_TRACKING
		const AllocatorOp::Enum op = getAllocatorOp(_blk, _size, _alignment);
#endif // BX_CONFIG_MEMORY_TRACKING

		Blk blk = reallocImpl(move(_blk), _size, _alignment);
		internal::stampAllocatorHandle(blk, m_handle);

#if BX_CONFIG_MEMORY_TRACKING
		if (AllocatorOp::Allocate == op
		&&  isValid(blk) )
		{
			internal::trackBlock(m_handle, 1);
		}
		else if (AllocatorOp::Free == op
		     &&  !isValid(blk) )
		{
			internal::trackBlock(m_handle, -1);
		}
#endif // BX_CONFIG_MEMORY_TRACKING

		return blk;
	}

	inline AllocatorSlabI::~AllocatorSlabI()
	{
	}

	inline AllocatorOp::Enum getAllocatorOp(const Blk& _blk, size_t _size, size_t _alignment)
	{
		if (!isValid(_blk) )
		{
			if (0 == _size)
			{
				return AllocatorOp::Noop;
			}

			return AllocatorOp::Allocate;
		}

		if (0 == _size)
		{
			return AllocatorOp::Free;
		}

		if (_blk.size < _size)
		{
			return AllocatorOp::Grow;
		}

		if (_blk.size > _size)
		{
			return AllocatorOp::Truncate;
		}

		if (!isAligned(_blk.ptr, _alignment) )
		{
			return AllocatorOp::Realign;
		}

		return AllocatorOp::Noop;
	}

	inline void set(const Blk& _blk, size_t _start, size_t _end, uint8_t _value)
	{
		BX_ASSERT(_blk.cpuAccessible, "Blk must be CPU accessible to perform set operation!");

		const size_t start = min(_start, _blk.size);
		const size_t   end = min(  _end, _blk.size);
		memSet( (uint8_t*)_blk + start, _value, end-start);
	}

	namespace internal
	{
		inline void addLocation(Blk& _inoutBlk, const Location& _location, uint32_t _skip)
		{
#if BX_CONFIG_MEMORY_TRACKING
			BX_ASSERT(0 == _inoutBlk.locationIdx, "");

			if (isValid(_inoutBlk)
			&&  NULL != _location.filePath
			&&  isAllocatorTracked(uint8_t(_inoutBlk.allocatorHandle) ) )
			{
				_inoutBlk.locationIdx = addLocationTracking(_location, _skip, _inoutBlk.size);
			}
#endif // BX_CONFIG_MEMORY_TRACKING

			BX_UNUSED(_inoutBlk, _location, _skip);
		}

		inline void updateLocation(Blk& _inoutBlk, size_t _oldSize)
		{
#if BX_CONFIG_MEMORY_TRACKING
			BX_ASSERT(isValid(_inoutBlk), "");

			if (0 != _inoutBlk.locationIdx)
			{
				updateLocationTracking(_inoutBlk.locationIdx, _oldSize, _inoutBlk.size);
			}
#endif // BX_CONFIG_MEMORY_TRACKING

			BX_UNUSED(_inoutBlk, _oldSize);
		}

		inline void freeLocation(Blk& _inoutBlk)
		{
#if BX_CONFIG_MEMORY_TRACKING
			BX_ASSERT(isValid(_inoutBlk), "");

			if (0 != _inoutBlk.locationIdx)
			{
				updateLocationTracking(_inoutBlk.locationIdx, _inoutBlk.size, 0);
			}
#endif // BX_CONFIG_MEMORY_TRACKING

			BX_UNUSED(_inoutBlk);
		}

		template<typename AllocatorBlkT, typename = void>
		struct HasAllocFreeT
		{
			static constexpr bool value = false;
		};

		template<typename AllocatorBlkT>
		struct HasAllocFreeT<AllocatorBlkT, decltype(
			  void(declVal<AllocatorBlkT&>().alloc(size_t(0), size_t(0) ) )
			, void(declVal<AllocatorBlkT&>().free(declVal<Blk&&>() ) )
			)>
		{
			static constexpr bool value = true;
		};

		template<typename AllocatorBlkT>
		BX_FORCE_INLINE void stampHandle(AllocatorBlkT* _allocator, Blk& _blk)
		{
			if constexpr (!isBaseOf<AllocatorBlkI, AllocatorBlkT>() )
			{
				_blk.allocatorHandle = kAllocatorHandleDefault;
			}

			BX_UNUSED(_allocator, _blk);
		}

		template<typename AllocatorBlkT>
		BX_FORCE_INLINE Blk allocRaw(AllocatorBlkT* _allocator, size_t _size, size_t _alignment)
		{
			if constexpr (HasAllocFreeT<AllocatorBlkT>::value)
			{
				Blk blk = _allocator->alloc(_size, _alignment);
				stampHandle(_allocator, blk);
				return blk;
			}
			else
			{
				Blk blk = _allocator->realloc(Blk(), _size, _alignment);
				stampHandle(_allocator, blk);
				return blk;
			}
		}

		template<typename AllocatorBlkT>
		BX_FORCE_INLINE Blk freeRaw(AllocatorBlkT* _allocator, Blk&& _blk)
		{
			if constexpr (HasAllocFreeT<AllocatorBlkT>::value)
			{
				return _allocator->free(move(_blk) );
			}
			else
			{
				return _allocator->realloc(move(_blk), 0, 0);
			}
		}

		template<typename AllocatorBlkT>
		BX_FORCE_INLINE Blk allocSanitized(AllocatorBlkT* _allocator, size_t _size, size_t _alignment)
		{
			BX_ASSERT(0 == _alignment || isPowerOf2(_alignment)
				, "Alignment must be power of two value (_alignment: %zu)!"
				, _alignment
				);

			_size = bx::alignUp(clamp(_size, 1, Blk::kMaxBlockSize+Blk::kMinBlockSize), Blk::kMinBlockSize);

			if (Blk::kMaxBlockSize < _size) [[unlikely]]
			{
				return Blk(Blk::InvalidReason::NoBlockLargeEnough);
			}

			return allocRaw(_allocator, _size, _alignment);
		}

		BX_FORCE_INLINE void assertAlignment(const Blk& _blk, size_t _alignment)
		{
			BX_ASSERT(true
				&& _alignment <= _blk.getAlignment()
				&& isAligned(_blk.ptr, _blk.getAlignment() )
				, "Bad allocator, invalid alignment (requested: %zu, returned %zu, ptr 0x%llx)!"
				, _alignment
				, _blk.getAlignment()
				, uint64_t(_blk.ptr)
				);
			BX_UNUSED(_blk, _alignment);
		}

		BX_NO_INLINE inline void allocFailed(Error* _err)
		{
			BX_ERROR_SCOPE(_err);
			BX_ERROR_SET(_err, kErrorAllocatorNoBlockLargeEnough
				, "NoBlockLargeEnough: Allocation failed, but there are smaller blocks."
				);
		}

		BX_NO_INLINE inline void allocFailedFatal(const Location& _location, size_t _size, size_t _alignment, const Blk& _blk)
		{
			_BX_ASSERT_LOC(_location, false
				, "Failed to allocate memory (_size: %zu, _alignment %zu)! Op: %s, Reason: \"%s\""
				, _size
				, _alignment
				, getName(AllocatorOp::Enum(_blk.allocatorOp) )
				, getName(_blk.getReason() )
				);
		}

		template<typename AllocatorBlkT>
		inline Blk tryAllocNoTrack(AllocatorBlkT* _allocator, size_t _size, size_t _alignment, Error* _err)
		{
			Blk blk = allocSanitized(_allocator, _size, _alignment);

			if (!isValid(blk) ) [[unlikely]]
			{
				allocFailed(_err);
				return blk;
			}

			assertAlignment(blk, _alignment);

			return blk;
		}

		template<typename AllocatorBlkT>
		BX_FORCE_INLINE Blk reallocSanitized(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment)
		{
			BX_ASSERT(0 == _alignment || isPowerOf2(_alignment)
				, "Alignment must be power of two value (_alignment: %zu)!"
				, _alignment
				);

			_size = bx::alignUp(clamp(_size, 1, Blk::kMaxBlockSize+Blk::kMinBlockSize), Blk::kMinBlockSize);

			if (Blk::kMaxBlockSize < _size) [[unlikely]]
			{
				return Blk(Blk::InvalidReason::NoBlockLargeEnough);
			}

			Blk blk = _allocator->realloc(move(_blk), _size, _alignment);
			stampHandle(_allocator, blk);
			return blk;
		}

		template<typename AllocatorBlkT>
		BX_FORCE_INLINE Blk reallocChecked(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment, bool& _failed)
		{
			BX_ASSERT(_blk.allocatorOp != AllocatorOp::Alias
				, "realloc: Can't operate on Blk copy."
				);

			Blk copy(Copy, _blk);

			Blk blk = reallocSanitized(_allocator, move(_blk), _size, _alignment);

			_blk.invalidate();

			_failed = !isValid(blk);

			if (_failed) [[unlikely]]
			{
				blk = move(copy);
				blk.allocatorOp = AllocatorOp::Noop;
				return blk;
			}

			if (0 != _alignment
			&&  isAligned(blk.ptr, _alignment) )
			{
				blk.log2Alignment = max<uint8_t>(uint8_t(blk.log2Alignment), ceilLog2(_alignment) );
			}

			assertAlignment(blk, _alignment);

			return blk;
		}

		BX_NO_INLINE inline void reallocFailedFatal(const Location& _location, size_t _size, size_t _alignment, const Blk& _blk)
		{
			_BX_ASSERT_LOC(_location, false
				, "Failed to re-allocate memory (_size: %zu, _alignment %zu)! Op: %s, Reason: \"%s\""
				, _size
				, _alignment
				, getName(AllocatorOp::Enum(_blk.allocatorOp) )
				, getName(_blk.getReason() )
				);
		}

		BX_FORCE_INLINE void reallocLocation(Blk& _blk, bool _srcValid, const Blk& _src, const Location& _location, uint32_t _skip)
		{
			if (isValid(_blk)
			&&  AllocatorOp::Noop != _blk.allocatorOp)
			{
#if BX_CONFIG_MEMORY_TRACKING
				if (_srcValid)
				{
					_blk.locationIdx = _src.locationIdx;
					updateLocation(_blk, _src.size);
				}
				else
				{
					addLocation(_blk, _location, _skip+1);
				}
#endif // BX_CONFIG_MEMORY_TRACKING
			}

			BX_UNUSED(_blk, _srcValid, _src, _location, _skip);
		}

		template<typename AllocatorBlkT>
		inline Blk tryReallocNoTrack(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment, Error* _err)
		{
			bool failed;
			Blk blk = reallocChecked(_allocator, move(_blk), _size, _alignment, failed);

			if (failed) [[unlikely]]
			{
				allocFailed(_err);
			}

			return blk;
		}

		template<typename AllocatorBlkT>
		inline Blk freeNoTrack(AllocatorBlkT* _allocator, Blk&& _blk)
		{
			BX_ASSERT(_blk.allocatorOp != AllocatorOp::Alias
				, "freeNoTrack: Can't operate on Blk copy."
				);

			if (!isValid(_blk) )
			{
				return move(_blk);
			}

			Blk blk = freeRaw(_allocator, move(_blk) );

			_blk.invalidate();

			BX_ASSERT(!isValid(blk)
				, "Failed to free memory! Op: %s, Reason: \"%s\""
				, getName(AllocatorOp::Enum(blk.allocatorOp) )
				, getName(blk.getReason() )
				);

			return blk;
		}

		template<typename AllocatorBlkT>
		inline Blk tryAllocCopyFreeNoTrack(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment, Error* _err)
		{
			BX_ERROR_SCOPE(_err);

			if (!_allocator->isCpuAccessible() )
			{
				BX_ERROR_SET(
					  _err
					, kErrorAllocatorAllocateFailed
					, "Blk allocator is not CPU accesible! Can't perform memory copy."
					);
				return move(_blk);
			}

			const size_t alignment = 0 == _alignment ? _blk.getAlignment() : _alignment;

			Blk blk = tryAllocNoTrack(_allocator, _size, alignment, _err);

			if (!isValid(blk) )
			{
				BX_ERROR_SET(_err, kErrorAllocatorNoBlockLargeEnough
					, "NoBlockLargeEnough: Allocation failed, but there are smaller blocks."
					);
				_blk.allocatorOp = AllocatorOp::Noop;
				return move(_blk);
			}

			const size_t blkSize = _blk.size;

			memCopy( (void*)blk.ptr, (void*)_blk.ptr, min(blkSize, _size) );

			(void)freeNoTrack(_allocator, move(_blk) );

			return blk;
		}

	} // namespace internal

	template<typename AllocatorBlkT, typename>
	inline Blk tryAlloc(AllocatorBlkT* _allocator, size_t _size, size_t _alignment, Error* _err, const Location& _location, uint32_t _skip)
	{
		Blk blk = internal::tryAllocNoTrack(_allocator, _size, _alignment, _err);

		if (isValid(blk) )
		{
			internal::addLocation(blk, _location, _skip+1);
		}

		return blk;
	}

	template<typename AllocatorBlkT, typename>
	inline Blk alloc(AllocatorBlkT* _allocator, size_t _size, size_t _alignment, const Location& _location, uint32_t _skip)
	{
		Blk blk = internal::allocSanitized(_allocator, _size, _alignment);

		if (!isValid(blk) ) [[unlikely]]
		{
			internal::allocFailedFatal(_location, _size, _alignment, blk);
			return blk;
		}

		internal::assertAlignment(blk, _alignment);
		internal::addLocation(blk, _location, _skip+1);

		return blk;
	}

	template<typename AllocatorBlkT, typename>
	inline Blk tryRealloc(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment, Error* _err, const Location& _location, uint32_t _skip)
	{
		const bool inBlkValid = isValid(_blk);
		Blk copy(Copy, _blk);
		BX_UNUSED(inBlkValid);

		Blk blk = internal::tryReallocNoTrack(_allocator, move(_blk), _size, _alignment, _err);

		internal::reallocLocation(blk, inBlkValid, copy, _location, _skip+1);

		return blk;
	}

	template<typename AllocatorBlkT, typename>
	inline Blk realloc(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment, const Location& _location, uint32_t _skip)
	{
		const bool inBlkValid = isValid(_blk);
		Blk copy(Copy, _blk);
		BX_UNUSED(inBlkValid);

		bool failed;
		Blk blk = internal::reallocChecked(_allocator, move(_blk), _size, _alignment, failed);

		if (failed) [[unlikely]]
		{
			internal::reallocFailedFatal(_location, _size, _alignment, blk);
			return blk;
		}

		internal::reallocLocation(blk, inBlkValid, copy, _location, _skip+1);

		return blk;
	}

	template<typename AllocatorBlkT, typename>
	inline Blk free(AllocatorBlkT* _allocator, Blk&& _blk, const Location& _location)
	{
		if (isValid(_blk) )
		{
			internal::freeLocation(_blk);
		}

		Blk blk = internal::freeNoTrack(_allocator, move(_blk) );

		BX_UNUSED(_location);

		return blk;
	}

	template<typename AllocatorBlkT, typename>
	inline Blk tryAllocCopyFree(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment, Error* _err, const Location& _location, uint32_t _skip)
	{
		const bool inBlkValid = isValid(_blk);
		Blk copy(Copy, _blk);
		BX_UNUSED(inBlkValid);

		Blk blk = internal::tryAllocCopyFreeNoTrack(_allocator, move(_blk), _size, _alignment, _err);

		if (isValid(blk)
		&&  AllocatorOp::Noop != blk.allocatorOp)
		{
#if BX_CONFIG_MEMORY_TRACKING
			if (inBlkValid)
			{
				blk.locationIdx = copy.locationIdx;
				internal::updateLocation(blk, copy.size);
			}
			else
			{
				internal::addLocation(blk, _location, _skip+1);
			}
#endif // BX_CONFIG_MEMORY_TRACKING
		}

		BX_UNUSED(inBlkValid, _location, _skip);

		return blk;
	}

	inline Blk tryRealloc(Blk&& _blk, size_t _size, size_t _alignment, Error* _err, const Location& _location, uint32_t _skip)
	{
		BX_ASSERT(!isValid(_blk) || kAllocatorHandleDefault != _blk.allocatorHandle
			, "Block wasn't allocated through an allocator with a handle, it must be resized through its allocator!"
			);
		return tryRealloc(getAllocator(getAllocatorHandle(_blk) ), move(_blk), _size, _alignment, _err, _location, _skip+1);
	}

	inline Blk realloc(Blk&& _blk, size_t _size, size_t _alignment, const Location& _location, uint32_t _skip)
	{
		BX_ASSERT(!isValid(_blk) || kAllocatorHandleDefault != _blk.allocatorHandle
			, "Block wasn't allocated through an allocator with a handle, it must be resized through its allocator!"
			);
		return realloc(getAllocator(getAllocatorHandle(_blk) ), move(_blk), _size, _alignment, _location, _skip+1);
	}

	inline Blk free(Blk&& _blk, const Location& _location)
	{
		if (!isValid(_blk) )
		{
			return move(_blk);
		}

		BX_ASSERT(kAllocatorHandleDefault != _blk.allocatorHandle
			, "Block wasn't allocated through an allocator with a handle, it must be freed through its allocator!"
			);
		return free(getAllocator(getAllocatorHandle(_blk) ), move(_blk), _location);
	}

	inline Blk alloc(AllocatorHandle _allocator, size_t _size, size_t _alignment, const Location& _location, uint32_t _skip)
	{
		return alloc(getAllocator(_allocator), _size, _alignment, _location, _skip+1);
	}

	template<typename AllocatorBlkT, typename>
	BX_FORCE_INLINE Blk realloc(UncheckedTag, AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, const Location& _location, uint32_t _skip)
	{
		BX_ASSERT(_blk.allocatorOp != AllocatorOp::Alias
			, "realloc: Can't operate on Blk copy."
			);
		BX_ASSERT(true
			&& Blk::kMinBlockSize <= _size
			&& _size <= Blk::kMaxBlockSize
			&& isAligned(_size, Blk::kMinBlockSize)
			, "Unchecked realloc requires sanitized size (_size: %zu)!"
			, _size
			);

#if BX_CONFIG_MEMORY_TRACKING
		const bool inBlkValid = isValid(_blk);
		Blk copy(Copy, _blk);
#endif // BX_CONFIG_MEMORY_TRACKING

		Blk blk = _allocator->realloc(move(_blk), _size, 0);
		internal::stampHandle(_allocator, blk);

		BX_ASSERT(isValid(blk)
			, "Failed to re-allocate memory (_size: %zu)! Op: %s, Reason: \"%s\""
			, _size
			, getName(AllocatorOp::Enum(blk.allocatorOp) )
			, getName(blk.getReason() )
			);

#if BX_CONFIG_MEMORY_TRACKING
		_blk.invalidate();
		internal::reallocLocation(blk, inBlkValid, copy, _location, _skip+1);
#endif // BX_CONFIG_MEMORY_TRACKING

		BX_UNUSED(_location, _skip);

		return blk;
	}

	template<typename AllocatorBlkT, typename>
	BX_FORCE_INLINE Blk free(UncheckedTag, AllocatorBlkT* _allocator, Blk&& _blk, const Location& _location)
	{
		BX_ASSERT(_blk.allocatorOp != AllocatorOp::Alias
			, "free: Can't operate on Blk copy."
			);

		if (!isValid(_blk) )
		{
			return move(_blk);
		}

		internal::freeLocation(_blk);

		Blk blk = internal::freeRaw(_allocator, move(_blk) );

		BX_ASSERT(!isValid(blk)
			, "Failed to free memory! Op: %s, Reason: \"%s\""
			, getName(AllocatorOp::Enum(blk.allocatorOp) )
			, getName(blk.getReason() )
			);

#if BX_CONFIG_MEMORY_TRACKING
		_blk.invalidate();
#endif // BX_CONFIG_MEMORY_TRACKING

		BX_UNUSED(_location);

		return blk;
	}

	inline Blk realloc(UncheckedTag, Blk&& _blk, size_t _size, const Location& _location, uint32_t _skip)
	{
		BX_ASSERT(!isValid(_blk) || kAllocatorHandleDefault != _blk.allocatorHandle
			, "Block wasn't allocated through an allocator with a handle, it must be resized through its allocator!"
			);
		return realloc(Unchecked, getAllocator(getAllocatorHandle(_blk) ), move(_blk), _size, _location, _skip+1);
	}

	inline Blk free(UncheckedTag, Blk&& _blk, const Location& _location)
	{
		if (!isValid(_blk) )
		{
			return move(_blk);
		}

		BX_ASSERT(kAllocatorHandleDefault != _blk.allocatorHandle
			, "Block wasn't allocated through an allocator with a handle, it must be freed through its allocator!"
			);
		return free(Unchecked, getAllocator(getAllocatorHandle(_blk) ), move(_blk), _location);
	}

	template<typename AllocatorBlkT, typename>
	inline Blk allocCopyFree(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment, const Location& _location, uint32_t _skip)
	{
		Error err;
		Blk blk = tryAllocCopyFree(_allocator, move(_blk), _size, _alignment, &err, _location, _skip+1);
		_BX_ASSERT_LOC(
			  _location
			, err.isOk()
			, "Failed to re-allocate memory (_size: %zu, _alignment %zu)! Op: %s, Reason: \"%s\""
			, _size
			, _alignment
			, getName(AllocatorOp::Enum(blk.allocatorOp) )
			, getName(blk.getReason() )
			);
		return blk;
	}

	template<typename AllocatorBlkT>
	inline ImplementAllocatorBlkInterfaceT<AllocatorBlkT>::ImplementAllocatorBlkInterfaceT(AllocatorBlkT& _allocator)
		: m_allocator(_allocator)
	{
	}

	template<typename AllocatorBlkT>
	inline Blk ImplementAllocatorBlkInterfaceT<AllocatorBlkT>::reallocImpl(Blk&& _blk, size_t _size, size_t _alignment)
	{
		if (AllocatorOp::Noop == getAllocatorOp(_blk, _size, _alignment) )
		{
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);
		}

		return m_allocator.realloc(move(_blk), _size, _alignment);
	}

	template<typename AllocatorBlkT>
	inline bool ImplementAllocatorBlkInterfaceT<AllocatorBlkT>::owns(const Blk& _blk)
	{
		return m_allocator.owns(_blk);
	}

	template<typename AllocatorBlkT>
	inline bool ImplementAllocatorBlkInterfaceT<AllocatorBlkT>::isCpuAccessible() const
	{
		return m_allocator.isCpuAccessible();
	}

	template<typename AllocatorBlkT>
	inline AllocatorBlkInterfaceHolderT<AllocatorBlkT>::AllocatorBlkInterfaceHolderT(AllocatorBlkT& _allocator)
		: m_interface(_allocator)
	{
	}

	template<typename AllocatorSlabT>
	inline ImplementAllocatorSlabInterfaceT<AllocatorSlabT>::ImplementAllocatorSlabInterfaceT(AllocatorSlabT& _allocator)
		: m_allocator(_allocator)
	{
	}

	template<typename AllocatorSlabT>
	inline Blk ImplementAllocatorSlabInterfaceT<AllocatorSlabT>::reallocImpl(Blk&& _blk, size_t _size, size_t _alignment)
	{
		if (AllocatorOp::Noop == getAllocatorOp(_blk, _size, _alignment) )
		{
			_blk.allocatorOp = AllocatorOp::Noop;
			return move(_blk);
		}

		return m_allocator.realloc(move(_blk), _size, _alignment);
	}

	template<typename AllocatorSlabT>
	inline bool ImplementAllocatorSlabInterfaceT<AllocatorSlabT>::owns(const Blk& _blk)
	{
		return m_allocator.owns(_blk);
	}

	template<typename AllocatorSlabT>
	inline bool ImplementAllocatorSlabInterfaceT<AllocatorSlabT>::isCpuAccessible() const
	{
		return m_allocator.isCpuAccessible();
	}

	template<typename AllocatorSlabT>
	inline void ImplementAllocatorSlabInterfaceT<AllocatorSlabT>::addSlab(Blk&& _blk)
	{
		return m_allocator.addSlab(move(_blk) );
	}

	template<typename AllocatorSlabT>
	inline Blk ImplementAllocatorSlabInterfaceT<AllocatorSlabT>::removeUnusedSlab()
	{
		return m_allocator.removeUnusedSlab();
	}

	template<typename AllocatorSlabT>
	inline size_t ImplementAllocatorSlabInterfaceT<AllocatorSlabT>::getTotal() const
	{
		return m_allocator.getTotal();
	}

	template<typename AllocatorSlabT>
	inline size_t ImplementAllocatorSlabInterfaceT<AllocatorSlabT>::getUsed() const
	{
		return m_allocator.getUsed();
	}

	template<typename Ty>
	inline void deletePtr(Ty* _object, const Location& _location)
	{
		if (NULL != _object)
		{
			_object->~Ty();
			freePtr(_object, _location);
		}
	}

} // namespace bx
