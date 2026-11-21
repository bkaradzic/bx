/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_STRINGPOOL_H_HEADER_GUARD
#	error "Must be included from bx/stringpool.h!"
#endif // BX_STRINGPOOL_H_HEADER_GUARD

#include <bx/cpu.h>

namespace bx
{
	namespace internal
	{
		constexpr uint32_t kStringPoolEmptyHash = hashWy("", 0);

	} // namespace internal

	inline const char* StringPoolEntry::getCPtr() const
	{
		return reinterpret_cast<const char*>(this) + sizeof(StringPoolEntry);
	}

	inline bool StringPoolEntry::isImmortal() const
	{
		return 0 != (m_flags & kFlagImmortal);
	}

	inline StringView StringPoolEntry::getView() const
	{
		return StringView(getCPtr(), int32_t(m_length) );
	}

	namespace internal
	{
		constexpr StringPoolEntry makeImmortalStringPoolEntry(uint32_t _hash, uint32_t _length)
		{
			StringPoolEntry entry{};
			entry.m_hash   = _hash;
			entry.m_length = _length;
			entry.m_flags  = StringPoolEntry::kFlagImmortal;

			return entry;
		}

	} // namespace internal

	template<uint32_t SizeT>
	inline constexpr StringLiteralT<SizeT>::StringLiteralT(const char (&_str)[SizeT])
		: StringPoolEntry(internal::makeImmortalStringPoolEntry(hashWy(_str, SizeT-1), SizeT-1) )
		, m_data{}
		, m_cachedHandle(0)
	{
		for (uint32_t ii = 0; ii < SizeT; ++ii)
		{
			m_data[ii] = _str[ii];
		}
	}

	namespace internal
	{
		template<uint32_t SizeT>
		inline consteval StringLiteralTextT<SizeT>::StringLiteralTextT(const char (&_str)[SizeT])
			: m_data{}
		{
			for (uint32_t ii = 0; ii < SizeT; ++ii)
			{
				m_data[ii] = _str[ii];
			}
		}

		template<StringLiteralTextT TextT>
		inline constinit StringLiteralT<sizeof(TextT.m_data)> s_stringLiteral(TextT.m_data);

	} // namespace internal

	inline String::String()
		: m_handle(0)
	{
	}

	inline String::String(const String& _src)
		: m_handle(_src.m_handle)
	{
		addRef();
	}

	inline String::String(String&& _src)
		: m_handle(_src.m_handle)
	{
		_src.m_handle = 0;
	}

	inline String::String(const StringView& _str)
		: String(_str, NULL)
	{
	}

	inline String::String(const StringView& _str, StringPool* _pool)
		: String( (NULL == _pool ? StringPool::getDefault() : _pool)->intern(_str) )
	{
	}

	inline String::String(const StringBuilder& _builder)
		: String(_builder.build() )
	{
	}

	inline String::String(const StringPoolEntry& _entry)
		: String(StringPool::getDefault()->intern(_entry) )
	{
	}

	template<uint32_t SizeT>
	inline String::String(const StringLiteralT<SizeT>& _literal)
	{
		StringPool*    pool   = StringPool::getDefault();
		const uint32_t cached = atomicLoad<uint32_t>(&_literal.m_cachedHandle);

		if (pool->getId() == uint8_t(cached >> kSlotBits)
		&&  &_literal == pool->findEntry(cached & kSlotMask) )
		{
			m_handle = cached;
			return;
		}

		String str = pool->intern(_literal);
		m_handle     = str.m_handle;
		str.m_handle = 0;

		if (&_literal == pool->findEntry(getSlot() ) )
		{
			atomicStore<uint32_t>(&_literal.m_cachedHandle, m_handle);
		}
	}

	inline String::String(uint32_t _handle)
		: m_handle(_handle)
	{
	}

	inline String::~String()
	{
		release();
	}

	inline String& String::operator=(const String& _src)
	{
		if (m_handle != _src.m_handle)
		{
			_src.addRef();
			release();
			m_handle = _src.m_handle;
		}

		return *this;
	}

	inline String& String::operator=(String&& _src)
	{
		if (this != &_src)
		{
			release();
			m_handle = _src.m_handle;
			_src.m_handle = 0;
		}

		return *this;
	}

	inline String& String::operator=(const StringBuilder& _builder)
	{
		return *this = String(_builder);
	}

	inline String& String::operator=(const StringPoolEntry& _entry)
	{
		return *this = String(_entry);
	}

	inline void String::clear()
	{
		release();
		m_handle = 0;
	}

	inline bool String::isEmpty() const
	{
		return kEmptySlot == getSlot();
	}

	inline int32_t String::getLength() const
	{
		return isEmpty()
			? 0
			: int32_t(getEntry()->m_length)
			;
	}

