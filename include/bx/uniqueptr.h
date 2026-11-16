/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_UNIQUEPTR_H_HEADER_GUARD
#define BX_UNIQUEPTR_H_HEADER_GUARD

#include <bx/blk.h>
#include <bx/storageblk.h>

/// Allocates and constructs `_type` with `_allocator`, returns `bx::UniquePtrT<_type>`. For
/// `_type` array `Ty[]` argument is element count.
#define BX_MAKE_UNIQUE(_allocator, _type, ...) \
	bx::makeUnique<_type>(_allocator, bx::Location::current() __VA_OPT__(,) __VA_ARGS__)

namespace bx
{
	/// Owning pointer to an object in a block. Destroys object and frees block through the
	/// allocator that returned it.
	///
	/// @tparam Ty Object type, `Ty[]` for array of objects.
	///
	template<typename Ty>
	class UniquePtrT final
	{
		BX_CLASS(UniquePtrT, NO_COPY, TRIVIALLY_RELOCATABLE);

	public:
		/// Constructs pointer to invalid block, on the default allocator.
		UniquePtrT();

		/// Destroys object and frees block.
		~UniquePtrT();

		/// Constructs pointer to invalid block, on `_allocator`.
		///
		/// @param[in] _allocator Allocator, kept while the pointer holds no block.
		///
		explicit UniquePtrT(AllocatorHandle _allocator);

		/// Constructs pointer, takes ownership of `_blk`.
		///
		/// @param[in] _blk Block holding a constructed `Ty`, from an allocator with a handle.
		///
		explicit UniquePtrT(Blk&& _blk);

		/// Constructs pointer, takes ownership of `_rhs` block.
		///
		/// @param[in] _rhs Source pointer, left invalid, keeps its allocator.
		///
		UniquePtrT(UniquePtrT&& _rhs);

		/// Destroys owned object, and takes ownership of `_rhs` block.
		///
		/// @param[in] _rhs Source pointer, left invalid, keeps its allocator.
		///
		/// @returns Reference to this.
		///
		UniquePtrT& operator=(UniquePtrT&& _rhs);

		/// Returns reference to the object.
		///
		/// @returns Reference to the object.
		///
		Ty& operator*() const;

		/// Returns pointer to the object.
		///
		/// @returns Pointer to the object.
		///
		Ty* operator->() const;

		/// Returns pointer to the object.
		///
		/// @returns Pointer to the object, NULL when invalid.
		///
		Ty* getPtr() const;

		/// Releases ownership without destroying object, and returns block.
		///
		/// @returns Owned block, invalid when none.
		///
		Blk release();

		/// Returns allocator handle.
		///
		/// @returns Handle of allocator of the block, kept while the pointer holds no block.
		///
		AllocatorHandle getAllocatorHandle() const;

		/// Destroys object and frees block, pointer is left invalid and keeps its allocator.
		///
		/// @param[in] _location Source location for memory tracking.
		///
		void reset(const Location& _location = Location::current() );

		/// Returns `true` if block is valid, otherwise returns `false`.
		///
		/// @returns `true` when block is valid.
		///
		bool isValid() const;

	private:
		void debugPtrUpdate();

		Blk m_blk;
#if BX_CONFIG_DEBUG
		Ty* m_debugPtr;
#endif // BX_CONFIG_DEBUG
	};

	/// Owning pointer to an array of objects in a block. Destroys objects and frees block
	/// through the allocator that returned it.
	///
	/// @tparam Ty Element type.
	///
	template<typename Ty>
	class UniquePtrT<Ty[]> final
	{
		BX_CLASS(UniquePtrT, NO_COPY, TRIVIALLY_RELOCATABLE);

	public:
		/// Element type.
		using ElementType = Ty;

		/// Constructs pointer to invalid block, on the default allocator.
		UniquePtrT();

		/// Destroys objects and frees block.
		~UniquePtrT();

