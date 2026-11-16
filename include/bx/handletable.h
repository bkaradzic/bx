/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HANDLETABLE_H_HEADER_GUARD
#define BX_HANDLETABLE_H_HEADER_GUARD

#include <bx/blk.h>
#include <bx/bitarray.h>
#include <bx/readerwriter.h>

namespace bx
{
	/// Typeless stable-index growable array that stores elements in equally sized blocks. Blocks
	/// are allocated on demand and released when empty.
	///
	/// Index 0 is never returned. Elements are raw memory, the caller constructs and destroys them.
	///
	class HandleTableBase
	{
		BX_CLASS_NO_COPY(HandleTableBase);

	public:
		HandleTableBase() = delete;

		/// Constructs an empty handle table on the process default allocator.
		///
		/// @param[in] _elementSize   Size of element in bytes.
		/// @param[in] _elementAlign  Alignment of element in bytes.
		/// @param[in] _blockCapacity Number of elements per block, power of two and at least 64.
		///
		HandleTableBase(uint32_t _elementSize, uint32_t _elementAlign, uint32_t _blockCapacity);

		/// Constructs handle table taking over storage of `_src`.
		///
		/// @param[in] _src Source handle table, left empty.
		///
		HandleTableBase(HandleTableBase&& _src);

		/// Constructs a handle table with an initial capacity, rounded up to whole blocks.
		///
		/// @param[in] _allocator     Allocator for block storage.
		/// @param[in] _elementSize   Size of element in bytes.
		/// @param[in] _elementAlign  Alignment of element in bytes.
		/// @param[in] _blockCapacity Number of elements per block, power of two and at least 64.
		/// @param[in] _capacity      Number of elements.
		///
		HandleTableBase(AllocatorHandle _allocator, uint32_t _elementSize, uint32_t _elementAlign, uint32_t _blockCapacity, uint32_t _capacity);

		/// Releases all blocks without destroying elements.
		~HandleTableBase();

		/// Releases own blocks without destroying elements and takes over storage of `_src`.
		///
		/// @param[in] _src Source handle table, left empty.
		///
		void set(HandleTableBase&& _src);

		/// Releases all blocks without destroying elements, the block table is kept.
		void clear();

		/// Returns allocator handle.
		///
		/// @returns Allocator handle.
		///
		AllocatorHandle getAllocatorHandle() const;

		/// Returns number of addressable slots, including reserved index 0.
		///
		/// @returns Number of slots.
		///
		uint32_t getCapacity() const;

		/// Returns number of active elements.
		///
		/// @returns Number of elements.
		///
		uint32_t getCount() const;

		/// Returns true if the table contains no elements.
		///
		/// @returns True when empty.
		///
		bool isEmpty() const;

		/// Returns true if an index currently contains an element.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns True when the index is used.
		///
		bool isUsed(uint32_t _idx) const;

		/// Returns index of first used element at or after `_idx`.
		///
		/// @param[in] _idx Element index to start from.
		///
		/// @returns Element index, `kInvalid` when not found.
		///
		uint32_t findUsed(uint32_t _idx) const;

		/// Returns pointer to element at index, which must be greater than 0 and in use.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		void* getPtr(uint32_t _idx);

		/// Returns pointer to element at index, which must be greater than 0 and in use.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		const void* getPtr(uint32_t _idx) const;

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

		/// Reserves free index, from the current block until it fills, then from the lowest block with
		/// free slots, and returns it.
		///
		/// @returns Element index.
		///
		[[nodiscard]] uint32_t acquire();

		/// Releases index.
		///
		/// @param[in] _idx Element index, must be in use.
		///
		void release(uint32_t _idx);

	protected:
		uintptr_t getBase(uint32_t _blockIdx) const;

	private:
		void selectBlock();
		uint32_t allocateBlock();
		void releaseBlock(uint32_t _blockIdx);
		void growTable(uint32_t _tableCapacity);
		void releaseBlocks();
		void reset();

