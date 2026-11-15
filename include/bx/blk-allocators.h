/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_BLK_ALLOCATORS_H_HEADER_GUARD
#define BX_BLK_ALLOCATORS_H_HEADER_GUARD

#include <bx/blk.h>
#include <bx/array.h>
#include <bx/bitarray.h>
#include <bx/handletable.h>
#include <bx/mutex.h>
#include <bx/os.h>
#include <bx/sort.h>
#include <bx/thread.h>
#include <bx/uniqueptr.h>

namespace bx
{
	/// Orders blocks by address.
	///
	/// @param[in] _lhs Pointer to first `Blk`.
	/// @param[in] _rhs Pointer to second `Blk`.
	///
	/// @returns Negative, zero or positive when `_lhs` is lower, equal or higher.
	///
	template<>
	int32_t compareAscending<Blk>(const void* _lhs, const void* _rhs);

	/// Block with its use state, as sub-allocators keep them.
	struct Range
	{
		BX_CLASS(Range, TRIVIALLY_RELOCATABLE);

		Blk  blk;   //!< Memory block.
		bool inUse; //!< True if block is allocated.
	};

	/// Orders ranges by block address.
	///
	/// @param[in] _lhs Pointer to first `Range`.
	/// @param[in] _rhs Pointer to second `Range`.
	///
	/// @returns Negative, zero or positive when `_lhs` is lower, equal or higher.
	///
	template<>
	int32_t compareAscending<Range>(const void* _lhs, const void* _rhs);

	/// Allocator wrapper that clears memory of freed and resized blocks.
	///
	/// @tparam AllocatorBlkT Wrapped allocator type, must be CPU accessible.
	///
	template<typename AllocatorBlkT>
	class SecureAllocatorT final
	{
	public:
		SecureAllocatorT() = delete;

		/// Constructs wrapper around `_allocator`.
		///
		/// @param[in] _allocator Wrapped allocator, must be CPU accessible.
		///
		explicit SecureAllocatorT(AllocatorBlkT& _allocator);

		/// Destroys wrapper.
		~SecureAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		AllocatorBlkT* m_allocator;
	};

	/// Allocator that hands out single block of fixed capacity from inline storage.
	///
	/// @tparam CapacityT Capacity in bytes.
	/// @tparam AlignT Alignment of storage in bytes.
	///
	template<uint32_t CapacityT, uint32_t AlignT = BX_CACHE_LINE_SIZE>
	class FixedCapacityAllocatorT final
	{
	public:
		/// Constructs allocator.
		FixedCapacityAllocatorT();

		/// Destroys allocator.
		~FixedCapacityAllocatorT() = default;

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		BX_ALIGN_DECL(AlignT, uint8_t) m_data[CapacityT];
		bool m_used;
	};

	/// Allocator that allocates from primary allocator, and from secondary when primary is out of memory.
	///
	/// @tparam PrimaryT Primary allocator type.
	/// @tparam SecondaryT Secondary allocator type.
	///
	template<typename PrimaryT, typename SecondaryT>
	class OverflowAllocatorT final
	{
	public:
		/// Constructs allocator.
		OverflowAllocatorT();

		/// Destroys allocator.
		~OverflowAllocatorT() = default;

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		Blk reallocInternal(Blk&& _blk, size_t _size, size_t _alignment);

		PrimaryT   m_primary;
		SecondaryT m_secondary;
		bool       m_cpuAccessible;
	};

	/// Allocator that routes requests up to `ThresholdT` bytes to primary allocator, and the rest to secondary.
	///
	/// @tparam ThresholdT Maximum size, rounded up to alignment, served by primary allocator.
	/// @tparam PrimaryT Allocator type, or reference type to use allocator passed to constructor.
	/// @tparam SecondaryT Allocator type, or reference type to use allocator passed to constructor.
	/// @tparam MaxAlignmentT Maximum alignment served by primary allocator.
	///
	template<size_t ThresholdT, typename PrimaryT, typename SecondaryT, size_t MaxAlignmentT = ThresholdT>
	class ThresholdAllocatorT final
	{
	public:
		/// Constructs allocator with default constructed allocators.
		ThresholdAllocatorT();

		/// Constructs allocator from `_primary` and `_secondary` allocators.
		///
		/// @param[in] _primary Allocator for requests up to `ThresholdT` bytes.
		/// @param[in] _secondary Allocator for larger requests.
		///
		ThresholdAllocatorT(PrimaryT _primary, SecondaryT _secondary);

		/// Destroys allocator.
		~ThresholdAllocatorT() = default;

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

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

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		static bool fitsPrimary(size_t _size, size_t _alignment);
		Blk reallocInternal(Blk&& _blk, size_t _size, size_t _alignment);