		/// Constructs pointer to invalid block, on `_allocator`.
		///
		/// @param[in] _allocator Allocator, kept while the pointer holds no block.
		///
		explicit UniquePtrT(AllocatorHandle _allocator);

		/// Constructs pointer, takes ownership of `_blk`.
		///
		/// @param[in] _blk Block holding `_count` constructed `Ty`, from an allocator with a handle.
		/// @param[in] _count Number of elements.
		///
		UniquePtrT(Blk&& _blk, uint32_t _count);

		/// Constructs pointer, takes ownership of `_rhs` block.
		///
		/// @param[in] _rhs Source pointer, left invalid, keeps its allocator.
		///
		UniquePtrT(UniquePtrT&& _rhs);

		/// Destroys owned objects, and takes ownership of `_rhs` block.
		///
		/// @param[in] _rhs Source pointer, left invalid, keeps its allocator.
		///
		/// @returns Reference to this.
		///
		UniquePtrT& operator=(UniquePtrT&& _rhs);

		/// Returns reference to element.
		///
		/// @param[in] _idx Element index, less than `getCount()`.
		///
		/// @returns Reference to element.
		///
		Ty& operator[](uint32_t _idx) const;

		/// Returns pointer to the first element.
		///
		/// @returns Pointer to the first element, NULL when invalid.
		///
		Ty* getPtr() const;

		/// Returns number of elements.
		///
		/// @returns Number of elements, 0 when invalid.
		///
		uint32_t getCount() const;

		/// Releases ownership without destroying objects, and returns block.
		///
		/// @returns Owned block, invalid when none.
		///
		Blk release();

		/// Returns allocator handle.
		///
		/// @returns Handle of allocator of the block, kept while the pointer holds no block.
		///
		AllocatorHandle getAllocatorHandle() const;

		/// Destroys objects and frees block, pointer is left invalid and keeps its allocator.
		///
		/// @param[in] _location Source location for memory tracking.
		///
		void reset(const Location& _location = Location::current() );

		/// Returns `true` if block is valid, otherwise returns `false`.
		///
		/// @returns `true` when block is valid.
		///
		bool isValid() const;

	private:
		void debugPtrUpdate();

		Blk      m_blk;
		uint32_t m_count;
#if BX_CONFIG_DEBUG
		Ty* m_debugPtr;
#endif // BX_CONFIG_DEBUG
	};

	/// Allocates and constructs `Ty` from `_args` with `_allocator`.
	///
	/// @tparam Ty Object type.
	/// @tparam ArgsT Constructor argument types.
	///
	/// @param[in] _allocator Allocator for the object.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _args Constructor arguments.
	///
	/// @returns Owning pointer to the object, invalid on `_allocator` when allocation fails.
	///
	template<typename Ty, typename... ArgsT>
	requires (!isArray<Ty>() )
	UniquePtrT<Ty> makeUnique(AllocatorHandle _allocator, const Location& _location, ArgsT&&... _args);

	/// Allocates and constructs `_count` elements of array `Ty` with `_allocator`, each from
	/// `_args`. Without `_args` elements constructible from `InitNone` are constructed with it,
	/// trivially constructible elements are left uninitialized, others are default constructed.
	///
	/// @tparam Ty Array type `ElementT[]`.
	/// @tparam ArgsT Element constructor argument types.
	///
	/// @param[in] _allocator Allocator for the elements.
	/// @param[in] _location Source location for memory tracking.
	/// @param[in] _count Number of elements.
	/// @param[in] _args Constructor arguments for every element, forwarded only to the last.
	///
	/// @returns Owning pointer to the elements, invalid on `_allocator` when `_count` is 0 or
	///   allocation fails.
	///
	template<typename Ty, typename... ArgsT>
	requires (isUnboundedArray<Ty>() )
	UniquePtrT<Ty> makeUnique(AllocatorHandle _allocator, const Location& _location, uint32_t _count, ArgsT&&... _args);

} // namespace bx

#include "inline/uniqueptr.inl"

#endif // BX_UNIQUEPTR_H_HEADER_GUARD
