/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_HASHMAP_H_HEADER_GUARD
#define BX_HASHMAP_H_HEADER_GUARD

#include <bx/array.h>
#include <bx/blockstorage.h>
#include <bx/hash.h>
#include <bx/simd_t.h>
#include <bx/storageblk.h>
#include <bx/string.h>

namespace bx
{
	namespace internal
	{
		using HashKeyFn = uint32_t (*)(const void* _key);

		using HashKeyMoveFn = void (*)(void* _dst, void* _src, bool _swap);

		template<typename Ty>
		void hashKeyMovePtr(void* _dst, void* _src, bool _swap);

	} // namespace internal

	/// Hash of characters, usable in constant expressions. wyhash with a fixed secret, low 32 bits.
	///
	/// @param[in] _data Characters.
	/// @param[in] _len Number of characters.
	///
	/// @returns Hash of characters.
	///
	constexpr uint32_t hashWy(const char* _data, uint32_t _len);

	/// Returns hash of trivially copyable key.
	///
	/// @tparam Ty Key type, trivially copyable.
	/// @param[in] _value Key.
	///
	/// @returns Hash.
	///
	template<typename Ty>
	uint32_t hashKey(const Ty& _value);

	/// Returns hash of characters in string view.
	///
	/// @param[in] _sv String view.
	///
	/// @returns Hash.
	///
	template<>
	uint32_t hashKey(const StringView& _sv);

	/// Returns hash of characters in string literal.
	///
	/// @param[in] _literal String literal.
	///
	/// @returns Hash.
	///
	template<>
	uint32_t hashKey(const StringLiteral& _literal);

	/// Scalar hash table group operations, 8 positions at a time.
	struct HashMapGroupRef
	{
		/// Group register type.
		using Type = simd64_t;

		/// Position mask type.
		using MaskType = uint64_t;

		static constexpr uint32_t kWidth = 8; //!< Number of positions per group.

		/// Loads group from unaligned `_ptr`.
		///
		/// @param[in] _ptr Pointer to group.
		///
		/// @returns Group.
		///
		static Type load(const uint8_t* _ptr);

		/// Returns mask of positions matching `_hash`.
		///
		/// @param[in] _group Group.
		/// @param[in] _hash  Hash to match.
		///
		/// @returns Position mask.
		///
		static MaskType match(Type _group, uint8_t _hash);

		/// Returns mask of empty positions.
		///
		/// @param[in] _group Group.
		///
		/// @returns Position mask.
		///
		static MaskType matchEmpty(Type _group);

		/// Returns true if any position matches `_hash`.
		///
		/// @param[in] _group Group.
		/// @param[in] _hash  Hash to match.
		///
		/// @returns True if any position matches.
		///
		static bool anyMatch(Type _group, uint8_t _hash);

		/// Returns lowest position matching `_hash`, or `kWidth` when none.
		///
		/// @param[in] _group Group.
		/// @param[in] _hash  Hash to match.
		///
		/// @returns Position, or `kWidth`.
		///
		static uint32_t matchFirst(Type _group, uint8_t _hash);

		/// Returns true if group has an empty position.
		///
		/// @param[in] _group Group.
		///
		/// @returns True if any position is empty.
		///
		static bool anyEmpty(Type _group);

		/// Returns mask of empty or deleted positions.
		///
		/// @param[in] _group Group.
		///
		/// @returns Position mask.
		///
		static MaskType matchEmptyOrDeleted(Type _group);

		/// Returns number of leading empty or deleted positions, or `kWidth` when all are.
		///
		/// @param[in] _group Group.
		///
		/// @returns Number of positions.
		///
		static uint32_t countLeadingEmptyOrDeleted(Type _group);

		/// Returns position of lowest set bit in `_mask`, which must not be 0.
		///
		/// @param[in] _mask Position mask, not 0.
		///
		/// @returns Position.
		///
		static uint32_t lowestIndex(MaskType _mask);

		/// Returns position of highest set bit in `_mask`, which must not be 0.
		///
		/// @param[in] _mask Position mask, not 0.
		///
		/// @returns Position.
		///
		static uint32_t highestIndex(MaskType _mask);

		/// Marks empty and deleted positions of group at `_ptr` empty, and full positions deleted.
		///
		/// @param[in,out] _ptr Pointer to group.
		///
		static void convertSpecialToEmptyAndFullToDeleted(uint8_t* _ptr);
	};

	/// SIMD hash table group operations, 16 positions at a time.
	struct HashMapGroupSimd
	{
		/// Group register type.
		using Type = simd128_t;

		/// Position mask type.
		using MaskType = uint32_t;

		static constexpr uint32_t kWidth = 16; //!< Number of positions per group.

		/// Loads group from unaligned `_ptr`.
		///
		/// @param[in] _ptr Pointer to group.
		///
		/// @returns Group.
		///
		static Type load(const uint8_t* _ptr);

		/// Returns mask of positions matching `_hash`.
		///
		/// @param[in] _group Group.
		/// @param[in] _hash  Hash to match.
		///
		/// @returns Position mask.
		///
		static MaskType match(Type _group, uint8_t _hash);

		/// Returns mask of empty positions.
		///
		/// @param[in] _group Group.
		///
		/// @returns Position mask.
		///
		static MaskType matchEmpty(Type _group);

		/// Returns true if any position matches `_hash`.
		///
		/// @param[in] _group Group.
		/// @param[in] _hash  Hash to match.
		///
		/// @returns True if any position matches.
		///
		static bool anyMatch(Type _group, uint8_t _hash);

		/// Returns lowest position matching `_hash`, or `kWidth` when none.
		///
		/// @param[in] _group Group.
		/// @param[in] _hash  Hash to match.
		///
		/// @returns Position, or `kWidth`.
		///
		static uint32_t matchFirst(Type _group, uint8_t _hash);

		/// Returns true if group has an empty position.
		///
		/// @param[in] _group Group.
		///
		/// @returns True if any position is empty.
		///
		static bool anyEmpty(Type _group);

		/// Returns mask of empty or deleted positions.
		///
		/// @param[in] _group Group.
		///
		/// @returns Position mask.
		///
		static MaskType matchEmptyOrDeleted(Type _group);