		PrimaryT   m_primary;
		SecondaryT m_secondary;
		bool       m_cpuAccessible;
	};

	/// First-fit allocator over slabs added with `addSlab`.
	///
	/// @tparam MinBlockSizeT Minimum block size in bytes, at least `Blk::kMinBlockSize`.
	/// @tparam CpuAccessibleT True if slabs are CPU accessible.
	///
	template<uint32_t MinBlockSizeT = BX_CACHE_LINE_SIZE, bool CpuAccessibleT = false>
	class FirstFitAllocatorT final
	{
	public:
		static_assert(MinBlockSizeT >= Blk::kMinBlockSize, "MinBlockSizeT must be at least bx::Blk::kMinBlockSize");

		/// Allocator traits.
		static constexpr AllocatorTraits kTraits
		{
			MinBlockSizeT,           // minBlockSize
			Blk::kMinBlockAlignment, // minBlockAlignment
			CpuAccessibleT,          // blocksAreCpuAccessible
			false,                   // threadSafe
		};

		/// Constructs allocator.
		///
		/// @param[in] _metaDataAllocator Allocator for metadata.
		///
		explicit FirstFitAllocatorT(AllocatorBlkI* _metaDataAllocator);

		FirstFitAllocatorT() = delete;

		/// Destroys allocator.
		~FirstFitAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

		/// Adds slab of memory to allocate from.
		///
		/// @param[in] _blk Slab.
		///
		void addSlab(Blk&& _blk);

		/// Removes slab that is entirely free and returns it.
		///
		/// @returns Removed slab, invalid block when there is no unused slab.
		///
		Blk removeUnusedSlab();

		/// Returns total number of bytes in all slabs.
		///
		/// @returns Total number of bytes in all slabs.
		///
		size_t getTotal() const;

		/// Returns number of bytes used.
		///
		/// @returns Number of bytes used.
		///
		size_t getUsed() const;

		/// Frees all blocks.
		void reset();

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

	private:
		using RangeArray = SortedArrayT<Range>;
		using FreeBlocks = SortedArrayT<Blk>;

		Blk allocFreeBlocks(size_t _size, size_t _alignment);
		uint32_t findRange(const Blk& _blk) const;
		void remove(FreeBlocks& _free, const Blk* _it);

		RangeArray m_range;
		FreeBlocks m_free;
		size_t     m_total;
		size_t     m_used;
	};

	/// Allocator over single region, tracking used blocks of `MinBlockSizeT` in bit array.
	///
	/// @tparam MinBlockSizeT Minimum block size in bytes, at least `Blk::kMinBlockSize`.
	/// @tparam CpuAccessibleT True if region is CPU accessible.
	/// @tparam BitArrayT Bit array type with one bit per `MinBlockSizeT` bytes of region.
	///
	template<
		  uint32_t MinBlockSizeT = BX_CACHE_LINE_SIZE
		, bool CpuAccessibleT = false
		, typename BitArrayT = MutableBitArrayView
		>
	class BitArrayAllocatorT final
	{
	public:
		static_assert(MinBlockSizeT >= Blk::kMinBlockSize, "MinBlockSizeT must be at least bx::Blk::kMinBlockSize");

		/// Allocator traits.
		static constexpr AllocatorTraits kTraits
		{
			MinBlockSizeT,           // minBlockSize
			Blk::kMinBlockAlignment, // minBlockAlignment
			CpuAccessibleT,          // blocksAreCpuAccessible
			false,                   // threadSafe
		};

		/// Constructs allocator over region `_blk`.
		///
		/// @param[in] _blk Region to allocate from.
		/// @param[in] _bitArray Bit array tracking used blocks.
		///
		BitArrayAllocatorT(Blk&& _blk, BitArrayT& _bitArray);

		/// Destroys allocator.
		~BitArrayAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

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

	private:
		Blk grow(Blk&& _blk, size_t _size);
		Blk truncate(Blk&& _blk, size_t _size);
		static uint32_t find(const MippedBitArrayBase& _bitArray, uint32_t _numBits);
		static uint32_t find(const BitArrayViewBase& _bitArray, uint32_t _numBits);

		Blk m_blk;
		BitArrayT& m_bitArray;
	};

	/// Ring allocator for transient per-frame allocations with multiple frames in flight.
	///
	/// Freeing a block is a noop, its memory is reclaimed when frame that allocated it is retired.
	/// Per frame: allocate, `endFrame(fence)` on submit, and `retire(completedFence)`.
	///
	/// @tparam CpuAccessibleT True if region is CPU accessible.
	/// @tparam MaxFramesT Maximum number of frames in flight.
	///
	template<bool CpuAccessibleT = false, uint32_t MaxFramesT = 8>
	class FrameRingAllocatorT final
	{
	public:
		/// Allocator traits.
		static constexpr AllocatorTraits kTraits
		{
			Blk::kMinBlockSize,      // minBlockSize
			Blk::kMinBlockAlignment, // minBlockAlignment
			CpuAccessibleT,          // blocksAreCpuAccessible
			false,                   // threadSafe
		};