	inline uint32_t String::getHash() const
	{
		return isEmpty()
			? internal::kStringPoolEmptyHash
			: getEntry()->m_hash
			;
	}

	template<>
	inline uint32_t hashKey(const String& _str)
	{
		return _str.getHash();
	}

	inline const char* String::getCPtr() const
	{
		return isEmpty()
			? ""
			: getEntry()->getCPtr()
			;
	}

	inline StringPool* String::getPool() const
	{
		return StringPool::getPool(getPoolId() );
	}

	inline uint32_t String::getHandle() const
	{
		return m_handle;
	}

	inline uint32_t String::getSlot() const
	{
		return m_handle & kSlotMask;
	}

	inline uint8_t String::getPoolId() const
	{
		return uint8_t(m_handle >> kSlotBits);
	}

	inline String::operator StringView() const
	{
		if (isEmpty() )
		{
			return StringView();
		}

		const StringPoolEntry* entry = getEntry();
		return StringView(entry->getCPtr(), int32_t(entry->m_length) );
	}

	inline bool String::operator==(const String& _rhs) const
	{
		if (getPoolId() == _rhs.getPoolId() )
		{
			return m_handle == _rhs.m_handle;
		}

		return true
			&& getLength() == _rhs.getLength()
			&& getHash()   == _rhs.getHash()
			&& 0 == memCmp(getCPtr(), _rhs.getCPtr(), getLength() )
			;
	}

	inline bool String::operator!=(const String& _rhs) const
	{
		return !(*this == _rhs);
	}

	inline bool String::operator==(const StringView& _rhs) const
	{
		return true
			&& getLength() == _rhs.getLength()
			&& 0 == memCmp(getCPtr(), _rhs.getPtr(), getLength() )
			;
	}

	inline bool String::operator!=(const StringView& _rhs) const
	{
		return !(*this == _rhs);
	}

	inline const StringPoolEntry* String::getEntry() const
	{
		return StringPool::s_pools[getPoolId()]->getEntry(getSlot() );
	}

	inline void String::addRef() const
	{
		if (!isEmpty() )
		{
			StringPoolEntry* entry = const_cast<StringPoolEntry*>(getEntry() );

			if (!entry->isImmortal() )
			{
				const uint32_t oldRefCount = atomicFetchAndAdd<uint32_t>(&entry->m_refCount, 1);
				BX_ASSERT(0 != oldRefCount, "String reference count was zero while a String referenced it!");
				BX_UNUSED(oldRefCount);
			}
		}
	}

	inline void String::release()
	{
		if (!isEmpty() )
		{
			StringPool*      pool  = StringPool::s_pools[getPoolId()];
			StringPoolEntry* entry = const_cast<StringPoolEntry*>(pool->getEntry(getSlot() ) );

			if (!entry->isImmortal() )
			{
				const uint32_t oldRefCount = atomicFetchAndSub<uint32_t>(&entry->m_refCount, 1);
				BX_ASSERT(0 != oldRefCount, "String reference count underflow!");

				if (1 == oldRefCount)
				{
					atomicFetchAndAdd<uint32_t>(&pool->m_numZero, 1);
					pool->markZero(getSlot() );
				}
			}
		}
	}

	inline bool internal::StringPoolKey::operator==(const StringPoolKey& _rhs) const
	{
		return true
			&& m_hash   == _rhs.m_hash
			&& m_length == _rhs.m_length
			&& (m_ptr == _rhs.m_ptr || 0 == memCmp(m_ptr, _rhs.m_ptr, m_length) )
			;
	}

	template<>
	inline uint32_t hashKey(const internal::StringPoolKey& _key)
	{
		return _key.m_hash;
	}

	inline constexpr uint32_t StringPool::calcEntrySize(uint32_t _length)
	{
		return alignUp(uint32_t(sizeof(StringPoolEntry) ) + _length + 1, Blk::kMinBlockSize);
	}

	inline void StringPool::calcSegment(uint32_t _slot, uint32_t& _segment, uint32_t& _index)
	{
		const uint32_t biased = _slot + 1;
		_segment = 31 - countLeadingZeros(biased | 1);
		_index   = biased ^ (1u << _segment);
	}

	inline uint32_t StringPool::makeHandle(uint32_t _slot) const
	{
		return (uint32_t(m_id) << String::kSlotBits) | _slot;
	}

	inline AllocatorHandle StringPool::getAllocatorHandle() const
	{
		return m_table.getAllocatorHandle();
	}

	inline uint32_t StringPool::getCount() const
	{
		return m_table.getCount();
	}

	inline uint32_t StringPool::getZeroCount() const
	{
		return m_numZero;
	}

	inline uint32_t StringPool::getMaxLength() const
	{
		return m_maxLength;
	}

	inline uint8_t StringPool::getId() const
	{
		return m_id;
	}