		/// Returns number of leading empty or deleted positions, or `kWidth` when all are.
		///
		/// @param[in] _group Group.
		///
		/// @returns Number of positions.
		///
		static uint32_t countLeadingEmptyOrDeleted(Type _group);

		/// Returns position of lowest set bit in `_mask`, which must not be 0.
		///
		/// @param[in] _mask Position mask, not 0.
		///
		/// @returns Position.
		///
		static uint32_t lowestIndex(MaskType _mask);

		/// Returns position of highest set bit in `_mask`, which must not be 0.
		///
		/// @param[in] _mask Position mask, not 0.
		///
		/// @returns Position.
		///
		static uint32_t highestIndex(MaskType _mask);

		/// Marks empty and deleted positions of group at `_ptr` empty, and full positions deleted.
		///
		/// @param[in,out] _ptr Pointer to group.
		///
		static void convertSpecialToEmptyAndFullToDeleted(uint8_t* _ptr);
	};

	/// SIMD hash table group operations, 32 positions at a time.
	struct HashMapGroupSimd256
	{
		/// Group register type.
		using Type = simd256_t;

		/// Position mask type.
		using MaskType = uint32_t;

		static constexpr uint32_t kWidth = 32; //!< Number of positions per group.

		/// Loads group from unaligned `_ptr`.
		///
		/// @param[in] _ptr Pointer to group.
		///
		/// @returns Group.
		///
		static Type load(const uint8_t* _ptr);

		/// Returns mask of positions matching `_hash`.
		///
		/// @param[in] _group Group.
		/// @param[in] _hash  Hash to match.
		///
		/// @returns Position mask.
		///
		static MaskType match(Type _group, uint8_t _hash);

		/// Returns mask of empty positions.
		///
		/// @param[in] _group Group.
		///
		/// @returns Position mask.
		///
		static MaskType matchEmpty(Type _group);

		/// Returns true if any position matches `_hash`.
		///
		/// @param[in] _group Group.
		/// @param[in] _hash  Hash to match.
		///
		/// @returns True if any position matches.
		///
		static bool anyMatch(Type _group, uint8_t _hash);

		/// Returns lowest position matching `_hash`, or `kWidth` when none.
		///
		/// @param[in] _group Group.
		/// @param[in] _hash  Hash to match.
		///
		/// @returns Position, or `kWidth`.
		///
		static uint32_t matchFirst(Type _group, uint8_t _hash);

		/// Returns true if group has an empty position.
		///
		/// @param[in] _group Group.
		///
		/// @returns True if any position is empty.
		///
		static bool anyEmpty(Type _group);

		/// Returns mask of empty or deleted positions.
		///
		/// @param[in] _group Group.
		///
		/// @returns Position mask.
		///
		static MaskType matchEmptyOrDeleted(Type _group);

		/// Returns number of leading empty or deleted positions, or `kWidth` when all are.
		///
		/// @param[in] _group Group.
		///
		/// @returns Number of positions.
		///
		static uint32_t countLeadingEmptyOrDeleted(Type _group);

		/// Returns position of lowest set bit in `_mask`, which must not be 0.
		///
		/// @param[in] _mask Position mask, not 0.
		///
		/// @returns Position.
		///
		static uint32_t lowestIndex(MaskType _mask);

		/// Returns position of highest set bit in `_mask`, which must not be 0.
		///
		/// @param[in] _mask Position mask, not 0.
		///
		/// @returns Position.
		///
		static uint32_t highestIndex(MaskType _mask);

		/// Marks empty and deleted positions of group at `_ptr` empty, and full positions deleted.
		///
		/// @param[in,out] _ptr Pointer to group.
		///
		static void convertSpecialToEmptyAndFullToDeleted(uint8_t* _ptr);
	};

#if BX_SIMD_SUPPORTED
	/// Default hash table group operations.
	using HashMapGroup = HashMapGroupSimd;
#else
	/// Default hash table group operations.
	using HashMapGroup = HashMapGroupRef;
#endif // BX_SIMD_SUPPORTED

	/// Slot index type for tables that reserve no slots, such as sets.
	struct HashMapNoSlots
	{
		/// Compares for equality, all values are equal.
		///
		/// @returns True.
		///
		constexpr bool operator==(HashMapNoSlots) const;

		/// Compares for inequality, all values are equal.
		///
		/// @returns False.
		///
		constexpr bool operator!=(HashMapNoSlots) const;
	};

	/// Slot index traits for `IndexT`.
	///
	/// @tparam IndexT Slot index type.
	///
	template<typename IndexT>
	struct HashMapSlotTraitsT
	{
		static constexpr bool   kHasSlots = true;                 //!< True when the table keeps slot indices.
		static constexpr bool   kIsValid  = isUnsigned<IndexT>(); //!< True when `IndexT` is a valid slot index type.
		static constexpr IndexT kInvalid  = IndexT(~IndexT(0) );  //!< Invalid slot index.
	};

	/// Slot index traits for tables without slots.
	template<>
	struct HashMapSlotTraitsT<HashMapNoSlots>
	{
		static constexpr bool           kHasSlots = false; //!< True when the table keeps slot indices.
		static constexpr bool           kIsValid  = true;  //!< True when `IndexT` is a valid slot index type.
		static constexpr HashMapNoSlots kInvalid  = {};    //!< Invalid slot index.
	};

	/// Typeless hash table of raw keys over a caller-owned block. Each key gets a slot index
	/// that stays fixed until the key is removed.
	///
	/// @tparam IndexT Slot index type (`uint16_t` or `uint32_t`), or `HashMapNoSlots`.
	/// @tparam GroupT Group operations, see `HashMapGroup`.
	///
	template<typename IndexT, typename GroupT>
	class HashMapControlViewT
	{
	public:
		/// Slot index type.
		using IndexType = IndexT;

		static constexpr bool kHasSlots = HashMapSlotTraitsT<IndexT>::kHasSlots; //!< True when the table keeps slot indices.

		static_assert(HashMapSlotTraitsT<IndexT>::kIsValid, "IndexT must be unsigned integer type or HashMapNoSlots.");