		/// Constructs allocator over region `_blk`.
		///
		/// @param[in] _blk Region to allocate from.
		///
		FrameRingAllocatorT(Blk&& _blk);

		/// Destroys allocator.
		~FrameRingAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Allocates block, same as `realloc(Blk(), _size, _alignment)`.
		///
		/// @param[in] _size Size in bytes.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Allocated block, invalid on failure.
		///
		Blk alloc(size_t _size, size_t _alignment);

		/// Frees valid block, same as `realloc(move(_blk), 0, 0)`. Memory is reclaimed when frame is retired.
		///
		/// @param[in] _blk Block to free.
		///
		/// @returns Invalid block.
		///
		Blk free(Blk&& _blk);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

		/// Ends current frame. Its blocks are reclaimed by `retire` once `_fence` is completed.
		///
		/// @param[in] _fence Fence signaled when GPU work using frame's blocks completes.
		///
		/// @returns False if there are already `MaxFramesT` frames in flight.
		///
		bool endFrame(uint64_t _fence);

		/// Reclaims blocks of all ended frames with fence less or equal to `_completedFence`.
		///
		/// @param[in] _completedFence Last completed fence.
		///
		void retire(uint64_t _completedFence);

		/// Returns number of bytes used by frames that are not retired, including current frame.
		///
		/// @returns Number of bytes used.
		///
		size_t getUsed() const;

		/// Returns number of frames ended, but not retired.
		///
		/// @returns Number of frames in flight.
		///
		uint32_t getNumFramesInFlight() const;

	private:
		struct Frame
		{
			uint64_t fence;
			uint64_t size;
		};

		Blk      m_blk;
		uint64_t m_head;
		uint64_t m_used;
		uint64_t m_frameSize;
		Frame    m_frame[MaxFramesT];
		uint32_t m_firstFrame;
		uint32_t m_numFrames;
	};

	/// Allocator over single region with two stack ends, low end grows up, high end grows down.
	///
	/// Freed block is reclaimed once all blocks allocated after it from same end are freed.
	///
	/// @tparam CpuAccessibleT True if region is CPU accessible.
	///
	template<bool CpuAccessibleT = false>
	class DoubleEndedStackAllocatorT final
	{
	public:
		/// Allocator traits.
		static constexpr AllocatorTraits kTraits
		{
			Blk::kMinBlockSize,      // minBlockSize
			Blk::kMinBlockAlignment, // minBlockAlignment
			CpuAccessibleT,          // blocksAreCpuAccessible
			false,                   // threadSafe
		};

		/// One end of double-ended stack, usable as allocator.
		class End final
		{
		public:
			/// Allocates, resizes, or frees block.
			///
			/// @param[in] _blk Block to resize or free, invalid block to allocate.
			/// @param[in] _size Size in bytes, 0 to free.
			/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
			///
			/// @returns Resulting block, invalid on failure.
			///
			Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

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

			/// Returns true if block is allocated by this end.
			///
			/// @param[in] _blk Block.
			///
			/// @returns True if `_blk` is allocated by this end.
			///
			bool owns(const Blk& _blk);

			/// Returns true if blocks are CPU accessible.
			///
			/// @returns True if blocks are CPU accessible.
			///
			bool isCpuAccessible() const;

			/// Frees all blocks allocated from this end. Blocks must not be used after reset.
			void reset();

			/// Returns number of bytes used by this end, including alignment padding.
			///
			/// @returns Number of bytes used.
			///
			size_t getUsed() const;

		private:
			friend class DoubleEndedStackAllocatorT;

			struct Entry
			{
				uint64_t begin;
				uint64_t ptr;
				uint64_t size;
				bool     freed;
			};

			End(AllocatorBlkI* _metaDataAllocator, uint64_t _start, bool _high);
			Blk resize(Blk&& _blk, size_t _size, size_t _alignment, AllocatorOp::Enum _allocatorOp);

			ArrayT<Entry> m_entry;
			End*          m_other;
			uint64_t      m_start;
			uint64_t      m_pos;
			bool          m_high;
		};

		/// Constructs allocator over region `_blk`.
		///
		/// @param[in] _blk Region to allocate from.
		/// @param[in] _metaDataAllocator Allocator for metadata.
		///
		DoubleEndedStackAllocatorT(Blk&& _blk, AllocatorBlkI* _metaDataAllocator);

		/// Destroys allocator.
		~DoubleEndedStackAllocatorT();

		/// Returns end that grows up from start of region.
		///
		/// @returns Low end.
		///
		End& getLow();

