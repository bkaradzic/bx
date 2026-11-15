/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_STRINGPOOL_H_HEADER_GUARD
#define BX_STRINGPOOL_H_HEADER_GUARD

#include <bx/blk.h>
#include <bx/array.h>
#include <bx/hashmap.h>
#include <bx/mutex.h>
#include <bx/string.h>

namespace bx
{
	class String;
	class StringBuilder;
	class StringPool;

	/// String pool entry. Immortal entries (literals, offline data) are never counted or freed.
	///
	struct StringPoolEntry
	{
		static constexpr uint8_t kFlagImmortal      = 1<<0; //!< Entry is never counted or freed.
		static constexpr uint8_t kFlagCpuAccessible = 1<<1; //!< Characters are readable by the CPU.

		uint32_t m_refCount;      //!< Number of references.
		uint32_t m_hash;          //!< Hash of characters.
		uint32_t m_length;        //!< Number of characters.
		uint32_t m_bookKeeping;   //!< Allocator book keeping.
		uint32_t m_size;          //!< Allocated block size.
		uint16_t m_slotIndex;     //!< Allocator slot index.
		uint8_t  m_log2Alignment; //!< Log2 of allocation alignment.
		uint8_t  m_flags;         //!< `kFlag*` flags.
#if BX_CONFIG_MEMORY_TRACKING
		uint32_t m_locationIdx;   //!< Allocation location index.
#endif // BX_CONFIG_MEMORY_TRACKING

		/// Returns characters.
		///
		/// @returns Zero terminated characters.
		///
		const char* getCPtr() const;

		/// Returns true if entry is immortal.
		///
		/// @returns True if entry is never counted or freed.
		///
		bool isImmortal() const;

		/// Returns view of characters.
		///
		/// @returns View of characters, without terminator.
		///
		StringView getView() const;
	};

	/// Literal string pool entry, interned without copying. Remembers the handle it was last
	/// interned with, so constructing `String` from it again takes no lock.
	///
	/// @tparam SizeT Size of literal, including terminator.
	///
	template<uint32_t SizeT>
	struct StringLiteralT : StringPoolEntry
	{
		/// Constructs entry from literal.
		///
		/// @param[in] _str String literal.
		///
		constexpr StringLiteralT(const char (&_str)[SizeT]);

		char             m_data[SizeT];  //!< Characters and terminator.
		mutable uint32_t m_cachedHandle; //!< Handle from the last intern into the default pool, 0 before first use.
	};

	/// Declares literal string pool entry, constant initialized. Usable at namespace, class, or
	/// function scope.
	///
	/// @param _name Entry name.
	/// @param _str C string literal.
	///
#define BX_STRING_LITERAL(_name, _str) \
	static constinit bx::StringLiteralT<sizeof(_str)> _name(_str)

	namespace internal
	{
		template<uint32_t SizeT>
		struct StringLiteralTextT
		{
			consteval StringLiteralTextT(const char (&_str)[SizeT]);

			char m_data[SizeT];
		};

	} // namespace internal

	/// Immutable, reference counted string in a `StringPool`. Default constructed string is the
	/// empty string, never null. Text is built or changed only with `StringBuilder`.
	///
	class String
	{
		BX_CLASS(String, TRIVIALLY_RELOCATABLE);

	public:
		static constexpr uint32_t kSlotBits  = 24;                //!< Number of slot bits in handle.
		static constexpr uint32_t kSlotMask  = (1<<kSlotBits)-1;  //!< Slot mask of handle.
		static constexpr uint32_t kEmptySlot = 0;                 //!< Slot of the empty string.

		/// Constructs empty string.
		///
		String();

		/// Copy constructor.
		///
		/// @param[in] _src String to copy.
		///
		String(const String& _src);

		/// Move constructor.
		///
		/// @param[in,out] _src String to move from, becomes empty.
		///
		String(String&& _src);

		/// Interns string in the default pool.
		///
		/// @param[in] _str Characters.
		///
		explicit String(const StringView& _str);

		/// Interns string.
		///
		/// @param[in] _str Characters.
		/// @param[in] _pool Pool, or default pool when NULL.
		///
		String(const StringView& _str, StringPool* _pool);

		/// Interns builder text in builder's pool.
		///
		/// @param[in] _builder Builder.
		///
		String(const StringBuilder& _builder);

