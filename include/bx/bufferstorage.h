/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_BUFFERSTORAGE_H_HEADER_GUARD
#define BX_BUFFERSTORAGE_H_HEADER_GUARD

#include <bx/bx.h>
#include <bx/allocator.h>

namespace bx
{
	/// Element enum.
	enum class Element { First, Last };
	constexpr  Element kFirst = Element::First;
	constexpr  Element kLast  = Element::Last;

	/// Oredering enum.
	enum class Ordering { Ordered, Unordered };
	constexpr  Ordering kUnordered = Ordering::Unordered;
	constexpr  Ordering kOrdered   = Ordering::Ordered;

	/// Invalid sentinel.
	constexpr  uint32_t kInvalid = UINT32_MAX;

	template<typename Ty>
	struct StorageViewT final
	{
		const Ty* m_ptr;
		uint32_t  m_count;

		///
		StorageViewT() = delete;

		///
		StorageViewT(const Ty* _ptr, uint32_t _count);

		///
		StorageViewT(const StorageViewT<Ty>& _other);

		///
		~StorageViewT();

		///
		StorageViewT<Ty>& operator=(const StorageViewT<Ty>& _other);

		///
		StorageViewT<Ty>& operator=(StorageViewT<Ty>&& _other);

		///
		template<typename StorageT>
		void setCapacity(StorageT& _storage, uint32_t _capacity);

		///
		[[nodiscard]] uint32_t getCount() const;

		///
		template<typename StorageT>
		void setCount(StorageT& _storage, uint32_t _count);

		///
		template<typename StorageT>
		void add(StorageT& _storage, uint32_t _pos, const Ty* _ptr, uint32_t _count);

		///
		template<typename StorageT, typename... ArgsT>
		void add(StorageT& _storage, uint32_t _pos, EmplaceTag, ArgsT&&... _args);

		///
		template<typename StorageT>
		void remove(StorageT& _storage, uint32_t _pos, Ordering _ordering, uint32_t _count);

		///
		template<typename StorageT>
		[[nodiscard]] uint32_t carve(StorageT& _storage, uint32_t _pos, uint32_t _count, bool _init);
	};

	///
	class StorageTraits final
	{
	public:
		///
		template<typename Ty>
		[[nodiscard]] static uint32_t copy(Ty* _dst, const Ty* _src, uint32_t _count);

		///
		template<typename Ty>
		static void move(Ty* _dst, Ty* _src, uint32_t _count);

		///
		template<typename Ty>
		static EnableIfType<true
			&& !IsTriviallyConstructibleT<Ty>::value
			&& !IsConstructibleT<Ty, InitNoneTag>::value
			, void>
		constructDefault(Ty* _ptr, uint32_t _count);

		///
		template<typename Ty>
		static EnableIfType<true
			&& !IsTriviallyConstructibleT<Ty>::value
			&&  IsConstructibleT<Ty, InitNoneTag>::value
			, void>
		constructDefault(Ty* _ptr, uint32_t _count);

		///
		template<typename Ty>
		static EnableIfType<true
			&& IsTriviallyConstructibleT<Ty>::value
			, void>
		constructDefault(Ty* _ptr, uint32_t _count);

		///
		template<typename Ty, typename... ArgsT>
		[[nodiscard]] static uint32_t constructEmplace(Ty* _ptr, uint32_t _count, ArgsT&&... _args);

		///
		template<typename Ty>
		static void constructMove(Ty* _dst, Ty* _src, uint32_t _count);

		///
		template<typename Ty>
		static void destruct(Ty* _ptr, uint32_t _count);
	};

	///
	template<typename Ty, uint32_t CapacityT>
	class FixedStorageT final
	{
	public:
		///
		FixedStorageT();

		///
		~FixedStorageT();

		///
		constexpr uint32_t getCapacity() const;

		///
		void setCapacity(uint32_t _capacity, uint32_t _count);

		///
		Ty* getPtr(uint32_t _idx = 0);

		///
		const Ty* getPtr(uint32_t _idx = 0) const;

		///
		Ty* getTerm();

		///
		const Ty* getTerm() const;

		///
		FixedStorageT<Ty, CapacityT>& operator=(const FixedStorageT<Ty, CapacityT>& _other);

		///
		FixedStorageT<Ty, CapacityT>& operator=(FixedStorageT<Ty, CapacityT>&& _other);

	private:
		BX_ALIGN_DECL(alignof(Ty), uint8_t) m_data[sizeof(Ty)*CapacityT];
	};

	///
	template<typename Ty>
	class DynamicStorageT final
	{
	public:
		///
		DynamicStorageT();

		///
		~DynamicStorageT();

		///
		void setAllocator(AllocatorI* _allocator);

		///
		AllocatorI* getAllocator() const;

		///
		uint32_t getCapacity() const;

		///
		void setCapacity(uint32_t _capacity, uint32_t _count);

		///
		Ty* getPtr(uint32_t _idx = 0);

		///
		const Ty* getPtr(uint32_t _idx = 0) const;

		///
		Ty* getTerm();

		///
		const Ty* getTerm() const;

		///
		DynamicStorageT<Ty>& operator=(DynamicStorageT<Ty>&& _other);

	private:
		Ty*         m_ptr;
		uint32_t    m_capacity;
		AllocatorI* m_allocator;
	};

} // namespace bx

#include "inline/bufferstorage.inl"

#endif // BX_BUFFERSTORAGE_H_HEADER_GUARD
