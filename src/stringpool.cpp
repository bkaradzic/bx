/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/stringpool.h>
#include <bx/allocators.h>
#include <bx/hash.h>

namespace bx
{
	StringPool* StringPool::s_pools[StringPool::kMaxPools] = {};
	StringPool* StringPool::s_default = NULL;

	BX_STRING_LITERAL(s_stringPoolEmpty, "");

	namespace
	{
		BX_FORCE_INLINE void moveChars(char* _dst, const char* _src, uint32_t _num)
		{
			if (16 < _num)
			{
				if (32 < _num)
				{
					memMove(_dst, _src, _num);
					return;
				}

				const uint64_t a = loadUnaligned<uint64_t>(_src);
				const uint64_t b = loadUnaligned<uint64_t>(_src + 8);
				const uint64_t c = loadUnaligned<uint64_t>(_src + _num - 16);
				const uint64_t d = loadUnaligned<uint64_t>(_src + _num - 8);
				storeUnaligned(_dst, a);
				storeUnaligned(_dst + 8, b);
				storeUnaligned(_dst + _num - 16, c);
				storeUnaligned(_dst + _num - 8, d);
			}
			else if (8 <= _num)
			{
				const uint64_t a = loadUnaligned<uint64_t>(_src);
				const uint64_t b = loadUnaligned<uint64_t>(_src + _num - 8);
				storeUnaligned(_dst, a);
				storeUnaligned(_dst + _num - 8, b);
			}
			else if (4 <= _num)
			{
				const uint32_t a = loadUnaligned<uint32_t>(_src);
				const uint32_t b = loadUnaligned<uint32_t>(_src + _num - 4);
				storeUnaligned(_dst, a);
				storeUnaligned(_dst + _num - 4, b);
			}
			else if (0 != _num)
			{
				const char a = _src[0];
				const char b = _src[_num / 2];
				const char c = _src[_num - 1];
				_dst[0]        = a;
				_dst[_num / 2] = b;
				_dst[_num - 1] = c;
			}
		}

	} // namespace

	static_assert(sizeof(StringLiteralT<4>) == sizeof(StringPoolEntry) + 4 + sizeof(uint32_t), "Literal characters must follow the entry header directly.");

	StringPool::StringPool()
		: StringPool(getDefaultAllocatorBlk()->getHandle(), 0, UINT8_MAX)
	{
	}

	StringPool::StringPool(AllocatorHandle _allocator)
		: StringPool(_allocator, 0, UINT8_MAX)
	{
	}

	StringPool::StringPool(AllocatorHandle _allocator, uint32_t _capacity)
		: StringPool(_allocator, _capacity, UINT8_MAX)
	{
	}

	StringPool::StringPool(AllocatorHandle _allocator, uint32_t _capacity, uint8_t _id)
		: m_table(_allocator, _capacity)
		, m_numZero(0)
		, m_maxLength(0)
		, m_id(_id)
	{
		memSet(m_segments, 0, sizeof(m_segments) );
		memSet(m_zeroBits, 0, sizeof(m_zeroBits) );
		memSet(m_inlineEntries, 0, sizeof(m_inlineEntries) );
		memSet(m_inlineZeroBits, 0, sizeof(m_inlineZeroBits) );

		for (uint32_t ii = 0; ii < kNumInlineSegments; ++ii)
		{
			m_segments[ii] = &m_inlineEntries[(1u<<ii)-1];
			m_zeroBits[ii] = &m_inlineZeroBits[ii];
		}

		if (UINT8_MAX == _id)
		{
			m_id = kDefaultId;

			for (uint32_t id = kDefaultId+1; id < kMaxPools; ++id)
			{
				volatile uintptr_t* slot = reinterpret_cast<volatile uintptr_t*>(&s_pools[id]);

				if (0 == atomicCompareAndSwapPtr(slot, 0, uintptr_t(this) ) )
				{
					m_id = uint8_t(id);
					break;
				}
			}

			BX_ASSERT(kDefaultId != m_id, "Too many string pools, maximum is %d!", kMaxPools-1);
		}
		else
		{
			BX_ASSERT(NULL == s_pools[_id], "String pool id %d is already registered!", _id);
			s_pools[_id] = this;
		}

		const Key key = { s_stringPoolEmpty.m_hash, 0, s_stringPoolEmpty.getCPtr() };
		const String empty = internLocked(key, &s_stringPoolEmpty);
		BX_ASSERT(String::kEmptySlot == empty.getSlot(), "Empty string must take slot 0!");
		BX_UNUSED(empty);
	}

