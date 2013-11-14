/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef BX_H_HEADER_GUARD
#define BX_H_HEADER_GUARD

#include <stdint.h> // uint32_t
#include <stdlib.h> // size_t

#include "platform.h"
#include "macros.h"

namespace bx
{
	// http://cnicholson.net/2011/01/stupid-c-tricks-a-better-sizeof_array/
	template<typename T, size_t N> char (&COUNTOF_REQUIRES_ARRAY_ARGUMENT(const T(&)[N]) )[N];
#define BX_COUNTOF(x) sizeof(bx::COUNTOF_REQUIRES_ARRAY_ARGUMENT(x) )

} // namespace bx

// Annoying C++0x stuff..
namespace std { namespace tr1 {}; using namespace tr1; }

#endif // BX_H_HEADER_GUARD