		/// References immortal entry through the default pool.
		///
		/// @param[in] _entry Immortal entry.
		///
		String(const StringPoolEntry& _entry);

		/// References literal through the default pool. After the first call the handle comes
		/// from `_literal`, without lock or table lookup.
		///
		/// @tparam SizeT Size of literal, including terminator.
		///
		/// @param[in] _literal Literal entry.
		///
		template<uint32_t SizeT>
		String(const StringLiteralT<SizeT>& _literal);

		/// Releases reference.
		///
		~String();

		/// Copy assignment.
		///
		/// @param[in] _src String to copy.
		///
		/// @returns Reference to this string.
		///
		String& operator=(const String& _src);

		/// Move assignment.
		///
		/// @param[in,out] _src String to move from, becomes empty.
		///
		/// @returns Reference to this string.
		///
		String& operator=(String&& _src);

		/// Interns builder text in builder's pool.
		///
		/// @param[in] _builder Builder.
		///
		/// @returns Reference to this string.
		///
		String& operator=(const StringBuilder& _builder);

		/// References immortal entry through the default pool.
		///
		/// @param[in] _entry Immortal entry.
		///
		/// @returns Reference to this string.
		///
		String& operator=(const StringPoolEntry& _entry);

		/// Releases reference, string becomes empty.
		///
		void clear();

		/// Returns true if string is empty.
		///
		/// @returns True if string has no characters.
		///
		bool isEmpty() const;

		/// Returns length.
		///
		/// @returns Number of characters.
		///
		int32_t getLength() const;

		/// Returns hash.
		///
		/// @returns Hash of characters, same as `hashWy` of them.
		///
		uint32_t getHash() const;

		/// Returns characters.
		///
		/// @returns Zero terminated characters, "" when empty.
		///
		const char* getCPtr() const;

		/// Returns pool.
		///
		/// @returns Pool holding the string.
		///
		StringPool* getPool() const;

		/// Returns handle.
		///
		/// @returns Pool id and slot, usable as POD key for strings of one pool.
		///
		uint32_t getHandle() const;

		/// Returns slot.
		///
		/// @returns Slot in pool.
		///
		uint32_t getSlot() const;

		/// Returns pool id.
		///
		/// @returns Id of pool holding the string.
		///
		uint8_t getPoolId() const;

		/// Returns view of characters.
		///
		/// @returns View of characters, without terminator.
		///
		operator StringView() const;

		/// Compares strings.
		///
		/// @param[in] _rhs String to compare with.
		///
		/// @returns True if same entry, or same characters across pools.
		///
		bool operator==(const String& _rhs) const;

		/// Compares strings.
		///
		/// @param[in] _rhs String to compare with.
		///
		/// @returns True if characters differ.
		///
		bool operator!=(const String& _rhs) const;

		/// Compares string with characters.
		///
		/// @param[in] _rhs Characters to compare with.
		///
		/// @returns True if characters are equal.
		///
		bool operator==(const StringView& _rhs) const;

		/// Compares string with characters.
		///
		/// @param[in] _rhs Characters to compare with.
		///
		/// @returns True if characters differ.
		///
		bool operator!=(const StringView& _rhs) const;

	private:
		friend class StringPool;

		explicit String(uint32_t _handle);
		const StringPoolEntry* getEntry() const;
		void addRef() const;
		void release();

		uint32_t m_handle;
	};

	/// Returns hash of string.
	///
	/// @param[in] _str String.
	///
	/// @returns Hash of characters.
	///
	template<>
	uint32_t hashKey(const String& _str);

	namespace internal
	{
		struct StringPoolKey
		{
			bool operator==(const StringPoolKey& _rhs) const;

			uint32_t    m_hash;
			uint32_t    m_length;
			const char* m_ptr;
		};

	} // namespace internal

	template<>
	uint32_t hashKey(const internal::StringPoolKey& _key);

	/// Deduplicated, reference counted storage for `String`. Pool id 0 is the default pool, pools
	/// constructed by the user get ids 1 to 255. `intern` locks the pool, copies and releases of
	/// strings never lock.
	///
	class StringPool
	{
		BX_CLASS(StringPool
			, NO_COPY
			);

