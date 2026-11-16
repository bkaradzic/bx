/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_BLK_H_HEADER_GUARD
#define BX_BLK_H_HEADER_GUARD

#include <bx/bx.h>
#include <bx/allocator.h>
#include <bx/cpu.h>
#include <bx/error.h>
#include <bx/math.h>
#include <bx/bufferstorage.h>

/// Allocates and constructs `_type` with `_allocator`, returns pointer to it. Destroy it with
/// `bx::deletePtr`.
#define BX_NEW_PTR(_allocator, _type) BX_PLACEMENT_NEW(bx::allocPtr(_allocator, sizeof(_type), alignof(_type) ), _type)

namespace bx
{
	BX_ERROR_RESULT(kErrorAllocatorAllocateFailed,     BX_MAKEFOURCC('b', 'x', '3', '1') ); //!< Allocation failed.
	BX_ERROR_RESULT(kErrorAllocatorNoBlockLargeEnough, BX_MAKEFOURCC('b', 'x', '3', '2') ); //!< Allocation failed, but smaller blocks are available.

	/// Allocator operation enum.
	struct AllocatorOp
	{
		/// Allocator operation.
		enum Enum : uint8_t
		{
			Noop,     //!< No operation occurred.
			Alias,    //!< Memory block is an alias of another block.
			Allocate, //!< Memory block is allocated.
			Free,     //!< Memory block is freed.
			Grow,     //!< Memory block is resized to larger size.
			Realign,  //!< Memory block alignment changed.
			Truncate, //!< Memory block is resized to smaller size.

			Count     //!< Number of operations.
		};
	};

