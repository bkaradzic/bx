/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"

#include <bx/allocators.h>
#include <bx/hash.h>
#include <bx/heap.h>
#include <bx/os.h>
#include <bx/rng.h>
#include <bx/stringpool.h>
#include <bx/thread.h>

#include <string>
#include <vector>

BX_STRING_LITERAL(s_literalHello, "hello");
BX_STRING_LITERAL(s_literalHelloAgain, "hello");
BX_STRING_LITERAL(s_literalEmpty, "");

static_assert(bx::hashWy("hello", 5) == bx::StringLiteralT<6>("hello").m_hash, "Literal hash must be computed at compile time.");
static_assert(5 == bx::StringLiteralT<6>("hello").m_length, "");
static_assert(0 == bx::StringLiteralT<1>("").m_length, "");

static uint32_t getRefCount(const bx::String& _str)
{
	return _str.getPool()->getEntry(_str.getSlot() )->m_refCount;
}

namespace
{
	constexpr char kHashText[] =
		"The quick brown fox jumps over the lazy dog. 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ "
		"abcdefghijklmnopqrstuvwxyz !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ The quick brown fox jumps "
		"over the lazy dog again, and again, and again, until the line is long enough to cover "
		"every tail length of the block loops.";

	constexpr uint32_t kNumHashLengths = sizeof(kHashText);

	struct HashTable
	{
		uint32_t m_hash[kNumHashLengths];
	};

	constexpr HashTable computeHashTable()
	{
		HashTable table{};

		for (uint32_t len = 0; len < kNumHashLengths; ++len)
		{
			table.m_hash[len] = bx::hashWy(kHashText, len);
		}

		return table;
	}

	constexpr HashTable s_hashTable = computeHashTable();

} // namespace

TEST_CASE("hashWy constant and runtime evaluation match", "[string]")
{
	char buffer[kNumHashLengths + 7];
	bx::memCopy(buffer + 3, kHashText, kNumHashLengths);

	const char* text = buffer + 3;

	for (uint32_t len = 0; len < kNumHashLengths; ++len)
	{
		REQUIRE(s_hashTable.m_hash[len] == bx::hashWy(text, len) );
	}

	REQUIRE(bx::hashWy("", 0) == bx::internal::kStringPoolEmptyHash);
	REQUIRE(bx::hashWy("a", 1) != bx::hashWy("b", 1) );
	REQUIRE(bx::hashWy("ab", 2) != bx::hashWy("ba", 2) );
}

TEST_CASE("String empty", "[string]")
{
	bx::String str;
	REQUIRE(str.isEmpty() );
	REQUIRE(0 == str.getLength() );
	REQUIRE(0 == str.getHandle() );
	REQUIRE(0 == bx::strLen(str.getCPtr() ) );
	REQUIRE(str == bx::String() );
	REQUIRE(str == bx::StringView("") );
	REQUIRE(bx::internal::kStringPoolEmptyHash == str.getHash() );

	bx::String copy(str);
	REQUIRE(copy.isEmpty() );

	bx::String fromEmptyView("", bx::StringPool::getDefault() );
	REQUIRE(0 == fromEmptyView.getHandle() );

	bx::String fromEmptyLiteral(s_literalEmpty);
	REQUIRE(0 == fromEmptyLiteral.getHandle() );

	bx::StringBuilder builder;
	bx::String fromEmptyBuilder = builder;
	REQUIRE(0 == fromEmptyBuilder.getHandle() );
	REQUIRE(0 == builder.getCapacity() );
}

#if BX_CONFIG_MEMORY_TRACKING
TEST_CASE("StringPool trim frees tracked size", "[string]")
{
	if constexpr (BX_ENABLED(BX_PLATFORM_EMSCRIPTEN) )
	{
		SKIP("Not supported by wasm, memory locations are told apart by call stack.");
	}

	bx::StringPool pool(bx::getHeapAllocatorHandle(), 0);

	char str[1000];
	bx::memSet(str, 'a', sizeof(str) );

	bx::String strs[2];

	for (uint32_t ii = 0; ii < BX_COUNTOF(strs); ++ii)
	{
		str[0] = char('a' + ii);
		strs[ii] = pool.intern(bx::StringView(str, sizeof(str) ) );
	}

	const uint32_t locationIdx = pool.getEntry(strs[0].getSlot() )->m_locationIdx;
	REQUIRE(locationIdx == pool.getEntry(strs[1].getSlot() )->m_locationIdx);
	REQUIRE(2 == bx::getMemoryLocation(locationIdx).numBlocks);

	strs[0].clear();
	strs[1].clear();
	REQUIRE(2 == pool.trim() );

	REQUIRE(0 == bx::getMemoryLocation(locationIdx).numBlocks);
	REQUIRE(0 == bx::getMemoryLocation(locationIdx).bytesAllocated);
}
#endif // BX_CONFIG_MEMORY_TRACKING

TEST_CASE("StringPool intern dedup refcount trim", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	REQUIRE(0 != pool.getId() );
	REQUIRE(1 == pool.getCount() );
	REQUIRE(0 == pool.getZeroCount() );

	bx::String a = pool.intern("texture");
	bx::String b = pool.intern("texture");
	bx::String c = pool.intern("buffer");

	REQUIRE(a.getHandle() == b.getHandle() );
	REQUIRE(a.getHandle() != c.getHandle() );
	REQUIRE(a == b);
	REQUIRE(a != c);
	REQUIRE(a == bx::StringView("texture") );
	REQUIRE(a.getCPtr() == b.getCPtr() );
	REQUIRE(0 == bx::strCmp(a.getCPtr(), "texture") );
	REQUIRE(7 == a.getLength() );
	REQUIRE(bx::hashWy("texture", 7) == a.getHash() );
	REQUIRE(pool.getId() == a.getPoolId() );
	REQUIRE(&pool == a.getPool() );
	REQUIRE(3 == pool.getCount() );
	REQUIRE(7 == pool.getMaxLength() );
	REQUIRE(2 == getRefCount(a) );
	REQUIRE(1 == getRefCount(c) );

	{
		bx::String d = a;
		REQUIRE(3 == getRefCount(a) );

		bx::String e = bx::move(d);
		REQUIRE(d.isEmpty() );
		REQUIRE(3 == getRefCount(a) );

		d = e;
		REQUIRE(4 == getRefCount(a) );

BX_PRAGMA_DIAGNOSTIC_PUSH();
BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG("-Wself-assign-overloaded");
		d = d;
BX_PRAGMA_DIAGNOSTIC_POP();
		REQUIRE(4 == getRefCount(a) );

		d = c;
		REQUIRE(3 == getRefCount(a) );
		REQUIRE(2 == getRefCount(c) );
	}

	REQUIRE(2 == getRefCount(a) );
	REQUIRE(1 == getRefCount(c) );

	REQUIRE(pool.find("texture") == a);
	REQUIRE(pool.find("missing").isEmpty() );

	c.clear();
	REQUIRE(c.isEmpty() );
	REQUIRE(1 == pool.getZeroCount() );
	REQUIRE(3 == pool.getCount() );

	bx::String again = pool.intern("buffer");
	REQUIRE(0 == pool.getZeroCount() );
	REQUIRE(3 == pool.getCount() );
	REQUIRE(1 == getRefCount(again) );

	again.clear();
	REQUIRE(1 == pool.getZeroCount() );
	REQUIRE(1 == pool.trim() );
	REQUIRE(0 == pool.getZeroCount() );
	REQUIRE(2 == pool.getCount() );
	REQUIRE(pool.find("buffer").isEmpty() );

	a.clear();
	b.clear();
	REQUIRE(1 == pool.trim() );
	REQUIRE(1 == pool.getCount() );
}

