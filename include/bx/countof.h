/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_COUNTOF_H__
#define __BX_COUNTOF_H__

#include "bx.h"

namespace bx
{
	// http://cnicholson.net/2011/01/stupid-c-tricks-a-better-sizeof_array/
	template<typename T, size_t N> char (&COUNTOF_REQUIRES_ARRAY_ARGUMENT(const T(&)[N]) )[N];
#define countof(x) sizeof(bx::COUNTOF_REQUIRES_ARRAY_ARGUMENT(x) )

} // namespace bx

#endif // __BX_COUNTOF_H__
