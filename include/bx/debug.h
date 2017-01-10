/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_DEBUG_H_HEADER_GUARD
#define BX_DEBUG_H_HEADER_GUARD

#include "bx.h"

namespace bx
{
	///
	void debugBreak();

	///
	void debugOutput(const char* _out);

} // namespace bx

#endif // BX_DEBUG_H_HEADER_GUARD