	StringPool::~StringPool()
	{
		for (const auto it : m_table)
		{
			StringPoolEntry* entry = const_cast<StringPoolEntry*>(getEntry(it.getSlot() ) );

			if (!entry->isImmortal() )
			{
				BX_ASSERT(0 == entry->m_refCount, "String \"%s\" is still referenced %d times at pool destruction!"
					, entry->getCPtr()
					, entry->m_refCount
					);
				freeEntry(entry);
			}
		}

		for (uint32_t ii = 0; ii < BX_COUNTOF(m_segmentBlk); ++ii)
		{
			if (isValid(m_segmentBlk[ii]) )
			{
				m_segmentBlk[ii] = free(move(m_segmentBlk[ii]) );
			}
		}

		s_pools[m_id] = NULL;
	}

	String StringPool::intern(const StringView& _str)
	{
		const uint32_t length = uint32_t(_str.getLength() );

		if (0 == length)
		{
			return String(makeHandle(String::kEmptySlot) );
		}

		const Key key = { hashWy(_str.getPtr(), length), length, _str.getPtr() };

		return internLocked(key, NULL);
	}

	String StringPool::intern(const StringPoolEntry& _entry)
	{
		BX_ASSERT(_entry.isImmortal(), "Only immortal entries can be interned by reference!");
		BX_ASSERT('\0' == _entry.getCPtr()[_entry.m_length], "Entry characters must be zero terminated!");

		if (0 == _entry.m_length)
		{
			return String(makeHandle(String::kEmptySlot) );
		}

		const Key key = { _entry.m_hash, _entry.m_length, _entry.getCPtr() };

		return internLocked(key, &_entry);
	}

	String StringPool::find(const StringView& _str)
	{
		const uint32_t length = uint32_t(_str.getLength() );

		if (0 == length)
		{
			return String(makeHandle(String::kEmptySlot) );
		}

		const Key key = { hashWy(_str.getPtr(), length), length, _str.getPtr() };

		MutexScope lock(m_mutex);

		const uint32_t slot = m_table.findSlot(key);

		if (Table::kInvalidSlot == slot)
		{
			return String(makeHandle(String::kEmptySlot) );
		}

		StringPoolEntry* entry = const_cast<StringPoolEntry*>(getEntry(slot) );

		if (!entry->isImmortal() )
		{
			if (0 == atomicFetchAndAdd<uint32_t>(&entry->m_refCount, 1) )
			{
				atomicFetchAndSub<uint32_t>(&m_numZero, 1);
			}
		}

		return String(makeHandle(slot) );
	}

	String StringPool::internEntry(StringPoolEntry* _entry)
	{
		_entry->m_hash = hashWy(_entry->getCPtr(), _entry->m_length);

		const Key key = { _entry->m_hash, _entry->m_length, _entry->getCPtr() };

		return internLocked(key, _entry);
	}