		/// Returns end that grows down from end of region.
		///
		/// @returns High end.
		///
		End& getHigh();

		/// Returns number of bytes between ends.
		///
		/// @returns Number of free bytes.
		///
		size_t getFree() const;

	private:
		Blk m_blk;
		End m_low;
		End m_high;
	};

	namespace internal
	{
		class TwoLevelSegregatedFitLookup
		{
		public:
#if BX_ARCH_64BIT
			static constexpr uint32_t kAlignShift        = 3;
			static constexpr uint32_t kFirstLevelMaxBits = 38;
#else
			static constexpr uint32_t kAlignShift        = 2;
			static constexpr uint32_t kFirstLevelMaxBits = 30;
#endif // BX_ARCH_64BIT

			static constexpr uint32_t kSecondLevelShift = 4;
			static constexpr uint32_t kSecondLevelCount = 1<<kSecondLevelShift;
			static constexpr uint32_t kFirstLevelShift  = kSecondLevelShift + kAlignShift;
			static constexpr uint32_t kFirstLevelCount  = kFirstLevelMaxBits - kFirstLevelShift + 1;
			static constexpr size_t   kBlockSizeSmall   = size_t(1) << kFirstLevelShift;

			TwoLevelSegregatedFitLookup();

			uint32_t find(size_t _size) const;

			void set(size_t _size, uint32_t _idx);

			uint32_t get(size_t _size) const;

			static uint32_t getBin(size_t _size);

			uint32_t getIdx(uint32_t _bin) const;

			void setIdx(uint32_t _bin, uint32_t _idx);

			static size_t roundBlockSize(size_t _size);

		private:
			static void mapping(size_t _size, uint32_t* _outFl, uint32_t* _outSl);

			uint32_t m_firstLevel;
			uint32_t m_secondLevel[kFirstLevelCount];
			uint32_t m_idx[kFirstLevelCount][kSecondLevelCount];
		};

		struct RangeTlsf
		{
			BX_CLASS(RangeTlsf, TRIVIALLY_RELOCATABLE);

			Blk      blk;
			uint32_t sentinelIdx;
		};

	} // namespace internal

	template<>
	int32_t compareAscending<internal::RangeTlsf>(const void* _lhs, const void* _rhs);

	/// Two-level segregated fit (TLSF) allocator over slabs added with `addSlab`.
	///
	/// @tparam MinBlockSizeT Minimum block size in bytes, at least `Blk::kMinBlockSize`.
	/// @tparam CpuAccessibleT True if slabs are CPU accessible.
	///
	template<uint32_t MinBlockSizeT = BX_CACHE_LINE_SIZE, bool CpuAccessibleT = false>
	class TwoLevelSegregatedFitAllocatorT final
	{
	public:
		static_assert(MinBlockSizeT >= Blk::kMinBlockSize, "MinBlockSizeT must be at least bx::Blk::kMinBlockSize");

		/// Allocator traits.
		static constexpr AllocatorTraits kTraits
		{
			MinBlockSizeT,  // minBlockSize
			max<size_t>(Blk::kMinBlockAlignment, 1u << internal::TwoLevelSegregatedFitLookup::kAlignShift), // minBlockAlignment
			CpuAccessibleT, // blocksAreCpuAccessible
			false,          // threadSafe
		};

		/// Constructs allocator.
		///
		/// @param[in] _metaDataAllocator Allocator for metadata.
		///
		explicit TwoLevelSegregatedFitAllocatorT(AllocatorBlkI* _metaDataAllocator);

		/// Destroys allocator.
		~TwoLevelSegregatedFitAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

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

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

		/// Adds slab of memory to allocate from.
		///
		/// @param[in] _blk Slab.
		///
		void addSlab(Blk&& _blk);

		/// Removes slab that is entirely free and returns it.
		///
		/// @returns Removed slab, invalid block when there is no unused slab.
		///
		Blk removeUnusedSlab();

		/// Returns total number of bytes in all slabs.
		///
		/// @returns Total number of bytes in all slabs.
		///
		size_t getTotal() const;

		/// Returns number of bytes used.
		///
		/// @returns Number of bytes used.
		///
		size_t getUsed() const;

	private:
		struct BlkMeta
		{
			static constexpr uint8_t kFlagNone     = 0;
			static constexpr uint8_t kFlagFree     = 1;
			static constexpr uint8_t kFlagPrevFree = 2;
			static constexpr uint8_t kFlagTail     = 4;

			BlkMeta();

			bool isFree() const;

			bool isPrevFree() const;

			bool isUnlinked() const;

			bool isSentinel() const;

			bool isTail() const;

			Blk      blk;
			uint32_t prevIdx;
			uint32_t nextIdx;
			uint32_t freePrevIdx;
			uint32_t freeNextIdx;
			uint8_t  flags;
		};

