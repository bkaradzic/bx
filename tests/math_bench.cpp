/*
 * Copyright 2010-2018 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include <bx/math.h>
#include <bx/timer.h>
#include <bx/file.h>

#include <math.h>

void math_bench()
{
	bx::WriterI* writer = bx::getStdOut();
	bx::writePrintf(writer, "Math bench\n\n");

	float result = 0.0f;
	float max = 1389.0f;

	{
		int64_t elapsed = -bx::getHPCounter();

		result = 0.0f;
		for (float xx = 0.0f; xx < max; xx += 0.1f)
		{
			result += ::sqrtf(xx);
		}

		elapsed += bx::getHPCounter();
		bx::writePrintf(writer, "     ::sqrtf: %15f, %f\n", double(elapsed), result);
	}

	{
		int64_t elapsed = -bx::getHPCounter();

		result = 0.0f;
		for (float xx = 0.0f; xx < max; xx += 0.1f)
		{
			result += bx::sqrtRef(xx);
		}

		elapsed += bx::getHPCounter();
		bx::writePrintf(writer, " bx::sqrtRef: %15f, %f\n", double(elapsed), result);
	}

	{
		int64_t elapsed = -bx::getHPCounter();

		result = 0.0f;
		for (float xx = 0.0f; xx < max; xx += 0.1f)
		{
			result += bx::sqrtRef(xx);
		}

		elapsed += bx::getHPCounter();
		bx::writePrintf(writer, "bx::sqrtSimd: %15f, %f\n", double(elapsed), result);
	}

	{
		int64_t elapsed = -bx::getHPCounter();

		result = 0.0f;
		for (float xx = 0.0f; xx < max; xx += 0.1f)
		{
			result += bx::sqrt(xx);
		}

		elapsed += bx::getHPCounter();
		bx::writePrintf(writer, "    bx::sqrt: %15f, %f\n", double(elapsed), result);
	}
}
