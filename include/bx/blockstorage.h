/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_BLOCKSTORAGE_H_HEADER_GUARD
#define BX_BLOCKSTORAGE_H_HEADER_GUARD

#include <bx/storageblk.h>

namespace bx
{
	/// Typeless storage of fixed-size elements in equally sized blocks allocated on demand.
	///
	/// Element addresses are stable. Elements are raw memory, the caller constructs and destroys them.
	///
	class BlockStorage
	{
		BX_CLASS_NO_COPY(BlockStorage);

	public:
		BlockStorage() = delete;

		/// Takes over blocks of `_other`, which is left empty. Element size and block capacity must
		/// match.
		///
		/// @param[in] _other Source storage.
		///
		/// @returns Reference to this.
		///
		BlockStorage& operator=(BlockStorage&& _other);

		/// Constructs empty storage.
		///
		/// @param[in] _elementSize   Size of element in bytes.
		/// @param[in] _elementAlign  Alignment of element in bytes.
		/// @param[in] _blockCapacity Number of elements per block.
		///
		BlockStorage(uint32_t _elementSize, uint32_t _elementAlign, uint32_t _blockCapacity);

		/// Releases all blocks without destroying elements.
		~BlockStorage();

		/// Sets allocator, only while no block is allocated.
		///
		/// @param[in] _allocator Allocator for block storage.
		///
		void setAllocatorHandle(AllocatorHandle _allocator);

		/// Returns allocator handle.
		///
		/// @returns Allocator handle.
		///
		AllocatorHandle getAllocatorHandle() const;

		/// Returns number of addressable elements, allocated or not.
		///
		/// @returns Number of elements.
		///
		uint32_t getCapacity() const;

		/// Returns number of elements per block.
		///
		/// @returns Number of elements.
		///
		uint32_t getBlockCapacity() const;

		/// Returns index of the block holding element index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Block index.
		///
		uint32_t getBlockIdx(uint32_t _idx) const;

		/// Returns number of block entries.
		///
		/// @returns Number of blocks.
		///
		uint32_t getNumBlocks() const;

		/// Sets number of addressable elements, rounded up to whole blocks. Blocks past the new
		/// capacity are released.
		///
		/// @param[in] _capacity Number of elements.
		///
		void setCapacity(uint32_t _capacity);

		/// Returns true when block memory is allocated.
		///
		/// @param[in] _blockIdx Block index.
		///
		/// @returns True when allocated.
		///
		bool isBlockAllocated(uint32_t _blockIdx) const;

		/// Allocates block memory if not allocated yet.
		///
		/// @param[in] _blockIdx Block index.
		///
		void allocateBlock(uint32_t _blockIdx);

		/// Releases block memory if allocated.
		///
		/// @param[in] _blockIdx Block index.
		///
		void releaseBlock(uint32_t _blockIdx);

		/// Returns pointer to element in an allocated block.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		void* getPtr(uint32_t _idx);

		/// Returns pointer to element in an allocated block.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		const void* getPtr(uint32_t _idx) const;

		/// Returns pointer to element, growing capacity and allocating its block as needed.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		void* acquirePtr(uint32_t _idx);

		/// Returns pointer to first element of an allocated block.
		///
		/// @param[in] _blockIdx Block index.
		///
		/// @returns Pointer to the first element of the block.
		///
		void* getBlockPtr(uint32_t _blockIdx);

		/// Returns pointer to first element of an allocated block.
		///
		/// @param[in] _blockIdx Block index.
		///
		/// @returns Pointer to the first element of the block.
		///
		const void* getBlockPtr(uint32_t _blockIdx) const;

	protected:
		DynamicStorageT<Blk> m_blocks;

	private:
		uint32_t m_numBlocks;
		uint32_t m_elementSize;
		uint32_t m_elementAlign;
		uint32_t m_blockCapacity;
		uint32_t m_blockShift;
	};

	/// Typed view of `BlockStorage`.
	///
	/// @tparam Ty             Element type.
	/// @tparam BlockCapacityT Number of elements per block, power of two.
	///
	template<typename Ty, uint32_t BlockCapacityT = 4096>
	class BlockStorageT final : public BlockStorage
	{
	public:
		static_assert(0 == (BlockCapacityT & (BlockCapacityT - 1) ), "BlockCapacityT must be power of two.");

		static constexpr uint32_t kBlockCapacity = BlockCapacityT;           //!< Number of elements per block.
		static constexpr uint32_t kBlockShift    = ceilLog2(BlockCapacityT); //!< Log2 of `BlockCapacityT`.

		/// Constructs empty storage.
		BlockStorageT();

		/// Takes over blocks of `_other`, which is left empty.
		///
		/// @param[in] _other Source storage.
		///
		/// @returns Reference to this.
		///
		BlockStorageT<Ty, BlockCapacityT>& operator=(BlockStorageT<Ty, BlockCapacityT>&& _other);

		/// Sets number of addressable elements, rounded up to whole blocks. Blocks past the new
		/// capacity are released.
		///
		/// @param[in] _capacity Number of elements.
		///
		void setCapacity(uint32_t _capacity);

		/// Returns pointer to element in an allocated block.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		Ty* getPtr(uint32_t _idx);

		/// Returns pointer to element in an allocated block.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		const Ty* getPtr(uint32_t _idx) const;

		/// Returns pointer to element, growing capacity and allocating its block as needed.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		Ty* acquirePtr(uint32_t _idx);

	private:
		void debugPtrUpdate();

#if BX_CONFIG_DEBUG
		Ty** m_debugBlocks;
#endif // BX_CONFIG_DEBUG
	};

} // namespace bx

#include "inline/blockstorage.inl"

#endif // BX_BLOCKSTORAGE_H_HEADER_GUARD
