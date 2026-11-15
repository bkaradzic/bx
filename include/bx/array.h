/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_ARRAY_H_HEADER_GUARD
#define BX_ARRAY_H_HEADER_GUARD

#include <bx/bx.h>
#include <bx/heap.h>
#include <bx/readerwriter.h>
#include <bx/sort.h>
#include <bx/storageblk.h>

namespace bx
{
	/// Base class for read-only views of a contiguous sequence of elements.
	///
	/// @tparam Ty Element type.
	///
	template<typename Ty>
	class ArrayViewBaseT
	{
	public:
		/// Element type.
		using ValueType = Ty;

		/// Constructs an empty view.
		ArrayViewBaseT();

		/// Returns a const pointer to the first element.
		///
		/// @returns Pointer to the first element.
		///
		const Ty* begin() const;

		/// Returns a const pointer past the last element.
		///
		/// @returns Pointer past the last element.
		///
		const Ty* end() const;

		/// Returns a const pointer to the element at the specified index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		const Ty* getPtr(uint32_t _idx = 0) const;

		/// Returns a const pointer past the last element.
		///
		/// @returns Pointer past the last element.
		///
		const Ty* getTerm() const;

		/// Returns a const reference to the element at the specified index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Reference to the element.
		///
		const Ty& operator[](uint32_t _idx) const;

		/// Returns true if both views have the same count and equal elements.
		///
		/// @param[in] _rhs Source view to compare against.
		///
		/// @returns True when equal.
		///
		bool operator==(const ArrayViewBaseT<Ty>& _rhs) const;

		/// Returns the number of elements in the view.
		///
		/// @returns Number of elements.
		///
		uint32_t getCount() const;

		/// Returns the index of the element at `_ptr`, which must point into the view.
		///
		/// @param[in] _ptr Pointer to an element of the view.
		///
		/// @returns Element index.
		///
		uint32_t getIndex(const Ty* _ptr) const;

		/// Returns true if the view contains no elements.
		///
		/// @returns True when empty.
		///
		bool isEmpty() const;

	protected:
		StorageViewT<Ty> m_storageView;
	};

	/// Returns true if content of both views is equal.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in] _lhs First view.
	/// @param[in] _rhs Second view.
	///
	/// @returns True when equal.
	///
	template<typename Ty>
	bool isEqual(const ArrayViewBaseT<Ty>& _lhs, const ArrayViewBaseT<Ty>& _rhs);

	/// Returns true if content of the view is sorted.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in] _src View to check.
	/// @param[in] _fn  Comparison function.
	///
	/// @returns True when sorted.
	///
	template<typename Ty>
	bool isSorted(const ArrayViewBaseT<Ty>& _src, const ComparisonFn _fn = compareAscending<Ty>);

	/// Returns index of the first element greater or equal than `_key`. `_src` must be sorted.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in] _key Key to search for.
	/// @param[in] _src Sorted view.
	/// @param[in] _fn  Comparison function.
	///
	/// @returns Element index, count of `_src` when every element is less than `_key`.
	///
	template<typename Ty>
	uint32_t lowerBound(const Ty& _key, const ArrayViewBaseT<Ty>& _src, const ComparisonFn _fn = compareAscending<Ty>);

	/// Returns index of the first element greater than `_key`. `_src` must be sorted.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in] _key Key to search for.
	/// @param[in] _src Sorted view.
	/// @param[in] _fn  Comparison function.
	///
	/// @returns Element index, count of `_src` when no element is greater than `_key`.
	///
	template<typename Ty>
	uint32_t upperBound(const Ty& _key, const ArrayViewBaseT<Ty>& _src, const ComparisonFn _fn = compareAscending<Ty>);

	/// Searches for `_key` in sorted `_src` using binary search.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in] _key Key to search for.
	/// @param[in] _src Sorted view.
	/// @param[in] _fn  Comparison function.
	///
	/// @returns Index of element if found, or bitwise complement (~) of the index of the next
	///   element that is larger than the key.
	///
	template<typename Ty>
	int32_t binarySearch(const Ty& _key, const ArrayViewBaseT<Ty>& _src, const ComparisonFn _fn = compareAscending<Ty>);

	/// Mutable, non-owning view into existing contiguous series of values.
	///
	/// @tparam Ty Element type.
	///
	template<typename Ty>
	class MutableArrayViewT : public ArrayViewBaseT<Ty>
	{
	public:
		/// Constructs an empty view.
		MutableArrayViewT();

		/// Constructs a view of `_count` elements at `_src`.
		///
		/// @param[in] _src   Pointer to the first element.
		/// @param[in] _count Number of elements.
		///
		MutableArrayViewT(Ty* _src, uint32_t _count);

		/// Sets the view to `_count` elements at `_src`.
		///
		/// @param[in] _src   Pointer to the first element.
		/// @param[in] _count Number of elements.
		///
		void set(Ty* _src, uint32_t _count);

		/// Constructs a view of the pointer range [`_src`, `_srcTerm`).
		///
		/// @param[in] _src     Pointer to the first element.
		/// @param[in] _srcTerm Pointer past the last element.
		///
		MutableArrayViewT(Ty* _src, Ty* _srcTerm);

		/// Sets the view to the pointer range [`_src`, `_srcTerm`).
		///
		/// @param[in] _src     Pointer to the first element.
		/// @param[in] _srcTerm Pointer past the last element.
		///
		void set(Ty* _src, Ty* _srcTerm);

		/// Constructs a view by reinterpreting a view of a different type.
		///
		/// @tparam Uy Element type of the source view.
		///
		/// @param[in] _src Source view.
		///
		template<typename Uy>
		MutableArrayViewT(ReinterpretTag, MutableArrayViewT<Uy>& _src);

		/// Sets the view by reinterpreting a view of a different type.
		///
		/// @tparam Uy Element type of the source view.
		///
		/// @param[in] _src Source view.
		///
		template<typename Uy>
		void set(ReinterpretTag, MutableArrayViewT<Uy>& _src);