	String StringPool::internLocked(const Key& _key, const StringPoolEntry* _entry)
	{
		MutexScope lock(m_mutex);

		uint32_t slot = m_table.findSlot(_key);

		if (Table::kInvalidSlot != slot)
		{
			StringPoolEntry* entry = const_cast<StringPoolEntry*>(getEntry(slot) );

			if (!entry->isImmortal() )
			{
				if (0 == atomicFetchAndAdd<uint32_t>(&entry->m_refCount, 1) )
				{
					atomicFetchAndSub<uint32_t>(&m_numZero, 1);
				}
			}

			if (NULL != _entry
			&&  !_entry->isImmortal() )
			{
				freeEntry(const_cast<StringPoolEntry*>(_entry) );
			}

			return String(makeHandle(slot) );
		}

		BX_ASSERT(m_table.getCount() < kMaxEntries, "String pool %d is full (%d entries)!", m_id, kMaxEntries);

		StringPoolEntry* entry = const_cast<StringPoolEntry*>(_entry);

		if (NULL == entry)
		{
			entry = allocEntry(_key.m_length);
			entry->m_hash = _key.m_hash;
			memCopy(const_cast<char*>(entry->getCPtr() ), _key.m_ptr, _key.m_length);
		}

		const Key stored = { _key.m_hash, _key.m_length, entry->getCPtr() };

		slot = m_table.add(stored);

		if (Table::kInvalidSlot == slot) [[unlikely]]
		{
			BX_ASSERT(false, "String pool %d failed to add entry!", m_id);

			if (NULL == _entry)
			{
				freeEntry(entry);
			}

			return String(makeHandle(String::kEmptySlot) );
		}

		setEntry(slot, entry);

		m_maxLength = max(m_maxLength, _key.m_length);

		return String(makeHandle(slot) );
	}

	StringPoolEntry* StringPool::allocEntry(uint32_t _length)
	{
		const uint32_t size = calcEntrySize(_length);

		Blk blk = alloc(m_table.getAllocatorHandle(), size, kEntryAlignment);
		BX_ASSERT(blk.size >= size, "Entry block size %d is smaller than derived size %d!", uint32_t(blk.size), size);
		BX_ASSERT(AllocatorOp::Allocate == blk.allocatorOp, "Entry block must come from allocate operation!");

		StringPoolEntry* entry = reinterpret_cast<StringPoolEntry*>(uintptr_t(blk.ptr) );
		entry->m_refCount        = 1;
		entry->m_hash            = 0;
		entry->m_length          = _length;
		entry->m_bookKeeping     = blk.bookKeeping;
		entry->m_size            = uint32_t(blk.size);
		entry->m_slotIndex = blk.slotIndex;
		entry->m_log2Alignment   = uint8_t(blk.log2Alignment);
		entry->m_flags           = blk.cpuAccessible ? StringPoolEntry::kFlagCpuAccessible : 0;
#if BX_CONFIG_MEMORY_TRACKING
		entry->m_locationIdx     = blk.locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING

		char* data = const_cast<char*>(entry->getCPtr() );
		data[_length] = '\0';

		blk.invalidate();

		return entry;
	}

	void StringPool::freeEntry(StringPoolEntry* _entry)
	{
		Blk blk(
			  uint64_t(uintptr_t(_entry) )
			, _entry->m_size
			, size_t(1) << _entry->m_log2Alignment
			, 0 != (_entry->m_flags & StringPoolEntry::kFlagCpuAccessible)
			, AllocatorOp::Allocate
			);
		blk.slotIndex = _entry->m_slotIndex;
		blk.bookKeeping     = _entry->m_bookKeeping;
#if BX_CONFIG_MEMORY_TRACKING
		blk.locationIdx     = _entry->m_locationIdx;
#endif // BX_CONFIG_MEMORY_TRACKING

		blk = free(getAllocator(m_table.getAllocatorHandle() ), move(blk) );
	}

	void StringPool::setEntry(uint32_t _slot, StringPoolEntry* _entry)
	{
		uint32_t segment = 0;
		uint32_t index   = 0;
		calcSegment(_slot, segment, index);

		if (NULL == m_segments[segment])
		{
			const size_t entriesSize = sizeof(StringPoolEntry*) << segment;
			const size_t bitsSize    = sizeof(uint32_t) * calcNumZeroWords(segment);

			Blk& blk = m_segmentBlk[segment - kNumInlineSegments];
			blk = alloc(m_table.getAllocatorHandle(), entriesSize + bitsSize, alignof(StringPoolEntry*) );

			uint8_t*  ptr  = reinterpret_cast<uint8_t*>(uintptr_t(blk.ptr) );
			uint32_t* bits = reinterpret_cast<uint32_t*>(ptr + entriesSize);
			memSet(bits, 0, bitsSize);

			m_zeroBits[segment] = bits;
			m_segments[segment] = reinterpret_cast<StringPoolEntry**>(ptr);
		}

		m_segments[segment][index] = _entry;
	}