	inline const StringPoolEntry* StringPool::getEntry(uint32_t _slot) const
	{
		uint32_t segment = 0;
		uint32_t index   = 0;
		calcSegment(_slot, segment, index);

		return m_segments[segment][index];
	}

	inline const StringPoolEntry* StringPool::findEntry(uint32_t _slot) const
	{
		uint32_t segment = 0;
		uint32_t index   = 0;
		calcSegment(_slot, segment, index);

		StringPoolEntry* const* entries = m_segments[segment];

		return NULL != entries
			? entries[index]
			: NULL
			;
	}

	inline constexpr uint32_t StringPool::calcNumZeroWords(uint32_t _segment)
	{
		return _segment < 5 ? 1 : 1u << (_segment - 5);
	}

	inline void StringPool::markZero(uint32_t _slot)
	{
		uint32_t segment = 0;
		uint32_t index   = 0;
		calcSegment(_slot, segment, index);

		volatile uint32_t* word = &m_zeroBits[segment][index >> 5];
		const uint32_t     mask = uint32_t(1) << (index & 31);

		uint32_t oldVal = atomicLoad<uint32_t>(word);

		while (0 == (oldVal & mask) )
		{
			const uint32_t prevVal = atomicCompareAndSwap<uint32_t>(word, oldVal, oldVal | mask);

			if (prevVal == oldVal)
			{
				break;
			}

			oldVal = prevVal;
		}
	}

	inline StringPool* StringPool::getPool(uint8_t _id)
	{
		return kDefaultId == _id
			? getBuiltin()
			: s_pools[_id]
			;
	}

	inline StringBuilder::StringBuilder()
		: StringBuilder(NULL, 0)
	{
	}

	inline StringBuilder::StringBuilder(StringPool* _pool)
		: StringBuilder(_pool, 0)
	{
	}

	inline StringBuilder::StringBuilder(StringPool* _pool, uint32_t _capacity)
		: m_chars( (NULL == _pool ? StringPool::getDefault() : _pool)->getAllocatorHandle(), _capacity)
		, m_text(m_chars.getAllocatorHandle() )
		, m_pool(NULL == _pool ? StringPool::getDefault() : _pool)
		, m_cursor(kCursorEnd)
		, m_delta(0)
		, m_numEdits(0)
		, m_editCapacity(0)
	{
	}

	inline StringBuilder::StringBuilder(const String& _str)
		: StringBuilder(_str.getPool() )
	{
		add(_str);
	}

	inline StringBuilder::StringBuilder(StringBuilder&& _src)
		: m_chars(bx::move(_src.m_chars) )
		, m_text(bx::move(_src.m_text) )
		, m_pool(_src.m_pool)
		, m_cursor(_src.m_cursor)
		, m_delta(_src.m_delta)
		, m_numEdits(_src.m_numEdits)
		, m_editCapacity(_src.m_editCapacity)
	{
		_src.m_cursor       = kCursorEnd;
		_src.m_delta        = 0;
		_src.m_numEdits     = 0;
		_src.m_editCapacity = 0;
	}

	inline StringBuilder& StringBuilder::operator=(StringBuilder&& _src)
	{
		m_chars        = bx::move(_src.m_chars);
		m_text         = bx::move(_src.m_text);
		m_pool         = _src.m_pool;
		m_cursor       = _src.m_cursor;
		m_delta        = _src.m_delta;
		m_numEdits     = _src.m_numEdits;
		m_editCapacity = _src.m_editCapacity;

		_src.m_cursor       = kCursorEnd;
		_src.m_delta        = 0;
		_src.m_numEdits     = 0;
		_src.m_editCapacity = 0;

		return *this;
	}

	inline StringBuilder& StringBuilder::operator=(const String& _str)
	{
		set(_str);
		return *this;
	}

	inline StringBuilder& StringBuilder::operator=(const StringView& _str)
	{
		set(_str);
		return *this;
	}

	inline void StringBuilder::setCapacity(uint32_t _capacity)
	{
		m_chars.setCapacity(_capacity);
	}

	inline uint32_t StringBuilder::getCapacity() const
	{
		return m_chars.getCapacity();
	}

	inline void StringBuilder::trim()
	{
		m_chars.trim();
		m_text.trim();
	}

	inline void StringBuilder::clear()
	{
		m_chars.clear();
		m_text.clear();
		m_cursor       = kCursorEnd;
		m_delta        = 0;
		m_numEdits     = 0;
		m_editCapacity = 0;
	}

	inline void StringBuilder::set(const StringView& _str)
	{
		clear();
		add(_str);
	}

	inline void StringBuilder::set(int32_t _pos, int32_t _len, const StringView& _str)
	{
		const uint32_t num = uint32_t(_str.getLength() );

		addEdit(uint32_t(max(_pos, 0) ), uint32_t(max(_len, 0) ), getTextCount(), num);
		m_text.add(ArrayViewT<char>(_str.getPtr(), num) );
	}