TEST_CASE("StringPool literal", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	bx::String a = pool.intern(s_literalHello);
	REQUIRE(a.getCPtr() == s_literalHello.m_data);
	REQUIRE(5 == a.getLength() );
	REQUIRE(a == bx::StringView("hello") );

	bx::String b = pool.intern(s_literalHelloAgain);
	REQUIRE(a.getHandle() == b.getHandle() );
	REQUIRE(a.getCPtr() == s_literalHello.m_data);

	bx::String c = pool.intern("hello");
	REQUIRE(a.getHandle() == c.getHandle() );
	REQUIRE(c.getCPtr() == s_literalHello.m_data);
	REQUIRE(0 == getRefCount(a) );

	{
		bx::String d = a;
		REQUIRE(0 == getRefCount(a) );
	}

	REQUIRE(0 == pool.getZeroCount() );
	REQUIRE(0 == pool.trim() );
	REQUIRE(2 == pool.getCount() );

	bx::String heapFirst = pool.intern("world");
	bx::String literalSecond = pool.intern(s_literalHello);
	BX_STRING_LITERAL(s_literalWorld, "world");
	bx::String shared = pool.intern(s_literalWorld);
	REQUIRE(heapFirst.getHandle() == shared.getHandle() );
	REQUIRE(2 == getRefCount(heapFirst) );
	BX_UNUSED(literalSecond);
}

TEST_CASE("String default pool", "[string]")
{
	bx::StringPool::getDefault()->trim();

	bx::String a("default", bx::StringPool::getDefault() );
	bx::String b("default");
	REQUIRE(0 == a.getPoolId() );
	REQUIRE(a.getHandle() == b.getHandle() );
	REQUIRE(bx::StringPool::getDefault() == a.getPool() );

	bx::String c = s_literalHello;
	REQUIRE(c.getCPtr() == s_literalHello.m_data);

	c = s_literalEmpty;
	REQUIRE(c.isEmpty() );

	a.clear();
	b.clear();
	REQUIRE(1 == bx::StringPool::getDefault()->getZeroCount() );
	REQUIRE(1 == bx::StringPool::getDefault()->trim() );
}

TEST_CASE("String across pools", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool poolA(allocator.getHandle(), 0);
	bx::StringPool poolB(allocator.getHandle(), 0);

	REQUIRE(poolA.getId() != poolB.getId() );
	REQUIRE(&poolA == bx::StringPool::getPool(poolA.getId() ) );
	REQUIRE(&poolB == bx::StringPool::getPool(poolB.getId() ) );

	bx::String a = poolA.intern("same");
	bx::String b = poolB.intern("same");
	bx::String c = poolB.intern("other");
	bx::String emptyB = poolB.intern("");

	REQUIRE(a.getHandle() != b.getHandle() );
	REQUIRE(a == b);
	REQUIRE(a != c);
	REQUIRE(emptyB == bx::String() );
	REQUIRE(emptyB.isEmpty() );
	REQUIRE(poolB.getId() == emptyB.getPoolId() );

	bx::String moved = bx::move(b);
	REQUIRE(b.isEmpty() );
	REQUIRE(0 == b.getPoolId() );
	REQUIRE(moved == a);

	a.clear();
	moved.clear();
	c.clear();
	REQUIRE(1 == poolA.trim() );
	REQUIRE(2 == poolB.trim() );
}

TEST_CASE("StringBuilder", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	bx::StringBuilder builder(&pool);
	REQUIRE(builder.isEmpty() );
	REQUIRE(0 == builder.getLength() );
	REQUIRE(&pool == builder.getPool() );

	builder.add("u_");
	builder.add(bx::StringView("params") );
	builder.addPrintf("[%d]", 7);
	builder.add('!');
	REQUIRE(12 == builder.getLength() );

	bx::String str = builder;
	REQUIRE(str == bx::StringView("u_params[7]!") );
	REQUIRE(&pool == str.getPool() );

	builder.remove(8);
	REQUIRE(8 == builder.getLength() );
	REQUIRE(bx::String(builder) == bx::StringView("u_params") );

	const uint32_t capacity = builder.getCapacity();
	builder.clear();
	REQUIRE(builder.isEmpty() );
	REQUIRE(capacity == builder.getCapacity() );

	builder.trim();
	REQUIRE(0 == builder.getCapacity() );

	builder.set("abc");
	builder.trim();
	REQUIRE(3 <= builder.getCapacity() );

	builder.addPrintf("%s", "defghijklmnopqrstuvwxyz0123456789");
	REQUIRE(bx::String(builder) == bx::StringView("abcdefghijklmnopqrstuvwxyz0123456789") );

	for (uint32_t ii = 0; ii < 1000; ++ii)
	{
		builder.addPrintf("%u,", ii);
	}

	REQUIRE(36 + 3890 == builder.getLength() );

	const bx::String numbers = builder;
	REQUIRE(bx::strTail(numbers, 4) == bx::StringView("999,") );

	bx::StringBuilder edit = str;
	REQUIRE(&pool == edit.getPool() );
	edit.add(".bak");
	str = edit;
	REQUIRE(str == bx::StringView("u_params[7]!.bak") );
	REQUIRE(3 == pool.getZeroCount() );

	bx::StringBuilder moved = bx::move(edit);
	REQUIRE(bx::String(moved) == str);
	REQUIRE(edit.isEmpty() );

	edit = str;
	REQUIRE(bx::String(edit) == str);

	edit = bx::StringView("view");
	REQUIRE(bx::String(edit) == bx::StringView("view") );

	str.clear();
	REQUIRE(5 == pool.trim() );
}

