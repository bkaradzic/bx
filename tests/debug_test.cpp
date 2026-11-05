/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/debug.h>
#include <bx/error.h>
#include <bx/readerwriter.h>

static BX_NO_INLINE uintptr_t getCalleeStackAddress()
{
	return BX_STACK_FRAME_ADDRESS();
}

TEST_CASE("BX_STACK_FRAME_ADDRESS", "[debug]")
{
	const uintptr_t local = uintptr_t(&local);
	const uintptr_t addr  = BX_STACK_FRAME_ADDRESS();

	// Stack grows down on all supported platforms, callee frame is below caller's.
	REQUIRE(getCalleeStackAddress() < addr);

	// AddressSanitizer keeps locals on its fake stack.
	if (!BX_SANITIZER_ADDRESS)
	{
		REQUIRE( (addr > local ? addr - local : local - addr) < 64<<10);
	}
}

TEST_CASE("getCallStackFast", "[debug]")
{
	uintptr_t stack[32];
	const uint32_t num = bx::getCallStackFast(0, BX_COUNTOF(stack), stack);

	if constexpr (!BX_ENABLED(BX_PLATFORM_LINUX) )
	{
		REQUIRE(0 < num);
	}
}

static int32_t errorContextText(const bx::Error& _error, char* _out, int32_t _max)
{
	bx::StaticMemoryBlockWriter writer(_out, uint32_t(_max - 1) );
	bx::Error err;
	const int32_t len = bx::write(&writer, _error, true, &err);
	_out[bx::min(int32_t(writer.seek() ), _max - 1)] = '\0';
	return len;
}

TEST_CASE("ErrorContext", "[debug]")
{
	char text[8<<10];

	bx::Error none;
	REQUIRE(0 == none.getContext() );
	REQUIRE(0 == errorContextText(none, text, BX_COUNTOF(text) ) );

	bx::Error err;
	BX_ERROR_CONTEXT_BEGIN(&err, bx::kErrorReaderWriterOpen, "Test.", 64);
		BX_ERROR_CONTEXT_WRITE("first line %d\n", 1);
		BX_ERROR_CONTEXT_WRITE("second line %s\n", "two");
	BX_ERROR_CONTEXT_END();

	REQUIRE(err == bx::kErrorReaderWriterOpen);
	REQUIRE(0 != err.getContext() );
	const bx::StringView lines("first line 1\nsecond line two\n");
	REQUIRE(lines.getLength() < errorContextText(err, text, BX_COUNTOF(text) ) );
	REQUIRE(bx::strFind(text, lines).getPtr() == text);

	{
		bx::SizerWriter sizer;
		bx::Error sizerErr;
		REQUIRE(lines.getLength() == bx::write(&sizer, err, false, &sizerErr) );
	}

	const uint32_t first = err.getContext();
	BX_ERROR_CONTEXT_BEGIN(&err, bx::kErrorReaderWriterRead, "Later.", 64);
		BX_ERROR_CONTEXT_WRITE("later\n");
	BX_ERROR_CONTEXT_END();
	REQUIRE(err == bx::kErrorReaderWriterOpen);
	REQUIRE(first == err.getContext() );

	err.reset();
	REQUIRE(0 == err.getContext() );

	BX_ERROR_SET(&err, bx::kErrorReaderWriterOpen, "Test.");
	BX_ERROR_CONTEXT_BEGIN(&err, bx::kErrorReaderWriterRead, "Later.", 64);
		BX_ERROR_CONTEXT_WRITE("dropped\n");
	BX_ERROR_CONTEXT_END();
	REQUIRE(0 == err.getContext() );

	BX_ERROR_CONTEXT_BEGIN(NULL, bx::kErrorReaderWriterOpen, "Test.", 64);
		BX_ERROR_CONTEXT_WRITE("dropped\n");
	BX_ERROR_CONTEXT_END();
}

TEST_CASE("ErrorContext truncates at the size reserved", "[debug]")
{
	char text[8<<10];
	char line[1<<10];
	bx::memSet(line, 'x', sizeof(line) - 1);
	line[sizeof(line) - 1] = '\0';

	bx::Error err;
	BX_ERROR_CONTEXT_BEGIN(&err, bx::kErrorReaderWriterOpen, "Test.", 16);
		BX_ERROR_CONTEXT_WRITE("%s", line);
		BX_ERROR_CONTEXT_WRITE("%s", line);
	BX_ERROR_CONTEXT_END();

	REQUIRE(0 < errorContextText(err, text, BX_COUNTOF(text) ) );

	int32_t xs = 0;
	while ('x' == text[xs])
	{
		++xs;
	}

	REQUIRE(16 <= xs);
	REQUIRE(xs < int32_t(sizeof(line) ) );
}

TEST_CASE("ErrorContext says when the ring reused it", "[debug]")
{
	char text[8<<10];

	bx::Error old;
	BX_ERROR_CONTEXT_BEGIN(&old, bx::kErrorReaderWriterOpen, "Test.", 64);
		BX_ERROR_CONTEXT_WRITE("old\n");
	BX_ERROR_CONTEXT_END();

	for (uint32_t ii = 0; ii < 1024; ++ii)
	{
		bx::Error err;
		BX_ERROR_CONTEXT_BEGIN(&err, bx::kErrorReaderWriterOpen, "Test.", 64);
			BX_ERROR_CONTEXT_WRITE("newer %d\n", ii);
		BX_ERROR_CONTEXT_END();
	}

	REQUIRE(0 < errorContextText(old, text, BX_COUNTOF(text) ) );
	REQUIRE(!bx::strFind(text, "overwritten").isEmpty() );
	REQUIRE(bx::strFind(text, "old").isEmpty() );
}

static BX_NO_INLINE void errorContextFrom(bx::Error* _err, uint32_t _skip)
{
	BX_ERROR_CONTEXT_BEGIN(_err, bx::kErrorReaderWriterOpen, "Test.", 64, _skip);
		BX_ERROR_CONTEXT_WRITE("from a helper\n");
	BX_ERROR_CONTEXT_END();
}

TEST_CASE("ErrorContext skips the helper that opens it", "[debug]")
{
	bx::Error helper;
	errorContextFrom(&helper, 0);

	bx::Error caller;
	errorContextFrom(&caller, 1);

	// Linux walks frame pointers, and compilers there omit them when optimizing.
	char text[8<<10];
	REQUIRE(0 < errorContextText(helper, text, BX_COUNTOF(text) ) );

	if (bx::strFind(text, "Callstack (0)").isEmpty() )
	{
		bx::SizerWriter withHelper;
		bx::SizerWriter withoutHelper;
		bx::Error err;
		REQUIRE(bx::write(&withoutHelper, caller, true, &err) < bx::write(&withHelper, helper, true, &err) );
	}
}
