/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/file.h>
#include <bx/os.h>

#if BX_PLATFORM_WINDOWS
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif // WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif // BX_PLATFORM_WINDOWS

// Names outside of any single-byte code page, so the ANSI file API cannot represent them.
static const char* kDirName   = "bx.test-\xc3\xbc\xe6\xb5\x8b\xe8\xaf\x95";                             // u-umlaut, Chinese
static const char* kFileName  = "\xd0\xb1\xd1\x80\xd0\xb0\xd0\xbd\xd0\xb8\xd0\xbc\xd0\xb8\xd1\x80.txt"; // Cyrillic
static const char* kMovedName = "\xe6\x96\x87\xe4\xbb\xb6.txt";                                         // Chinese

struct ScopedCwd
{
	ScopedCwd()
		: m_cwd(bx::Dir::Current)
	{
	}

	~ScopedCwd()
	{
		bx::chdir(m_cwd.getCPtr() );
	}

	bx::FilePath m_cwd;
};

TEST_CASE("File paths are UTF-8", "[file][filepath][utf8]")
{
	if (BX_ENABLED(BX_PLATFORM_EMSCRIPTEN) )
	{
		SKIP("Not supported by wasm.");
	}

	bx::FilePath dir(bx::Dir::Temp);
	dir.join(kDirName);
	bx::removeAll(dir, bx::ErrorIgnore{});

	REQUIRE(bx::makeAll(dir, bx::ErrorAssert{}) );

	bx::FileInfo fi;
	REQUIRE(bx::stat(fi, dir) );
	REQUIRE(bx::FileType::Dir == fi.type);

	bx::FilePath file(dir);
	file.join(kFileName);

	{
		bx::FileWriter writer;
		REQUIRE(bx::open(&writer, file, false, bx::ErrorAssert{}) );
		REQUIRE(4 == bx::write(&writer, "utf8", 4, bx::ErrorAssert{}) );
		bx::close(&writer);
	}

	REQUIRE(bx::stat(fi, file) );
	REQUIRE(bx::FileType::File == fi.type);
	REQUIRE(4 == fi.size);

	{
		bx::FileReader reader;
		REQUIRE(bx::open(&reader, file, bx::ErrorAssert{}) );

		char data[4];
		REQUIRE(4 == bx::read(&reader, data, 4, bx::ErrorAssert{}) );
		REQUIRE(0 == bx::memCmp(data, "utf8", 4) );
		bx::close(&reader);
	}

	{
		bx::DirectoryReader dr;
		REQUIRE(bx::open(&dr, dir, bx::ErrorAssert{}) );

		bool found = false;
		bx::Error err;

		while (err.isOk() )
		{
			bx::read(&dr, fi, &err);

			if (err.isOk()
			&&  0 == bx::strCmp(fi.filePath, kFileName) )
			{
				found = true;
				REQUIRE(bx::FileType::File == fi.type);
			}
		}

		bx::close(&dr);
		REQUIRE(found);
	}

	{
		ScopedCwd cwd;
		REQUIRE(0 == bx::chdir(dir.getCPtr() ) );

		const bx::FilePath current(bx::Dir::Current);
		REQUIRE(0 == bx::strCmp(current.getFileName(), kDirName) );

		REQUIRE(bx::stat(fi, kFileName) );
		REQUIRE(bx::FileType::File == fi.type);

#if BX_PLATFORM_WINDOWS
		// The name on disk is the Unicode one, not the UTF-8 bytes reinterpreted in the code page.
		REQUIRE(INVALID_FILE_ATTRIBUTES != ::GetFileAttributesW(L"\u0431\u0440\u0430\u043d\u0438\u043c\u0438\u0440.txt") );
#endif // BX_PLATFORM_WINDOWS
	}

	bx::FilePath moved(dir);
	moved.join(kMovedName);

	REQUIRE(bx::move(file, moved, bx::ErrorAssert{}) );
	REQUIRE(!bx::stat(fi, file) );
	REQUIRE(bx::stat(fi, moved) );
	REQUIRE(4 == fi.size);

	REQUIRE(bx::removeAll(dir, bx::ErrorAssert{}) );
	REQUIRE(!bx::stat(fi, dir) );
}
