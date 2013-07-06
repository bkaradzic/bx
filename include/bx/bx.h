/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_H__
#define __BX_H__

#include <stdint.h>
#include "platform.h"
#include "macros.h"

namespace bx
{
}// namespace bx

#ifndef BX_NAMESPACE
#	define BX_NAMESPACE 0
#elif BX_NAMESPACE
using namespace bx;
#endif // BX_NAMESPACE

#endif // __BX_H__ 