		static constexpr IndexT kInvalidSlot = HashMapSlotTraitsT<IndexT>::kInvalid; //!< Slot index returned when key is not found.

		static constexpr uint32_t kMinCapacity = 2; //!< Smallest table capacity.

		/// Constructs a table with no block; call `init` before use.
		HashMapControlViewT();

		/// Returns capacity mask (number of table positions minus one) for requested capacity.
		///
		/// @param[in] _capacity Number of elements.
		///
		/// @returns Capacity mask.
		///
		static constexpr uint32_t calcCapacityMask(uint32_t _capacity);

		/// Returns capacity mask for the table grown one step from `_capacityMask`.
		///
		/// @param[in] _capacityMask Current capacity mask.
		///
		/// @returns Grown capacity mask.
		///
		static constexpr uint32_t calcGrownCapacityMask(uint32_t _capacityMask);

		/// Returns largest number of elements a table with capacity mask can hold.
		///
		/// @param[in] _capacityMask Capacity mask.
		///
		/// @returns Maximum number of elements.
		///
		static constexpr uint32_t calcMaxCount(uint32_t _capacityMask);

		/// Returns size in bytes of block needed for a table.
		///
		/// @param[in] _keySize      Key size in bytes.
		/// @param[in] _keyAlign     Key alignment in bytes, power of two.
		/// @param[in] _capacityMask Capacity mask.
		/// @param[in] _maxFreeSlots Minimum number of released slots the block can hold.
		///
		/// @returns Size in bytes.
		///
		static constexpr uint32_t calcSize(uint32_t _keySize, uint32_t _keyAlign, uint32_t _capacityMask, uint32_t _maxFreeSlots);

		/// Sets up an empty table in a block of `calcSize` bytes.
		///
		/// @param[in] _data         Block, or NULL for a table with capacity 0 that must grow on first add.
		/// @param[in] _keySize      Key size in bytes.
		/// @param[in] _keyAlign     Key alignment in bytes, power of two.
		/// @param[in] _capacityMask Capacity mask.
		/// @param[in] _maxFreeSlots Minimum number of released slots the block can hold.
		/// @param[in] _hashKeyFn    Key hash function.
		/// @param[in] _keyMoveFn    Key move function, or NULL for keys moved with memory copy.
		///
		void init(void* _data, uint32_t _keySize, uint32_t _keyAlign, uint32_t _capacityMask, uint32_t _maxFreeSlots, internal::HashKeyFn _hashKeyFn, internal::HashKeyMoveFn _keyMoveFn);

		/// Moves all elements into block `_data` of `calcSize` bytes; the old block is no longer referenced.
		///
		/// @param[in] _data         New block.
		/// @param[in] _capacityMask Must hold `getCount()` elements and `getNumFreeSlots()` free slots.
		///
		void rehashInto(void* _data, uint32_t _capacityMask);

		/// Removes all elements.
		void clear();

		/// Returns number of table positions.
		///
		/// @returns Number of table positions.
		///
		uint32_t getCapacity() const;

		/// Returns capacity mask.
		///
		/// @returns Capacity mask.
		///
		uint32_t getCapacityMask() const;

		/// Returns number of elements.
		///
		/// @returns Number of elements.
		///
		uint32_t getCount() const;

		/// Returns true when the table holds no elements.
		///
		/// @returns True when empty.
		///
		bool isEmpty() const;

		/// Returns one past the highest slot index reserved so far.
		///
		/// @returns Slot capacity.
		///
		uint32_t getSlotCapacity() const;

		/// Returns largest slot capacity reachable without growing the table.
		///
		/// @returns Slot limit.
		///
		uint32_t getSlotLimit() const;

		/// Returns number of released slots waiting for reuse.
		///
		/// @returns Number of free slots.
		///
		uint32_t getNumFreeSlots() const;

		/// Returns size of block in use.
		///
		/// @returns Size in bytes.
		///
		uint32_t getSizeInBytes() const;

		/// Returns size of key in bytes.
		///
		/// @returns Size in bytes.
		///
		uint32_t getKeySize() const;

		/// Returns control bytes.
		///
		/// @returns Pointer to control bytes.
		///
		const uint8_t* getControl() const;

		/// Returns keys as raw bytes, `getKeySize()` per table position.
		///
		/// @returns Pointer to keys.
		///
		const uint8_t* getKeys() const;

		/// Returns keys as raw bytes, `getKeySize()` per table position.
		///
		/// @returns Pointer to keys.
		///
		uint8_t* getKeys();

		/// Returns slot per table position.
		///
		/// @returns Pointer to slots.
		///
		const IndexT* getSlots() const;

		/// Returns first occupied position at or after `_idx`, or the end position.
		///
		/// @param[in] _idx Table position to start at.
		///
		/// @returns Table position.
		///
		uint32_t findFirstFull(uint32_t _idx) const;

		/// Returns true when `_idx` is the end position.
		///
		/// @param[in] _idx Table position.
		///
		/// @returns True at the end position.
		///
		bool isSentinel(uint32_t _idx) const;

		/// Returns first free position for `_hash`.
		///
		/// @param[in] _hash Key hash.
		///
		/// @returns Table position.
		///
		uint32_t findFirstEmpty(uint32_t _hash) const;

		/// Reserves position and slot for a key that is not in the table. On success the caller
		/// stores the key at `getKeys() + _idx*getKeySize()`.
		///
		/// @param[in]  _hash Key hash.
		/// @param[out] _idx  Table position for the key.
		/// @param[out] _slot Slot of the key.
		///
		/// @returns False when the table must grow first via `rehashInto`.
		///
		bool tryAdd(uint32_t _hash, uint32_t& _idx, IndexT& _slot);

		/// Removes element at position `_idx`. Returns released slot.
		///
		/// @param[in] _idx Table position, must hold an element.
		///
		/// @returns Released slot.
		///
		IndexT removeAt(uint32_t _idx);

	protected:
		static constexpr uint8_t  kEmpty        = 0x80;
		static constexpr uint8_t  kNotEmptyMask = 0x7f;
		static constexpr uint8_t  kDeleted      = 0xfe;
		static constexpr uint8_t  kSentinel     = 0xff;
		static constexpr uint32_t kWidth        = GroupT::kWidth;

