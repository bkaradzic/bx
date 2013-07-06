/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_FLOAT4_T_H__
#define __BX_FLOAT4_T_H__

#include "bx.h"

#define BX_FLOAT4_INLINE BX_FORCE_INLINE

#if defined(__SSE2__) || (BX_COMPILER_MSVC && (BX_ARCH_64BIT || _M_IX86_FP >= 2) )
#	include "float4_sse.h"
#elif 0 // __ARM_NEON__
#	include "float4_neon.h"
#else
#	include "float4_ref.h"
#endif //

#endif // __BX_FLOAT4_T_H__