TEST_CASE("StringBuilder addPrintf", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	char text[1000];
	for (uint32_t ii = 0; ii < BX_COUNTOF(text); ++ii)
	{
		text[ii] = char('a' + ii%26);
	}

	bx::StringBuilder builder(&pool);

	for (int32_t len = 1; len <= int32_t(BX_COUNTOF(text) ); len += 37)
	{
		builder.clear();
		builder.trim();
		builder.addPrintf("%.*s", len, text);
		REQUIRE(len == builder.getLength() );
		REQUIRE(bx::String(builder) == bx::StringView(text, len) );

		builder.clear();
		builder.add("x");
		builder.addPrintf("%.*s", len, text);
		REQUIRE(len + 1 == builder.getLength() );

		builder.set("xy");
		builder.setCursor(1);
		builder.addPrintf("%.*s", len, text);
		REQUIRE(len + 2 == builder.getLength() );
		REQUIRE(0 == bx::strCmp(builder.getView().getPtr() + 1, bx::StringView(text, len), len) );
		REQUIRE('y' == builder.getView().getPtr()[len + 1]);
	}
}

static bool isBuilt(const bx::StringBuilder& _builder, const char* _expected)
{
	const bx::String str = _builder;
	return str == bx::StringView(_expected)
		&& _builder.getLength() == str.getLength()
		;
}

TEST_CASE("StringBuilder getView", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	bx::StringBuilder builder(&pool);
	REQUIRE(builder.getView().isEmpty() );

	builder.add("float4 main() { return x; }");
	const bx::StringView view = builder.getView();
	REQUIRE(0 == bx::strCmp(view, "float4 main() { return x; }") );

	builder.set(23, 1, "y");
	builder.remove(7, 7);
	builder.set(0, 6, "half4");
	builder.setCursor(6);
	builder.add('_');
	REQUIRE(0 == bx::strCmp(builder.getView(), "half4_ { return y; }") );
	REQUIRE(isBuilt(builder, "half4_ { return y; }") );

	builder.set(0, 1, "H");
	builder.add("!");
	REQUIRE(0 == bx::strCmp(builder.getView(), "Half4_ { return y; }!") );
	REQUIRE(isBuilt(builder, "Half4_ { return y; }!") );

	const bx::StringView terminated = builder.getView();
	REQUIRE('\0' == terminated.getPtr()[terminated.getLength()]);
	REQUIRE(terminated.getLength() == bx::strLen(terminated.getPtr() ) );

	builder.remove(0);
	REQUIRE(builder.getView().isEmpty() );
	REQUIRE('\0' == builder.getView().getPtr()[0]);
	REQUIRE(builder.isEmpty() );
}

TEST_CASE("StringBuilder getView random edits", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	struct Interval
	{
		uint32_t pos;
		uint32_t len;
		char     text[9];
		uint32_t textLen;
	};

	bx::RngMwc rng;

	for (uint32_t iteration = 0; iteration < 2000; ++iteration)
	{
		char base[96];
		const uint32_t baseLen = 1 + rng.gen() % 80;

		for (uint32_t ii = 0; ii < baseLen; ++ii)
		{
			base[ii] = char('a' + rng.gen() % 26);
		}

		const bx::StringView baseView(base, int32_t(baseLen) );

		bx::StringBuilder viewBuilder(&pool);
		bx::StringBuilder buildBuilder(&pool);
		viewBuilder.add(baseView);
		buildBuilder.add(baseView);

		Interval intervals[32];
		uint32_t numIntervals = 0;

		const uint32_t grow = rng.gen() % 3;

		for (uint32_t pos = rng.gen() % 4; pos <= baseLen && numIntervals < BX_COUNTOF(intervals); pos += 1 + rng.gen() % 6)
		{
			Interval& interval = intervals[numIntervals];
			interval.pos     = pos;
			interval.len     = bx::min<uint32_t>(rng.gen() % 5, baseLen - pos);
			interval.textLen = 0 == grow
				? rng.gen() % (interval.len + 1)
				: rng.gen() % 9
				;

			if (0 == interval.len
			&&  0 == interval.textLen)
			{
				interval.textLen = 1;
			}

			for (uint32_t ii = 0; ii < interval.textLen; ++ii)
			{
				interval.text[ii] = char('A' + rng.gen() % 26);
			}

			++numIntervals;
			pos += interval.len;
		}

		uint32_t order[32];

		for (uint32_t ii = 0; ii < numIntervals; ++ii)
		{
			order[ii] = ii;
		}

		if (0 != rng.gen() % 2)
		{
			for (uint32_t ii = numIntervals; ii > 1; --ii)
			{
				bx::swap(order[ii-1], order[rng.gen() % ii]);
			}
		}

		for (uint32_t ii = 0; ii < numIntervals; ++ii)
		{
			const Interval& interval = intervals[order[ii] ];
			const bx::StringView text(interval.text, int32_t(interval.textLen) );
			viewBuilder.set(int32_t(interval.pos), int32_t(interval.len), text);
			buildBuilder.set(int32_t(interval.pos), int32_t(interval.len), text);
		}

		char expected[96 + 32*9];
		uint32_t expectedLen = 0;
		uint32_t src = 0;

		for (uint32_t ii = 0; ii < numIntervals; ++ii)
		{
			const Interval& interval = intervals[ii];

			for (; src < interval.pos; ++src)
			{
				expected[expectedLen++] = base[src];
			}

			for (uint32_t jj = 0; jj < interval.textLen; ++jj)
			{
				expected[expectedLen++] = interval.text[jj];
			}

			src += interval.len;
		}

		for (; src < baseLen; ++src)
		{
			expected[expectedLen++] = base[src];
		}

		const bx::StringView expectedView(expected, int32_t(expectedLen) );

		const bx::String built = buildBuilder;
		REQUIRE(built == expectedView);

		const bx::StringView view = viewBuilder.getView();
		REQUIRE(view == expectedView);
		REQUIRE('\0' == view.getPtr()[view.getLength()]);
		REQUIRE(int32_t(expectedLen) == viewBuilder.getLength() );

		viewBuilder.set(0, 1, "#");
		REQUIRE(viewBuilder.getView().getLength() == view.getLength() + (0 == expectedLen ? 1 : 0) );
		REQUIRE('#' == viewBuilder.getView().getPtr()[0]);
	}
}

TEST_CASE("String write", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	bx::ArrayT<char> out(allocator.getHandle() );
	bx::ArrayWriterT<char> writer(&out);

	const bx::String str("float4", &pool);
	REQUIRE(6 == bx::write(&writer, str, bx::ErrorAssert{}) );
	REQUIRE(0 == bx::write(&writer, bx::String(), bx::ErrorAssert{}) );
	REQUIRE(out == bx::ArrayViewT<char>("float4", 6) );
}