		struct MatchIterator
		{
			MatchIterator(uint32_t _capacityMask, uint32_t _hash);

			void next();

			uint32_t m_index;
			uint32_t m_offset;
			uint32_t m_capacityMask;
		};

		void setControl(uint32_t _idx, uint8_t _control);
		IndexT allocSlot();
		void moveKey(uint8_t* _dst, uint8_t* _src);
		void swapKey(uint8_t* _lhs, uint8_t* _rhs);
		void dropDeletesWithoutResize();
		void convertDeletedToEmptyAndFullToDeleted();
		static bool isInSameGroup(uint32_t _hash, uint32_t _capacityMask, uint32_t _idxA, uint32_t _idxB);

		uint8_t*      m_control;
		uint8_t*      m_keys;
		IndexT*       m_slots;
		IndexT*       m_freeSlots;
		internal::HashKeyFn     m_hashKeyFn;
		internal::HashKeyMoveFn m_keyMoveFn;

		uint32_t m_capacityMask;
		uint32_t m_count;
		uint32_t m_growthLeft;
		uint32_t m_slotCapacity;
		uint32_t m_numFreeSlots;
		uint32_t m_maxFreeSlots;
		uint32_t m_keySize;
		uint32_t m_keyAlign;
#if BX_CONFIG_DEBUG
		uint32_t m_generation;
#endif // BX_CONFIG_DEBUG
	};

	/// Hash table of keys with slot indices, owning its block through `StorageT`.
	///
	/// @tparam KeyT         Key type.
	/// @tparam StorageT     Block storage (`DynamicStorageT<uint8_t>` or `FixedStorageT<uint8_t, N>`).
	/// @tparam IndexT       Slot index type (`uint16_t` or `uint32_t`).
	/// @tparam MinCapacityT Minimum table capacity.
	/// @tparam GroupT       Group operations, see `HashMapGroup`.
	///
	template<
		  typename KeyT
		, typename StorageT
		, typename IndexT = uint32_t
		, uint32_t MinCapacityT = 0
		, typename GroupT = HashMapGroup
		>
	class HashMapControlBaseT : public HashMapControlViewT<IndexT, GroupT>
	{
		BX_CLASS_NO_COPY(HashMapControlBaseT);

	public:
		/// Typeless table type.
		using ViewType = HashMapControlViewT<IndexT, GroupT>;

		using ViewType::kInvalidSlot; //!< Slot index returned when key is not found.

		/// Key iterator. Removing the element under the iterator keeps it valid, adding elements may invalidate it.
		struct Iterator
		{
			/// Constructs iterator at table position `_idx`, end by default.
			///
			/// @param[in] _control Table to iterate.
			/// @param[in] _idx     Table position, `UINT32_MAX` for end.
			///
			Iterator(const HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>& _control, uint32_t _idx = UINT32_MAX);

			/// Returns the iterator itself.
			///
			/// @returns Reference to this.
			///
			const Iterator& operator*() const;

			/// Returns true when iterators point at different elements.
			///
			/// @param[in] _rhs Iterator to compare with.
			///
			/// @returns True when different.
			///
			bool operator!=(const Iterator& _rhs) const;

			/// Advances to the next element.
			///
			/// @returns Reference to this.
			///
			const Iterator& operator++();

			/// Advances to the next element.
			///
			/// @returns False at the end.
			///
			bool next();

			/// Returns key of element.
			///
			/// @returns Key.
			///
			const KeyT& getKey() const;

			/// Returns slot of element.
			///
			/// @returns Slot.
			///
			IndexT getSlot() const;

			/// Returns table position of element.
			///
			/// @returns Table position.
			///
			uint32_t getPosition() const;

		private:
			const HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>& m_control;
			uint32_t m_idx;
#if BX_CONFIG_DEBUG
			uint32_t m_generation;
#endif // BX_CONFIG_DEBUG
		};

		/// Sets table capacity. Fixed storage can't grow past its size.
		///
		/// @param[in] _capacity Number of elements.
		///
		void setCapacity(uint32_t _capacity);

		/// Returns iterator at the first element.
		///
		/// @returns Iterator.
		///
		Iterator begin() const;

		/// Returns end iterator.
		///
		/// @returns End iterator.
		///
		Iterator end() const;

		/// Returns key at table position.
		///
		/// @param[in] _idx Table position, must hold an element.
		///
		/// @returns Key.
		///
		const KeyT& getKey(uint32_t _idx) const;

		/// Returns true when key is in the table.
		///
		/// @param[in] _key Key.
		///
		/// @returns True when found.
		///
		bool contains(const KeyT& _key) const;

		/// Returns slot of key, or `kInvalidSlot`.
		///
		/// @param[in] _key Key.
		///
		/// @returns Slot, `kInvalidSlot` when not found.
		///
		IndexT findSlot(const KeyT& _key) const;

		/// Reserves slot for key.
		///
		/// @param[in]  _key  Key.
		/// @param[out] _slot Slot of key, or `kInvalidSlot` when the table is full.
		///
		/// @returns False when key already exists, or when the table is full and can't grow.
		///
		bool tryAdd(const KeyT& _key, IndexT& _slot);

		/// Adds key. Returns false when key already exists, or when the table is full and can't grow.
		///
		/// @param[in] _key Key.
		///
		/// @returns False when key exists or the table is full.
		///
		bool tryAdd(const KeyT& _key);

		/// Reserves slot for key that must not exist.
		///
		/// @param[in] _key Key, must not exist.
		///
		/// @returns Slot of key.
		///
		[[nodiscard]] IndexT add(const KeyT& _key);

		/// Removes key. Returns false when key doesn't exist, otherwise `_slot` receives released slot.
		///
		/// @param[in]  _key  Key.
		/// @param[out] _slot Released slot.
		///
		/// @returns False when key doesn't exist.
		///
		bool tryRemove(const KeyT& _key, IndexT& _slot);

		/// Removes key. Returns false when key doesn't exist.
		///
		/// @param[in] _key Key.
		///
		/// @returns False when key doesn't exist.
		///
		bool tryRemove(const KeyT& _key);