	protected:
		uintptr_t*     m_base;

	private:
		uint32_t*      m_freeHead;
		uint32_t*      m_blockCount;
		Blk*           m_blk;
		Blk            m_table;
		BitArray    m_usedSlots;
		BitArray    m_nonFullBlocks;
		BitArray    m_allocatedBlocks;
		uint32_t       m_elementSize;
		uint32_t       m_elementAlign;
		uint32_t       m_blockCapacity;
		uint32_t       m_blockShift;
		uint32_t       m_numBlocks;
		uint32_t       m_tableCapacity;
		uint32_t       m_count;
		uint32_t       m_curBlock;
	};

	/// Range-for iterator over used elements of a handle table, in index order. Removing any
	/// element, including the one under the iterator, keeps it valid. Elements added during
	/// iteration may or may not be visited.
	///
	/// @tparam ContainerT Handle table type the iterator walks.
	/// @tparam Ty         Element type as returned by `getValue`.
	///
	template<typename ContainerT, typename Ty>
	struct HandleTableIteratorT
	{
		/// Constructs iterator at `_idx`, which must be a used index or `kInvalid`.
		///
		/// @param[in] _container Handle table to iterate.
		/// @param[in] _idx       Element index.
		///
		HandleTableIteratorT(ContainerT& _container, uint32_t _idx);

		/// Returns the iterator itself, for access to index and value.
		///
		/// @returns Reference to this.
		///
		const HandleTableIteratorT<ContainerT, Ty>& operator*() const;

		/// Returns true if iterators point at different indices.
		///
		/// @param[in] _rhs Iterator to compare against.
		///
		/// @returns True when iterators differ.
		///
		bool operator!=(const HandleTableIteratorT<ContainerT, Ty>& _rhs) const;

		/// Advances to the next used element.
		///
		/// @returns Reference to this.
		///
		const HandleTableIteratorT<ContainerT, Ty>& operator++();

		/// Advances to the next used element.
		///
		/// @returns False at the end.
		///
		bool next();

		/// Returns stable index of element.
		///
		/// @returns Element index.
		///
		uint32_t getIndex() const;

		/// Returns element.
		///
		/// @returns Reference to the element.
		///
		Ty& getValue() const;

	private:
		ContainerT& m_container;
		uint32_t    m_idx;
	};

	/// Table of values addressed by stable handles, stored in equally sized blocks. Handles are
	/// indices starting at 1, 0 is never valid. A removed handle is reused by a later add, so a
	/// stale copy refers to the new value, there is no generation check.
	///
	/// @tparam Ty             Element type.
	/// @tparam BlockCapacityT Number of elements per block, power of two and at least 64.
	///
	template<typename Ty, uint32_t BlockCapacityT = 4096>
	class HandleTableT final : public HandleTableBase
	{
		BX_CLASS(HandleTableT, TRIVIALLY_RELOCATABLE);

	public:
		static_assert(64 <= BlockCapacityT, "BlockCapacityT must be at least 64.");
		static_assert(0 == (BlockCapacityT & (BlockCapacityT - 1) ), "BlockCapacityT must be power of two.");

		static constexpr uint32_t kBlockShift = ceilLog2(BlockCapacityT); //!< Log2 of `BlockCapacityT`.

		/// Iterator over used elements.
		using Iterator = HandleTableIteratorT<HandleTableT<Ty, BlockCapacityT>, Ty>;

		/// Const iterator over used elements.
		using ConstIterator = HandleTableIteratorT<const HandleTableT<Ty, BlockCapacityT>, const Ty>;

		/// Constructs an empty handle table on the process default allocator.
		HandleTableT();

		/// Constructs handle table taking over storage of `_src`.
		///
		/// @param[in] _src Source handle table, left empty.
		///
		HandleTableT(HandleTableT<Ty, BlockCapacityT>&& _src);