TEST_CASE("StringBuilder release", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	bx::StringBuilder builder(&pool);
	REQUIRE(builder.release().isEmpty() );

	builder.add("float4 main() { return x; }");
	builder.set(23, 1, "y");
	builder.addPrintf("// %d", 1389);

	const char* ptr = builder.getView().getPtr();
	bx::ArrayT<char> chars = builder.release();
	REQUIRE(ptr == chars.getPtr() );
	REQUIRE(allocator.getHandle() == chars.getAllocatorHandle() );
	REQUIRE(chars == bx::ArrayViewT<char>("float4 main() { return y; }// 1389", 34) );
	REQUIRE('\0' == chars.getPtr()[chars.getCount()]);

	REQUIRE(builder.isEmpty() );
	REQUIRE(&pool == builder.getPool() );

	builder.add("abc");
	REQUIRE(allocator.getHandle() == builder.release().getAllocatorHandle() );
}

TEST_CASE("StringBuilderWriter", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	bx::StringBuilder builder(&pool);
	bx::StringBuilderWriter writer(&builder);

	bx::Error err;
	REQUIRE(5 == bx::write(&writer, "float", &err) );
	REQUIRE(0 == bx::write(&writer, "", &err) );
	REQUIRE(0 < bx::write(&writer, &err, "%d(%s)", 4, "x") );
	REQUIRE(err.isOk() );
	REQUIRE(builder.getView() == bx::StringView("float4(x)") );

	builder.setCursor(7);
	REQUIRE(4 == bx::write(&writer, "1.0,", &err) );
	REQUIRE(bx::String(builder) == bx::StringView("float4(1.0,x)") );
}

TEST_CASE("StringBuilder edits", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	const bx::String source("float4 main() { return x; }", &pool);

	{
		bx::StringBuilder builder(source);
		builder.set(23, 1, "y");
		builder.remove(7, 7);
		builder.set(0, 6, "half4");
		REQUIRE(isBuilt(builder, "half4 { return y; }") );
	}

	{
		bx::StringBuilder lhs(source);
		lhs.add("xyz");

		bx::StringBuilder rhs(&pool);
		rhs.add(source);
		rhs.add("xyz");

		REQUIRE(isBuilt(lhs, "float4 main() { return x; }xyz") );
		REQUIRE(bx::String(lhs) == bx::String(rhs) );
	}

	{
		bx::StringBuilder builder(&pool);
		builder.add("abc");
		builder.set(0, 1, "X");
		builder.add("def");
		REQUIRE(isBuilt(builder, "Xbcdef") );
	}

	{
		bx::StringBuilder builder(&pool);
		builder.add("abcdef");
		builder.set(2, 2, "R");
		builder.set(2, 0, "1");
		builder.set(2, 0, "2");
		builder.set(4, 0, "3");
		REQUIRE(isBuilt(builder, "ab12R3ef") );
	}

	{
		bx::StringBuilder builder(&pool);
		builder.add("abcdef");
		builder.setCursor(3);
		builder.add("<");
		builder.add('x');
		builder.addPrintf("%d>", 42);
		builder.setCursor(1);
		builder.add('_');
		builder.setCursor(INT32_MAX);
		builder.add("!");
		REQUIRE(isBuilt(builder, "a_bc<x42>def!") );
	}

	{
		bx::StringBuilder builder(&pool);
		builder.add("abcdef");
		builder.remove(4);
		builder.set(100, 5, "Z");
		builder.remove(-5, 2);
		REQUIRE(isBuilt(builder, "cdZ") );
	}

	{
		bx::StringBuilder builder(&pool);
		builder.add("abc");
		builder.set(1, 1, "");
		builder.remove(0, 0);
		builder.setCursor(0);
		builder.add("");
		REQUIRE(isBuilt(builder, "ac") );
	}

	{
		constexpr uint32_t kNum = 1000;

		char base[kNum];
		char expected[kNum*2];

		for (uint32_t ii = 0; ii < kNum; ++ii)
		{
			base[ii] = char('a' + ii % 26);
			expected[ii*2+0] = base[ii];
			expected[ii*2+1] = ',';
		}

		bx::StringBuilder builder(&pool);
		builder.add(bx::StringView(base, kNum) );

		for (uint32_t ii = kNum; 0 < ii; --ii)
		{
			builder.setCursor(int32_t(ii) );
			builder.add(',');
		}

		const bx::String str = builder;
		REQUIRE(str == bx::StringView(expected, kNum*2) );
	}
}

TEST_CASE("StringPool many strings", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	constexpr uint32_t kNum = 100000;

	bx::ArrayT<bx::String> strings(allocator.getHandle(), kNum);
	bx::StringBuilder builder(&pool);

	for (uint32_t ii = 0; ii < kNum; ++ii)
	{
		builder.clear();
		builder.addPrintf("name_%u", ii);
		strings.add(bx::String(builder) );
	}

	REQUIRE(kNum + 1 == pool.getCount() );

	for (uint32_t ii = 0; ii < kNum; ++ii)
	{
		char name[32];
		bx::snprintf(name, sizeof(name), "name_%u", ii);
		REQUIRE(strings[ii] == bx::StringView(name) );
		REQUIRE(ii + 1 == strings[ii].getSlot() );
		REQUIRE(pool.find(name) == strings[ii]);
		REQUIRE(1 == getRefCount(strings[ii]) );
	}

	for (uint32_t ii = 0; ii < kNum; ii += 2)
	{
		strings[ii].clear();
	}

	REQUIRE(kNum/2 == pool.getZeroCount() );
	REQUIRE(kNum/2 == pool.trim() );
	REQUIRE(kNum/2 + 1 == pool.getCount() );

	for (uint32_t ii = 0; ii < kNum; ++ii)
	{
		char name[32];
		bx::snprintf(name, sizeof(name), "name_%u", ii);
		REQUIRE( (1 == (ii & 1) ) == !pool.find(name).isEmpty() );
	}

	for (uint32_t ii = 0; ii < kNum; ii += 2)
	{
		builder.clear();
		builder.addPrintf("name_%u", ii);
		strings[ii] = bx::String(builder);
		REQUIRE(1 == getRefCount(strings[ii]) );
	}

	REQUIRE(kNum + 1 == pool.getCount() );

	strings.clear();
	REQUIRE(kNum == pool.getZeroCount() );
	REQUIRE(kNum == pool.trim() );
	REQUIRE(1 == pool.getCount() );
}

#if BX_CONFIG_SUPPORTS_THREADING
struct StringThreadData
{
	bx::StringPool* m_pool;
	bx::String*     m_shared;
	uint32_t        m_numShared;
	uint32_t        m_numIterations;
};