		using BlkFreeList = HandleTableT<BlkMeta, 256>;
		BlkFreeList m_blkFreeList;

		using RangeArray = SortedArrayT<internal::RangeTlsf>;
		RangeArray m_range;

		internal::TwoLevelSegregatedFitLookup m_lookup;

		size_t   m_total;
		size_t   m_used;
		size_t   m_lastCarveSize;
		uint32_t m_lastCarveIdx;
		uint32_t m_tailIdx;

		static size_t adjustSize(size_t _size, size_t _alignment);
		void setFree(uint32_t _idx, bool _free);
		void splitFreeBlock(uint32_t _idx, size_t _size, size_t _alignment);
		void dump(const StringLiteral& _str, uint32_t _idx);
		void linkFreeBlock(uint32_t _idx);
		void linkFreeBlock(uint32_t _idx, uint32_t _bin);
		void linkFreeBlock(uint32_t _idx, BlkMeta& _meta, uint32_t _bin);
		void unlinkFreeBlock(uint32_t _idx);
		void unlinkFreeBlock(uint32_t _idx, uint32_t _bin);
		void unlinkFreeBlock(uint32_t _idx, BlkMeta& _meta, uint32_t _bin);
		uint32_t findBlock(const uint32_t _sentinelIdx, const Blk& _blk) const;
		uint32_t findBlock(const Blk& _blk) const;
		void tryMergeBlocksNext(uint32_t _idx);
		uint32_t findTail(size_t _size) const;
		Blk allocBlock(size_t _size, size_t _alignment);
		Blk usedBlock(uint32_t _idx, uint64_t _ptr, size_t _size, size_t _alignment);
		Blk carveFreeBlock(uint32_t _idx, BlkMeta& _meta, size_t _size, size_t _alignment);
		Blk carveFreeBlockAligned(uint32_t _idx, BlkMeta& _meta, size_t _size, size_t _alignment);
		Blk freeBlock(Blk&& _blk);
		void splitTail(uint32_t _idx, size_t _size);
		Blk grow(Blk&& _blk, size_t _size, size_t _alignment);
		Blk truncate(Blk&& _blk, size_t _size, size_t _alignment);
	};

	/// Allocator of pages from operating system virtual memory.
	class SystemMemoryAllocator final
	{
	public:
		/// Allocator traits.
		static constexpr AllocatorTraits kTraits
		{
			uint32_t(1_MiB),         // minBlockSize
			Blk::kMinBlockAlignment, // minBlockAlignment
			true,                    // blocksAreCpuAccessible
			false,                   // threadSafe
		};

		/// Constructs allocator.
		SystemMemoryAllocator();

		/// Destroys allocator.
		~SystemMemoryAllocator();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		size_t m_pageSize;
	};

	/// Allocator that reserves address space for each block and commits pages as the block grows,
	/// so a block grows and shrinks in place within its reservation and never moves. Growing past
	/// the reservation allocates a larger one and copies.
	///
	/// Thread safe.
	///
	class PageReserveAllocator final
	{
	public:
		/// Allocator traits.
		static constexpr AllocatorTraits kTraits
		{
			uint32_t(4_KiB),         // minBlockSize
			Blk::kMinBlockAlignment, // minBlockAlignment
			true,                    // blocksAreCpuAccessible
			true,                    // threadSafe
		};

		/// Constructs allocator.
		///
		/// @param[in] _reserveSize Address space reserved for each block, rounded up to the allocation
		///   granularity. Blocks larger than it reserve their own size.
		///
		PageReserveAllocator(size_t _reserveSize);

		/// Destroys allocator.
		~PageReserveAllocator();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, at most the allocation granularity,
		///   0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

		/// Returns address space reserved for each block.
		///
		/// @returns Reservation in bytes.
		///
		size_t getReserveSize() const;

	private:
		Blk allocBlock(size_t _size, size_t _alignment, AllocatorOp::Enum _allocatorOp);

		size_t m_pageSize;
		size_t m_granularity;
		size_t m_reserveSize;
	};

	/// Debug allocator that places each block against guard page, so out-of-bounds access and use-after-free trap.
	///
	/// Not thread safe, wrap with `MutexAllocatorT` for concurrent access.
	///
	class DebugStompAllocator final
	{
	public:
		/// Guard page placement.
		struct Mode
		{
			/// Guard page placement:
			enum Enum
			{
				Overrun,  //!< Guard page after block.
				Underrun, //!< Guard page before block.

				Count
			};
		};

		/// Allocator traits.
		static constexpr AllocatorTraits kTraits
		{
			uint32_t(4_KiB),         // minBlockSize
			Blk::kMinBlockAlignment, // minBlockAlignment
			true,                    // blocksAreCpuAccessible
			false,                   // threadSafe
		};