	public:
		static constexpr uint32_t kMaxPools    = 256;                    //!< Maximum number of pools.
		static constexpr uint32_t kMaxEntries  = 1<<String::kSlotBits;   //!< Maximum number of entries in pool.
		static constexpr uint8_t  kDefaultId   = 0;                      //!< Id of default pool.

		/// Constructs pool on the process default allocator.
		///
		StringPool();

		/// Constructs pool.
		///
		/// @param[in] _allocator Allocator.
		///
		explicit StringPool(AllocatorHandle _allocator);

		/// Constructs pool.
		///
		/// @param[in] _allocator Allocator.
		/// @param[in] _capacity Number of entries to reserve.
		///
		StringPool(AllocatorHandle _allocator, uint32_t _capacity);

		/// Destructor.
		///
		~StringPool();

		/// Interns string.
		///
		/// @param[in] _str Characters.
		///
		/// @returns String for characters, adding counted entry when pool doesn't have one.
		///
		String intern(const StringView& _str);

		/// Interns immortal entry.
		///
		/// @param[in] _entry Immortal entry.
		///
		/// @returns String for entry characters, referencing entry without copying when pool
		///   doesn't have its characters yet.
		///
		String intern(const StringPoolEntry& _entry);

		/// Finds string.
		///
		/// @param[in] _str Characters.
		///
		/// @returns String for characters when pool has it, otherwise empty string.
		///
		String find(const StringView& _str);

		/// Frees entries with zero references. Other threads may use the pool during the call. Visits
		/// only entries whose reference count dropped to zero since the last call.
		///
		/// @returns Number of entries freed.
		///
		uint32_t trim();

		/// Returns allocator handle.
		///
		/// @returns Handle of allocator of pool.
		///
		AllocatorHandle getAllocatorHandle() const;

		/// Returns number of entries.
		///
		/// @returns Number of entries, including the empty string and immortal entries.
		///
		uint32_t getCount() const;

		/// Returns number of unreferenced entries.
		///
		/// @returns Number of counted entries with zero references.
		///
		uint32_t getZeroCount() const;

		/// Returns maximum length.
		///
		/// @returns Longest length interned so far.
		///
		uint32_t getMaxLength() const;

		/// Returns pool id.
		///
		/// @returns Id of pool.
		///
		uint8_t getId() const;

		/// Returns entry.
		///
		/// @param[in] _slot Slot.
		///
		/// @returns Entry in slot.
		///
		const StringPoolEntry* getEntry(uint32_t _slot) const;

		/// Returns default pool, the built-in pool on CRT allocator until `setDefault`.
		///
		/// @returns Default pool.
		///
		static StringPool* getDefault();

		/// Sets default pool, strings keep the pool that interned them. Pool must outlive
		/// its strings.
		///
		/// @param[in] _pool Pool, or NULL for the built-in pool.
		///
		static void setDefault(StringPool* _pool);

		/// Returns pool.
		///
		/// @param[in] _id Pool id.
		///
		/// @returns Pool with id, NULL if none.
		///
		static StringPool* getPool(uint8_t _id);

	private:
		friend class String;
		friend class StringBuilder;

		using Key = internal::StringPoolKey;
		using Table = HashMapControlT<Key>;

		static constexpr uint32_t kNumSegments       = String::kSlotBits;
		static constexpr uint32_t kNumInlineSegments = 6;
		static constexpr uint32_t kNumInlineEntries  = (1<<kNumInlineSegments)-1;
		static constexpr size_t   kEntryAlignment    = 8;

		StringPool(AllocatorHandle _allocator, uint32_t _capacity, uint8_t _id);
		static constexpr uint32_t calcEntrySize(uint32_t _length);
		static void calcSegment(uint32_t _slot, uint32_t& _segment, uint32_t& _index);
		String internLocked(const Key& _key, const StringPoolEntry* _entry);
		String internEntry(StringPoolEntry* _entry);
		StringPoolEntry* allocEntry(uint32_t _length);
		void freeEntry(StringPoolEntry* _entry);
		void setEntry(uint32_t _slot, StringPoolEntry* _entry);
		uint32_t makeHandle(uint32_t _slot) const;
		const StringPoolEntry* findEntry(uint32_t _slot) const;
		void markZero(uint32_t _slot);
		static constexpr uint32_t calcNumZeroWords(uint32_t _segment);
		static StringPool* getBuiltin();

		static StringPool* s_pools[kMaxPools];
		static StringPool* s_default;