static int32_t stringThreadFn(bx::Thread* _self, void* _userData)
{
	BX_UNUSED(_self);
	StringThreadData* data = reinterpret_cast<StringThreadData*>(_userData);

	const uint32_t seed = uint32_t(uintptr_t(_self) );
	bx::RngMwc rng(seed);
	bx::String local[16];

	for (uint32_t ii = 0; ii < data->m_numIterations; ++ii)
	{
		const uint32_t src = rng.gen() % data->m_numShared;
		const uint32_t dst = rng.gen() % BX_COUNTOF(local);

		switch (rng.gen() % 4)
		{
		case 0: local[dst] = data->m_shared[src];                                break;
		case 1: local[dst] = local[rng.gen() % BX_COUNTOF(local)];           break;
		case 2: local[dst].clear();                                          break;
		case 3: local[dst] = bx::move(local[rng.gen() % BX_COUNTOF(local)]); break;
		}
	}

	return 0;
}

TEST_CASE("StringPool concurrent copies while interning", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	constexpr uint32_t kNumShared  = 64;
	constexpr uint32_t kNumThreads = 4;
	constexpr uint32_t kNumIntern  = 20000;

	bx::String shared[kNumShared];
	bx::StringBuilder builder(&pool);

	for (uint32_t ii = 0; ii < kNumShared; ++ii)
	{
		builder.clear();
		builder.addPrintf("shared_%u", ii);
		shared[ii] = bx::String(builder);
	}

	StringThreadData data = { &pool, shared, kNumShared, 200000 };

	bx::Thread threads[kNumThreads];

	for (uint32_t ii = 0; ii < kNumThreads; ++ii)
	{
		threads[ii].init(stringThreadFn, &data);
	}

	bx::ArrayT<bx::String> interned(allocator.getHandle(), kNumIntern);

	for (uint32_t ii = 0; ii < kNumIntern; ++ii)
	{
		builder.clear();
		builder.addPrintf("interned_%u", ii);
		interned.add(bx::String(builder) );
	}

	for (uint32_t ii = 0; ii < kNumThreads; ++ii)
	{
		threads[ii].shutdown();
	}

	for (uint32_t ii = 0; ii < kNumShared; ++ii)
	{
		REQUIRE(1 == getRefCount(shared[ii]) );
	}

	for (uint32_t ii = 0; ii < kNumIntern; ++ii)
	{
		REQUIRE(1 == getRefCount(interned[ii]) );
	}

	REQUIRE(0 == pool.getZeroCount() );
	REQUIRE(0 == pool.trim() );
}

struct StringTrimThreadData
{
	bx::StringPool* m_pool;
	uint32_t        m_numIterations;
	uint32_t        m_numFailed;
	uint32_t        m_done;
};

static int32_t stringTrimThreadFn(bx::Thread* _self, void* _userData)
{
	StringTrimThreadData* data = reinterpret_cast<StringTrimThreadData*>(_userData);

	const uint32_t seed = uint32_t(uintptr_t(_self) );
	bx::RngMwc rng(seed);
	bx::String local[16];
	uint32_t   key[16] = {};
	bx::StringBuilder builder(data->m_pool);
	char expected[32];

	for (uint32_t ii = 0; ii < data->m_numIterations; ++ii)
	{
		const uint32_t dst = rng.gen() % BX_COUNTOF(local);

		if (0 == rng.gen() % 2)
		{
			key[dst] = rng.gen() % 256;
			builder.clear();
			builder.addPrintf("trim_%u", key[dst]);
			local[dst] = bx::String(builder);
		}
		else
		{
			local[dst].clear();
		}

		const uint32_t chk = rng.gen() % BX_COUNTOF(local);

		if (!local[chk].isEmpty() )
		{
			bx::snprintf(expected, sizeof(expected), "trim_%u", key[chk]);

			if (!(local[chk] == bx::StringView(expected) ) )
			{
				bx::atomicFetchAndAdd<uint32_t>(&data->m_numFailed, 1);
			}
		}
	}

	bx::atomicFetchAndAdd<uint32_t>(&data->m_done, 1);

	return 0;
}

TEST_CASE("StringPool trim while interning", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	constexpr uint32_t kNumThreads = 4;

	StringTrimThreadData data = { &pool, 100000, 0, 0 };

	bx::Thread threads[kNumThreads];

	for (uint32_t ii = 0; ii < kNumThreads; ++ii)
	{
		threads[ii].init(stringTrimThreadFn, &data);
	}

	uint32_t numFreed = 0;

	while (kNumThreads != bx::atomicLoad(&data.m_done) )
	{
		numFreed += pool.trim();
		bx::sleep(1);
	}

	for (uint32_t ii = 0; ii < kNumThreads; ++ii)
	{
		threads[ii].shutdown();
	}

	REQUIRE(0 == data.m_numFailed);
	REQUIRE(0 < numFreed);
	pool.trim();
	REQUIRE(0 == pool.getZeroCount() );
	REQUIRE(1 == pool.getCount() );
}
#endif // BX_CONFIG_SUPPORTS_THREADING

TEST_CASE("HashMapT 64-bit key to String", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);
	bx::HashMapT<uint64_t, bx::String> table(allocator.getHandle(), 0);

	bx::StringBuilder text(&pool);

	constexpr uint32_t kNum = 10000;

	for (uint32_t ii = 0; ii < kNum; ++ii)
	{
		char key[32];
		const int32_t len = bx::snprintf(key, sizeof(key), "menu.item.%u", ii);
		text.clear();
		text.addPrintf("Item %u", ii % 100);

		bx::HashMurmur3_64 hasher;
		hasher.begin();
		hasher.add(key, len);
		const uint64_t hash = hasher.end();
		table.add(hash, bx::String(text) );
	}

	REQUIRE(kNum == table.getCount() );
	REQUIRE(100 + 1 == pool.getCount() );

	for (uint32_t ii = 0; ii < kNum; ++ii)
	{
		char key[32];
		const int32_t len = bx::snprintf(key, sizeof(key), "menu.item.%u", ii);
		char expected[32];
		bx::snprintf(expected, sizeof(expected), "Item %u", ii % 100);

		bx::HashMurmur3_64 hasher;
		hasher.begin();
		hasher.add(key, len);
		const uint64_t hash = hasher.end();
		const bx::String* found = table.find(hash);
		REQUIRE( (NULL != found) );
		REQUIRE(*found == bx::StringView(expected) );
		REQUIRE(100 == getRefCount(*found) );
	}

	table.clear();
	REQUIRE(100 == pool.getZeroCount() );
	REQUIRE(100 == pool.trim() );
}

TEST_CASE("StringPool default allocator", "[string]")
{
	bx::StringPool pool;
	REQUIRE(bx::getDefaultAllocatorBlk() == bx::getAllocator(pool.getAllocatorHandle() ) );
	REQUIRE(0 != pool.getId() );

	bx::String a = pool.intern("texture");
	REQUIRE(a == bx::StringView("texture") );
	REQUIRE(&pool == a.getPool() );
}