		/// Constructs allocator.
		///
		/// @param[in] _mode Guard page placement.
		/// @param[in] _reserveSize Size of each virtual address space reservation in bytes.
		///
		DebugStompAllocator(Mode::Enum _mode = Mode::Overrun, size_t _reserveSize = 2_GiB);

		/// Destroys allocator.
		~DebugStompAllocator();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		static constexpr uint64_t kSentinel64 = 0
			| (uint64_t(BX_MAKEFOURCC('b', 'x', 'G', 'U') )      )
			| (uint64_t(BX_MAKEFOURCC('A', 'R', 'D', '!') ) << 32)
			;
		static constexpr size_t kSentinel = size_t(kSentinel64 & SIZE_MAX);

		struct Header
		{
			uintptr_t base;
			size_t    fullSize;
			size_t    userSize;
			size_t    sentinel;
		};

		struct Reservation
		{
			Reservation* next;
			uintptr_t    base;
			size_t       size;
		};

		Blk doAlloc(size_t _size, size_t _alignment);
		void doFree(const Blk& _blk);

		Mode::Enum   m_mode;
		size_t       m_pageSize;
		size_t       m_reserveSize;
		uintptr_t    m_cursor;
		uintptr_t    m_end;
		Reservation* m_reservations;
	};

	/// Adapter exposing Blk allocator as pointer based `AllocatorI`, block info is stored in header before pointer.
	///
	/// @tparam AllocatorBlkT Wrapped allocator type, must be CPU accessible.
	///
	template<typename AllocatorBlkT>
	class IntrusiveBlkAllocatorT final : public AllocatorI
	{
	public:
		/// Constructs adapter around `_allocator`.
		///
		/// @param[in] _allocator Allocator for memory blocks.
		///
		explicit IntrusiveBlkAllocatorT(AllocatorBlkT& _allocator);

		/// Destroys adapter.
		~IntrusiveBlkAllocatorT() override;

		/// Allocates, resizes, or frees memory.
		///
		/// @param[in] _ptr Pointer to memory to resize or free, NULL to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		/// @param[in] _filePath Source file path for memory tracking.
		/// @param[in] _line Source line for memory tracking.
		///
		/// @returns Pointer to memory, NULL when freed or on failure.
		///
		void* realloc(
			  void* _ptr
			, size_t _size
			, size_t _alignment
			, const char* _filePath
			, uint32_t _line
			) override;

		/// Returns number of bytes usable at `_ptr`.
		///
		/// @param[in] _ptr Pointer to memory allocated by this allocator.
		///
		/// @returns Number of usable bytes.
		///
		size_t usableSize(const void* _ptr) const;

	private:
		struct IntrusiveBlk
		{
			static constexpr uint8_t  kShiftLog2Alignment     = Blk::kFieldBitsSize;
			static constexpr uint8_t  kShiftLog2Prefix        = kShiftLog2Alignment + Blk::kFieldBitsLog2Alignment;
			static constexpr uint8_t  kShiftLocationIdxHi     = kShiftLog2Prefix + Blk::kFieldBitsLog2Alignment;
			static constexpr uint8_t  kFieldBitsLocationIdxLo = 16;
			static constexpr uint8_t  kFieldBitsLocationIdxHi = 64 - kShiftLocationIdxHi;
			static constexpr uint64_t kMaskSize               = (uint64_t(1)<<Blk::kFieldBitsSize)-1;
			static constexpr uint64_t kMaskLog2               = (uint64_t(1)<<Blk::kFieldBitsLog2Alignment)-1;

			uint64_t bits;
			uint16_t slotIndex;
#if BX_CONFIG_MEMORY_TRACKING
			uint16_t locationIdxLo;
#else
			uint16_t unused0;
#endif // BX_CONFIG_MEMORY_TRACKING
			uint32_t bookKeeping;
		};

		static constexpr uint32_t kHeaderSize = Blk::kMinBlockAlignment;
		static_assert(sizeof(IntrusiveBlk) <= kHeaderSize, "Intrusive block header must fit minimum block alignment.");

		static uint8_t calcLog2Prefix(size_t _alignment);
		static IntrusiveBlk* getHeader(void* _ptr);
		static Blk toBlk(const void* _ptr);
		void* fromBlk(Blk&& _blk, uint8_t _log2Prefix);

		AllocatorBlkT* m_allocator;
	};

	/// Allocator wrapper that tracks allocated bytes, blocks, and operations, thread safe.
	///
	/// @tparam AllocatorBlkT Wrapped allocator type.
	///
	template<typename AllocatorBlkT>
	class StatsAllocatorT final
	{
	public:
		/// Constructs wrapper around `_allocator`.
		///
		/// @param[in] _allocator Wrapped allocator.
		/// @param[in] _tag Tag identifying allocator.
		///
		explicit StatsAllocatorT(AllocatorBlkT& _allocator, uint16_t _tag = 0);