	uint32_t StringPool::trim()
	{
		MutexScope lock(m_mutex);

		uint32_t numFreed = 0;

		for (uint32_t segment = 0; segment < kNumSegments; ++segment)
		{
			uint32_t* bits = m_zeroBits[segment];

			if (NULL == bits)
			{
				continue;
			}

			const uint32_t numWords = calcNumZeroWords(segment);

			for (uint32_t word = 0; word < numWords; ++word)
			{
				if (0 == atomicLoad<uint32_t>(&bits[word]) )
				{
					continue;
				}

				for (uint32_t mask = atomicExchange<uint32_t>(&bits[word], 0); 0 != mask; mask &= mask - 1)
				{
					const uint32_t   index = word*32 + countTrailingZeros(mask);
					const uint32_t   slot  = ( (1u << segment) | index) - 1;
					StringPoolEntry* entry = m_segments[segment][index];

					if (NULL != entry
					&&  !entry->isImmortal()
					&&  0 == atomicLoad<uint32_t>(&entry->m_refCount) )
					{
						const Key key = { entry->m_hash, entry->m_length, entry->getCPtr() };
						const uint32_t removed = m_table.remove(key);
						BX_ASSERT(removed == slot, "Zero bit of slot %d removed slot %d!", slot, removed);
						BX_UNUSED(removed, slot);

						m_segments[segment][index] = NULL;
						freeEntry(entry);
						++numFreed;
					}
				}
			}
		}

		atomicFetchAndSub<uint32_t>(&m_numZero, numFreed);

		return numFreed;
	}

	StringPool* StringPool::getBuiltin()
	{
		static CrtAllocator s_allocator;
		static StringPool   s_pool(s_allocator.getHandle(), 0, kDefaultId);

		return &s_pool;
	}

	StringPool* StringPool::getDefault()
	{
		StringPool* pool = s_default;

		return NULL == pool
			? getBuiltin()
			: pool
			;
	}

	void StringPool::setDefault(StringPool* _pool)
	{
		atomicExchangePtr( (void**)&s_default, _pool);
	}

	void StringBuilder::addPrintf(const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		addPrintfVargs(_format, argList);
		va_end(argList);
	}

	void StringBuilder::addPrintfVargs(const char* _format, va_list _argList)
	{
		ArrayT<char>& target = getTarget();

		const uint32_t offset   = target.getCount();
		const uint32_t capacity = target.getCapacity();
		const int32_t  avail    = int32_t(capacity - offset);

		va_list argListCopy;
		va_copy(argListCopy, _argList);

		char temp[kPrintfCapacity];

		const bool    useTemp = avail < int32_t(kPrintfCapacity);
		char*         dst     = useTemp ? temp : target.getPtr() + offset;
		const int32_t size    = useTemp ? int32_t(kPrintfCapacity) : avail;

		const int32_t num = vsnprintf(dst, size, _format, _argList);

		if (0 >= num)
		{
			va_end(argListCopy);
			return;
		}

		if (num >= size)
		{
			target.setCount(offset + uint32_t(num) + 1, InitNone);
			vsnprintf(target.getPtr() + offset, num + 1, _format, argListCopy);
			target.setCount(offset + uint32_t(num), InitNone);
		}
		else if (useTemp)
		{
			target.add(ArrayViewT<char>(temp, uint32_t(num) ) );
		}
		else
		{
			target.setCount(offset + uint32_t(num), InitNone);
		}

		va_end(argListCopy);

		if (kCursorEnd != m_cursor)
		{
			addEdit(m_cursor, 0, offset - getTextBase(), uint32_t(num) );
		}
	}