TEST_CASE("StringPool setDefault", "[string]")
{
	bx::StringPool* builtin = bx::StringPool::getDefault();
	builtin->trim();

	bx::String before("set_default_before");
	REQUIRE(builtin == before.getPool() );

	bx::CrtAllocator allocator;

	{
		bx::StringPool pool(allocator.getHandle(), 0);
		bx::StringPool::setDefault(&pool);
		REQUIRE(&pool == bx::StringPool::getDefault() );

		{
			const bx::String after("set_default_after");
			REQUIRE(&pool == after.getPool() );

			bx::StringBuilder builder;
			REQUIRE(&pool == builder.getPool() );

			const bx::String copy = before;
			REQUIRE(builtin == copy.getPool() );
		}

		bx::StringPool::setDefault(NULL);
	}

	REQUIRE(builtin == bx::StringPool::getDefault() );
	REQUIRE(before == bx::StringView("set_default_before") );

	before.clear();
	REQUIRE(1 == builtin->trim() );
}

TEST_CASE("StringPool and StringBuilder constructors", "[string]")
{
	bx::StringPool::getDefault()->trim();

	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle() );
	REQUIRE(allocator.getHandle() == pool.getAllocatorHandle() );

	bx::String inPool(bx::StringView("mesh"), &pool);
	REQUIRE(&pool == inPool.getPool() );

	{
		bx::String inDefault(bx::StringView("mesh") );
		REQUIRE(bx::StringPool::getDefault() == inDefault.getPool() );

		bx::StringBuilder defaultBuilder;
		REQUIRE(bx::StringPool::getDefault() == defaultBuilder.getPool() );
	}

	REQUIRE(1 == bx::StringPool::getDefault()->trim() );

	bx::StringBuilder builder(&pool);
	REQUIRE(&pool == builder.getPool() );

	builder.add("mesh");
	bx::String built(builder);
	REQUIRE(inPool == built);

	bx::StringBuilder reserved(&pool, 64);
	REQUIRE(64 <= reserved.getCapacity() );
}

TEST_CASE("StringBuilder getDebugPreview", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	bx::StringBuilder builder(&pool);
	REQUIRE(0 == bx::strCmp(builder.getDebugPreview(), "") );

	builder.add("float4 main() { return x; }");
	REQUIRE(0 == bx::strCmp(builder.getDebugPreview(), "float4 main() { return x; }") );

	builder.set(23, 1, "y");
	builder.remove(7, 7);
	builder.set(0, 6, "half4");
	builder.setCursor(16);
	builder.add("/* */ ");
	REQUIRE(0 == bx::strCmp(builder.getDebugPreview(), "half4 { /* */ return y; }") );
	REQUIRE(isBuilt(builder, "half4 { /* */ return y; }") );

	bx::StringBuilder large(&pool);
	for (uint32_t ii = 0; ii < 3000; ++ii)
	{
		large.add("abcdefgh");
	}
	large.set(0, 1, "X");

	const char* preview = large.getDebugPreview();
	REQUIRE( (16<<10) - 1 == bx::strLen(preview) );
	REQUIRE('X' == preview[0]);
	REQUIRE('b' == preview[1]);
}

TEST_CASE("StringBuilder overlapping edits", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	{
		bx::StringBuilder builder(&pool);
		builder.add("g[1][0]");
		builder.remove(4, 1);
		REQUIRE_ASSERTS(builder.remove(2) );
	}

	{
		bx::StringBuilder builder(&pool);
		builder.add("g[1][0]");
		builder.remove(2, 3);
		REQUIRE_ASSERTS(builder.remove(3, 1) );
	}

	{
		bx::StringBuilder builder(&pool);
		builder.add("g[1][0]");
		builder.remove(2, 3);
		REQUIRE_ASSERTS(builder.set(2, 1, "x") );
	}

	{
		bx::StringBuilder builder(&pool);
		builder.add("g[1][0]");
		builder.remove(2, 3);
		builder.setCursor(3);
		REQUIRE_ASSERTS(builder.add('x') );
	}

	{
		bx::StringBuilder builder(&pool);
		builder.add("g[1][0]");
		builder.remove(4, 1);
		builder.remove(6, 1);
		REQUIRE_ASSERTS(builder.remove(1, 4) );
	}
}

TEST_CASE("StringBuilder cut back", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	bx::StringBuilder builder(&pool);
	builder.add("g[1]");

	const int32_t outer = builder.getLength();
	builder.add("[0], ");
	builder.remove(builder.getLength() - 2);
	REQUIRE(7 == builder.getLength() );

	const int32_t inner = builder.getLength();
	builder.add("junk");
	builder.remove(inner);
	builder.add(" = (float)0");
	REQUIRE(builder.getView() == bx::StringView("g[1][0] = (float)0") );

	builder.remove(outer);
	builder.add(" = 0");
	REQUIRE(builder.getView() == bx::StringView("g[1] = 0") );

	builder.set(0, 1, "h");
	const int32_t saved = builder.getLength();
	builder.add(" + 1");
	builder.remove(saved);
	REQUIRE(builder.getView() == bx::StringView("h[1] = 0") );

	builder.remove(2);
	builder.setCursor(1);
	builder.add("z");
	builder.remove(1);
	builder.add("]");
	REQUIRE(builder.getView() == bx::StringView("hz]") );

	builder.setCursor(1);
	builder.add("y");
	REQUIRE_ASSERTS(builder.remove(0) );
}

namespace
{
	struct BuilderCell
	{
		std::string before;
		std::string replace;
		char        ch;
		bool        hasChar;
		bool        covered;
		bool        replaceStart;
	};

	struct BuilderModel
	{
		BuilderModel()
		{
			reset("");
		}

		void reset(const std::string& _str)
		{
			cells.clear();
			cells.push_back(BuilderCell{ "", "", 0, false, false, false });
			cursor = UINT32_MAX;
			addBase(_str);
		}

		uint32_t getCount() const
		{
			return uint32_t(cells.size() ) - 1;
		}

		std::string build() const
		{
			std::string result;

			for (const BuilderCell& cell : cells)
			{
				result += cell.before;

				if (cell.replaceStart)
				{
					result += cell.replace;
				}

				if (cell.hasChar
				&&  !cell.covered)
				{
					result += cell.ch;
				}
			}

			return result;
		}

		bool hasEdits() const
		{
			for (const BuilderCell& cell : cells)
			{
				if (!cell.before.empty()
				||  cell.covered)
				{
					return true;
				}
			}

			return false;
		}

		uint32_t getEditEnd() const
		{
			uint32_t editEnd = 0;

			for (uint32_t ii = 0, num = uint32_t(cells.size() ); ii < num; ++ii)
			{
				if (!cells[ii].before.empty() )
				{
					editEnd = bx::max(editEnd, ii);
				}

				if (cells[ii].covered)
				{
					editEnd = bx::max(editEnd, ii + 1);
				}
			}

			return editEnd;
		}

