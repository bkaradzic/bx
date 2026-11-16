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

/// Error context ring buffer size in bytes.
#ifndef BX_CONFIG_ERROR_CONTEXT_RING_SIZE
#	define BX_CONFIG_ERROR_CONTEXT_RING_SIZE (64<<10)
#endif // BX_CONFIG_ERROR_CONTEXT_RING_SIZE
static_assert(0 == BX_CONFIG_ERROR_CONTEXT_RING_SIZE % 256
	, "BX_CONFIG_ERROR_CONTEXT_RING_SIZE must be a multiple of 256."
	);

/// Maximum size of one error context in bytes.
#ifndef BX_CONFIG_ERROR_CONTEXT_MAX_SIZE
#	define BX_CONFIG_ERROR_CONTEXT_MAX_SIZE (4<<10)
#endif // BX_CONFIG_ERROR_CONTEXT_MAX_SIZE
static_assert(0 == BX_CONFIG_ERROR_CONTEXT_MAX_SIZE % 256
	, "BX_CONFIG_ERROR_CONTEXT_MAX_SIZE must be a multiple of 256."
	);
static_assert(BX_CONFIG_ERROR_CONTEXT_MAX_SIZE <= BX_CONFIG_ERROR_CONTEXT_RING_SIZE
	, "BX_CONFIG_ERROR_CONTEXT_MAX_SIZE must not exceed BX_CONFIG_ERROR_CONTEXT_RING_SIZE."
	);

/// Records allocation sites and live block counts, enabled in debug builds.
#ifndef BX_CONFIG_MEMORY_TRACKING
#	define BX_CONFIG_MEMORY_TRACKING BX_CONFIG_DEBUG
#endif // BX_CONFIG_MEMORY_TRACKING

/// Number of allocator handles including the default allocator's, 2 to 256. The bx heap takes about 12.
#ifndef BX_CONFIG_MAX_ALLOCATOR_HANDLES
#	define BX_CONFIG_MAX_ALLOCATOR_HANDLES 256
#endif // BX_CONFIG_MAX_ALLOCATOR_HANDLES
static_assert(2 <= BX_CONFIG_MAX_ALLOCATOR_HANDLES && BX_CONFIG_MAX_ALLOCATOR_HANDLES <= 256
	, "BX_CONFIG_MAX_ALLOCATOR_HANDLES must be 2 to 256, the handle is 8-bit and handle 0 is the default allocator."
	);

/// Largest size in bytes `Heap::Balanced` takes from `ThreadSpanAllocatorT`.
#ifndef BX_CONFIG_HEAP_BALANCED_SPLIT
#	define BX_CONFIG_HEAP_BALANCED_SPLIT (8<<10)
#endif // BX_CONFIG_HEAP_BALANCED_SPLIT

/// Large blocks freed at or above this size in bytes keep their pages until purged.
#ifndef BX_CONFIG_HEAP_LARGE_PURGE_SIZE
#	define BX_CONFIG_HEAP_LARGE_PURGE_SIZE (4<<20)
#endif // BX_CONFIG_HEAP_LARGE_PURGE_SIZE

/// Time in milliseconds freed large block pages are kept before they're returned to the OS.
#ifndef BX_CONFIG_HEAP_LARGE_PURGE_DELAY_MS
#	define BX_CONFIG_HEAP_LARGE_PURGE_DELAY_MS 1000
#endif // BX_CONFIG_HEAP_LARGE_PURGE_DELAY_MS

/// Bytes of kept freed large block pages above which they're returned to the OS without delay.
#ifndef BX_CONFIG_HEAP_LARGE_DIRTY_SIZE
#	define BX_CONFIG_HEAP_LARGE_DIRTY_SIZE (256<<20)
#endif // BX_CONFIG_HEAP_LARGE_DIRTY_SIZE

/// Time in milliseconds pages of empty segments are kept before they're returned to the OS.
#ifndef BX_CONFIG_HEAP_THREAD_SPAN_PURGE_DELAY_MS
#	define BX_CONFIG_HEAP_THREAD_SPAN_PURGE_DELAY_MS 10
#endif // BX_CONFIG_HEAP_THREAD_SPAN_PURGE_DELAY_MS

/// Time in milliseconds between returns of a thread's cached large class blocks to their pages.
#ifndef BX_CONFIG_HEAP_THREAD_SPAN_SWEEP_DELAY_MS
#	define BX_CONFIG_HEAP_THREAD_SPAN_SWEEP_DELAY_MS 100
#endif // BX_CONFIG_HEAP_THREAD_SPAN_SWEEP_DELAY_MS

/// Number of empty segments each thread keeps instead of returning them to the upstream allocator.
#ifndef BX_CONFIG_HEAP_THREAD_SPAN_EMPTY_SEGMENTS
#	define BX_CONFIG_HEAP_THREAD_SPAN_EMPTY_SEGMENTS 4
#endif // BX_CONFIG_HEAP_THREAD_SPAN_EMPTY_SEGMENTS

/// Time in milliseconds pages of empty segments are kept before they're returned to the OS.
#ifndef BX_CONFIG_HEAP_SHARED_SPAN_PURGE_DELAY_MS
#	define BX_CONFIG_HEAP_SHARED_SPAN_PURGE_DELAY_MS 10
#endif // BX_CONFIG_HEAP_SHARED_SPAN_PURGE_DELAY_MS

/// Number of empty segments kept instead of being returned to the upstream allocator.
#ifndef BX_CONFIG_HEAP_SHARED_SPAN_EMPTY_SEGMENTS
#	define BX_CONFIG_HEAP_SHARED_SPAN_EMPTY_SEGMENTS 2
#endif // BX_CONFIG_HEAP_SHARED_SPAN_EMPTY_SEGMENTS

/// Bytes of blocks moved at once between a thread cache and the shared pages.
#ifndef BX_CONFIG_HEAP_SHARED_SPAN_BATCH_SIZE
#	define BX_CONFIG_HEAP_SHARED_SPAN_BATCH_SIZE (16<<10)
#endif // BX_CONFIG_HEAP_SHARED_SPAN_BATCH_SIZE

/// Smallest number of edit records `StringBuilder` grows its edit list by.
#ifndef BX_CONFIG_STRING_BUILDER_EDIT_CAPACITY
#	define BX_CONFIG_STRING_BUILDER_EDIT_CAPACITY 32
#endif // BX_CONFIG_STRING_BUILDER_EDIT_CAPACITY

/// Smallest number of bytes `StringBuilder` grows its text buffer by.
#ifndef BX_CONFIG_STRING_BUILDER_TEXT_CAPACITY
#	define BX_CONFIG_STRING_BUILDER_TEXT_CAPACITY 256
#endif // BX_CONFIG_STRING_BUILDER_TEXT_CAPACITY

/// Size in bytes of the stack buffer `StringBuilder` formats into when its text buffer has less room.
#ifndef BX_CONFIG_STRING_BUILDER_PRINTF_CAPACITY
#	define BX_CONFIG_STRING_BUILDER_PRINTF_CAPACITY 256
#endif // BX_CONFIG_STRING_BUILDER_PRINTF_CAPACITY

#endif // BX_CONFIG_H_HEADER_GUARD