		/// Constructs a view by reinterpreting raw memory as `_count` elements of `Ty`.
		///
		/// @param[in] _src   Pointer to raw memory.
		/// @param[in] _count Number of `Ty` elements.
		///
		MutableArrayViewT(ReinterpretTag, void* _src, uint32_t _count);

		/// Sets the view by reinterpreting raw memory as `_count` elements of `Ty`.
		///
		/// @param[in] _src   Pointer to raw memory.
		/// @param[in] _count Number of `Ty` elements.
		///
		void set(ReinterpretTag, void* _src, uint32_t _count);

		/// Constructs a view by reinterpreting the raw memory range [`_src`, `_srcTerm`).
		///
		/// @param[in] _src     Pointer to the beginning of raw memory.
		/// @param[in] _srcTerm Pointer past the end of raw memory.
		///
		MutableArrayViewT(ReinterpretTag, void* _src, void* _srcTerm);

		/// Sets the view by reinterpreting the raw memory range [`_src`, `_srcTerm`).
		///
		/// @param[in] _src     Pointer to the beginning of raw memory.
		/// @param[in] _srcTerm Pointer past the end of raw memory.
		///
		void set(ReinterpretTag, void* _src, void* _srcTerm);

		/// Constructs a view of a C-style array.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _src C-style array.
		///
		template<size_t SizeT>
		explicit MutableArrayViewT(Ty (&_src)[SizeT]);

		/// Sets the view to a C-style array.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _src C-style array.
		///
		template<size_t SizeT>
		void set(Ty (&_src)[SizeT]);

		/// Constructs a view of the same elements as `_src`.
		///
		/// @param[in] _src Source view.
		///
		MutableArrayViewT(MutableArrayViewT<Ty>& _src);

		/// Sets the view to the same elements as `_src`.
		///
		/// @param[in] _src Source view.
		///
		void set(MutableArrayViewT<Ty>& _src);

		/// Returns a mutable pointer to the element at the specified index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		Ty* getPtr(uint32_t _idx = 0);

		/// Returns a const pointer to the element at the specified index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		const Ty* getPtr(uint32_t _idx = 0) const;

		/// Returns a mutable pointer past the last element.
		///
		/// @returns Pointer past the last element.
		///
		Ty* getTerm();

		/// Returns a const pointer past the last element.
		///
		/// @returns Pointer past the last element.
		///
		const Ty* getTerm() const;

		/// Returns a mutable sub-view of `_count` elements starting at `_idx`.
		///
		/// @param[in] _idx   Index of the first element.
		/// @param[in] _count Number of elements.
		///
		/// @returns Sub-view.
		///
		MutableArrayViewT<Ty> get(uint32_t _idx, uint32_t _count);

		/// Returns a mutable sub-view of `_count` elements from the first or last position.
		///
		/// @param[in] _element `kFirst` or `kLast`.
		/// @param[in] _count   Number of elements.
		///
		/// @returns Sub-view.
		///
		MutableArrayViewT<Ty> get(Element _element, uint32_t _count);

		/// Returns a mutable reference to the element at the specified index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Reference to the element.
		///
		Ty& operator[](uint32_t _idx);

		/// Returns a const reference to the element at the specified index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Reference to the element.
		///
		const Ty& operator[](uint32_t _idx) const;
	};

	/// Sorts elements of a mutable array view in place.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in,out] _inoutSrc View to sort.
	/// @param[in]     _fn       Comparison function.
	///
	template<typename Ty>
	void sort(MutableArrayViewT<Ty>& _inoutSrc, const ComparisonFn _fn = compareAscending<Ty>);

	/// Sorts elements of a mutable array view in place, keeping order of equal elements.
	/// Scratch memory comes from the heap.
	///
	/// @tparam Ty     Element type.
	/// @tparam LessFn Comparison function type.
	///
	/// @param[in,out] _inoutSrc View to sort.
	/// @param[in]     _less     Returns true if `_lhs` sorts before `_rhs`, strict weak ordering.
	///
	template<typename Ty, typename LessFn>
	void stableSort(MutableArrayViewT<Ty>& _inoutSrc, LessFn _less);

	/// Removes consecutive duplicate elements from a sorted mutable array view.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in,out] _inoutSrc Sorted view to deduplicate.
	/// @param[in]     _fn       Comparison function.
	///
	/// @returns Number of unique elements remaining.
	///
	template<typename Ty>
	uint32_t unique(MutableArrayViewT<Ty>& _inoutSrc, const ComparisonFn _fn = compareAscending<Ty>);

	/// Immutable, non-owning view into existing contiguous series of values.
	///
	/// @tparam Ty Element type.
	///
	template<typename Ty>
	class ArrayViewT : public ArrayViewBaseT<Ty>
	{
	public:
		/// Constructs an empty view.
		ArrayViewT();

		/// Constructs a view of `_count` elements at `_src`.
		///
		/// @param[in] _src   Pointer to the first element.
		/// @param[in] _count Number of elements.
		///
		ArrayViewT(const Ty* _src, uint32_t _count);

		/// Sets the view to `_count` elements at `_src`.
		///
		/// @param[in] _src   Pointer to the first element.
		/// @param[in] _count Number of elements.
		///
		void set(const Ty* _src, uint32_t _count);

		/// Constructs a view of the pointer range [`_src`, `_srcTerm`).
		///
		/// @param[in] _src     Pointer to the first element.
		/// @param[in] _srcTerm Pointer past the last element.
		///
		ArrayViewT(const Ty* _src, const Ty* _srcTerm);

		/// Sets the view to the pointer range [`_src`, `_srcTerm`).
		///
		/// @param[in] _src     Pointer to the first element.
		/// @param[in] _srcTerm Pointer past the last element.
		///
		void set(const Ty* _src, const Ty* _srcTerm);

