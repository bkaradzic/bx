/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HANDLE_ALLOC_H_HEADER_GUARD
#define BX_HANDLE_ALLOC_H_HEADER_GUARD

#include "bx.h"
#include "simd_t.h"
#include "bitarray.h"

namespace bx
{
	constexpr uint16_t kInvalidHandle = UINT16_MAX;

	/// Handle allocator backed by a bit set in caller-provided storage.
	///
	class HandleAlloc
	{
		BX_CLASS(HandleAlloc, NO_COPY);

	public:
		/// `_bits` must hold at least `_maxHandles` bits; the allocator starts empty.
		///
		HandleAlloc(MippedBitArrayBase& _bits, uint16_t _maxHandles);

		///
		uint16_t getNumHandles() const;

		///
		uint16_t getMaxHandles() const;

		/// Allocates the lowest free handle, or returns `kInvalidHandle`.
		///
		uint16_t alloc();

		///
		bool isValid(uint16_t _handle) const;

		///
		void free(uint16_t _handle);

		///
		void reset();

		/// Returns the lowest allocated handle, or `kInvalidHandle`.
		///
		uint16_t findFirst() const;

		/// Returns the lowest allocated handle above `_handle`, or `kInvalidHandle`.
		///
		uint16_t findNext(uint16_t _handle) const;

		///
		const BitArrayViewBase& getBits() const;

	protected:
		///
		HandleAlloc(InitNoneTag, MippedBitArrayBase& _bits, uint16_t _maxHandles);

	private:
		HandleAlloc();
		uint16_t findFrom(uint32_t _start) const;

		MippedBitArrayBase& m_bits;
		uint16_t m_maxHandles;
		uint16_t m_numHandles;
	};

	/// Bit set handle allocator with embedded storage.
	///
	template<uint16_t MaxHandlesT>
	class HandleAllocT : public HandleAlloc
	{
	public:
		///
		HandleAllocT();

	private:
		FixedMippedBitArrayT<MaxHandlesT> m_storage;
	};

	/// Bit set handle allocator with storage from a block allocator.
	///
	class DynamicHandleAlloc final : public HandleAlloc
	{
	public:
		/// Constructs allocator for `_maxHandles` handles.
		///
		/// @param[in] _allocator Allocator for bit set storage.
		/// @param[in] _maxHandles Maximum number of handles.
		///
		DynamicHandleAlloc(AllocatorHandle _allocator, uint16_t _maxHandles);

	private:
		MippedBitArray m_storage;
	};

	/// Former name of `HandleAlloc`, kept until dependents move to it.
	///
	using HandleAlloc2 = HandleAlloc;

	/// Former name of `HandleAllocT`, kept until dependents move to it.
	///
	/// @tparam MaxHandlesT Maximum number of handles.
	///
	template<uint16_t MaxHandlesT>
	using HandleAlloc2T = HandleAllocT<MaxHandlesT>;

	///
	template <uint16_t MaxHandlesT>
	class HandleListT
	{
	public:
		///
		HandleListT();

		///
		void pushBack(uint16_t _handle);

		///
		uint16_t popBack();

		///
		void pushFront(uint16_t _handle);

		///
		uint16_t popFront();

		///
		uint16_t getFront() const;

		///
		uint16_t getBack() const;

		///
		uint16_t getNext(uint16_t _handle) const;

		///
		uint16_t getPrev(uint16_t _handle) const;

		///
		void remove(uint16_t _handle);

		///
		void reset();

	private:
		///
		void insertBefore(uint16_t _before, uint16_t _handle);

		///
		void insertAfter(uint16_t _after, uint16_t _handle);

		///
		bool isValid(uint16_t _handle) const;

		///
		void updateFrontBack(uint16_t _handle);

		uint16_t m_front;
		uint16_t m_back;

		struct Link
		{
			uint16_t m_prev;
			uint16_t m_next;
		};

		Link m_links[MaxHandlesT];
	};

	/// Handle allocator that keeps allocated handles in least recently used order.
	///
	template <uint16_t MaxHandlesT>
	class HandleAllocLruT
	{
	public:
		///
		HandleAllocLruT();

		///
		~HandleAllocLruT();

		///
		uint16_t getNumHandles() const;

		///
		uint16_t getMaxHandles() const;

		///
		uint16_t alloc();

		///
		bool isValid(uint16_t _handle) const;

		///
		void free(uint16_t _handle);

		///
		void touch(uint16_t _handle);

		///
		uint16_t getFront() const;

		///
		uint16_t getBack() const;

		///
		uint16_t getNext(uint16_t _handle) const;

		///
		uint16_t getPrev(uint16_t _handle) const;

		///
		void reset();

	private:
		HandleListT<MaxHandlesT>  m_list;
		HandleAllocT<MaxHandlesT> m_alloc;
	};

	///
	template <uint32_t MaxCapacityT, typename KeyT = uint32_t>
	class HandleHashMapT
	{
	public:
		///
		HandleHashMapT();

		///
		~HandleHashMapT();

		///
		bool insert(KeyT _key, uint16_t _handle);

		///
		bool removeByKey(KeyT _key);

		///
		bool removeByHandle(uint16_t _handle);

		///
		uint16_t find(KeyT _key) const;

		///
		void reset();

		///
		uint32_t getNumElements() const;

		///
		uint32_t getMaxCapacity() const;

		///
		struct Iterator
		{
			uint16_t handle;

		private:
			friend class HandleHashMapT<MaxCapacityT, KeyT>;
			uint32_t pos;
			uint32_t num;
		};

		///
		Iterator first() const;

		///
		bool next(Iterator& _it) const;

	private:
		///
		uint32_t findIndex(KeyT _key) const;

		///
		void removeIndex(uint32_t _idx);

		///
		uint32_t mix(uint32_t _x) const;

		///
		uint64_t mix(uint64_t _x) const;

		uint32_t m_maxCapacity;
		uint32_t m_numElements;

		KeyT     m_key[MaxCapacityT];
		uint16_t m_handle[MaxCapacityT];
	};

} // namespace bx

#include "inline/handlealloc.inl"

#endif // BX_HANDLE_ALLOC_H_HEADER_GUARD