		/// Destroys wrapper.
		~StatsAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

		/// Returns number of bytes currently allocated.
		///
		/// @returns Number of bytes allocated.
		///
		size_t getUsed() const;

		/// Returns number of blocks currently allocated.
		///
		/// @returns Number of blocks allocated.
		///
		size_t getNumBlocks() const;

		/// Returns number of operations in last frame.
		///
		/// @returns Number of operations.
		///
		size_t getNumOps() const;

		/// Ends frame for per-frame operation count.
		void endFrame();

	private:
		static constexpr uint32_t kNumShards = 16;

		struct alignas(BX_CACHE_LINE_SIZE) Shard
		{
			volatile int64_t  numBytes;
			volatile int64_t  numBlocks;
			volatile uint64_t numOps;
		};

		Shard& getShard();

		Shard             m_shards[kNumShards];
		AllocatorBlkT*    m_allocator;
		volatile uint64_t m_perFrameOpsLast;
		uint16_t          m_tag;
	};

	/// Debug allocator wrapper that fills new memory with `kMemoryInit` and freed memory with `kMemoryDead`.
	///
	/// @tparam AllocatorBlkT Wrapped allocator type, must be CPU accessible.
	///
	template<typename AllocatorBlkT>
	class DebugClearAllocatorT final
	{
	public:
		static constexpr uint8_t kMemoryInit = 0xcd; //!< Fill value of newly allocated memory.
		static constexpr uint8_t kMemoryDead = 0xdd; //!< Fill value of freed memory.

		/// Constructs wrapper around `_allocator`.
		///
		/// @param[in] _allocator Wrapped allocator.
		///
		explicit DebugClearAllocatorT(AllocatorBlkT& _allocator);

		/// Destroys wrapper.
		~DebugClearAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		AllocatorBlkT* m_allocator;
	};

	/// Allocator that adds slabs from secondary allocator to primary allocator when primary is out of memory.
	///
	/// @tparam PrimaryAllocatorT Allocator with `addSlab` and `removeUnusedSlab`.
	/// @tparam SecondaryAllocatorT Allocator for slabs.
	///
	template<typename PrimaryAllocatorT, typename SecondaryAllocatorT>
	class MoreCoreAllocatorT
	{
	public:
		/// Constructs allocator.
		///
		/// @param[in] _primary Allocator for blocks.
		/// @param[in] _secondary Allocator for slabs.
		/// @param[in] _slabMinSize Minimum slab size in bytes.
		///
		MoreCoreAllocatorT(PrimaryAllocatorT& _primary, SecondaryAllocatorT& _secondary, size_t _slabMinSize);

		/// Returns unused primary slabs to secondary allocator. All primary slabs must be allocated by this allocator.
		~MoreCoreAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

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

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		BX_NO_INLINE bool addSlab(size_t _size, size_t _alignment);

		PrimaryAllocatorT&   m_primary;
		SecondaryAllocatorT& m_secondary;
		size_t               m_slabMinSize;
		bool                 m_cpuAccessible;
	};

	/// Thread safe allocator wrapper that serializes calls with mutex.
	///
	/// @tparam AllocatorBlkT Wrapped allocator type.
	///
	template<typename AllocatorBlkT>
	class MutexAllocatorT final
	{
	public:
		/// Constructs wrapper around `_allocator`.
		///
		/// @param[in] _allocator Wrapped allocator.
		///
		explicit MutexAllocatorT(AllocatorBlkT& _allocator);

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

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

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		AllocatorBlkT& m_allocator;
		Mutex m_lock;
	};

	/// Thread safe allocator that gives each thread its own `UpstreamT` instance, blocks can be freed on any thread.
	///
	/// At most `MaxThreadsT` threads may use the allocator over its lifetime. All threads must be quiesced
	/// before destruction.
	///
	/// @tparam UpstreamT Allocator with nested `Config` type, `UpstreamT(const Config&)` constructor, and
	///   `static constexpr AllocatorTraits kTraits`. It must not use `Blk::slotIndex`.
	/// @tparam MaxThreadsT Maximum number of threads, in [1, 65535].
	///
	template<typename UpstreamT, uint32_t MaxThreadsT = 64>
	class ThreadLocalAllocatorT final
	{
		BX_CLASS_NO_COPY(ThreadLocalAllocatorT);

	public:
		static_assert(MaxThreadsT >= 1u
			, "MaxThreadsT must be at least 1."
			);
		static_assert(MaxThreadsT <= 65535u
			, "MaxThreadsT must fit into uint16_t (Blk::slotIndex)."
			);