		/// Constructs a view by reinterpreting a view of a different type.
		///
		/// @tparam Uy Element type of the source view.
		///
		/// @param[in] _src Source view.
		///
		template<typename Uy>
		ArrayViewT(ReinterpretTag, const ArrayViewT<Uy>& _src);

		/// Sets the view by reinterpreting a view of a different type.
		///
		/// @tparam Uy Element type of the source view.
		///
		/// @param[in] _src Source view.
		///
		template<typename Uy>
		void set(ReinterpretTag, const ArrayViewT<Uy>& _src);

		/// Constructs a view by reinterpreting raw memory as `_count` elements of `Ty`.
		///
		/// @param[in] _src   Pointer to raw memory.
		/// @param[in] _count Number of `Ty` elements.
		///
		ArrayViewT(ReinterpretTag, const void* _src, uint32_t _count);

		/// Sets the view by reinterpreting raw memory as `_count` elements of `Ty`.
		///
		/// @param[in] _src   Pointer to raw memory.
		/// @param[in] _count Number of `Ty` elements.
		///
		void set(ReinterpretTag, const void* _src, uint32_t _count);

		/// Constructs a view by reinterpreting the raw memory range [`_src`, `_srcTerm`).
		///
		/// @param[in] _src     Pointer to the beginning of raw memory.
		/// @param[in] _srcTerm Pointer past the end of raw memory.
		///
		ArrayViewT(ReinterpretTag, const void* _src, const void* _srcTerm);

		/// Sets the view by reinterpreting the raw memory range [`_src`, `_srcTerm`).
		///
		/// @param[in] _src     Pointer to the beginning of raw memory.
		/// @param[in] _srcTerm Pointer past the end of raw memory.
		///
		void set(ReinterpretTag, const void* _src, const void* _srcTerm);

		/// Constructs a view of a const C-style array.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _src C-style array.
		///
		template<size_t SizeT>
		explicit ArrayViewT(const Ty (&_src)[SizeT]);

		/// Sets the view to a const C-style array.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _src C-style array.
		///
		template<size_t SizeT>
		void set(const Ty (&_src)[SizeT]);

		/// Constructs a view of the same elements as `_src`.
		///
		/// @param[in] _src Source view.
		///
		ArrayViewT(const ArrayViewBaseT<Ty>& _src);

		/// Sets the view to the same elements as `_src`.
		///
		/// @param[in] _src Source view.
		///
		void set(const ArrayViewBaseT<Ty>& _src);

		/// Returns a sub-view of `_count` elements starting at `_idx`.
		///
		/// @param[in] _idx   Index of the first element.
		/// @param[in] _count Number of elements.
		///
		/// @returns Sub-view.
		///
		ArrayViewT<Ty> get(uint32_t _idx, uint32_t _count) const;

		/// Returns a sub-view of `_count` elements from the first or last position.
		///
		/// @param[in] _element `kFirst` or `kLast`.
		/// @param[in] _count   Number of elements.
		///
		/// @returns Sub-view.
		///
		ArrayViewT<Ty> get(Element _element, uint32_t _count) const;
	};

	/// Range-for iterator over owning arrays. Removing the element under the iterator with
	/// `remove(const Ty*)` keeps it valid, and the next step visits the element that took its
	/// place. Removing other elements or adding elements during iteration is not supported.
	///
	/// @tparam ContainerT Array view type the iterator walks.
	/// @tparam Ty         Element type as returned by dereference.
	///
	template<typename ContainerT, typename Ty>
	struct ArrayIteratorT
	{
		/// Constructs iterator at `_idx`.
		///
		/// @param[in] _container Array to iterate.
		/// @param[in] _idx       Element index.
		///
		ArrayIteratorT(ContainerT& _container, uint32_t _idx);

		/// Returns element.
		///
		/// @returns Reference to the element.
		///
		Ty& operator*() const;

		/// Returns true if iterators differ.
		///
		/// @param[in] _rhs Iterator to compare against.
		///
		/// @returns True when iterators differ.
		///
		bool operator!=(const ArrayIteratorT<ContainerT, Ty>& _rhs) const;

		/// Advances to the next element.
		///
		/// @returns Reference to this.
		///
		const ArrayIteratorT<ContainerT, Ty>& operator++();

	private:
		ContainerT& m_container;
		uint32_t    m_idx;
		uint32_t    m_count;
	};

	/// Base class for owning array containers with contiguous storage.
	///
	/// @tparam Ty       Element type.
	/// @tparam StorageT Storage backend type (fixed or dynamic).
	///
	template<typename Ty, typename StorageT>
	class ArrayBaseT : public MutableArrayViewT<Ty>
	{
	public:
		/// Iterator over elements.
		using Iterator = ArrayIteratorT<MutableArrayViewT<Ty>, Ty>;

		/// Const iterator over elements.
		using ConstIterator = ArrayIteratorT<const ArrayViewBaseT<Ty>, const Ty>;

		/// Constructs an empty array.
		ArrayBaseT();

		/// Constructs an array by copying elements from an array view.
		///
		/// @param[in] _src Source view.
		///
		ArrayBaseT(const ArrayViewBaseT<Ty>& _src);

		/// Sets the array contents from an array view.
		///
		/// @param[in] _src Source view.
		///
		void set(const ArrayViewBaseT<Ty>& _src);

		/// Constructs copy of `_src`.
		///
		/// @param[in] _src Source array.
		///
		ArrayBaseT(const ArrayBaseT<Ty, StorageT>& _src);

		/// Constructs an array filled to storage capacity, each element constructed in place
		/// from `_args`.
		///
		/// @tparam ArgsT Element constructor argument types.
		///
		/// @param[in] _args Element constructor arguments.
		///
		template<typename... ArgsT>
		ArrayBaseT(EmplaceTag, ArgsT&&... _args);