	/// Memory tracking changes the layout of `Blk`, the namespace keeps translation units built with
	/// different `BX_CONFIG_MEMORY_TRACKING` from linking together.
	inline namespace BX_CONCATENATE(memoryTracking, BX_CONFIG_MEMORY_TRACKING)
	{
		/// Memory block pointer and associated metadata.
		struct [[nodiscard]] Blk final
		{
			BX_CLASS(Blk
				, NO_COPY
				, TRIVIALLY_RELOCATABLE
				);

			static constexpr uint64_t kInvalid                  = UINT64_MAX; //!< Invalid block constant.
			static constexpr uint8_t  kFieldBitsSize            = BX_ARCH_32BIT ? 31 : 40; //!< Number of bits reserved for block size info.
			static constexpr uint8_t  kFieldBitsLog2Alignment   = 5; //!< Number of bits reserved for log2 alignment.
			static constexpr uint8_t  kFieldBitsCpuAccessible   = 1; //!< Number of bits reserved for CPU accessible flag.
			static constexpr uint8_t  kFieldBitsAllocatorOp     = 3; //!< Number of bits reserved for allocator operation info.
			static constexpr uint8_t  kFieldBitsAllocatorHandle = 8; //!< Number of bits reserved for the allocator handle.
			static constexpr uint8_t  kBitsShiftLog2Alignment   = BX_ARCH_32BIT ? 32 : kFieldBitsSize; //!< Bit position of log2 alignment field in `bits`.
			static constexpr uint8_t  kBitsShiftCpuAccessible   = kBitsShiftLog2Alignment + kFieldBitsLog2Alignment; //!< Bit position of CPU accessible field in `bits`.
			static constexpr uint8_t  kBitsShiftAllocatorOp     = BX_ARCH_32BIT ? 40 : kBitsShiftCpuAccessible + kFieldBitsCpuAccessible; //!< Bit position of allocator operation field in `bits`.
			static constexpr uint8_t  kBitsShiftAllocatorHandle = BX_ARCH_32BIT ? 48 : kBitsShiftAllocatorOp + kFieldBitsAllocatorOp; //!< Bit position of allocator handle field in `bits`.
			static constexpr uint8_t  kMinBlockLog2Alignment    = 4; //!< Minimum block log2 alignment.
			static constexpr size_t   kMinBlockAlignment        = size_t(1) << kMinBlockLog2Alignment; //!< Minimum block alignment.

			/// Bits that must be clear in a valid block size.
			static constexpr size_t kSizeMask = 0
				| ( (~size_t(0) )<<kFieldBitsSize)
				| ( (kMinBlockAlignment)-1)
				;

			static constexpr uint32_t kMinBlockSize = 16; //!< Minimum block size.
			static constexpr size_t   kMaxBlockSize = (size_t(1)<<kFieldBitsSize)-kMinBlockSize; //!< Maximum block size.

			/// Invalid reason enum.
			struct InvalidReason
			{
				/// Reason why block is invalid.
				enum Enum
				{
					Default,             //!< Default value set by default constructor (can't be set by constructor).
					NotInvalid,          //!< Block is not invalid (can't be set by constructor).
					Noop,                //!< Operation resulted in no-op.
					Freed,               //!< Block was freed.
					AllocateFailed,      //!< Allocation failed.
					NoBlockLargeEnough,  //!< Allocation failed, but there are smaller blocks.
					ResizeFailed,        //!< Allocator failed to resize block.
					RealignFailed,       //!< Allocator failed to realign block.
					NotSupported,        //!< Functionality is not supported.
					NotImplemented,      //!< Functionality is not implemented.

					Count                //!< Number of reasons.
				};
			};

			/// Constructs invalid block.
			constexpr Blk();

			/// Constructs invalid block with `_reason`.
			///
			/// @param[in] _reason Any reason except `Default` and `NotInvalid`.
			///
			constexpr Blk(InvalidReason::Enum _reason);

			/// Constructs CPU accessible block.
			///
			/// @param[in] _ptr Address aligned to `_alignment`.
			/// @param[in] _size Size in bytes, less than `kMaxBlockSize`.
			/// @param[in] _alignment Alignment in bytes, power of two.
			/// @param[in] _allocatorOp Operation that produced the block.
			///
			constexpr Blk(
				  uint64_t _ptr
				, size_t _size
				, size_t _alignment
				, AllocatorOp::Enum _allocatorOp
				);

			/// Constructs block.
			///
			/// @param[in] _ptr Address aligned to `_alignment`.
			/// @param[in] _size Size in bytes, less than `kMaxBlockSize`.
			/// @param[in] _alignment Alignment in bytes, power of two.
			/// @param[in] _cpuAccessible True if block is CPU accessible.
			/// @param[in] _allocatorOp Operation that produced the block.
			///
			constexpr Blk(
				  uint64_t _ptr
				, size_t _size
				, size_t _alignment
				, bool _cpuAccessible
				, AllocatorOp::Enum _allocatorOp
				);

			/// Constructs block from `_blk` with its CPU accessible flag replaced, `_blk` is invalidated.
			///
			/// @param[in] _blk Source block.
			/// @param[in] _cpuAccessible True if block is CPU accessible.
			///
			Blk(Blk&& _blk, bool _cpuAccessible);

			/// Constructs block from `_rhs`, `_rhs` is invalidated.
			///
			/// @param[in] _rhs Source block.
			///
			Blk(Blk&& _rhs);

			/// Constructs non-owning copy of `_rhs`, the copy can't be resized or freed.
			///
			/// @param[in] _rhs Source block.
			///
			Blk(CopyTag, const Blk& _rhs);

			/// Moves `_rhs` into this block, `_rhs` is invalidated.
			///
			/// @param[in] _rhs Source block.
			///
			/// @returns Reference to this.
			///
			Blk& operator=(Blk&& _rhs);

			/// Returns bit fields packed as they're stored in `bits`.
			///
			/// @param[in] _size Size in bytes.
			/// @param[in] _log2Alignment Alignment as log2.
			/// @param[in] _cpuAccessible True if block is CPU accessible.
			/// @param[in] _allocatorOp Allocator operation.
			///
			/// @returns Packed bit fields.
			///
			static constexpr uint64_t packBits(size_t _size, uint8_t _log2Alignment, bool _cpuAccessible, uint8_t _allocatorOp);

			/// Invalidates block without freeing it.
			void invalidate();

			/// Invalidates block with `_reason` without freeing it.
			///
			/// @param[in] _reason Any reason except `Default` and `NotInvalid`.
			///
			void invalidate(InvalidReason::Enum _reason);

			/// Returns true if block must be freed to avoid a leak.
			///
			/// @returns True if leaking, always false without memory tracking.
			///
			constexpr bool isLeaking() const;

			/// Returns reason why block is invalid.
			///
			/// @returns Invalid reason, `NotInvalid` for valid block.
			///
			InvalidReason::Enum getReason() const;

			/// Returns source code location of the allocation.
			///
			/// @returns Source location, `kUnknownLocation` without memory tracking.
			///
			const Location& getLocation() const;

			/// Returns block alignment.
			///
			/// @returns Alignment in bytes.
			///
			size_t getAlignment() const;

			/// Returns true if blocks have the same pointer and size.
			///
			/// @param[in] _rhs Block to compare with.
			///
			/// @returns True if equal.
			///
			constexpr bool operator==(const Blk& _rhs) const;

			/// Returns pointer to data, block must be CPU accessible.
			///
			/// @tparam Ty Pointed-to type.
			/// @returns Pointer to data.
			///
			template<typename Ty>
			explicit constexpr operator Ty*() const;

			uint64_t ptr; //!< Pointer.

			BX_PRAGMA_DIAGNOSTIC_PUSH()
			BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wpedantic")

			union
			{
				struct
				{
#if BX_ARCH_32BIT
					uint32_t size            : kFieldBitsSize;            //!< Size.
					uint8_t  log2Alignment   : kFieldBitsLog2Alignment;   //!< Alignment expressed as log2.
					uint8_t  cpuAccessible   : kFieldBitsCpuAccessible;   //!< Is memory CPU accessible.
					uint8_t  allocatorOp     : kFieldBitsAllocatorOp;     //!< Allocator operation.
					uint8_t  allocatorHandle : kFieldBitsAllocatorHandle; //!< Handle of the allocator that returned the block, see `getAllocator`.
#else
					uint64_t size            : kFieldBitsSize;            //!< Size.
					uint64_t log2Alignment   : kFieldBitsLog2Alignment;   //!< Alignment expressed as log2.
					uint64_t cpuAccessible   : kFieldBitsCpuAccessible;   //!< Is memory CPU accessible.
					uint64_t allocatorOp     : kFieldBitsAllocatorOp;     //!< Allocator operation.
					uint64_t allocatorHandle : kFieldBitsAllocatorHandle; //!< Handle of the allocator that returned the block, see `getAllocator`.
#endif // BX_ARCH_32BIT
				};

				uint64_t bits; //!< All bit fields as one value.
			};

			BX_PRAGMA_DIAGNOSTIC_POP()

			uint32_t bookKeeping; //!< Allocator book keeping.
			uint16_t slotIndex;   //!< Routing inside a composite allocator, see `ThreadLocalAllocatorT`.
			uint16_t unused0;     //!< Padding.

#if BX_CONFIG_MEMORY_TRACKING
			/// Destroys block, asserts when it is leaking.
			~Blk();

			uint32_t locationIdx; //!< Location tracking index.
			uint32_t unused1;     //!< Padding.
#endif // BX_CONFIG_MEMORY_TRACKING
		};
	} // namespace memoryTracking

