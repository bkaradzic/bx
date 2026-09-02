/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_CONFIG_H_HEADER_GUARD
#define BX_CONFIG_H_HEADER_GUARD

#ifndef BX_CONFIG_DEBUG
#	error "BX_CONFIG_DEBUG must be defined in build script!"
#endif // BX_CONFIG_DEBUG

#ifndef BX_CONFIG_ALLOCATOR_DEBUG
#	define BX_CONFIG_ALLOCATOR_DEBUG BX_CONFIG_DEBUG
#endif // BX_CONFIG_ALLOCATOR_DEBUG

#ifndef BX_CONFIG_SUPPORTS_THREADING
#	define BX_CONFIG_SUPPORTS_THREADING !(0 \
			|| BX_PLATFORM_EMSCRIPTEN       \
			)
#endif // BX_CONFIG_SUPPORTS_THREADING

/// Use fused multiply-add instructions for bx::mad and simd_f32_madd/msub/nmsub.
#ifndef BX_CONFIG_FMA
#	if BX_CPU_X86 \
	&& (defined(__FMA__) || (BX_COMPILER_MSVC && defined(__AVX2__) ) )
#		define BX_CONFIG_FMA 1
#	elif BX_CPU_ARM               \
	&& defined(__ARM_FEATURE_FMA) \
	&& (defined(__ARM_NEON) || defined(__ARM_NEON__) )
#		define BX_CONFIG_FMA 1
#	elif BX_CPU_ARM && BX_ARCH_64BIT && BX_COMPILER_MSVC
#		define BX_CONFIG_FMA 1
#	else
#		define BX_CONFIG_FMA 0
#	endif
#endif // BX_CONFIG_FMA

#endif // BX_CONFIG_H_HEADER_GUARD