		/// Constructs an array with `_count` elements, each constructed in place from `_args`.
		///
		/// @tparam ArgsT Element constructor argument types.
		///
		/// @param[in] _count Number of elements.
		/// @param[in] _args  Element constructor arguments.
		///
		template<typename... ArgsT>
		ArrayBaseT(uint32_t _count, EmplaceTag, ArgsT&&... _args);

		/// Destroys all elements and releases storage.
		~ArrayBaseT();

		/// Copies elements from an array view.
		///
		/// @param[in] _src Source view.
		///
		/// @returns Reference to this.
		///
		ArrayBaseT<Ty, StorageT>& operator=(const ArrayViewBaseT<Ty>& _src);

		/// Copies elements from `_src`.
		///
		/// @param[in] _src Source array.
		///
		/// @returns Reference to this.
		///
		ArrayBaseT<Ty, StorageT>& operator=(const ArrayBaseT<Ty, StorageT>& _src);

		/// Returns iterator to the first element.
		///
		/// @returns Iterator.
		///
		Iterator begin();

		/// Returns iterator past the last element.
		///
		/// @returns Iterator.
		///
		Iterator end();

		/// Returns iterator to the first element.
		///
		/// @returns Iterator.
		///
		ConstIterator begin() const;

		/// Returns iterator past the last element.
		///
		/// @returns Iterator.
		///
		ConstIterator end() const;

		/// Returns the maximum number of elements the array can hold without reallocation.
		///
		/// @returns Number of elements.
		///
		uint32_t getCapacity() const;

		/// Sets the storage capacity, reallocating as needed.
		///
		/// @param[in] _capacity Number of elements.
		///
		void setCapacity(uint32_t _capacity);

		/// Sets the number of elements, new elements are value initialized (zero for trivial types).
		///
		/// @param[in] _count Number of elements.
		///
		void setCount(uint32_t _count);

		/// Sets the number of elements, new elements are copies of `_value`.
		///
		/// @param[in] _count Number of elements.
		/// @param[in] _value Value copied into new elements.
		///
		void setCount(uint32_t _count, const Ty& _value);

		/// Sets the number of elements, new elements of trivial types keep storage contents.
		///
		/// @param[in] _count Number of elements.
		///
		void setCount(uint32_t _count, InitNoneTag);

		/// Removes all elements from the array.
		void clear();

		/// Reduces capacity to match the current element count.
		void trim();

		/// Appends a copy of an element to the end of the array.
		///
		/// @param[in] _src Element to copy.
		///
		void add(const Ty& _src);

		/// Inserts a copy of an element at the specified position.
		///
		/// @param[in] _pos Position to insert at.
		/// @param[in] _src Element to copy.
		///
		void add(uint32_t _pos, const Ty& _src);

		/// Appends a moved element to the end of the array.
		///
		/// @param[in] _src Element to move.
		///
		void add(Ty&& _src);

		/// Inserts a moved element at the specified position.
		///
		/// @param[in] _pos Position to insert at.
		/// @param[in] _src Element to move.
		///
		void add(uint32_t _pos, Ty&& _src);

		/// Appends all elements from a C-style array.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _src C-style array.
		///
		template<size_t SizeT>
		void add(const Ty (&_src)[SizeT]);

		/// Inserts all elements from a C-style array at the specified position.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _pos Position to insert at.
		/// @param[in] _src C-style array.
		///
		template<size_t SizeT>
		void add(uint32_t _pos, const Ty (&_src)[SizeT]);

		/// Appends an element constructed in place from `_args`.
		///
		/// @tparam ArgsT Element constructor argument types.
		///
		/// @param[in] _args Element constructor arguments.
		///
		template<typename... ArgsT>
		void add(EmplaceTag, ArgsT&&... _args);

		/// Inserts an element constructed in place from `_args` at the specified position.
		///
		/// @tparam ArgsT Element constructor argument types.
		///
		/// @param[in] _pos  Position to insert at.
		/// @param[in] _args Element constructor arguments.
		///
		template<typename... ArgsT>
		void add(uint32_t _pos, EmplaceTag, ArgsT&&... _args);

		/// Appends all elements from an array view.
		///
		/// @param[in] _src Source view.
		///
		void add(const ArrayViewBaseT<Ty>& _src);

		/// Inserts all elements from an array view at the specified position. View may point
		/// into this array.
		///
		/// @param[in] _pos Position to insert at.
		/// @param[in] _src Source view.
		///
		void add(uint32_t _pos, const ArrayViewBaseT<Ty>& _src);

		/// Removes `_count` elements starting at index `_idx`, elements from the end may take their
		/// place, same as `kUnordered`.
		///
		/// @param[in] _idx   Index of the first element.
		/// @param[in] _count Number of elements.
		///
		void remove(uint32_t _idx, uint32_t _count = 1);

		/// Removes `_count` elements starting at index `_idx` with specified ordering.
		///
		/// @param[in] _idx      Index of the first element.
		/// @param[in] _ordering kOrdered preserves order, kUnordered may swap with last element.
		/// @param[in] _count    Number of elements.
		///
		void remove(uint32_t _idx, Ordering _ordering, uint32_t _count = 1);

		/// Removes `_count` elements from the first or last position. The array is assumed ordered,
		/// the remaining elements keep their order.
		///
		/// @param[in] _element `kFirst` or `kLast`.
		/// @param[in] _count   Number of elements.
		///
		void remove(Element _element, uint32_t _count = 1);

		/// Removes the element at `_element`. Safe to call on the element under a range-for
		/// iterator, which then visits the element that took its place.
		///
		/// @tparam Uy Element type, `Ty` or `const Ty`.
		///
		/// @param[in] _element  Pointer to an element of this array.
		/// @param[in] _ordering kOrdered preserves order, kUnordered may swap with last element.
		///
		template<typename Uy>
		void remove(const Uy* _element, Ordering _ordering = kUnordered);