		/// Removes key that must exist. Returns released slot.
		///
		/// @param[in] _key Key, must exist.
		///
		/// @returns Released slot.
		///
		IndexT remove(const KeyT& _key);

		/// Removes element at iterator, which stays valid. Returns released slot.
		///
		/// @param[in] _it Iterator at element to remove.
		///
		/// @returns Released slot.
		///
		IndexT remove(const Iterator& _it);

		/// Removes all keys, keeps capacity.
		void clear();

		/// Shrinks storage to fit the elements.
		void trim();

	protected:
		static constexpr internal::HashKeyMoveFn kKeyMoveFn = isTriviallyCopyable<KeyT>() ? NULL : internal::hashKeyMovePtr<KeyT>;

		HashMapControlBaseT();
		void set(HashMapControlBaseT<KeyT, StorageT, IndexT, MinCapacityT, GroupT>&& _src);
		~HashMapControlBaseT();
		void destructKeys();
		void destructKey(uint32_t _idx);
		uint32_t findIndex(const KeyT& _key, uint32_t _hash) const;
		bool resize(uint32_t _capacityMask);
		void debugPtrUpdate();

		StorageT m_storage;

#if BX_CONFIG_DEBUG
		KeyT* m_debugKeys;
#endif // BX_CONFIG_DEBUG
	};

	/// Hash table of keys with slot indices and dynamic storage.
	///
	/// @tparam KeyT         Key type.
	/// @tparam IndexT       Slot index type (`uint16_t` or `uint32_t`).
	/// @tparam MinCapacityT Minimum table capacity.
	/// @tparam GroupT       Group operations, see `HashMapGroup`.
	///
	template<
		  typename KeyT
		, typename IndexT = uint32_t
		, uint32_t MinCapacityT = 0
		, typename GroupT = HashMapGroup
		>
	class HashMapControlT final : public HashMapControlBaseT<
		  KeyT
		, DynamicStorageT<uint8_t>
		, IndexT
		, MinCapacityT
		, GroupT
		>
	{
	public:
		/// Constructs an empty table on the process default allocator.
		HashMapControlT();

		/// Constructs by taking over `_src`, which is left empty.
		///
		/// @param[in] _src Table to move from.
		///
		HashMapControlT(HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>&& _src);

		/// Constructs an empty table on `_allocator`.
		///
		/// @param[in] _allocator Allocator for table storage.
		///
		explicit HashMapControlT(AllocatorHandle _allocator);

		/// Constructs an empty table on `_allocator` with room for `_capacity` elements.
		///
		/// @param[in] _allocator Allocator for table storage.
		/// @param[in] _capacity  Number of elements.
		///
		HashMapControlT(AllocatorHandle _allocator, uint32_t _capacity);

		/// Takes over `_src`, which is left empty.
		///
		/// @param[in] _src Table to move from.
		///
		/// @returns Reference to this.
		///
		HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>& operator=(HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>&& _src);

		/// Takes over table of `_src`, which is left empty.
		///
		/// @param[in] _src Table to move from.
		///
		void set(HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>&& _src);

		/// Returns allocator handle.
		///
		/// @returns Allocator handle.
		///
		AllocatorHandle getAllocatorHandle() const;
	};

	/// Fixed storage for the block of a table holding `CapacityT` keys.
	///
	/// @tparam KeyT      Key type.
	/// @tparam CapacityT Maximum number of elements.
	/// @tparam IndexT    Slot index type (`uint16_t` or `uint32_t`), or `HashMapNoSlots`.
	/// @tparam GroupT    Group operations, see `HashMapGroup`.
	///
	template<typename KeyT, uint32_t CapacityT, typename IndexT, typename GroupT>
	using FixedHashMapControlStorageT = FixedStorageT<
		  uint64_t
		, (HashMapControlViewT<IndexT, GroupT>::calcSize(
			  sizeof(KeyT)
			, alignof(KeyT)
			, HashMapControlViewT<IndexT, GroupT>::calcCapacityMask(CapacityT)
			, 0
			) + 7)/8
		>;

	/// Hash table of keys with slot indices and fixed storage. Adding to a full table fails.
	///
	/// @tparam KeyT      Key type.
	/// @tparam CapacityT Maximum number of elements.
	/// @tparam IndexT    Slot index type (`uint16_t` or `uint32_t`).
	/// @tparam GroupT    Group operations, see `HashMapGroup`.
	///
	template<
		  typename KeyT
		, uint32_t CapacityT
		, typename IndexT = uint32_t
		, typename GroupT = HashMapGroup
		>
	class FixedHashMapControlT final : public HashMapControlBaseT<
		  KeyT
		, FixedHashMapControlStorageT<KeyT, CapacityT, IndexT, GroupT>
		, IndexT
		, CapacityT
		, GroupT
		>
	{
	public:
		static_assert(alignof(KeyT) <= alignof(uint64_t), "KeyT alignment exceeds fixed storage alignment.");

		/// Constructs an empty table.
		FixedHashMapControlT();
	};

	/// Hash set built from a table.
	///
	/// @tparam KeyT     Key type.
	/// @tparam ControlT Table type.
	///
	template<typename KeyT, typename ControlT>
	class HashSetBaseT
	{
		BX_CLASS_NO_COPY(HashSetBaseT);

	public:
		/// Table type.
		using ControlType = ControlT;

		/// Key iterator. Removing the element under the iterator keeps it valid, adding elements may invalidate it.
		using Iterator = typename ControlT::Iterator;

		/// Sets table capacity. Fixed storage can't grow past its size.
		///
		/// @param[in] _capacity Number of elements.
		///
		void setCapacity(uint32_t _capacity);

		/// Returns iterator at the first element.
		///
		/// @returns Iterator.
		///
		Iterator begin() const;

		/// Returns end iterator.
		///
		/// @returns End iterator.
		///
		Iterator end() const;

		/// Returns number of table positions.
		///
		/// @returns Number of table positions.
		///
		uint32_t getCapacity() const;

		/// Returns number of elements.
		///
		/// @returns Number of elements.
		///
		uint32_t getCount() const;

		/// Returns true when the set holds no elements.
		///
		/// @returns True when empty.
		///
		bool isEmpty() const;

