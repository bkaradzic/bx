/*
 * Copyright 2010-2011 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_FLOAT4_T_H__
#define __BX_FLOAT4_T_H__

#include <stdint.h>
#include "bx.h"

#define BX_FLOAT4_INLINE BX_FORCE_INLINE

#if 0 // defined(__SSE2__)
#	include "float4_sse.h"
#elif 0 // __ARM_NEON__
#	include "float4_neon.h"
#else
#	include "float4_ref.h"
#endif //

#endif // __BX_FLOAT4_T_H__