		/// Constructs an empty handle table with the specified allocator.
		///
		/// @param[in] _allocator Allocator for block storage.
		///
		explicit HandleTableT(AllocatorHandle _allocator);

		/// Constructs a handle table with an initial capacity, rounded up to whole blocks.
		///
		/// @param[in] _allocator Allocator for block storage.
		/// @param[in] _capacity  Number of elements.
		///
		HandleTableT(AllocatorHandle _allocator, uint32_t _capacity);

		/// Destroys all elements and releases storage.
		~HandleTableT();

		/// Takes over storage of `_src`.
		///
		/// @param[in] _src Source handle table, left empty.
		///
		/// @returns Reference to this.
		///
		HandleTableT<Ty, BlockCapacityT>& operator=(HandleTableT<Ty, BlockCapacityT>&& _src);

		/// Destroys all elements and takes over storage of `_src`.
		///
		/// @param[in] _src Source handle table, left empty.
		///
		void set(HandleTableT<Ty, BlockCapacityT>&& _src);

		/// Destroys all elements and releases their blocks.
		void clear();

		/// Returns iterator to the first used element.
		///
		/// @returns Iterator.
		///
		Iterator begin();

		/// Returns iterator past the last used element.
		///
		/// @returns Iterator.
		///
		Iterator end();

		/// Returns iterator to the first used element.
		///
		/// @returns Iterator.
		///
		ConstIterator begin() const;

		/// Returns iterator past the last used element.
		///
		/// @returns Iterator.
		///
		ConstIterator end() const;

		/// Returns pointer to element at index, which must be greater than 0 and in use.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		Ty* getPtr(uint32_t _idx);

		/// Returns pointer to element at index, which must be greater than 0 and in use.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		const Ty* getPtr(uint32_t _idx) const;

		/// Returns reference to element at index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Reference to the element.
		///
		Ty& operator[](uint32_t _idx);

		/// Returns const reference to element at index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Reference to the element.
		///
		const Ty& operator[](uint32_t _idx) const;

		/// Adds a copy of an element and returns assigned stable index.
		///
		/// @param[in] _src Element to copy.
		///
		/// @returns Element index.
		///
		[[nodiscard]] uint32_t add(const Ty& _src);

		/// Adds a moved element and returns assigned stable index.
		///
		/// @param[in] _src Element to move.
		///
		/// @returns Element index.
		///
		[[nodiscard]] uint32_t add(Ty&& _src);

		/// Removes element at index and discards removed value.
		///
		/// @param[in] _idx Element index, must be in use.
		///
		void remove(uint32_t _idx);

		/// Removes element at iterator, which stays valid.
		///
		/// @param[in] _it Iterator at the element.
		///
		void remove(const Iterator& _it);

		/// Removes and returns element at index.
		///
		/// @param[in] _idx Element index, must be in use.
		///
		/// @returns Removed element.
		///
		[[nodiscard]] Ty pop(uint32_t _idx);

		/// Reserves free index without constructing element, and returns it.
		///
		/// @returns Element index.
		///
		[[nodiscard]] uint32_t acquire();

	private:
		void destroyElements();
		void debugPtrUpdate();

#if BX_CONFIG_DEBUG
		Ty** m_debugBase;
#endif // BX_CONFIG_DEBUG
	};

	/// Writes used elements in iteration order, without count or indices. Element type must
	/// be trivially copyable.
	///
	/// @tparam Ty             Element type.
	/// @tparam BlockCapacityT Number of elements per block.
	///
	/// @param[in]  _writer Writer.
	/// @param[in]  _table  Handle table.
	/// @param[out] _err    Error.
	///
	/// @returns Number of bytes written.
	///
	template<typename Ty, uint32_t BlockCapacityT>
	int32_t write(WriterI* _writer, const HandleTableT<Ty, BlockCapacityT>& _table, Error* _err);

} // namespace bx

#include "inline/handletable.inl"

#endif // BX_HANDLETABLE_H_HEADER_GUARD