		/// Returns underlying table.
		///
		/// @returns Table.
		///
		const ControlT& getControl() const;

		/// Returns true when key is in the set.
		///
		/// @param[in] _key Key.
		///
		/// @returns True when found.
		///
		bool contains(const KeyT& _key) const;

		/// Adds key. Returns false when key exists, or when the set is full and can't grow.
		///
		/// @param[in] _key Key.
		///
		/// @returns False when key exists or the set is full.
		///
		bool tryAdd(const KeyT& _key);

		/// Adds key that must not exist.
		///
		/// @param[in] _key Key, must not exist.
		///
		void add(const KeyT& _key);

		/// Removes key. Returns false when key doesn't exist.
		///
		/// @param[in] _key Key.
		///
		/// @returns False when key doesn't exist.
		///
		bool tryRemove(const KeyT& _key);

		/// Removes key that must exist.
		///
		/// @param[in] _key Key, must exist.
		///
		void remove(const KeyT& _key);

		/// Removes element at iterator, which stays valid.
		///
		/// @param[in] _it Iterator at element to remove.
		///
		void remove(const Iterator& _it);

		/// Removes all elements.
		void clear();

		/// Shrinks storage to fit the elements.
		void trim();

	protected:
		template<typename... ArgsT>
		HashSetBaseT(ArgsT&&... _args);
		~HashSetBaseT();
		void set(HashSetBaseT<KeyT, ControlT>&& _src);

		ControlT m_control;
	};

	/// Hash set with dynamic storage.
	///
	/// @tparam KeyT         Key type.
	/// @tparam MinCapacityT Minimum table capacity.
	/// @tparam GroupT       Group operations, see `HashMapGroup`.
	///
	template<typename KeyT, uint32_t MinCapacityT = 0, typename GroupT = HashMapGroup>
	class HashSetT final : public HashSetBaseT<KeyT, HashMapControlT<KeyT, HashMapNoSlots, MinCapacityT, GroupT>>
	{
		BX_CLASS(HashSetT, TRIVIALLY_RELOCATABLE);

	public:
		/// Constructs an empty set on the process default allocator.
		HashSetT();

		/// Constructs by taking over `_src`, which is left empty.
		///
		/// @param[in] _src Set to move from.
		///
		HashSetT(HashSetT<KeyT, MinCapacityT, GroupT>&& _src);

		/// Constructs an empty set on `_allocator`.
		///
		/// @param[in] _allocator Allocator for set storage.
		///
		explicit HashSetT(AllocatorHandle _allocator);

		/// Constructs an empty set on `_allocator` with room for `_capacity` elements.
		///
		/// @param[in] _allocator Allocator for set storage.
		/// @param[in] _capacity  Number of elements.
		///
		HashSetT(AllocatorHandle _allocator, uint32_t _capacity);

		/// Takes over `_src`, which is left empty.
		///
		/// @param[in] _src Set to move from.
		///
		/// @returns Reference to this.
		///
		HashSetT<KeyT, MinCapacityT, GroupT>& operator=(HashSetT<KeyT, MinCapacityT, GroupT>&& _src);

		/// Takes over elements of `_src`, which is left empty.
		///
		/// @param[in] _src Set to move from.
		///
		void set(HashSetT<KeyT, MinCapacityT, GroupT>&& _src);

		/// Returns allocator handle.
		///
		/// @returns Allocator handle.
		///
		AllocatorHandle getAllocatorHandle() const;
	};

	/// Hash set with fixed storage for `CapacityT` elements. Adding to a full set fails.
	///
	/// @tparam KeyT      Key type.
	/// @tparam CapacityT Maximum number of elements.
	/// @tparam GroupT    Group operations, see `HashMapGroup`.
	///
	template<typename KeyT, uint32_t CapacityT, typename GroupT = HashMapGroup>
	class FixedHashSetT final : public HashSetBaseT<KeyT, FixedHashMapControlT<KeyT, CapacityT, HashMapNoSlots, GroupT>>
	{
	public:
		/// Constructs an empty set.
		FixedHashSetT();
	};

	/// Hash map built from a table and slot-indexed value storage.
	///
	/// Values are constructed when added, destroyed when removed, and move only when value
	/// storage grows.
	///
	/// @tparam KeyT     Key type.
	/// @tparam ValueT   Value type.
	/// @tparam ControlT Table type.
	/// @tparam StorageT Value storage, addressed by slot.
	///
	template<typename KeyT, typename ValueT, typename ControlT, typename StorageT>
	class HashMapBaseT
	{
		BX_CLASS_NO_COPY(HashMapBaseT);

	public:
		/// Table type.
		using ControlType = ControlT;

		/// Slot index type.
		using IndexType = typename ControlT::IndexType;

		static constexpr IndexType kInvalidSlot = ControlT::kInvalidSlot; //!< Slot index returned when key is not found.

		/// Key and value iterator. Removing the element under the iterator keeps it valid, adding elements may invalidate it.
		///
		/// @tparam MapT Map type the iterator walks.
		/// @tparam Vy   Value type as returned by `getValue`.
		///
		template<typename MapT, typename Vy>
		struct IteratorT
		{
			/// Constructs iterator over `_hashMap` at table iterator `_it`.
			///
			/// @param[in] _hashMap Map to iterate.
			/// @param[in] _it      Table iterator.
			///
			IteratorT(MapT& _hashMap, const typename ControlT::Iterator& _it);

			/// Returns the iterator itself.
			///
			/// @returns Reference to this.
			///
			const IteratorT<MapT, Vy>& operator*() const;

			/// Returns true when iterators point at different elements.
			///
			/// @param[in] _rhs Iterator to compare with.
			///
			/// @returns True when different.
			///
			bool operator!=(const IteratorT<MapT, Vy>& _rhs) const;

			/// Advances to the next element.
			///
			/// @returns Reference to this.
			///
			const IteratorT<MapT, Vy>& operator++();

			/// Advances to the next element.
			///
			/// @returns False at the end.
			///
			bool next();

			/// Returns key of element.
			///
			/// @returns Key.
			///
			const KeyT& getKey() const;

			/// Returns value of element.
			///
			/// @returns Value.
			///
			Vy& getValue() const;

