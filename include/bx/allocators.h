/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_ALLOCATORS_H_HEADER_GUARD
#define BX_ALLOCATORS_H_HEADER_GUARD

#include <bx/blk-allocators.h>

namespace bx
{
	/// Block allocator over the C runtime heap.
	class CrtAllocator final : public AllocatorBlkI
	{
	public:
		/// Constructs allocator.
		CrtAllocator();

		/// Destroys allocator.
		~CrtAllocator();

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
		/// @param[in] _blk Block.
		///
		/// @returns Invalid block.
		///
		Blk free(Blk&& _blk);

		/// Returns true for any block.
		///
		/// @param[in] _blk Block.
		///
		/// @returns Always true.
		///
		bool owns(const Blk& _blk) override;

		/// Returns true, blocks are CPU accessible.
		///
		/// @returns Always true.
		///
		bool isCpuAccessible() const override;

	protected:
		/// Allocates, resizes or frees `_blk`.
		///
		/// @param[in] _blk Block, invalid to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure or after free.
		///
		Blk reallocImpl(Blk&& _blk, size_t _size, size_t _alignment) override;
	};

	/// Block allocator over pointer based `AllocatorI`, for code that is handed one and allocates
	/// blocks. Blocks keep the alignment they were allocated with.
	class PointerAllocator final : public AllocatorBlkI
	{
	public:
		static constexpr size_t kDefaultNaturalAlignment = BX_ARCH_64BIT ? 16 : 8; //!< Alignment C runtime heaps return.

		/// Constructs allocator over `_allocator`.
		///
		/// @param[in] _allocator Pointer based allocator, it must outlive this allocator.
		/// @param[in] _naturalAlignment Alignment `_allocator` returns when asked for none, blocks
		///   aligned to it or less are allocated without alignment.
		///
		explicit PointerAllocator(AllocatorI* _allocator, size_t _naturalAlignment = kDefaultNaturalAlignment);

		/// Destroys allocator.
		~PointerAllocator();

		/// Returns true for any block.
		///
		/// @param[in] _blk Block.
		///
		/// @returns Always true.
		///
		bool owns(const Blk& _blk) override;

		/// Returns true, blocks are CPU accessible.
		///
		/// @returns Always true.
		///
		bool isCpuAccessible() const override;

	protected:
		/// Allocates, resizes or frees `_blk`.
		///
		/// @param[in] _blk Block, invalid to allocate.
		/// @param[in] _size Size in bytes, 0 to free.
		/// @param[in] _alignment Alignment in bytes, power of two, 0 for default.
		///
		/// @returns Resulting block, invalid on failure or after free.
		///
		Blk reallocImpl(Blk&& _blk, size_t _size, size_t _alignment) override;

	private:
		AllocatorI* m_allocator;
		size_t m_naturalAlignment;
	};

	namespace internal
	{
		template<uint32_t MinBlockSizeT>
		struct SystemArenaStorageT;

	} // namespace internal

	/// Heap over system memory that grows in slabs, not thread safe.
	///
	/// @tparam MinBlockSizeT Minimum block size in bytes.
	///
	template<uint32_t MinBlockSizeT = BX_CACHE_LINE_SIZE>
	class SystemArenaAllocatorT final
		: private internal::SystemArenaStorageT<MinBlockSizeT>
		, public  MoreCoreAllocatorT<typename internal::SystemArenaStorageT<MinBlockSizeT>::TlsfT, SystemMemoryAllocator>
	{
	public:
		static constexpr size_t kDefaultSlabSize = BX_ARCH_32BIT ? 32_MiB : 256_MiB; //!< Default size of slabs the arena grows by.

		static constexpr AllocatorTraits kTraits = internal::SystemArenaStorageT<MinBlockSizeT>::TlsfT::kTraits; //!< Allocator traits.

		/// Construction parameters, as `ThreadLocalAllocatorT` passes them.
		struct Config
		{
			size_t slabSize; //!< Size of slabs the arena grows by.
		};

		/// Constructs arena that grows by `_slabSize`.
		///
		/// @param[in] _slabSize Slab size in bytes.
		///
		explicit SystemArenaAllocatorT(size_t _slabSize = kDefaultSlabSize);

		/// Constructs arena from `_config`.
		///
		/// @param[in] _config Construction parameters.
		///
		explicit SystemArenaAllocatorT(const Config& _config);
	};

} // namespace bx

#include "inline/allocators.inl"

#endif // BX_ALLOCATORS_H_HEADER_GUARD