		Table             m_table;
		StringPoolEntry** m_segments[kNumSegments];
		uint32_t*         m_zeroBits[kNumSegments];
		Blk               m_segmentBlk[kNumSegments-kNumInlineSegments];
		StringPoolEntry*  m_inlineEntries[kNumInlineEntries];
		uint32_t          m_inlineZeroBits[kNumInlineSegments];
		Mutex             m_mutex;
		uint32_t          m_numZero;
		uint32_t          m_maxLength;
		uint8_t           m_id;
	};

	/// Write-only string builder, the only way to build or change text. Positions refer to
	/// base text, everything added at end in order. Edits are recorded against base text,
	/// never shift positions, and are applied when builder is assigned to `String`, which
	/// interns result in builder's pool. `getLength` returns length with edits applied,
	/// while `set`, `remove` and `setCursor` take base positions. Edits must not overlap.
	///
	class StringBuilder
	{
		BX_CLASS(StringBuilder
			, NO_COPY
			, TRIVIALLY_RELOCATABLE
			);

	public:
		/// Constructs empty builder in the default pool.
		StringBuilder();

		/// Constructs empty builder.
		///
		/// @param[in] _pool Pool, or default pool when NULL.
		///
		explicit StringBuilder(StringPool* _pool);

		/// Constructs empty builder.
		///
		/// @param[in] _pool Pool, or default pool when NULL.
		/// @param[in] _capacity Number of base characters to reserve.
		///
		StringBuilder(StringPool* _pool, uint32_t _capacity);

		/// Constructs builder with string as base text, in the string's pool.
		///
		/// @param[in] _str String.
		///
		StringBuilder(const String& _str);

		/// Move constructor.
		///
		/// @param[in,out] _src Builder to move from, becomes empty.
		///
		StringBuilder(StringBuilder&& _src);

		/// Move assignment.
		///
		/// @param[in,out] _src Builder to move from, becomes empty.
		///
		/// @returns Reference to this builder.
		///
		StringBuilder& operator=(StringBuilder&& _src);

		/// Replaces all text.
		///
		/// @param[in] _str String.
		///
		/// @returns Reference to this builder.
		///
		StringBuilder& operator=(const String& _str);

		/// Replaces all text.
		///
		/// @param[in] _str Characters.
		///
		/// @returns Reference to this builder.
		///
		StringBuilder& operator=(const StringView& _str);

		/// Reserves base text capacity.
		///
		/// @param[in] _capacity Number of characters.
		///
		void setCapacity(uint32_t _capacity);

		/// Returns base text capacity.
		///
		/// @returns Number of base characters that fit without growing.
		///
		uint32_t getCapacity() const;

		/// Shrinks capacity to what is held.
		///
		void trim();

		/// Removes all text and edits, moves cursor to end, keeps capacity.
		///
		void clear();

		/// Replaces all text.
		///
		/// @param[in] _str Characters.
		///
		void set(const StringView& _str);

		/// Replaces base text range.
		///
		/// @param[in] _pos Base position, clamped to base text.
		/// @param[in] _len Number of base characters, clamped to base text.
		/// @param[in] _str Replacement characters.
		///
		void set(int32_t _pos, int32_t _len, const StringView& _str);

		/// Removes base text range. Range reaching end of base text, with no edit past it, is
		/// cut right away, so cutting back to saved `getLength` works while no edit before it
		/// changes length.
		///
		/// @param[in] _pos Base position, clamped to base text.
		/// @param[in] _len Number of base characters, clamped to base text.
		///
		void remove(int32_t _pos, int32_t _len = INT32_MAX);

		/// Sets where `add` writes. At or past end of base text, `add` appends to base text,
		/// otherwise it inserts at base position, in order of calls.
		///
		/// @param[in] _pos Base position, or INT32_MAX for end.
		///
		void setCursor(int32_t _pos);

		/// Adds characters at cursor.
		///
		/// @param[in] _str Characters.
		///
		void add(const StringView& _str);

		/// Adds character at cursor.
		///
		/// @param[in] _ch Character.
		///
		void add(char _ch);

		/// Adds formatted text at cursor.
		///
		/// @param[in] _format Format string.
		///
		void addPrintf(const char* _format, ...);

