/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/file.h>
#include <bx/process.h>
#include <bx/string.h>

#if BX_PLATFORM_WINDOWS
TEST_CASE("ProcessReader quoted path and arguments", "[process]")
{
	bx::FilePath dir(bx::Dir::Temp);
	dir.join("bx process test");
	bx::removeAll(dir, bx::ErrorIgnore{});
	REQUIRE(bx::makeAll(dir, bx::ErrorAssert{}) );

	bx::FilePath file(dir);
	file.join("echo.bat");

	{
		const bx::StringView script = "@echo %*\r\n";

		bx::FileWriter writer;
		REQUIRE(bx::open(&writer, file, false, bx::ErrorAssert{}) );
		REQUIRE(script.getLength() == bx::write(&writer, script.getPtr(), script.getLength(), bx::ErrorAssert{}) );
		bx::close(&writer);
	}

	bx::ProcessReader reader;
	bx::Error err;

	REQUIRE(reader.open(file, "\"a b\" c", &err) );

	char tmp[256];
	const int32_t size = bx::read(&reader, tmp, sizeof(tmp)-1, &err);
	reader.close();
	tmp[size] = '\0';

	bx::removeAll(dir, bx::ErrorIgnore{});

	REQUIRE(0 == reader.getExitCode() );
	REQUIRE(bx::hasPrefix(tmp, "\"a b\" c") );
}

TEST_CASE("ProcessReader command line too long", "[process]")
{
	char args[bx::kMaxFilePath*2];
	bx::memSet(args, 'a', sizeof(args)-1);
	args[sizeof(args)-1] = '\0';

	bx::ProcessReader reader;
	bx::Error err;

	const bool opened = reader.open("C:/Windows/System32/cmd.exe", args, &err);
	if (opened)
	{
		reader.close();
	}

	REQUIRE(!opened);
	REQUIRE(!err.isOk() );
}
#endif // BX_PLATFORM_WINDOWS