	static_assert(Blk::kMaxBlockSize <= PTRDIFF_MAX, "kMaxBlockSize should not be larger than PTRDIFF_MAX.");
	static_assert(sizeof(Blk) == (BX_CONFIG_MEMORY_TRACKING ? 32 : 24), "Unexpected Blk size!");
	static_assert(Blk::InvalidReason::Default == Blk::InvalidReason::Enum(0), "Blk::InvalidReason::Default must be 0!");

	/// Returns true if block is valid.
	///
	/// @param[in] _blk Block.
	///
	/// @returns True if valid.
	///
	constexpr bool isValid(const Blk& _blk);

	/// Returns true if blocks `_a` and `_b` are overlapped.
	///
	/// @param[in] _a First block.
	/// @param[in] _b Second block.
	///
	/// @returns True if overlapped.
	///
	constexpr bool overlap(const Blk& _a, const Blk& _b);

	/// Returns true if block `_a` contains block `_b`.
	///
	/// @param[in] _a Containing block.
	/// @param[in] _b Contained block.
	///
	/// @returns True if `_a` contains `_b`.
	///
	constexpr bool contain(const Blk& _a, const Blk& _b);

	/// Traits an allocator declares about the blocks it hands out.
	struct AllocatorTraits
	{
		const size_t minBlockSize;           //!< Minimum size of returned blocks.
		const size_t minBlockAlignment;      //!< Minimum alignment of returned blocks.
		const bool   blocksAreCpuAccessible; //!< True if returned blocks are CPU accessible.
		const bool   threadSafe;             //!< True if allocator is thread safe.
	};