			/// Returns slot of element.
			///
			/// @returns Slot.
			///
			IndexType getSlot() const;

		private:
			friend class HashMapBaseT<KeyT, ValueT, ControlT, StorageT>;

			MapT& m_hashMap;
			typename ControlT::Iterator m_it;
		};

		/// Key and value iterator.
		using Iterator = IteratorT<HashMapBaseT<KeyT, ValueT, ControlT, StorageT>, ValueT>;

		/// Key and const value iterator.
		using ConstIterator = IteratorT<const HashMapBaseT<KeyT, ValueT, ControlT, StorageT>, const ValueT>;

		/// Sets table capacity. Fixed storage can't grow past its size.
		///
		/// @param[in] _capacity Number of elements.
		///
		void setCapacity(uint32_t _capacity);

		/// Returns iterator at the first element.
		///
		/// @returns Iterator.
		///
		Iterator begin();

		/// Returns end iterator.
		///
		/// @returns End iterator.
		///
		Iterator end();

		/// Returns iterator at the first element.
		///
		/// @returns Iterator.
		///
		ConstIterator begin() const;

		/// Returns end iterator.
		///
		/// @returns End iterator.
		///
		ConstIterator end() const;

		/// Returns number of table positions.
		///
		/// @returns Number of table positions.
		///
		uint32_t getCapacity() const;

		/// Returns number of elements.
		///
		/// @returns Number of elements.
		///
		uint32_t getCount() const;

		/// Returns true when the map holds no elements.
		///
		/// @returns True when empty.
		///
		bool isEmpty() const;

		/// Returns underlying table.
		///
		/// @returns Table.
		///
		const ControlT& getControl() const;

		/// Returns value at `_slot`.
		///
		/// @param[in] _slot Slot of element.
		///
		/// @returns Value.
		///
		ValueT& getValue(IndexType _slot);

		/// Returns value at `_slot`.
		///
		/// @param[in] _slot Slot of element.
		///
		/// @returns Value.
		///
		const ValueT& getValue(IndexType _slot) const;

		/// Returns slot of key, or `kInvalidSlot`. The slot stays valid while the element exists.
		///
		/// @param[in] _key Key.
		///
		/// @returns Slot, `kInvalidSlot` when not found.
		///
		IndexType findSlot(const KeyT& _key) const;

		/// Returns pointer to value of key, or NULL. Valid until an add grows value storage.
		///
		/// @param[in] _key Key.
		///
		/// @returns Pointer to value, NULL when not found.
		///
		ValueT* find(const KeyT& _key);

		/// Returns pointer to value of key, or NULL. Valid until an add grows value storage.
		///
		/// @param[in] _key Key.
		///
		/// @returns Pointer to value, NULL when not found.
		///
		const ValueT* find(const KeyT& _key) const;

		/// Returns value of key, adding key with value initialized value when it doesn't exist.
		/// Map must not be full. Adding can move values, invalidating references into the map
		/// taken before the call, use `add(_key, _value, true)` to copy a value of the same map.
		///
		/// @param[in] _key Key.
		///
		/// @returns Reference to value.
		///
		ValueT& getOrAdd(const KeyT& _key);

		/// Adds key with copy of value. Returns false when key exists and `_update` is false, or
		/// when the map is full and can't grow.
		///
		/// @param[in] _key    Key.
		/// @param[in] _value  Value to copy.
		/// @param[in] _update Replace value when key exists.
		///
		/// @returns False when key exists and `_update` is false, or the map is full.
		///
		bool tryAdd(const KeyT& _key, const ValueT& _value, bool _update = false);

		/// Adds key with moved value. Returns false when key exists and `_update` is false, or
		/// when the map is full and can't grow.
		///
		/// @param[in] _key    Key.
		/// @param[in] _value  Value to move.
		/// @param[in] _update Replace value when key exists.
		///
		/// @returns False when key exists and `_update` is false, or the map is full.
		///
		bool tryAdd(const KeyT& _key, ValueT&& _value, bool _update = false);

		/// Adds key with copy of value. Key must not exist unless `_update` is true.
		///
		/// @param[in] _key    Key.
		/// @param[in] _value  Value to copy.
		/// @param[in] _update Replace value when key exists.
		///
		void add(const KeyT& _key, const ValueT& _value, bool _update = false);

		/// Adds key with moved value. Key must not exist unless `_update` is true.
		///
		/// @param[in] _key    Key.
		/// @param[in] _value  Value to move.
		/// @param[in] _update Replace value when key exists.
		///
		void add(const KeyT& _key, ValueT&& _value, bool _update = false);

		/// Adds key with value constructed in place from `_args`. Returns false when key exists,
		/// or when the map is full and can't grow.
		///
		/// @tparam ArgsT Value constructor argument types.
		/// @param[in] _key  Key.
		/// @param[in] _args Value constructor arguments.
		///
		/// @returns False when key exists or the map is full.
		///
		template<typename... ArgsT>
		bool tryAdd(const KeyT& _key, EmplaceTag, ArgsT&&... _args);

		/// Adds key that must not exist, with value constructed in place from `_args`.
		///
		/// @tparam ArgsT Value constructor argument types.
		/// @param[in] _key  Key, must not exist.
		/// @param[in] _args Value constructor arguments.
		///
		template<typename... ArgsT>
		void add(const KeyT& _key, EmplaceTag, ArgsT&&... _args);

		/// Removes key. Returns false when key doesn't exist.
		///
		/// @param[in] _key Key.
		///
		/// @returns False when key doesn't exist.
		///
		bool tryRemove(const KeyT& _key);

		/// Removes key that must exist.
		///
		/// @param[in] _key Key, must exist.
		///
		void remove(const KeyT& _key);

		/// Removes element at iterator, which stays valid.
		///
		/// @param[in] _it Iterator at element to remove.
		///
		void remove(const Iterator& _it);

		/// Removes all elements.
		void clear();

		/// Shrinks storage to fit the elements.
		void trim();