	inline void StringBuilder::remove(int32_t _pos, int32_t _len)
	{
		const uint32_t count = m_chars.getCount();
		const uint32_t pos   = min(uint32_t(max(_pos, 0) ), count);
		const uint32_t len   = uint32_t(max(_len, 0) );

		const uint32_t numEdits = m_numEdits;
		const uint32_t editEnd  = 0 == numEdits
			? 0
			: getEdits()[numEdits-1].m_pos + getEdits()[numEdits-1].m_len
			;

		if (len >= count - pos
		&&  editEnd <= pos)
		{
			m_chars.setCount(pos, InitNone);

			if (pos <= m_cursor)
			{
				m_cursor = kCursorEnd;
			}

			return;
		}

		addEdit(pos, len, getTextCount(), 0);
	}

	inline void StringBuilder::setCursor(int32_t _pos)
	{
		const uint32_t pos = uint32_t(max(_pos, 0) );

		m_cursor = pos >= m_chars.getCount()
			? kCursorEnd
			: pos
			;
	}

	inline void StringBuilder::add(const StringView& _str)
	{
		const uint32_t num = uint32_t(_str.getLength() );

		if (0 == num)
		{
			return;
		}

		if (kCursorEnd != m_cursor)
		{
			addAtCursor(_str.getPtr(), num);
			return;
		}

		m_chars.add(ArrayViewT<char>(_str.getPtr(), num) );
	}

	inline void StringBuilder::add(char _ch)
	{
		if (kCursorEnd != m_cursor)
		{
			addAtCursor(&_ch, 1);
			return;
		}

		m_chars.add(_ch);
	}

	inline bool StringBuilder::isEmpty() const
	{
		return 0 == getLength();
	}

	inline int32_t StringBuilder::getLength() const
	{
		return int32_t(m_chars.getCount() ) + m_delta;
	}

	inline StringPool* StringBuilder::getPool() const
	{
		return m_pool;
	}

	inline ArrayT<char>& StringBuilder::getTarget()
	{
		return kCursorEnd == m_cursor
			? m_chars
			: m_text
			;
	}

	inline StringBuilder::Edit* StringBuilder::getEdits()
	{
		return reinterpret_cast<Edit*>(m_text.getPtr() );
	}

	inline const StringBuilder::Edit* StringBuilder::getEdits() const
	{
		return reinterpret_cast<const Edit*>(m_text.getPtr() );
	}

	inline uint32_t StringBuilder::getTextBase() const
	{
		return m_editCapacity*uint32_t(sizeof(Edit) );
	}

	inline uint32_t StringBuilder::getTextCount() const
	{
		return m_text.getCount() - getTextBase();
	}

	BX_FORCE_INLINE void StringBuilder::addEdit(uint32_t _pos, uint32_t _len, uint32_t _textOffset, uint32_t _textLen)
	{
		const uint32_t count = m_chars.getCount();
		const uint32_t pos   = min(_pos, count);
		const uint32_t len   = min(_len, count - pos);

		if (0 == len
		&&  0 == _textLen)
		{
			return;
		}

		m_delta += int32_t(_textLen) - int32_t(len);

		const uint32_t num = m_numEdits;

		if (0 != num)
		{
			Edit* edits = getEdits();
			Edit& last  = edits[num-1];

			if (last.m_pos + last.m_len <= pos)
			{
				if (0 != len
				||  pos != last.m_pos)
				{
					if (num < m_editCapacity)
					{
						edits[num] = { pos, len, _textOffset, _textLen };
						m_numEdits = num + 1;
						return;
					}
				}
				else if (_textOffset == last.m_textOffset + last.m_textLen)
				{
					last.m_textLen += _textLen;
					return;
				}
			}
		}

		addEditSlow(pos, len, _textOffset, _textLen);
	}

	inline StringBuilderWriter::StringBuilderWriter(StringBuilder* _builder)
		: m_builder(_builder)
	{
	}

	inline StringBuilderWriter::~StringBuilderWriter()
	{
	}

	inline int32_t StringBuilderWriter::write(const void* _data, int32_t _size, Error* _err)
	{
		BX_UNUSED(_err);
		BX_ASSERT(0 <= _size, "Size must not be negative!");

		m_builder->add(StringView(static_cast<const char*>(_data), _size) );

		return _size;
	}

	inline int32_t write(WriterI* _writer, const String& _str, Error* _err)
	{
		return write(_writer, _str.getCPtr(), _str.getLength(), _err);
	}

} // namespace bx

template<bx::internal::StringLiteralTextT TextT>
inline constexpr const typename decltype(TextT)::LiteralT& operator""_str()
{
	return bx::internal::s_stringLiteral<TextT>;
}