	/// Memory tracking statistics of one allocation site.
	struct MemoryLocation
	{
		Location location; //!< Source code location.

		size_t numBlocks; //!< Number of live blocks.
		size_t maxBlocks; //!< Peak number of live blocks.

		size_t bytesAllocated;    //!< Number of live bytes.
		size_t maxBytesAllocated; //!< Peak number of live bytes.

		size_t numOps; //!< Number of operations.

		uint32_t numCallstackFrames; //!< Number of valid `callstack` frames.
		uintptr_t callstack[16];     //!< Call stack of the first allocation.
	};

	/// Returns number of tracked allocation sites, sites are numbered from 1.
	///
	/// @returns Number of allocation sites, 0 without `BX_CONFIG_MEMORY_TRACKING`.
	///
	uint32_t getNumMemoryLocations();

	/// Returns statistics of allocation site `_locationIdx`.
	///
	/// @param[in] _locationIdx Allocation site index, 0 for totals of all sites.
	///
	/// @returns Allocation site statistics.
	///
	const MemoryLocation& getMemoryLocation(uint32_t _locationIdx);

	static constexpr uint8_t kAllocatorHandleDefault = 0; //!< Handle of the process default allocator, see `setDefaultAllocatorBlk`.

	static constexpr uint32_t kMaxAllocatorHandles = BX_CONFIG_MAX_ALLOCATOR_HANDLES; //!< Number of allocator handles, including default handle.

	/// Names an allocator. Blocks carry it, see `Blk::allocatorHandle` and `getAllocator`.
	struct AllocatorHandle
	{
		/// Returns true if both handles name the same allocator.
		///
		/// @param[in] _rhs Other handle.
		///
		/// @returns True if equal.
		///
		constexpr bool operator==(const AllocatorHandle& _rhs) const = default;

		uint8_t idx; //!< Handle index.
	};

	/// Returns handle of the allocator that returned `_blk`.
	///
	/// @param[in] _blk Block.
	///
	/// @returns Allocator handle.
	///
	constexpr AllocatorHandle getAllocatorHandle(const Blk& _blk);

	/// Block allocator interface. Blocks carry the handle of their allocator, so they can be freed
	/// or resized without naming it.
	class BX_NO_VTABLE [[nodiscard]] AllocatorBlkI
	{
		BX_CLASS_NO_COPY(AllocatorBlkI);

	public:
		/// Constructs allocator with a new handle, it must outlive every block it returns. Handles of
		/// destroyed allocators are reused in turn, see `setAllocatorPermanent`.
		AllocatorBlkI();

		/// Destroys allocator and releases its handle, all its blocks must be freed.
		virtual ~AllocatorBlkI();

		/// Allocates, resizes or frees `_blk`, the result carries this allocator's handle.
		///
		/// @param[in] _blk Block, invalid to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure or after free.
		///
		Blk realloc(Blk&& _blk, size_t _size, size_t _alignment);

		/// Returns true if `_blk` belongs to this allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if owned.
		///
		virtual bool owns(const Blk& _blk) = 0;

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if CPU accessible.
		///
		virtual bool isCpuAccessible() const = 0;

		/// Returns handle of this allocator.
		///
		/// @returns Allocator handle.
		///
		AllocatorHandle getHandle() const;

	protected:
		/// Allocates, resizes or frees `_blk`, implemented by every allocator.
		///
		/// @param[in] _blk Block, invalid to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure or after free.
		///
		virtual Blk reallocImpl(Blk&& _blk, size_t _size, size_t _alignment) = 0;

	private:
		uint8_t m_handle;
	};