	protected:
		template<typename... ArgsT>
		HashMapBaseT(ArgsT&&... _args);
		~HashMapBaseT();
		void set(HashMapBaseT<KeyT, ValueT, ControlT, StorageT>&& _src);
		ValueT* getValuePtrForSlot(IndexType _slot);
		bool isAliased(const ValueT* _ptr) const;
		void relocateValues(uint32_t _capacity, IndexType _skipSlot);
		void destructValues();

		ControlT m_control;
		StorageT m_values;
	};

	/// Hash map with dynamic storage for table and values. Values move when adding grows value
	/// storage, pointers and references into the map are invalidated, use `BlockHashMapT` for
	/// stable value addresses.
	///
	/// @tparam KeyT         Key type.
	/// @tparam ValueT       Value type.
	/// @tparam IndexT       Slot index type (`uint16_t` or `uint32_t`).
	/// @tparam MinCapacityT Minimum table capacity.
	/// @tparam GroupT       Group operations, see `HashMapGroup`.
	///
	template<
		  typename KeyT
		, typename ValueT
		, typename IndexT = uint32_t
		, uint32_t MinCapacityT = 0
		, typename GroupT = HashMapGroup
		>
	class HashMapT final : public HashMapBaseT<
		  KeyT
		, ValueT
		, HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>
		, DynamicStorageT<ValueT>
		>
	{
		BX_CLASS(HashMapT, TRIVIALLY_RELOCATABLE);

	public:
		/// Constructs an empty map on the process default allocator.
		HashMapT();

		/// Constructs by taking over `_src`, which is left empty.
		///
		/// @param[in] _src Map to move from.
		///
		HashMapT(HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>&& _src);

		/// Constructs an empty map on `_allocator`.
		///
		/// @param[in] _allocator Allocator for table and value storage.
		///
		explicit HashMapT(AllocatorHandle _allocator);

		/// Constructs an empty map on `_allocator` with room for `_capacity` elements.
		///
		/// @param[in] _allocator Allocator for table and value storage.
		/// @param[in] _capacity  Number of elements.
		///
		HashMapT(AllocatorHandle _allocator, uint32_t _capacity);

		/// Takes over `_src`, which is left empty.
		///
		/// @param[in] _src Map to move from.
		///
		/// @returns Reference to this.
		///
		HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>& operator=(HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>&& _src);

		/// Takes over elements of `_src`, which is left empty.
		///
		/// @param[in] _src Map to move from.
		///
		void set(HashMapT<KeyT, ValueT, IndexT, MinCapacityT, GroupT>&& _src);

		/// Returns allocator handle.
		///
		/// @returns Allocator handle.
		///
		AllocatorHandle getAllocatorHandle() const;
	};

	/// Hash map with dynamic table storage and values in fixed-size blocks. Value addresses
	/// are stable for the lifetime of the element. Each block is allocated whole, use smaller
	/// `BlockCapacityT` for small maps.
	///
	/// @tparam KeyT           Key type.
	/// @tparam ValueT         Value type.
	/// @tparam BlockCapacityT Number of values per block, power of two.
	/// @tparam IndexT         Slot index type (`uint16_t` or `uint32_t`).
	/// @tparam MinCapacityT   Minimum table capacity.
	/// @tparam GroupT         Group operations, see `HashMapGroup`.
	///
	template<
		  typename KeyT
		, typename ValueT
		, uint32_t BlockCapacityT = 4096
		, typename IndexT = uint32_t
		, uint32_t MinCapacityT = 0
		, typename GroupT = HashMapGroup
		>
	class BlockHashMapT final : public HashMapBaseT<
		  KeyT
		, ValueT
		, HashMapControlT<KeyT, IndexT, MinCapacityT, GroupT>
		, BlockStorageT<ValueT, BlockCapacityT>
		>
	{
		BX_CLASS(BlockHashMapT, TRIVIALLY_RELOCATABLE);

	public:
		/// Constructs an empty map on the process default allocator.
		BlockHashMapT();

		/// Constructs by taking over `_src`, which is left empty.
		///
		/// @param[in] _src Map to move from.
		///
		BlockHashMapT(BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>&& _src);

		/// Constructs an empty map on `_allocator`.
		///
		/// @param[in] _allocator Allocator for table and value storage.
		///
		explicit BlockHashMapT(AllocatorHandle _allocator);

		/// Constructs an empty map on `_allocator` with room for `_capacity` elements.
		///
		/// @param[in] _allocator Allocator for table and value storage.
		/// @param[in] _capacity  Number of elements.
		///
		BlockHashMapT(AllocatorHandle _allocator, uint32_t _capacity);

		/// Takes over `_src`, which is left empty.
		///
		/// @param[in] _src Map to move from.
		///
		/// @returns Reference to this.
		///
		BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>& operator=(BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>&& _src);

		/// Takes over elements of `_src`, which is left empty.
		///
		/// @param[in] _src Map to move from.
		///
		void set(BlockHashMapT<KeyT, ValueT, BlockCapacityT, IndexT, MinCapacityT, GroupT>&& _src);

		/// Returns allocator handle.
		///
		/// @returns Allocator handle.
		///
		AllocatorHandle getAllocatorHandle() const;
	};

	/// Hash map with fixed storage for `CapacityT` elements. Adding to a full map fails.
	///
	/// @tparam KeyT      Key type.
	/// @tparam ValueT    Value type.
	/// @tparam CapacityT Maximum number of elements.
	/// @tparam IndexT    Slot index type (`uint16_t` or `uint32_t`).
	/// @tparam GroupT    Group operations, see `HashMapGroup`.
	///
	template<
		  typename KeyT
		, typename ValueT
		, uint32_t CapacityT
		, typename IndexT = uint32_t
		, typename GroupT = HashMapGroup
		>
	class FixedHashMapT final : public HashMapBaseT<
		  KeyT
		, ValueT
		, FixedHashMapControlT<KeyT, CapacityT, IndexT, GroupT>
		, FixedStorageT<ValueT, CapacityT>
		>
	{
	public:
		/// Constructs an empty map.
		FixedHashMapT();

	private:
#if BX_CONFIG_DEBUG
		ValueT* m_debugValues;
#endif // BX_CONFIG_DEBUG
	};

} // namespace bx

#include "inline/hashmap.inl"

#endif // BX_HASHMAP_H_HEADER_GUARD