		/// Removes and returns the element at the specified index.
		///
		/// @param[in] _idx      Element index.
		/// @param[in] _ordering kOrdered preserves order, kUnordered may swap with last element.
		///
		/// @returns Removed element.
		///
		[[nodiscard]] Ty pop(uint32_t _idx, Ordering _ordering = kUnordered);

		/// Removes and returns the first or last element. The array is assumed ordered, the remaining
		/// elements keep their order.
		///
		/// @param[in] _element `kFirst` or `kLast`.
		///
		/// @returns Removed element.
		///
		[[nodiscard]] Ty pop(Element _element);

	protected:
		static constexpr bool isTypeless();
		static constexpr bool isRelocatable();

		void grow(uint32_t _num);
		bool isAliased(const Ty* _ptr) const;
		const Ty* insertTypeless(uint32_t _pos, const Ty* _src, uint32_t _num);

		BX_NO_INLINE const Ty* addSlow(uint32_t _pos, const Ty& _src);
		BX_NO_INLINE const Ty* addSlow(uint32_t _pos, const ArrayViewBaseT<Ty>& _src);

		template<typename... ArgsT>
		BX_NO_INLINE const Ty* addSlow(uint32_t _pos, EmplaceTag, ArgsT&&... _args);

		StorageT m_storage;
	};

	/// Fixed-capacity array with initial count 0.
	///
	/// @tparam Ty        Element type.
	/// @tparam CapacityT Maximum number of elements.
	///
	template<typename Ty, uint32_t CapacityT>
	using FixedArrayT = ArrayBaseT<Ty, FixedStorageT<Ty, CapacityT>>;

	/// Fixed-capacity array with initial count equal to capacity.
	///
	/// @tparam Ty        Element type.
	/// @tparam CapacityT Maximum number of elements.
	///
	template<typename Ty, uint32_t CapacityT>
	class StaticArrayT final : public FixedArrayT<Ty, CapacityT>
	{
	public:
		/// Constructs an array with count equal to capacity.
		StaticArrayT();
	};

	/// Dynamic array with memory managed by an allocator.
	///
	/// @tparam Ty Element type.
	///
	template<typename Ty>
	class ArrayT final : public ArrayBaseT<Ty, DynamicStorageT<Ty>>
	{
		BX_CLASS(ArrayT, TRIVIALLY_RELOCATABLE);

	public:
		/// Constructs an empty array on the process default allocator. Copy assignment into an
		/// empty default allocator array adopts the source's allocator.
		ArrayT();

		/// Constructs copy of `_src`.
		///
		/// @param[in] _src Source array.
		///
		ArrayT(const ArrayT<Ty>& _src);

		/// Constructs array taking over storage of `_src`.
		///
		/// @param[in] _src Source array, left empty.
		///
		ArrayT(ArrayT<Ty>&& _src);

		/// Constructs an empty array with the specified allocator.
		///
		/// @param[in] _allocator Allocator for element storage.
		///
		explicit ArrayT(AllocatorHandle _allocator);

		/// Constructs an empty array with the specified allocator and initial capacity.
		///
		/// @param[in] _allocator Allocator for element storage.
		/// @param[in] _capacity  Number of elements.
		///
		ArrayT(AllocatorHandle _allocator, uint32_t _capacity);

		/// Constructs an array with `_count` elements, each constructed in place from `_args`.
		///
		/// @tparam ArgsT Element constructor argument types.
		///
		/// @param[in] _allocator Allocator for element storage.
		/// @param[in] _count     Number of elements.
		/// @param[in] _args      Element constructor arguments.
		///
		template<typename... ArgsT>
		ArrayT(AllocatorHandle _allocator, uint32_t _count, EmplaceTag, ArgsT&&... _args);

		/// Constructs an array by copying elements from a C-style array.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _allocator Allocator for element storage.
		/// @param[in] _src       C-style array.
		///
		template<size_t SizeT>
		ArrayT(AllocatorHandle _allocator, const Ty (&_src)[SizeT]);

		/// Constructs an array by copying elements from an array view.
		///
		/// @param[in] _allocator Allocator for element storage.
		/// @param[in] _src       Source view.
		///
		ArrayT(AllocatorHandle _allocator, const ArrayViewBaseT<Ty>& _src);

		/// Sets the allocator of an array that holds no storage.
		///
		/// @param[in] _allocator Allocator.
		///
		void setAllocatorHandle(AllocatorHandle _allocator);

		/// Returns the handle of the allocator used by this array.
		///
		/// @returns Allocator handle.
		///
		AllocatorHandle getAllocatorHandle() const;

		/// Copies elements from an array view.
		///
		/// @param[in] _src Source view.
		///
		/// @returns Reference to this.
		///
		ArrayT<Ty>& operator=(const ArrayViewBaseT<Ty>& _src);

		/// Copies elements from `_src`. An array without allocator adopts the source's allocator.
		///
		/// @param[in] _src Source array.
		///
		/// @returns Reference to this.
		///
		ArrayT<Ty>& operator=(const ArrayT<Ty>& _src);

		/// Takes over storage of `_src`.
		///
		/// @param[in] _src Source array, left empty.
		///
		/// @returns Reference to this.
		///
		ArrayT<Ty>& operator=(ArrayT<Ty>&& _src);

		/// Sets the array contents from a C-style array.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _src C-style array.
		///
		template<size_t SizeT>
		void set(const Ty (&_src)[SizeT]);

		/// Sets the array contents from an array view.
		///
		/// @param[in] _src Source view.
		///
		void set(const ArrayViewBaseT<Ty>& _src);

		/// Sets the array contents by moving from another array.
		///
		/// @param[in] _src Source array, left empty.
		///
		void set(ArrayT<Ty>&& _src);