	int32_t StringBuilder::compareEdit(const void* _lhs, const void* _rhs)
	{
		const Edit& lhs = *reinterpret_cast<const Edit*>(_lhs);
		const Edit& rhs = *reinterpret_cast<const Edit*>(_rhs);

		if (lhs.m_pos != rhs.m_pos)
		{
			return lhs.m_pos < rhs.m_pos ? -1 : 1;
		}

		return int32_t(0 != lhs.m_len) - int32_t(0 != rhs.m_len);
	}

	void StringBuilder::growEdits()
	{
		const uint32_t oldBase   = getTextBase();
		const uint32_t textCount = m_text.getCount() - oldBase;
		const uint32_t capacity  = max(kEditCapacity, m_editCapacity*2);
		const uint32_t newBase   = capacity*uint32_t(sizeof(Edit) );

		m_text.setCapacity(max(m_text.getCapacity(), newBase + max(textCount, kTextCapacity) ) );
		m_text.setCount(newBase + textCount, InitNone);

		char* ptr = m_text.getPtr();
		memMove(ptr + newBase, ptr + oldBase, textCount);

		m_editCapacity = capacity;
	}

	void StringBuilder::addAtCursor(const char* _ptr, uint32_t _num)
	{
		addEdit(m_cursor, 0, getTextCount(), _num);
		m_text.add(ArrayViewT<char>(_ptr, _num) );
	}

	void StringBuilder::addEditSlow(uint32_t _pos, uint32_t _len, uint32_t _textOffset, uint32_t _textLen)
	{
		const Edit edit = { _pos, _len, _textOffset, _textLen };

		if (m_numEdits == m_editCapacity)
		{
			growEdits();
		}

		Edit*    edits = getEdits();
		uint32_t idx   = m_numEdits;

		if (0 != idx
		&&  0 > compareEdit(&edit, &edits[idx-1]) )
		{
			idx = upperBound(edit, edits, m_numEdits, compareEdit);
		}

		if (0 != idx)
		{
			Edit& prev = edits[idx-1];

			if (0 == _len
			&&  0 == prev.m_len
			&&  _pos == prev.m_pos
			&&  _textOffset == prev.m_textOffset + prev.m_textLen)
			{
				prev.m_textLen += _textLen;
				return;
			}

			BX_ASSERT(prev.m_pos + prev.m_len <= _pos, "Edit overlaps previous edit!");
		}

		BX_ASSERT(idx == m_numEdits || _pos + _len <= edits[idx].m_pos, "Edit overlaps next edit!");

		memMove(&edits[idx+1], &edits[idx], (m_numEdits - idx)*sizeof(Edit) );
		edits[idx] = edit;
		++m_numEdits;
	}

	void StringBuilder::applyEdits(char* _dst) const
	{
		const Edit*    edits = getEdits();
		const char*    base  = m_chars.getPtr();
		const uint32_t count = m_chars.getCount();
		const char*    text  = m_text.getPtr() + getTextBase();

		char*    dst = _dst;
		uint32_t src = 0;

		for (uint32_t ii = 0, num = m_numEdits; ii < num; ++ii)
		{
			const Edit& edit = edits[ii];

			const uint32_t pos = max(edit.m_pos, src);

			moveChars(dst, base + src, pos - src);
			dst += pos - src;

			moveChars(dst, text + edit.m_textOffset, edit.m_textLen);
			dst += edit.m_textLen;

			src = max(src, edit.m_pos + edit.m_len);
		}

		memCopy(dst, base + src, count - src);
		dst += count - src;

		BX_ASSERT(dst == _dst + getLength(), "Built length %d differs from expected %d!", int32_t(dst - _dst), getLength() );
	}

