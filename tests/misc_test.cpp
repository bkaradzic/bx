#include "test.h"
#include <bx/os.h>

TEST(getProcessMemoryUsed)
{
	CHECK(0 != bx::getProcessMemoryUsed() );
//	DBG("bx::getProcessMemoryUsed %d", bx::getProcessMemoryUsed() );

	char tmpDir[PATH_MAX];
	uint32_t len = BX_COUNTOF(tmpDir);
	CHECK(bx::getTempPath(tmpDir, &len) );
}