		/// Sets the array to a single element constructed in place from `_args`.
		///
		/// @tparam ArgsT Element constructor argument types.
		///
		/// @param[in] _args Element constructor arguments.
		///
		template<typename... ArgsT>
		void set(EmplaceTag, ArgsT&&... _args);
	};

	/// Writer that appends to array.
	///
	/// @tparam Ty Element type, `char` or `uint8_t`.
	///
	template<typename Ty>
	class ArrayWriterT final : public WriterI
	{
		static_assert(1 == sizeof(Ty), "Element type must be one byte.");

	public:
		/// Constructs writer.
		///
		/// @param[in] _array Array to append to.
		///
		ArrayWriterT(ArrayT<Ty>* _array);

		/// Destructor.
		virtual ~ArrayWriterT();

		/// Appends data to array.
		///
		/// @param[in]  _data Data.
		/// @param[in]  _size Size of data in bytes.
		/// @param[out] _err  Error, unused.
		///
		/// @returns Number of bytes written.
		///
		virtual int32_t write(const void* _data, int32_t _size, Error* _err) override;

	private:
		ArrayT<Ty>* m_array;
	};

	/// Base class for sorted array containers that keep elements ordered by `CmpFnT`.
	///
	/// @tparam Ty       Element type.
	/// @tparam StorageT Storage backend type (fixed or dynamic).
	/// @tparam CmpFnT   Comparison function.
	///
	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	class SortedArrayBaseT : public ArrayViewBaseT<Ty>
	{
	public:
		/// Const iterator over elements.
		using ConstIterator = ArrayIteratorT<const ArrayViewBaseT<Ty>, const Ty>;

		/// Constructs an empty sorted array.
		SortedArrayBaseT();

		/// Constructs a sorted array by copying from another sorted array.
		///
		/// @tparam StorageU Storage backend type of the source.
		/// @tparam CmpFnU   Comparison function of the source.
		///
		/// @param[in] _src Source sorted array.
		///
		template<typename StorageU, ComparisonFn CmpFnU>
		SortedArrayBaseT(const SortedArrayBaseT<Ty, StorageU, CmpFnU>& _src);

		/// Copy constructor.
		///
		/// @param[in] _src Source sorted array.
		///
		SortedArrayBaseT(const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src);

		/// Constructs a sorted array by copying and sorting elements from an array view.
		///
		/// @param[in] _src Source view.
		///
		SortedArrayBaseT(const ArrayViewBaseT<Ty>& _src);

		/// Sets the sorted array contents from an array view, sorting the elements.
		///
		/// @param[in] _src Source view.
		///
		void set(const ArrayViewBaseT<Ty>& _src);

		/// Copies and sorts elements from an array view.
		///
		/// @param[in] _src Source view.
		///
		/// @returns Reference to this.
		///
		SortedArrayBaseT<Ty, StorageT, CmpFnT>& operator=(const ArrayViewBaseT<Ty>& _src);

		/// Copies elements from `_src`.
		///
		/// @param[in] _src Source sorted array.
		///
		/// @returns Reference to this.
		///
		SortedArrayBaseT<Ty, StorageT, CmpFnT>& operator=(const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src);

		/// Destroys all elements and releases storage.
		~SortedArrayBaseT();

		/// Returns iterator to the first element.
		///
		/// @returns Iterator.
		///
		ConstIterator begin() const;

		/// Returns iterator past the last element.
		///
		/// @returns Iterator.
		///
		ConstIterator end() const;

		using ArrayViewBaseT<Ty>::getPtr;
		using ArrayViewBaseT<Ty>::getTerm;

		/// Returns a mutable pointer to the element at `_idx`, caller must not change the sort order.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Pointer to the element.
		///
		Ty* getPtr(UncheckedTag, uint32_t _idx = 0);

		/// Returns a mutable pointer past the last element, caller must not change the sort order.
		///
		/// @returns Pointer past the last element.
		///
		Ty* getTerm(UncheckedTag);

		/// Returns the maximum number of elements the array can hold without reallocation.
		///
		/// @returns Number of elements.
		///
		uint32_t getCapacity() const;

		/// Sets the storage capacity, reallocating as needed.
		///
		/// @param[in] _capacity Number of elements.
		///
		void setCapacity(uint32_t _capacity);

		/// Sets the number of elements. Constructs or destroys elements as needed.
		///
		/// @param[in] _count Number of elements.
		///
		void setCount(uint32_t _count);

		/// Removes all elements from the array.
		void clear();

		/// Reduces capacity to match the current element count.
		void trim();

		/// Inserts an element into its sorted position.
		///
		/// @param[in] _src Element to copy.
		///
		void add(const Ty& _src);

		/// Inserts an element into its sorted position, searching from `_pos`.
		///
		/// @param[in] _pos Position to start the search from.
		/// @param[in] _src Element to copy.
		///
		void add(uint32_t _pos, const Ty& _src);

		/// Inserts an element at `_pos`, which must be its sorted position.
		///
		/// @param[in] _pos Position to insert at.
		/// @param[in] _src Element to copy.
		///
		void add(UncheckedTag, uint32_t _pos, const Ty& _src);

		/// Inserts a moved element into its sorted position.
		///
		/// @param[in] _src Element to move.
		///
		void add(Ty&& _src);

		/// Inserts a moved element into its sorted position, searching from `_pos`.
		///
		/// @param[in] _pos Position to start the search from.
		/// @param[in] _src Element to move.
		///
		void add(uint32_t _pos, Ty&& _src);

		/// Inserts a moved element at `_pos`, which must be its sorted position.
		///
		/// @param[in] _pos Position to insert at.
		/// @param[in] _src Element to move.
		///
		void add(UncheckedTag, uint32_t _pos, Ty&& _src);

		/// Inserts all elements from a C-style array, maintaining sort order.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _src C-style array.
		///
		template<size_t SizeT>
		void add(const Ty (&_src)[SizeT]);

		/// Inserts all elements from an array view, maintaining sort order.
		///
		/// @param[in] _src Source view.
		///
		void add(const ArrayViewBaseT<Ty>& _src);

		/// Removes `_count` elements starting at index `_idx`.
		///
		/// @param[in] _idx   Index of the first element.
		/// @param[in] _count Number of elements.
		///
		void remove(uint32_t _idx, uint32_t _count = 1);

		/// Removes `_count` elements from the first or last position.
		///
		/// @param[in] _element `kFirst` or `kLast`.
		/// @param[in] _count   Number of elements.
		///
		void remove(Element _element, uint32_t _count = 1);

		/// Removes the element at `_element`. Safe to call on the element under a range-for
		/// iterator, which then visits the element that took its place.
		///
		/// @tparam Uy Element type, `Ty` or `const Ty`.
		///
		/// @param[in] _element Pointer to an element of this array.
		///
		template<typename Uy>
		void remove(const Uy* _element);

		/// Removes and returns the element at the specified index.
		///
		/// @param[in] _idx Element index.
		///
		/// @returns Removed element.
		///
		[[nodiscard]] Ty pop(uint32_t _idx);

		/// Removes and returns the first or last element.
		///
		/// @param[in] _element `kFirst` or `kLast`.
		///
		/// @returns Removed element.
		///
		[[nodiscard]] Ty pop(Element _element);

	protected:
		static constexpr bool isTypeless();
		static constexpr bool isRelocatable();

		void grow(uint32_t _num);
		bool isAliased(const Ty* _ptr) const;
		void addTypeless(uint32_t _pos, bool _search, const Ty& _src);
		void insertAt(uint32_t _pos, bool _search, const Ty& _src);
		void insertAt(uint32_t _pos, bool _search, Ty&& _src);
		bool isSortedAt(uint32_t _pos, const Ty& _value) const;

		StorageT m_storage;
	};