		bool canInsert(uint32_t _pos) const
		{
			return !cells[_pos].covered
				|| cells[_pos].replaceStart
				;
		}

		bool canReplace(uint32_t _pos, uint32_t _len) const
		{
			for (uint32_t ii = _pos; ii < _pos + _len; ++ii)
			{
				if (cells[ii].covered
				|| (ii != _pos && !cells[ii].before.empty() ) )
				{
					return false;
				}
			}

			return true;
		}

		void addBase(const std::string& _str)
		{
			for (char ch : _str)
			{
				BuilderCell& last = cells.back();
				last.ch      = ch;
				last.hasChar = true;
				cells.push_back(BuilderCell{ "", "", 0, false, false, false });
			}
		}

		bool canAdd() const
		{
			return UINT32_MAX == cursor
				|| canInsert(cursor)
				;
		}

		void add(const std::string& _str)
		{
			if (UINT32_MAX == cursor)
			{
				addBase(_str);
				return;
			}

			cells[cursor].before += _str;
		}

		void clamp(int32_t _pos, int32_t _len, uint32_t& _outPos, uint32_t& _outLen) const
		{
			const uint32_t count = getCount();
			_outPos = bx::min(uint32_t(bx::max(_pos, 0) ), count);
			_outLen = bx::min(uint32_t(bx::max(_len, 0) ), count - _outPos);
		}

		bool canSet(int32_t _pos, int32_t _len) const
		{
			uint32_t pos, len;
			clamp(_pos, _len, pos, len);

			return 0 == len
				? canInsert(pos)
				: canReplace(pos, len)
				;
		}

		void set(int32_t _pos, int32_t _len, const std::string& _str)
		{
			uint32_t pos, len;
			clamp(_pos, _len, pos, len);

			if (0 == len)
			{
				cells[pos].before += _str;
				return;
			}

			cells[pos].replaceStart = true;
			cells[pos].replace      = _str;

			for (uint32_t ii = pos; ii < pos + len; ++ii)
			{
				cells[ii].covered = true;
			}
		}

		bool isCut(int32_t _pos, int32_t _len) const
		{
			const uint32_t count = getCount();
			const uint32_t pos   = bx::min(uint32_t(bx::max(_pos, 0) ), count);

			return uint32_t(bx::max(_len, 0) ) >= count - pos
				&& getEditEnd() <= pos
				;
		}

		bool canRemove(int32_t _pos, int32_t _len) const
		{
			return isCut(_pos, _len)
				|| canSet(_pos, _len)
				;
		}

		void remove(int32_t _pos, int32_t _len)
		{
			if (isCut(_pos, _len) )
			{
				const uint32_t pos = bx::min(uint32_t(bx::max(_pos, 0) ), getCount() );
				cells.resize(pos + 1);
				cells[pos].ch      = 0;
				cells[pos].hasChar = false;

				if (pos <= cursor)
				{
					cursor = UINT32_MAX;
				}

				return;
			}

			set(_pos, _len, "");
		}

		void setCursor(int32_t _pos)
		{
			const uint32_t pos = uint32_t(bx::max(_pos, 0) );

			cursor = pos >= getCount()
				? UINT32_MAX
				: pos
				;
		}

		std::vector<BuilderCell> cells;
		uint32_t cursor;
	};

	std::string genText(bx::RngMwc& _rng)
	{
		static const char kChars[] = "abcdefghijklmnopqrstuvwxyz0123456789 _[]()";

		const uint32_t len = 0 == _rng.gen() % 16
			? 64 + _rng.gen() % 300
			: _rng.gen() % 12
			;

		std::string result;

		for (uint32_t ii = 0; ii < len; ++ii)
		{
			result += kChars[_rng.gen() % (sizeof(kChars) - 1)];
		}

		return result;
	}

	bx::StringView toView(const std::string& _str)
	{
		return bx::StringView(_str.c_str(), int32_t(_str.size() ) );
	}

	std::string toStd(const bx::StringView& _str)
	{
		return std::string(_str.getPtr(), size_t(_str.getLength() ) );
	}

} // namespace

TEST_CASE("StringBuilder random edits against model", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	for (uint32_t seed = 1; seed <= 8; ++seed)
	{
		bx::RngMwc rng(seed*7919, seed*104729);

		bx::StringBuilder builder(&pool);
		BuilderModel model;

		for (uint32_t ii = 0; ii < 4000; ++ii)
		{
			const uint32_t count = model.getCount();
			const int32_t  pos   = 0 == rng.gen() % 32
				? int32_t(rng.gen() % 8) - 4
				: int32_t(rng.gen() % (count + 3) )
				;
			const int32_t  len   = 0 == rng.gen() % 8
				? INT32_MAX
				: int32_t(rng.gen() % 12)
				;

			switch (rng.gen() % 16)
			{
			case 0:
			case 1:
			case 2:
				if (model.canAdd() )
				{
					const std::string text = genText(rng);
					builder.add(toView(text) );
					model.add(text);
				}
				break;

			case 3:
				if (model.canAdd() )
				{
					const char ch = char('A' + rng.gen() % 26);
					builder.add(ch);
					model.add(std::string(1, ch) );
				}
				break;

			case 4:
				if (model.canAdd() )
				{
					const uint32_t    value = rng.gen();
					const std::string digits = std::to_string(value);

					if (0 == rng.gen() % 4)
					{
						builder.addPrintf("%0300u", value);
						model.add(std::string(300 - digits.size(), '0') + digits);
					}
					else
					{
						builder.addPrintf("<%u>", value);
						model.add("<" + digits + ">");
					}
				}
				break;

			case 5:
				if (model.canAdd() )
				{
					const std::string text = genText(rng);
					bx::StringBuilderWriter writer(&builder);
					bx::Error err;
					REQUIRE(int32_t(text.size() ) == bx::write(&writer, text.c_str(), int32_t(text.size() ), &err) );
					model.add(text);
				}
				break;

			case 6:
			case 7:
				{
					const int32_t cursor = 0 == rng.gen() % 8 ? INT32_MAX : pos;
					builder.setCursor(cursor);
					model.setCursor(cursor);
				}
				break;

			case 8:
			case 9:
				if (model.canSet(pos, len) )
				{
					const std::string text = genText(rng);
					builder.set(pos, len, toView(text) );
					model.set(pos, len, text);
				}
				break;

			case 10:
			case 11:
				if (model.canRemove(pos, len) )
				{
					builder.remove(pos, len);
					model.remove(pos, len);
				}
				break;

			case 12:
				REQUIRE(model.build().substr(0, (16<<10) - 1) == builder.getDebugPreview() );
				REQUIRE(model.build() == toStd(bx::String(builder) ) );
				break;

			case 13:
				if (0 == rng.gen() % 4)
				{
					const bool        hadEdits = model.hasEdits();
					const uint32_t    cursor   = model.cursor;
					const std::string expected = model.build();

					const bx::StringView view = builder.getView();
					REQUIRE(expected == toStd(view) );
					REQUIRE( (0 == view.getLength() || '\0' == view.getPtr()[view.getLength()]) );

					model.reset(expected);
					model.cursor = hadEdits ? UINT32_MAX : cursor;
				}
				break;

			case 14:
				switch (rng.gen() % 6)
				{
				case 0:
					{
						const std::string expected = model.build();
						bx::ArrayT<char> chars = builder.release();
						REQUIRE(expected == std::string(chars.getPtr(), chars.getCount() ) );
						REQUIRE(builder.isEmpty() );
						model.reset("");
					}
					break;

				case 1:
					{
						const std::string text = genText(rng);
						builder = toView(text);
						model.reset(text);
					}
					break;

				case 2:
					builder.clear();
					model.reset("");
					break;

				case 3:
					{
						const std::string expected = model.build();
						const bx::String  str(builder);
						bx::StringBuilder other(str);
						builder = bx::move(other);
						model.reset(expected);
					}
					break;

				case 4:
					{
						bx::StringBuilder other(bx::move(builder) );
						REQUIRE(builder.isEmpty() );
						builder = bx::move(other);
					}
					break;

				default:
					builder.trim();
					break;
				}
				break;

			default:
				if (0 == rng.gen() % 8)
				{
					builder.setCapacity(rng.gen() % 512);
				}
				break;
			}

			const std::string expected = model.build();
			REQUIRE(int32_t(expected.size() ) == builder.getLength() );
			REQUIRE(expected.empty() == builder.isEmpty() );
		}

		REQUIRE(model.build() == toStd(builder.getView() ) );
	}
}

