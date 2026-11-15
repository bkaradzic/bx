/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_STORAGEBLK_H_HEADER_GUARD
#define BX_STORAGEBLK_H_HEADER_GUARD

#include <bx/bufferstorage.h>
#include <bx/blk.h>

namespace bx
{
	/// Heap storage for container elements, allocates from the default allocator unless set.
	///
	/// @tparam Ty Element type.
	///
	template<typename Ty>
	class DynamicStorageT final
	{
	public:
		/// Constructs empty storage.
		DynamicStorageT();

		/// Frees storage block.
		~DynamicStorageT();

		/// Sets the allocator by handle, only while the storage holds no block.
		///
		/// @param[in] _handle Handle of allocator for element storage.
		///
		void setAllocatorHandle(AllocatorHandle _handle);

		/// Returns allocator handle.
		///
		/// @returns Handle of allocator for element storage.
		///
		AllocatorHandle getAllocatorHandle() const;

		/// Returns number of elements that fit in storage.
		///
		/// @returns Capacity in elements.
		///
		uint32_t getCapacity() const;

		/// Resizes storage to at least `_capacity` elements, moving the first `_count` elements.
		///
		/// @param[in] _capacity Minimum capacity in elements, 0 with `_count` 0 frees the block.
		/// @param[in] _count Number of live elements, destroyed in the old block after the move.
		///
		void setCapacity(uint32_t _capacity, uint32_t _count);

		/// Returns pointer to element `_idx`, NULL when storage holds no block.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to element, NULL when storage holds no block.
		///
		Ty* getPtr(uint32_t _idx = 0);

		/// Returns pointer to element `_idx`, NULL when storage holds no block.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to element, NULL when storage holds no block.
		///
		const Ty* getPtr(uint32_t _idx = 0) const;

		/// Returns pointer one past the last element of capacity.
		///
		/// @returns Pointer past the last element, NULL when storage holds no block.
		///
		Ty* getTerm();

		/// Returns pointer one past the last element of capacity.
		///
		/// @returns Pointer past the last element, NULL when storage holds no block.
		///
		const Ty* getTerm() const;

		/// Frees own block and takes `_other` block, `_other` keeps its allocator.
		///
		/// @param[in] _other Source storage.
		///
		/// @returns Reference to this.
		///
		DynamicStorageT<Ty>& operator=(DynamicStorageT<Ty>&& _other);

		/// Grows storage for `_num` more elements, trivially relocatable elements only.
		///
		/// @param[in,out] _ptr Pointer to first element, updated when storage moves.
		/// @param[in] _count Number of elements.
		/// @param[in] _num Number of elements to make room for.
		///
		/// @returns Pointer past the last element.
		///
		const Ty* grow(const Ty*& _ptr, uint32_t _count, uint32_t _num);

		/// Makes room for `_num` elements at `_pos`, moving the elements after it, trivially
		/// relocatable elements only. The room is not constructed.
		///
		/// @param[in,out] _ptr Pointer to first element, updated when storage moves.
		/// @param[in] _count Number of elements.
		/// @param[in] _pos Position, clamped to `_count`.
		/// @param[in] _num Number of elements to make room for.
		///
		/// @returns Pointer to the room.
		///
		Ty* carve(const Ty*& _ptr, uint32_t _count, uint32_t _pos, uint32_t _num);

		/// Inserts copies of elements at `_pos`, trivially copyable elements only.
		///
		/// @param[in,out] _ptr Pointer to first element, updated when storage moves.
		/// @param[in] _count Number of elements.
		/// @param[in] _pos Position, clamped to `_count`.
		/// @param[in] _src Elements to copy, must not point into this storage.
		/// @param[in] _num Number of elements to copy.
		///
		/// @returns Pointer past the last element.
		///
		const Ty* insert(const Ty*& _ptr, uint32_t _count, uint32_t _pos, const Ty* _src, uint32_t _num);

	private:
		void setCapacityNonTrivial(uint32_t _capacity, uint32_t _count);
		void updateTerm();

		Blk m_blk;
		Ty* m_term;
	};

	/// True for storage types that allocate from an allocator and can be replaced by a larger one.
	///
	/// @tparam StorageT Storage type.
	///
	template<typename StorageT>
	struct IsDynamicStorageT
	{
		static constexpr bool value = false; //!< Trait result.
	};

	/// True for `DynamicStorageT`.
	///
	/// @tparam Ty Element type.
	///
	template<typename Ty>
	struct IsDynamicStorageT<DynamicStorageT<Ty>>
	{
		static constexpr bool value = true; //!< Trait result.
	};

} // namespace bx

#include "inline/storageblk.inl"

#endif // BX_STORAGEBLK_H_HEADER_GUARD