		/// Adds formatted text at cursor.
		///
		/// @param[in] _format Format string.
		/// @param[in] _argList Arguments.
		///
		void addPrintfVargs(const char* _format, va_list _argList);

		/// Returns true if built string is empty.
		///
		/// @returns True if built string has no characters.
		///
		bool isEmpty() const;

		/// Returns built string length.
		///
		/// @returns Number of characters after edits are applied.
		///
		int32_t getLength() const;

		/// Returns text with pending edits applied, for the debugger Watch window
		/// (`builder.getDebugPreview()`). Copies into a static buffer, cut at 16 KiB and
		/// overwritten by the next call, doesn't allocate, isn't thread safe.
		///
		/// @returns Text, null terminated.
		///
		const char* getDebugPreview() const;

		/// Returns built string without interning it. Applies edits, built string becomes base
		/// text and cursor moves to end. Valid until builder changes.
		///
		/// @returns Built string, null terminated.
		///
		StringView getView();

		/// Moves built string out without interning it, builder becomes empty and keeps its
		/// pool. Count excludes null terminator, which follows last character until array
		/// changes.
		///
		/// @returns Built string characters.
		///
		ArrayT<char> release();

		/// Returns pool.
		///
		/// @returns Pool the builder interns into.
		///
		StringPool* getPool() const;

	private:
		friend class String;

		static constexpr uint32_t kEditCapacity = BX_CONFIG_STRING_BUILDER_EDIT_CAPACITY;
		static constexpr uint32_t kTextCapacity = BX_CONFIG_STRING_BUILDER_TEXT_CAPACITY;
		static constexpr uint32_t kPrintfCapacity = BX_CONFIG_STRING_BUILDER_PRINTF_CAPACITY;

		struct Edit
		{
			uint32_t m_pos;
			uint32_t m_len;
			uint32_t m_textOffset;
			uint32_t m_textLen;
		};

		static constexpr uint32_t kCursorEnd = UINT32_MAX;

		static int32_t compareEdit(const void* _lhs, const void* _rhs);
		ArrayT<char>& getTarget();
		Edit* getEdits();
		const Edit* getEdits() const;
		uint32_t getTextBase() const;
		uint32_t getTextCount() const;
		void growEdits();
		BX_NO_INLINE void addAtCursor(const char* _ptr, uint32_t _num);
		void addEdit(uint32_t _pos, uint32_t _len, uint32_t _textOffset, uint32_t _textLen);
		void addEditSlow(uint32_t _pos, uint32_t _len, uint32_t _textOffset, uint32_t _textLen);
		String build() const;
		void applyEdits(char* _dst) const;
		bool applyEditsInPlace();

		ArrayT<char> m_chars;
		ArrayT<char> m_text;
		StringPool*  m_pool;
		uint32_t     m_cursor;
		int32_t      m_delta;
		uint32_t     m_numEdits;
		uint32_t     m_editCapacity;
	};

	/// Writer that adds to string builder at its cursor.
	///
	class StringBuilderWriter final : public WriterI
	{
	public:
		/// Constructs writer.
		///
		/// @param[in] _builder Builder to add to.
		///
		explicit StringBuilderWriter(StringBuilder* _builder);

		/// Destructor.
		virtual ~StringBuilderWriter();

		/// Adds data to builder at its cursor.
		///
		/// @param[in]  _data Characters.
		/// @param[in]  _size Number of characters.
		/// @param[out] _err  Error, unused.
		///
		/// @returns Number of bytes written.
		///
		virtual int32_t write(const void* _data, int32_t _size, Error* _err) override;

	private:
		StringBuilder* m_builder;
	};

	/// Writes string characters, without length and null terminator.
	///
	/// @param[in]  _writer Writer.
	/// @param[in]  _str    String.
	/// @param[out] _err    Error.
	///
	/// @returns Number of bytes written.
	///
	int32_t write(WriterI* _writer, const String& _str, Error* _err);

} // namespace bx

/// Literal string pool entry in an expression, one constant initialized entry per text shared
/// by every use in the program.
///
/// @tparam TextT Literal text.
///
/// @returns Literal entry.
///
template<bx::internal::StringLiteralTextT TextT>
constexpr const bx::StringLiteralT<sizeof(TextT.m_data)>& operator""_str();

#include "inline/stringpool.inl"

#endif // BX_STRINGPOOL_H_HEADER_GUARD