		/// Configuration used to construct each thread's `UpstreamT`.
		using ConfigT = typename UpstreamT::Config;

		/// Constructs allocator.
		///
		/// @param[in] _config Configuration for each thread's `UpstreamT`.
		/// @param[in] _metaDataAllocator Allocator for per-thread instances.
		///
		ThreadLocalAllocatorT(const ConfigT& _config, AllocatorBlkI* _metaDataAllocator);

		/// Destroys allocator and all per-thread instances.
		~ThreadLocalAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

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

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

	private:
		uintptr_t getOrCreateSlot();
		uintptr_t findOrCreateSlot();
		uint16_t  createSlotLocked();

		ConfigT         m_config;
		AllocatorBlkI*  m_metaDataAllocator;
		Mutex           m_lock;
		TlsData         m_tls;
		UpstreamT*      m_slots[MaxThreadsT];
		uint32_t        m_slotThreadId[MaxThreadsT];
		Blk             m_slotBlk[MaxThreadsT];
		uint32_t        m_slotCount;
	};

	namespace internal
	{
		template<typename AllocatorBlkT
			, uint32_t MaxCachedSizeT
			, uint32_t MaxBlocksPerClassT
			>
		class BonwickMagazineT;

	} // namespace internal

	/// Allocator wrapper with per-thread cache of small blocks, blocks can be freed on any thread.
	///
	/// Upstream allocator must be CPU accessible and thread safe. All threads must be quiesced before
	/// destruction.
	///
	/// @tparam AllocatorBlkT Upstream allocator type.
	/// @tparam MaxCachedSizeT Largest cached size in bytes, power of two.
	/// @tparam MaxBlocksPerClassT Maximum number of cached blocks per class, at least 1.
	///
	template<typename AllocatorBlkT
		, uint32_t MaxCachedSizeT = 4096
		, uint32_t MaxBlocksPerClassT = 32
		>
	class ThreadCacheAllocatorT final
	{
	public:
		static_assert(0 == (MaxCachedSizeT & (MaxCachedSizeT - 1u) )
			, "MaxCachedSizeT must be a power of two."
			);
		static_assert(MaxCachedSizeT >= Blk::kMinBlockSize
			, "MaxCachedSizeT must be at least Blk::kMinBlockSize."
			);
		static_assert(MaxBlocksPerClassT >= 1u
			, "MaxBlocksPerClassT must be at least 1."
			);

		/// Constructs wrapper around `_allocator`.
		///
		/// @param[in] _allocator Upstream allocator.
		///
		explicit ThreadCacheAllocatorT(AllocatorBlkT& _allocator);

		/// Destroys wrapper, returning cached blocks of all threads to upstream.
		~ThreadCacheAllocatorT();

		/// Allocates, resizes, or frees block.
		///
		/// @param[in] _blk Block to resize or free, invalid block to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

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

		/// Returns true if block is allocated by this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if `_blk` is allocated by this allocator.
		///
		bool owns(const Blk& _blk);

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if blocks are CPU accessible.
		///
		bool isCpuAccessible() const;

		/// Returns calling thread's cached blocks to upstream.
		void flush();

	private:
		struct CachedNode
		{
			void*    next;
			uint32_t bookKeeping;
			uint16_t slotIndex;
			uint16_t unused0;
		};

		struct FlushRealloc
		{
			void operator()(void* _base, uint32_t _classSize) const;

			AllocatorBlkT* m_allocator;
		};

		struct FlushFreeRaw
		{
			void operator()(void* _base, uint32_t _classSize) const;

			AllocatorBlkT* m_allocator;
		};

		using MagazineT = internal::BonwickMagazineT<AllocatorBlkT, MaxCachedSizeT, MaxBlocksPerClassT>;

		static constexpr uint32_t kMinLog2    = MagazineT::kMinLog2;
		static constexpr uint32_t kMinSize    = MagazineT::kMinSize;
		static constexpr uint32_t kNumClasses = MagazineT::kNumClasses;

		static_assert(sizeof(CachedNode) <= kMinSize
			, "CachedNode overlay must fit into smallest cached class."
			);

		static bool     isCacheableAlloc(size_t _size, size_t _alignment);
		static bool     isCacheableFree(const Blk& _blk);
		static uint32_t sizeToClass(size_t _size);

		Blk  reallocUpstream(Blk&& _blk, size_t _size, size_t _alignment);
		Blk  allocUpstream(size_t _size, size_t _alignment);
		Blk  freeUpstream(Blk&& _blk);
		void freeOverflow(uint32_t _classIndex, void* _block);

		AllocatorBlkT* m_allocator;
		MagazineT      m_magazine;
	};

} // namespace bx

#include "inline/blk-allocators.inl"

#endif // BX_BLK_ALLOCATORS_H_HEADER_GUARD