	/// Returns true if content of sorted array container is sorted.
	///
	/// @tparam Ty       Element type.
	/// @tparam StorageT Storage backend type.
	/// @tparam CmpFnT   Comparison function.
	///
	/// @param[in] _src Sorted array to check.
	///
	/// @returns True when sorted.
	///
	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	bool isSorted(const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src);

	/// Returns index of the first element greater or equal than `_key`.
	///
	/// @tparam Ty       Element type.
	/// @tparam StorageT Storage backend type.
	/// @tparam CmpFnT   Comparison function.
	///
	/// @param[in] _key Key to search for.
	/// @param[in] _src Sorted array.
	///
	/// @returns Element index, count of `_src` when every element is less than `_key`.
	///
	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	uint32_t lowerBound(const Ty& _key, const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src);

	/// Returns index of the first element greater than `_key`.
	///
	/// @tparam Ty       Element type.
	/// @tparam StorageT Storage backend type.
	/// @tparam CmpFnT   Comparison function.
	///
	/// @param[in] _key Key to search for.
	/// @param[in] _src Sorted array.
	///
	/// @returns Element index, count of `_src` when no element is greater than `_key`.
	///
	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	uint32_t upperBound(const Ty& _key, const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src);

	/// Searches for `_key` in a sorted array using binary search.
	///
	/// @tparam Ty       Element type.
	/// @tparam StorageT Storage backend type.
	/// @tparam CmpFnT   Comparison function.
	///
	/// @param[in] _key Key to search for.
	/// @param[in] _src Sorted array.
	///
	/// @returns Index of element if found, or bitwise complement (~) of the index of the next
	///   element that is larger than the key.
	///
	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	int32_t binarySearch(const Ty& _key, const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _src);

	/// Fixed-capacity sorted array.
	///
	/// @tparam Ty        Element type.
	/// @tparam CapacityT Maximum number of elements.
	/// @tparam CmpFnT    Comparison function.
	///
	template<typename Ty, uint32_t CapacityT, ComparisonFn CmpFnT = compareAscending<Ty>>
	using FixedSortedArrayT = SortedArrayBaseT<Ty, FixedStorageT<Ty, CapacityT>, CmpFnT>;

	/// Dynamic sorted array with memory managed by an allocator.
	///
	/// @tparam Ty     Element type.
	/// @tparam CmpFnT Comparison function.
	///
	template<typename Ty, ComparisonFn CmpFnT = compareAscending<Ty>>
	class SortedArrayT final : public SortedArrayBaseT<Ty, DynamicStorageT<Ty>, CmpFnT>
	{
		BX_CLASS(SortedArrayT, TRIVIALLY_RELOCATABLE);

	public:
		/// Constructs an empty sorted array on the process default allocator.
		SortedArrayT();

		/// Constructs an empty sorted array with the specified allocator.
		///
		/// @param[in] _allocator Allocator for element storage.
		///
		explicit SortedArrayT(AllocatorHandle _allocator);

		/// Constructs an empty sorted array with the specified allocator and initial capacity.
		///
		/// @param[in] _allocator Allocator for element storage.
		/// @param[in] _capacity  Number of elements.
		///
		SortedArrayT(AllocatorHandle _allocator, uint32_t _capacity);

		/// Copy constructor, uses the allocator of `_src`.
		///
		/// @param[in] _src Source sorted array.
		///
		SortedArrayT(const SortedArrayT<Ty, CmpFnT>& _src);

		/// Move constructor, takes the storage of `_src`.
		///
		/// @param[in] _src Source sorted array, left empty.
		///
		SortedArrayT(SortedArrayT<Ty, CmpFnT>&& _src);

		/// Constructs a sorted array with `_count` elements, each constructed in place from `_args`.
		///
		/// @tparam ArgsT Element constructor argument types.
		///
		/// @param[in] _allocator Allocator for element storage.
		/// @param[in] _count     Number of elements.
		/// @param[in] _args      Element constructor arguments.
		///
		template<typename... ArgsT>
		SortedArrayT(AllocatorHandle _allocator, uint32_t _count, EmplaceTag, ArgsT&&... _args);