	BX_NO_INLINE const char* StringBuilder::getDebugPreview() const
	{
		static char s_preview[16<<10];

		const Edit*    edits = getEdits();
		const char*    base  = m_chars.getPtr();
		const uint32_t count = m_chars.getCount();
		const char*    text  = m_text.getPtr() + getTextBase();
		const uint32_t limit = uint32_t(sizeof(s_preview) ) - 1;

		uint32_t len = 0;
		uint32_t src = 0;

		for (uint32_t ii = 0, num = m_numEdits; ii < num; ++ii)
		{
			const Edit&    edit = edits[ii];
			const uint32_t pos  = max(edit.m_pos, src);

			const uint32_t baseLen = min(pos - src, limit - len);
			memCopy(&s_preview[len], base + src, baseLen);
			len += baseLen;

			const uint32_t textLen = min(edit.m_textLen, limit - len);
			memCopy(&s_preview[len], text + edit.m_textOffset, textLen);
			len += textLen;

			src = max(src, edit.m_pos + edit.m_len);
		}

		const uint32_t tailLen = min(count - src, limit - len);
		memCopy(&s_preview[len], base + src, tailLen);
		len += tailLen;

		s_preview[len] = '\0';

		return s_preview;
	}

	bool StringBuilder::applyEditsInPlace()
	{
		const Edit* edits = getEdits();
		int32_t delta = 0;

		for (uint32_t ii = 0, num = m_numEdits; ii < num; ++ii)
		{
			const Edit& edit = edits[ii];
			delta += int32_t(edit.m_textLen) - int32_t(edit.m_len);

			if (0 < delta)
			{
				return false;
			}
		}

		char*          base  = m_chars.getPtr();
		const uint32_t count = m_chars.getCount();
		const char*    text  = m_text.getPtr() + getTextBase();

		char*    dst = base;
		uint32_t src = 0;

		for (uint32_t ii = 0, num = m_numEdits; ii < num; ++ii)
		{
			const Edit& edit = edits[ii];

			const uint32_t pos = max(edit.m_pos, src);

			moveChars(dst, base + src, pos - src);
			dst += pos - src;

			moveChars(dst, text + edit.m_textOffset, edit.m_textLen);
			dst += edit.m_textLen;

			src = max(src, edit.m_pos + edit.m_len);
		}

		memMove(dst, base + src, count - src);
		dst += count - src;

		BX_ASSERT(dst == base + getLength(), "Built length %d differs from expected %d!", int32_t(dst - base), getLength() );

		m_chars.setCount(uint32_t(dst - base), InitNone);

		return true;
	}

	String StringBuilder::build() const
	{
		if (0 == m_numEdits)
		{
			return m_pool->intern(StringView(m_chars.getPtr(), int32_t(m_chars.getCount() ) ) );
		}

		const uint32_t length = uint32_t(getLength() );

		if (0 == length)
		{
			return m_pool->intern(StringView() );
		}

		StringPoolEntry* entry = m_pool->allocEntry(length);
		applyEdits(const_cast<char*>(entry->getCPtr() ) );

		return m_pool->internEntry(entry);
	}

	StringView StringBuilder::getView()
	{
		if (0 != m_numEdits)
		{
			if (!applyEditsInPlace() )
			{
				const uint32_t length = uint32_t(getLength() );

				ArrayT<char> chars(m_chars.getAllocatorHandle(), length + 1);
				chars.setCount(length, InitNone);
				applyEdits(chars.getPtr() );

				m_chars = move(chars);
			}

			m_cursor = kCursorEnd;
			m_delta  = 0;
			m_text.clear();
			m_numEdits     = 0;
			m_editCapacity = 0;
		}

		const uint32_t count = m_chars.getCount();

		if (0 == count)
		{
			return StringView();
		}

		m_chars.add('\0');
		m_chars.setCount(count, InitNone);

		return StringView(m_chars.getPtr(), int32_t(count) );
	}

	ArrayT<char> StringBuilder::release()
	{
		getView();

		ArrayT<char> chars(move(m_chars) );
		clear();

		return chars;
	}

} // namespace bx