	namespace internal
	{
		template<typename AllocatorBlkT>
		using EnableIfAllocatorBlkType = EnableIfType<!isBaseOf<AllocatorI, AllocatorBlkT>(), void>;

	} // namespace internal

	/// Returns allocator registered under `_handle`.
	///
	/// @param[in] _handle Allocator handle, 0 for the process default.
	///
	/// @returns Allocator.
	///
	AllocatorBlkI* getAllocator(AllocatorHandle _handle);

	/// Includes or excludes blocks of `_allocator` from memory tracking, excluded for allocator metadata.
	///
	/// @param[in] _allocator Allocator.
	/// @param[in] _enabled True to track blocks.
	///
	/// @returns `_allocator`.
	///
	AllocatorBlkI* setAllocatorTracking(AllocatorBlkI* _allocator, bool _enabled);

	/// Makes `_allocator` permanent: its handle is never given to another allocator, so pointers to
	/// it can be cached. Other allocators are scoped, their handles are reused in turn after them.
	///
	/// @param[in] _allocator Allocator, it must live as long as the process.
	///
	/// @returns `_allocator`.
	///
	AllocatorBlkI* setAllocatorPermanent(AllocatorBlkI* _allocator);

	/// Returns true if `_handle` is a permanent allocator's, see `setAllocatorPermanent`.
	///
	/// @param[in] _handle Allocator handle.
	///
	/// @returns True if permanent, false for the default handle, it can be set to another allocator.
	///
	bool isAllocatorPermanent(AllocatorHandle _handle);

	/// Returns the process default allocator, the heap from `getHeapAllocator` until `setDefaultAllocatorBlk`.
	///
	/// @returns Default allocator.
	///
	AllocatorBlkI* getDefaultAllocatorBlk();

	/// Sets the process default allocator, existing blocks keep the allocator that returned them.
	///
	/// @param[in] _allocator Allocator for blocks with the default handle.
	///
	void setDefaultAllocatorBlk(AllocatorBlkI* _allocator);

	/// Block allocator that allocates from slabs added to it.
	class BX_NO_VTABLE [[nodiscard]] AllocatorSlabI : public AllocatorBlkI
	{
	public:
		/// Destroys allocator.
		virtual ~AllocatorSlabI() = 0;

		/// Adds `_blk` as a slab to allocate from.
		///
		/// @param[in] _blk Block.
		///
		virtual void addSlab(Blk&& _blk) = 0;

		/// Removes slab with no allocated blocks.
		///
		/// @returns Removed slab, invalid block when there is none.
		///
		virtual Blk removeUnusedSlab() = 0;

		/// Returns total size of all slabs.
		///
		/// @returns Size in bytes.
		///
		virtual size_t getTotal() const = 0;

		/// Returns number of bytes in use.
		///
		/// @returns Size in bytes.
		///
		virtual size_t getUsed() const = 0;
	};

	/// Returns operation `realloc` would perform with these arguments.
	///
	/// @param[in] _blk Block, can be invalid.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	///
	/// @returns Allocator operation.
	///
	AllocatorOp::Enum getAllocatorOp(const Blk& _blk, size_t _size, size_t _alignment);

	/// Returns name of allocator operation.
	///
	/// @param[in] _op Allocator operation.
	///
	/// @returns Operation name.
	///
	const char* getName(AllocatorOp::Enum _op);

	/// Returns description of invalid reason.
	///
	/// @param[in] _reason Invalid reason.
	///
	/// @returns Reason description.
	///
	const char* getName(Blk::InvalidReason::Enum _reason);

	/// Sets bytes `_start` to `_end` of `_blk` to `_value`, clamped to block size.
	///
	/// @param[in] _blk Block, must be CPU accessible.
	/// @param[in] _start Start offset in bytes.
	/// @param[in] _end End offset in bytes.
	/// @param[in] _value Byte value.
	///
	void set(const Blk& _blk, size_t _start, size_t _end, uint8_t _value);

