#include "test.h"
#include <bx/os.h>

TEST_CASE("getProcessMemoryUsed", "")
{
	REQUIRE(0 != bx::getProcessMemoryUsed() );
//	DBG("bx::getProcessMemoryUsed %d", bx::getProcessMemoryUsed() );
}

TEST_CASE("getTempPath", "")
{
	char tmpDir[512];
	uint32_t len = BX_COUNTOF(tmpDir);
	REQUIRE(bx::getTempPath(tmpDir, &len) );
}
