/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/superluminal.h>

TEST_CASE("Superluminal", "[superluminal]")
{
	bx::Superluminal superluminal;
	REQUIRE(!superluminal.isLoaded() );

	// Calls without API loaded do nothing.
	superluminal.beginEvent("test", "data", 0xff0000ff);
	superluminal.endEvent();
	superluminal.registerFiber(1);
	superluminal.unregisterFiber(1);

	if (!superluminal.load() )
	{
		REQUIRE(!superluminal.isLoaded() );
		SKIP("Superluminal is not installed.");
	}

	REQUIRE(superluminal.isLoaded() );
	REQUIRE(superluminal.load() );

	superluminal.beginEvent("test");
	superluminal.beginEvent("test", "data", 0xff00ff00);
	superluminal.endEvent();
	superluminal.endEvent();

	// Thread is fiber too, before it switches to other fibers.
	const uint64_t thread = 1;
	const uint64_t fiber  = 2;
	superluminal.registerFiber(thread);
	superluminal.registerFiber(fiber);
	superluminal.unregisterFiber(fiber);
	superluminal.unregisterFiber(thread);

	superluminal.unload();
	REQUIRE(!superluminal.isLoaded() );

	superluminal.beginEvent("test");
	superluminal.endEvent();
}