		/// Constructs a sorted array by copying and sorting elements from a C-style array.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _allocator Allocator for element storage.
		/// @param[in] _src       C-style array.
		///
		template<size_t SizeT>
		SortedArrayT(AllocatorHandle _allocator, const Ty (&_src)[SizeT]);

		/// Constructs a sorted array by copying and sorting elements from an array view.
		///
		/// @param[in] _allocator Allocator for element storage.
		/// @param[in] _src       Source view.
		///
		SortedArrayT(AllocatorHandle _allocator, const ArrayViewBaseT<Ty>& _src);

		/// Returns the handle of the allocator used by this sorted array.
		///
		/// @returns Allocator handle.
		///
		AllocatorHandle getAllocatorHandle() const;

		/// Copies and sorts elements from an array view.
		///
		/// @param[in] _src Source view.
		///
		/// @returns Reference to this.
		///
		SortedArrayT<Ty, CmpFnT>& operator=(const ArrayViewBaseT<Ty>& _src);

		/// Copies elements from `_src`. An array without allocator adopts the source's allocator.
		///
		/// @param[in] _src Source sorted array.
		///
		/// @returns Reference to this.
		///
		SortedArrayT<Ty, CmpFnT>& operator=(const SortedArrayT<Ty, CmpFnT>& _src);

		/// Takes over storage of `_src`.
		///
		/// @param[in] _src Source sorted array, left empty.
		///
		/// @returns Reference to this.
		///
		SortedArrayT<Ty, CmpFnT>& operator=(SortedArrayT<Ty, CmpFnT>&& _src);

		/// Sets the sorted array contents from a C-style array.
		///
		/// @tparam SizeT Number of elements in the array.
		///
		/// @param[in] _src C-style array.
		///
		template<size_t SizeT>
		void set(const Ty (&_src)[SizeT]);

		/// Sets the sorted array contents from an array view.
		///
		/// @param[in] _src Source view.
		///
		void set(const ArrayViewBaseT<Ty>& _src);

		/// Sets the sorted array contents by moving from another sorted array.
		///
		/// @param[in] _src Source sorted array, left empty.
		///
		void set(SortedArrayT<Ty, CmpFnT>&& _src);

		/// Sets the sorted array to a single element constructed in place from `_args`.
		///
		/// @tparam ArgsT Element constructor argument types.
		///
		/// @param[in] _args Element constructor arguments.
		///
		template<typename... ArgsT>
		void set(EmplaceTag, ArgsT&&... _args);
	};

	/// Writes elements, without count. Element type must be trivially copyable.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in]  _writer Writer.
	/// @param[in]  _array  Array.
	/// @param[out] _err    Error.
	///
	/// @returns Number of bytes written.
	///
	template<typename Ty>
	int32_t write(WriterI* _writer, const ArrayViewT<Ty>& _array, Error* _err);

	/// Writes elements, without count. Element type must be trivially copyable.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in]  _writer Writer.
	/// @param[in]  _array  Array.
	/// @param[out] _err    Error.
	///
	/// @returns Number of bytes written.
	///
	template<typename Ty>
	int32_t write(WriterI* _writer, const MutableArrayViewT<Ty>& _array, Error* _err);

	/// Writes elements, without count. Element type must be trivially copyable.
	///
	/// @tparam Ty       Element type.
	/// @tparam StorageT Storage type.
	///
	/// @param[in]  _writer Writer.
	/// @param[in]  _array  Array.
	/// @param[out] _err    Error.
	///
	/// @returns Number of bytes written.
	///
	template<typename Ty, typename StorageT>
	int32_t write(WriterI* _writer, const ArrayBaseT<Ty, StorageT>& _array, Error* _err);

	/// Writes elements, without count. Element type must be trivially copyable.
	///
	/// @tparam Ty        Element type.
	/// @tparam CapacityT Maximum number of elements.
	///
	/// @param[in]  _writer Writer.
	/// @param[in]  _array  Array.
	/// @param[out] _err    Error.
	///
	/// @returns Number of bytes written.
	///
	template<typename Ty, uint32_t CapacityT>
	int32_t write(WriterI* _writer, const StaticArrayT<Ty, CapacityT>& _array, Error* _err);

	/// Writes elements, without count. Element type must be trivially copyable.
	///
	/// @tparam Ty Element type.
	///
	/// @param[in]  _writer Writer.
	/// @param[in]  _array  Array.
	/// @param[out] _err    Error.
	///
	/// @returns Number of bytes written.
	///
	template<typename Ty>
	int32_t write(WriterI* _writer, const ArrayT<Ty>& _array, Error* _err);

	/// Writes elements, without count. Element type must be trivially copyable.
	///
	/// @tparam Ty       Element type.
	/// @tparam StorageT Storage type.
	/// @tparam CmpFnT   Element comparison function.
	///
	/// @param[in]  _writer Writer.
	/// @param[in]  _array  Array.
	/// @param[out] _err    Error.
	///
	/// @returns Number of bytes written.
	///
	template<typename Ty, typename StorageT, ComparisonFn CmpFnT>
	int32_t write(WriterI* _writer, const SortedArrayBaseT<Ty, StorageT, CmpFnT>& _array, Error* _err);

	/// Writes elements, without count. Element type must be trivially copyable.
	///
	/// @tparam Ty     Element type.
	/// @tparam CmpFnT Element comparison function.
	///
	/// @param[in]  _writer Writer.
	/// @param[in]  _array  Array.
	/// @param[out] _err    Error.
	///
	/// @returns Number of bytes written.
	///
	template<typename Ty, ComparisonFn CmpFnT>
	int32_t write(WriterI* _writer, const SortedArrayT<Ty, CmpFnT>& _array, Error* _err);

} // namespace bx

#include "inline/array.inl"

#endif // BX_ARRAY_H_HEADER_GUARD