TEST_CASE("String literal caches handle", "[string]")
{
	BX_STRING_LITERAL(s_literalCached, "literal_cached");
	BX_STRING_LITERAL(s_literalDedup,  "literal_dedup");

	bx::StringPool* builtin = bx::StringPool::getDefault();

	REQUIRE(0 == s_literalCached.m_cachedHandle);

	const bx::String a = s_literalCached;
	REQUIRE(builtin == a.getPool() );
	REQUIRE(a.getCPtr() == s_literalCached.m_data);
	REQUIRE(a.getHandle() == s_literalCached.m_cachedHandle);

	const bx::String b = s_literalCached;
	REQUIRE(a == b);
	REQUIRE(a.getHandle() == b.getHandle() );

	bx::CrtAllocator allocator;

	{
		bx::StringPool pool(allocator.getHandle(), 0);
		bx::StringPool::setDefault(&pool);

		{
			const bx::String other = s_literalCached;
			REQUIRE(&pool == other.getPool() );
			REQUIRE(other.getCPtr() == s_literalCached.m_data);
			REQUIRE(other.getHandle() == s_literalCached.m_cachedHandle);

			const bx::String runtime("literal_dedup");
			REQUIRE(1 == getRefCount(runtime) );

			{
				const bx::String dedup = s_literalDedup;
				REQUIRE(dedup == runtime);
				REQUIRE(dedup.getCPtr() == runtime.getCPtr() );
				REQUIRE(2 == getRefCount(runtime) );
				REQUIRE(0 == s_literalDedup.m_cachedHandle);
			}

			REQUIRE(1 == getRefCount(runtime) );
		}

		bx::StringPool::setDefault(NULL);
		REQUIRE(1 == pool.trim() );
	}

	const bx::String c = s_literalCached;
	REQUIRE(builtin == c.getPool() );
	REQUIRE(c.getHandle() == a.getHandle() );
	REQUIRE(c.getCPtr() == s_literalCached.m_data);
}

const bx::StringPoolEntry& stringLiteralExprOtherTu();

static const bx::StringPoolEntry& stringLiteralExpr()
{
	return "literal_expr"_str;
}

TEST_CASE("String literal in expression", "[string]")
{
	BX_STRING_LITERAL(s_literalExprNamed, "literal_expr_named");

	const bx::StringPoolEntry& entry = "literal_expr"_str;

	REQUIRE(&entry == &stringLiteralExpr() );
	REQUIRE(&entry == &stringLiteralExprOtherTu() );
	REQUIRE(entry.isImmortal() );
	REQUIRE(bx::hashWy("literal_expr", 12) == entry.m_hash);
	REQUIRE(12 == entry.m_length);
	REQUIRE(0 == bx::strCmp("literal_expr", entry.getCPtr() ) );
	REQUIRE(&entry != static_cast<const bx::StringPoolEntry*>(&"literal_expr2"_str) );

	const bx::String a = "literal_expr"_str;
	REQUIRE(a.getCPtr() == entry.getCPtr() );
	REQUIRE(a.getHandle() == "literal_expr"_str.m_cachedHandle);

	const bx::String b = "literal_expr"_str;
	REQUIRE(a == b);
	REQUIRE(a.getHandle() == b.getHandle() );

	const bx::String empty = ""_str;
	REQUIRE(empty.isEmpty() );
	REQUIRE(empty == bx::String() );

	const bx::String named = "literal_expr_named"_str;
	REQUIRE(named == bx::String(s_literalExprNamed) );
	REQUIRE(named.getCPtr() == "literal_expr_named"_str.m_data);
}

TEST_CASE("StringPool trim visits released entries", "[string]")
{
	bx::CrtAllocator allocator;
	bx::StringPool pool(allocator.getHandle(), 0);

	constexpr uint32_t kNum = 1000;

	bx::ArrayT<bx::String> strs(allocator.getHandle() );
	char name[32];

	for (uint32_t ii = 0; ii < kNum; ++ii)
	{
		bx::snprintf(name, sizeof(name), "trim/%u", ii);
		strs.add(pool.intern(name) );
	}

	REQUIRE(0 == pool.trim() );

	for (uint32_t ii = 0; ii < kNum; ii += 2)
	{
		strs[ii].clear();
	}

	REQUIRE(kNum/2 == pool.getZeroCount() );

	const bx::String resurrected = pool.intern("trim/10");
	REQUIRE(kNum/2-1 == pool.getZeroCount() );

	REQUIRE(kNum/2-1 == pool.trim() );
	REQUIRE(0 == pool.getZeroCount() );
	REQUIRE(kNum/2+2 == pool.getCount() );
	REQUIRE(0 == pool.trim() );

	REQUIRE(resurrected == bx::StringView("trim/10") );

	for (uint32_t ii = 1; ii < kNum; ii += 2)
	{
		bx::snprintf(name, sizeof(name), "trim/%u", ii);
		REQUIRE(strs[ii] == bx::StringView(name) );
	}

	bx::snprintf(name, sizeof(name), "trim/%u", 4);
	REQUIRE(pool.find(name).isEmpty() );

	strs.clear();
}