	/// Allocates block of at least `_size` bytes.
	///
	/// @tparam AllocatorBlkT Block allocator type.
	/// @param[in] _allocator Allocator for the block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	/// @param[out] _err Error state, set on failure.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Allocated block, invalid on failure.
	///
	template<typename AllocatorBlkT, typename = internal::EnableIfAllocatorBlkType<AllocatorBlkT>>
	Blk tryAlloc(AllocatorBlkT* _allocator, size_t _size, size_t _alignment, Error* _err, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Allocates block of at least `_size` bytes, stops program execution on failure.
	///
	/// @tparam AllocatorBlkT Block allocator type.
	/// @param[in] _allocator Allocator for the block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Allocated block.
	///
	template<typename AllocatorBlkT, typename = internal::EnableIfAllocatorBlkType<AllocatorBlkT>>
	Blk alloc(AllocatorBlkT* _allocator, size_t _size, size_t _alignment = 0, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Allocates block of at least `_size` bytes, stops program execution on failure.
	///
	/// @param[in] _allocator Handle of allocator for the block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Allocated block.
	///
	Blk alloc(AllocatorHandle _allocator, size_t _size, size_t _alignment = 0, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Resizes `_blk` by allocating a new block, copying contents and freeing the old one.
	///
	/// @tparam AllocatorBlkT Block allocator type.
	/// @param[in] _allocator Allocator for the block, must be CPU accessible.
	/// @param[in] _blk Block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 keeps alignment of `_blk`.
	/// @param[out] _err Error state, set on failure.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Resized block, `_blk` on failure.
	///
	template<typename AllocatorBlkT, typename = internal::EnableIfAllocatorBlkType<AllocatorBlkT>>
	Blk tryAllocCopyFree(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment, Error* _err, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Resizes `_blk` by allocating a new block, copying contents and freeing the old one, stops
	/// program execution on failure.
	///
	/// @tparam AllocatorBlkT Block allocator type.
	/// @param[in] _allocator Allocator for the block, must be CPU accessible.
	/// @param[in] _blk Block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 keeps alignment of `_blk`.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Resized block.
	///
	template<typename AllocatorBlkT, typename = internal::EnableIfAllocatorBlkType<AllocatorBlkT>>
	Blk allocCopyFree(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment = 0, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Resizes `_blk` to at least `_size` bytes, invalid `_blk` is allocated.
	///
	/// @tparam AllocatorBlkT Block allocator type.
	/// @param[in] _allocator Allocator for the block.
	/// @param[in] _blk Block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	/// @param[out] _err Error state, set on failure.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Resized block, `_blk` with `AllocatorOp::Noop` on failure.
	///
	template<typename AllocatorBlkT, typename = internal::EnableIfAllocatorBlkType<AllocatorBlkT>>
	Blk tryRealloc(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment, Error* _err, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Resizes `_blk` to at least `_size` bytes, stops program execution on failure.
	///
	/// @tparam AllocatorBlkT Block allocator type.
	/// @param[in] _allocator Allocator for the block.
	/// @param[in] _blk Block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Resized block.
	///
	template<typename AllocatorBlkT, typename = internal::EnableIfAllocatorBlkType<AllocatorBlkT>>
	Blk realloc(AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, size_t _alignment = 0, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Frees `_blk`.
	///
	/// @tparam AllocatorBlkT Block allocator type.
	/// @param[in] _allocator Allocator that returned the block.
	/// @param[in] _blk Block.
	/// @param[in] _location Source location for memory tracking.
	///
	/// @returns Invalid block.
	///
	template<typename AllocatorBlkT, typename = internal::EnableIfAllocatorBlkType<AllocatorBlkT>>
	Blk free(AllocatorBlkT* _allocator, Blk&& _blk, const Location& _location = Location::current() );

	/// Resizes `_blk` through the allocator that returned it, invalid `_blk` is allocated from the
	/// process default allocator.
	///
	/// @param[in] _blk Block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	/// @param[out] _err Error state, set on failure.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Resized block, `_blk` with `AllocatorOp::Noop` on failure.
	///
	Blk tryRealloc(Blk&& _blk, size_t _size, size_t _alignment, Error* _err, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Resizes `_blk` through the allocator that returned it, stops program execution on failure.
	///
	/// @param[in] _blk Block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Resized block.
	///
	Blk realloc(Blk&& _blk, size_t _size, size_t _alignment = 0, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Frees `_blk` through the allocator that returned it.
	///
	/// @param[in] _blk Block.
	/// @param[in] _location Source location for memory tracking.
	///
	/// @returns Invalid block.
	///
	Blk free(Blk&& _blk, const Location& _location = Location::current() );

	/// Allocates memory and returns pointer to it, for code that has to name memory by pointer
	/// alone. Block is kept in a header before the pointer.
	///
	/// @param[in] _allocator Allocator for the block, must be CPU accessible.
	/// @param[in] _size Size in bytes.
	/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
	/// @param[in] _location Source location for memory tracking.
	///
	/// @returns Pointer to memory.
	///
	void* allocPtr(AllocatorBlkI* _allocator, size_t _size, size_t _alignment = 0, const Location& _location = Location::current() );

	/// Resizes memory returned by `allocPtr`, contents are preserved. Memory keeps the alignment
	/// it was allocated with.
	///
	/// @param[in] _allocator Allocator for the block when `_ptr` is NULL, must be CPU accessible.
	/// @param[in] _ptr Pointer returned by `allocPtr` or `reallocPtr`, NULL to allocate.
	/// @param[in] _size Size in bytes, 0 to free.
	/// @param[in] _alignment Alignment in bytes when `_ptr` is NULL, power of two, 0 for default.
	/// @param[in] _location Source location for memory tracking.
	///
	/// @returns Pointer to memory, NULL when freed.
	///
	void* reallocPtr(AllocatorBlkI* _allocator, void* _ptr, size_t _size, size_t _alignment = 0, const Location& _location = Location::current() );

	/// Frees memory returned by `allocPtr` through the allocator that returned it.
	///
	/// @param[in] _ptr Pointer returned by `allocPtr` or `reallocPtr`, NULL is ignored.
	/// @param[in] _location Source location for memory tracking.
	///
	void freePtr(void* _ptr, const Location& _location = Location::current() );

	/// Destroys object created with `BX_NEW_PTR` and frees its memory.
	///
	/// @tparam Ty Object type.
	/// @param[in] _object Object, NULL is ignored.
	/// @param[in] _location Source location for memory tracking.
	///
	template<typename Ty>
	void deletePtr(Ty* _object, const Location& _location = Location::current() );

	/// Resizes `_blk` without sanitizing or recovering, caller guarantees `_size` is in
	/// `[Blk::kMinBlockSize, Blk::kMaxBlockSize]` and a multiple of `Blk::kMinBlockSize`.
	/// Failure is fatal, the result is invalid when asserts are disabled.
	///
	/// @tparam AllocatorBlkT Block allocator type.
	/// @param[in] _allocator Allocator for the block.
	/// @param[in] _blk Block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Resized block, default alignment.
	///
	template<typename AllocatorBlkT, typename = internal::EnableIfAllocatorBlkType<AllocatorBlkT>>
	Blk realloc(UncheckedTag, AllocatorBlkT* _allocator, Blk&& _blk, size_t _size, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Frees `_blk` without recovering, caller guarantees it's not a copy.
	///
	/// @tparam AllocatorBlkT Block allocator type.
	/// @param[in] _allocator Allocator that returned the block.
	/// @param[in] _blk Block.
	/// @param[in] _location Source location for memory tracking.
	///
	/// @returns Invalid block.
	///
	template<typename AllocatorBlkT, typename = internal::EnableIfAllocatorBlkType<AllocatorBlkT>>
	Blk free(UncheckedTag, AllocatorBlkT* _allocator, Blk&& _blk, const Location& _location = Location::current() );

	/// Resizes `_blk` through the allocator that returned it, see `realloc(Unchecked, ...)`.
	///
	/// @param[in] _blk Block.
	/// @param[in] _size Size in bytes.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _skip Call stack frames to skip for memory tracking.
	///
	/// @returns Resized block, default alignment.
	///
	Blk realloc(UncheckedTag, Blk&& _blk, size_t _size, const Location& _location = Location::current(), uint32_t _skip = 1);

	/// Frees `_blk` through the allocator that returned it, see `free(Unchecked, ...)`.
	///
	/// @param[in] _blk Block.
	/// @param[in] _location Source location for memory tracking.
	///
	/// @returns Invalid block.
	///
	Blk free(UncheckedTag, Blk&& _blk, const Location& _location = Location::current() );

	/// Adapts duck typed allocator to `AllocatorBlkI`.
	///
	/// @tparam AllocatorBlkT Adapted allocator type.
	///
	template<typename AllocatorBlkT>
	class ImplementAllocatorBlkInterfaceT final : public AllocatorBlkI
	{
	public:
		/// Constructs adapter over `_allocator`.
		///
		/// @param[in] _allocator Adapted allocator.
		///
		explicit ImplementAllocatorBlkInterfaceT(AllocatorBlkT& _allocator);

		ImplementAllocatorBlkInterfaceT() = delete;

		/// Destroys adapter.
		~ImplementAllocatorBlkInterfaceT() = default;

		/// Returns true if `_blk` belongs to the adapted allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if owned.
		///
		bool owns(const Blk& _blk) override;

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if CPU accessible.
		///
		bool isCpuAccessible() const override;

	protected:
		/// Forwards to the adapted allocator's `realloc`.
		///
		/// @param[in] _blk Block, invalid to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure or after free.
		///
		Blk reallocImpl(Blk&& _blk, size_t _size, size_t _alignment) override;

	private:
		AllocatorBlkT& m_allocator;
	};

	/// Holds `AllocatorBlkI` adapter of a duck typed allocator. List it as the first base of a class
	/// derived from an `AllocatorBlkI` based class that uses the adapter.
	///
	/// @tparam AllocatorBlkT Adapted allocator type.
	///
	template<typename AllocatorBlkT>
	struct AllocatorBlkInterfaceHolderT
	{
		/// Constructs adapter over `_allocator`.
		///
		/// @param[in] _allocator Adapted allocator.
		///
		explicit AllocatorBlkInterfaceHolderT(AllocatorBlkT& _allocator);

		ImplementAllocatorBlkInterfaceT<AllocatorBlkT> m_interface; //!< Adapter.
	};

	/// Adapts duck typed slab allocator to `AllocatorSlabI`.
	///
	/// @tparam AllocatorSlabT Adapted slab allocator type.
	///
	template<typename AllocatorSlabT>
	class ImplementAllocatorSlabInterfaceT final : public AllocatorSlabI
	{
	public:
		/// Constructs adapter over `_allocator`.
		///
		/// @param[in] _allocator Adapted slab allocator.
		///
		explicit ImplementAllocatorSlabInterfaceT(AllocatorSlabT& _allocator);

		ImplementAllocatorSlabInterfaceT() = delete;

		/// Destroys adapter.
		~ImplementAllocatorSlabInterfaceT() = default;

		/// Returns true if `_blk` belongs to the adapted allocator.
		///
		/// @param[in] _blk Block.
		///
		/// @returns True if owned.
		///
		bool owns(const Blk& _blk) override;

		/// Returns true if blocks are CPU accessible.
		///
		/// @returns True if CPU accessible.
		///
		bool isCpuAccessible() const override;

		/// Adds `_blk` as a slab to allocate from.
		///
		/// @param[in] _blk Block.
		///
		void addSlab(Blk&& _blk) override;

		/// Removes slab with no allocated blocks.
		///
		/// @returns Removed slab, invalid block when there is none.
		///
		Blk removeUnusedSlab() override;

		/// Returns total size of all slabs.
		///
		/// @returns Size in bytes.
		///
		size_t getTotal() const override;

		/// Returns number of bytes in use.
		///
		/// @returns Size in bytes.
		///
		size_t getUsed() const override;

	protected:
		/// Forwards to the adapted allocator's `realloc`.
		///
		/// @param[in] _blk Block, invalid to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure or after free.
		///
		Blk reallocImpl(Blk&& _blk, size_t _size, size_t _alignment) override;

	private:
		AllocatorSlabT& m_allocator;
	};

} // namespace bx

#include "inline/blk.inl"

#endif // BX_BLK_H_HEADER_GUARD
