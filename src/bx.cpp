/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/debug.h>
#include <bx/readerwriter.h>
#include <bx/os.h>
#include <bx/simd_t.h>

#if !BX_CRT_NONE
#	include <string.h> // memcpy, memmove, memset
#endif // !BX_CRT_NONE

namespace bx
{
	Location Location::current(const char* _filePath, uint32_t _line)
	{
		return Location(_filePath, _line);
	}

	LocationFull LocationFull::current(const char* _function, const char* _filePath, uint32_t _line)
	{
		return LocationFull(_function, _filePath, _line);
	}

	static bool defaultAssertHandler(const Location& _location, uint32_t _skip, const char* _format, va_list _argList)
	{
		char    temp[8192];
		int32_t total = 0;

		StaticMemoryBlockWriter smb(temp, BX_COUNTOF(temp) );

		ErrorIgnore err;

		total += write(&smb, &err, "\n--- ASSERT ---\n\n");

		total += write(&smb, &err, "%s(%d): "
			, _location.filePath
			, _location.line
			);
		total += write(&smb, _format, _argList, &err);
		total += write(&smb, "\n\n", &err);

		uintptr_t stack[32];
		const uint32_t num = getCallStackExact(2 /* skip self */ + _skip, BX_COUNTOF(stack), stack);
		total += writeCallstack(&smb, stack, num, &err);

		total += write(&smb, &err,
			"\nBuild info:\n"
			"\tCompiler: " BX_COMPILER_NAME
			", CPU: " BX_CPU_NAME
			", Arch: " BX_ARCH_NAME
			", OS: " BX_PLATFORM_NAME
			", CRT: " BX_CRT_NAME
			", C++: " BX_CPP_NAME

			", Date: " __DATE__
			", Time: " __TIME__
			"\n"
			);

		total += write(&smb, &err, "\n--- END ---\n\n");

		write(getDebugOut(), temp, total, ErrorIgnore{});

		return true;
	}

	static AssertHandlerFn s_assertHandler = defaultAssertHandler;

	void setAssertHandler(AssertHandlerFn _assertHandlerFn)
	{
		BX_WARN(defaultAssertHandler == s_assertHandler, "Assert handler is already set.");

		if (defaultAssertHandler == s_assertHandler)
		{
			s_assertHandler = NULL == _assertHandlerFn
				? defaultAssertHandler
				: _assertHandlerFn
				;
		}
	}

	bool assertFunction(const Location& _location, uint32_t _skip, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		const bool result = s_assertHandler(_location, _skip + 1 /* skip self */, _format, argList);
		va_end(argList);

		return result;
	}

	void swap(void* _a, void* _b, size_t _numBytes)
	{
		uint8_t* lhs = (uint8_t*)_a;
		uint8_t* rhs = (uint8_t*)_b;
		const uint8_t* end = rhs + _numBytes;
		while (rhs != end)
		{
			swap(*lhs++, *rhs++);
		}
	}

	namespace
	{
#if BX_SIMD_AVX
		typedef simd256_t SimdT;
#else
		typedef simd128_t SimdT;
#endif // BX_SIMD_AVX

		constexpr size_t kSimdSize = sizeof(SimdT);

#if BX_COMPILER_MSVC
		typedef __unaligned uint32_t UnalignedU32;
		typedef __unaligned uint64_t UnalignedU64;
#else
		typedef uint32_t __attribute__( (aligned(1), may_alias) ) UnalignedU32;
		typedef uint64_t __attribute__( (aligned(1), may_alias) ) UnalignedU64;
#endif // BX_COMPILER_MSVC

		BX_FORCE_INLINE void copySmall(uint8_t* _dst, const uint8_t* _src, size_t _numBytes)
		{
			if (8 <= _numBytes)
			{
				const uint64_t head = *(const UnalignedU64*)_src;
				const uint64_t tail = *(const UnalignedU64*)(_src + _numBytes - 8);
				*(UnalignedU64*)_dst                     = head;
				*(UnalignedU64*)(_dst + _numBytes - 8)   = tail;
			}
			else if (4 <= _numBytes)
			{
				const uint32_t head = *(const UnalignedU32*)_src;
				const uint32_t tail = *(const UnalignedU32*)(_src + _numBytes - 4);
				*(UnalignedU32*)_dst                     = head;
				*(UnalignedU32*)(_dst + _numBytes - 4)   = tail;
			}
			else if (0 < _numBytes)
			{
				const uint8_t first  = _src[0];
				const uint8_t middle = _src[_numBytes/2];
				const uint8_t last   = _src[_numBytes - 1];
				_dst[0]              = first;
				_dst[_numBytes/2]    = middle;
				_dst[_numBytes - 1]  = last;
			}
		}

		template<typename Ty>
		BX_FORCE_INLINE void copyTwo(uint8_t* _dst, const uint8_t* _src, size_t _numBytes)
		{
			const Ty head = simd_ldu<Ty>(_src);
			const Ty tail = simd_ldu<Ty>(_src + _numBytes - sizeof(Ty) );
			simd_stu(_dst, head);
			simd_stu(_dst + _numBytes - sizeof(Ty), tail);
		}

		BX_FORCE_INLINE void copyForward(uint8_t* _dst, const uint8_t* _src, size_t _begin, size_t _end)
		{
			size_t ii = _begin;

			for (; ii + 4*kSimdSize <= _end; ii += 4*kSimdSize)
			{
				const SimdT a0 = simd_ldu<SimdT>(_src + ii);
				const SimdT a1 = simd_ldu<SimdT>(_src + ii +   kSimdSize);
				const SimdT a2 = simd_ldu<SimdT>(_src + ii + 2*kSimdSize);
				const SimdT a3 = simd_ldu<SimdT>(_src + ii + 3*kSimdSize);
				simd_st(_dst + ii,               a0);
				simd_st(_dst + ii +   kSimdSize, a1);
				simd_st(_dst + ii + 2*kSimdSize, a2);
				simd_st(_dst + ii + 3*kSimdSize, a3);
			}

			for (; ii + kSimdSize <= _end; ii += kSimdSize)
			{
				const SimdT a0 = simd_ldu<SimdT>(_src + ii);
				simd_st(_dst + ii, a0);
			}
		}

		BX_FORCE_INLINE int32_t cmpBytes(const uint8_t* _lhs, const uint8_t* _rhs, size_t _numBytes)
		{
			for (size_t ii = 0; ii < _numBytes; ++ii)
			{
				if (_lhs[ii] != _rhs[ii])
				{
					return int32_t(_lhs[ii]) - int32_t(_rhs[ii]);
				}
			}

			return 0;
		}

		template<typename Ty>
		BX_FORCE_INLINE bool memIsEqual(const uint8_t* _lhs, const uint8_t* _rhs)
		{
			const Ty lhs  = simd_ldu<Ty>(_lhs);
			const Ty rhs  = simd_ldu<Ty>(_rhs);
			const Ty diff = simd_xor(lhs, rhs);
			return simd_test_zero(diff, diff);
		}

	} // namespace

	void memCopyRef(void* _dst, const void* _src, size_t _numBytes)
	{
		uint8_t* dst = (uint8_t*)_dst;
		const uint8_t* end = dst + _numBytes;
		const uint8_t* src = (const uint8_t*)_src;
		while (dst != end)
		{
			*dst++ = *src++;
		}
	}

	void memCopySimd(void* _dst, const void* _src, size_t _numBytes)
	{
		uint8_t* dst = (uint8_t*)_dst;
		const uint8_t* src = (const uint8_t*)_src;

		if (16 > _numBytes)
		{
			copySmall(dst, src, _numBytes);
			return;
		}

		if (32 >= _numBytes)
		{
			copyTwo<simd128_t>(dst, src, _numBytes);
			return;
		}

		if (2*kSimdSize >= _numBytes)
		{
			copyTwo<SimdT>(dst, src, _numBytes);
			return;
		}

		const SimdT head = simd_ldu<SimdT>(src);
		const SimdT tail = simd_ldu<SimdT>(src + _numBytes - kSimdSize);
		simd_stu(dst, head);

		copyForward(dst, src, kSimdSize - (uintptr_t(dst) & (kSimdSize - 1) ), _numBytes);

		simd_stu(dst + _numBytes - kSimdSize, tail);
	}

	void memCopy(void* _dst, const void* _src, size_t _numBytes)
	{
#if BX_SIMD_SUPPORTED
		memCopySimd(_dst, _src, _numBytes);
#elif BX_CRT_NONE
		memCopyRef(_dst, _src, _numBytes);
#else
		::memcpy(_dst, _src, _numBytes);
#endif // BX_*
	}

	void memCopy(
		  void* _dst
		, uint32_t _dstStride
		, const void* _src
		, uint32_t _srcStride
		, uint32_t _stride
		, uint32_t _numStrides
		)
	{
		if (_stride == _srcStride
		&&  _stride == _dstStride)
		{
			memCopy(_dst, _src, _stride*_numStrides);
			return;
		}

		const uint8_t* src = (const uint8_t*)_src;
		      uint8_t* dst = (uint8_t*)_dst;

		for (uint32_t ii = 0; ii < _numStrides; ++ii, src += _srcStride, dst += _dstStride)
		{
			memCopy(dst, src, _stride);
		}
	}

	void memMoveRef(void* _dst, const void* _src, size_t _numBytes)
	{
		uint8_t* dst = (uint8_t*)_dst;
		const uint8_t* src = (const uint8_t*)_src;

		if (_numBytes == 0
		||  dst == src)
		{
			return;
		}

		//	if (src+_numBytes <= dst || end <= src)
		if (dst < src)
		{
			memCopy(_dst, _src, _numBytes);
			return;
		}

		for (intptr_t ii = _numBytes-1; ii >= 0; --ii)
		{
			dst[ii] = src[ii];
		}
	}

	void memMoveSimd(void* _dst, const void* _src, size_t _numBytes)
	{
		uint8_t* dst = (uint8_t*)_dst;
		const uint8_t* src = (const uint8_t*)_src;

		if (0 == _numBytes
		||  dst == src)
		{
			return;
		}

		if (16 > _numBytes)
		{
			copySmall(dst, src, _numBytes);
			return;
		}

		if (32 >= _numBytes)
		{
			copyTwo<simd128_t>(dst, src, _numBytes);
			return;
		}

		if (2*kSimdSize >= _numBytes)
		{
			copyTwo<SimdT>(dst, src, _numBytes);
			return;
		}

		const SimdT head = simd_ldu<SimdT>(src);
		const SimdT tail = simd_ldu<SimdT>(src + _numBytes - kSimdSize);

		if (dst < src
		||  dst >= src + _numBytes)
		{
			copyForward(dst, src, kSimdSize - (uintptr_t(dst) & (kSimdSize - 1) ), _numBytes);
		}
		else
		{
			size_t ii = _numBytes - (uintptr_t(dst + _numBytes) & (kSimdSize - 1) );

			for (; ii >= 4*kSimdSize; ii -= 4*kSimdSize)
			{
				const SimdT a0 = simd_ldu<SimdT>(src + ii -   kSimdSize);
				const SimdT a1 = simd_ldu<SimdT>(src + ii - 2*kSimdSize);
				const SimdT a2 = simd_ldu<SimdT>(src + ii - 3*kSimdSize);
				const SimdT a3 = simd_ldu<SimdT>(src + ii - 4*kSimdSize);
				simd_st(dst + ii -   kSimdSize, a0);
				simd_st(dst + ii - 2*kSimdSize, a1);
				simd_st(dst + ii - 3*kSimdSize, a2);
				simd_st(dst + ii - 4*kSimdSize, a3);
			}

			for (; ii >= kSimdSize; ii -= kSimdSize)
			{
				const SimdT a0 = simd_ldu<SimdT>(src + ii - kSimdSize);
				simd_st(dst + ii - kSimdSize, a0);
			}
		}

		simd_stu(dst, head);
		simd_stu(dst + _numBytes - kSimdSize, tail);
	}

	void memMove(void* _dst, const void* _src, size_t _numBytes)
	{
#if BX_SIMD_SUPPORTED
		memMoveSimd(_dst, _src, _numBytes);
#elif BX_CRT_NONE
		memMoveRef(_dst, _src, _numBytes);
#else
		::memmove(_dst, _src, _numBytes);
#endif // BX_*
	}

	void memMove(
		  void* _dst
		, uint32_t _dstStride
		, const void* _src
		, uint32_t _srcStride
		, uint32_t _stride
		, uint32_t _numStrides
		)
	{
		if (_stride == _srcStride
		&&  _stride == _dstStride)
		{
			memMove(_dst, _src, _stride*_numStrides);
			return;
		}

		const uint8_t* src = (const uint8_t*)_src;
		      uint8_t* dst = (uint8_t*)_dst;

		for (uint32_t ii = 0; ii < _numStrides; ++ii, src += _srcStride, dst += _dstStride)
		{
			memMove(dst, src, _stride);
		}
	}

	void memSetRef(void* _dst, uint8_t _ch, size_t _numBytes)
	{
		uint8_t* dst = (uint8_t*)_dst;
		const uint8_t* end = dst + _numBytes;
		while (dst != end)
		{
			*dst++ = char(_ch);
		}
	}

	void memSetSimd(void* _dst, uint8_t _ch, size_t _numBytes)
	{
		uint8_t* dst = (uint8_t*)_dst;

		if (16 > _numBytes)
		{
			const uint64_t value = uint64_t(_ch) * UINT64_C(0x0101010101010101);

			if (8 <= _numBytes)
			{
				*(UnalignedU64*)dst                    = value;
				*(UnalignedU64*)(dst + _numBytes - 8)  = value;
			}
			else if (4 <= _numBytes)
			{
				*(UnalignedU32*)dst                    = uint32_t(value);
				*(UnalignedU32*)(dst + _numBytes - 4)  = uint32_t(value);
			}
			else if (0 < _numBytes)
			{
				dst[0]              = _ch;
				dst[_numBytes/2]    = _ch;
				dst[_numBytes - 1]  = _ch;
			}

			return;
		}

		const uint32_t pattern = uint32_t(_ch) * 0x01010101u;

		if (32 >= _numBytes)
		{
			const simd128_t value = simd_splat<simd128_t>(pattern);
			simd_stu(dst, value);
			simd_stu(dst + _numBytes - 16, value);
			return;
		}

		const SimdT value = simd_splat<SimdT>(pattern);

		simd_stu(dst, value);
		simd_stu(dst + _numBytes - kSimdSize, value);

		if (2*kSimdSize >= _numBytes)
		{
			return;
		}

		size_t ii = kSimdSize - (uintptr_t(dst) & (kSimdSize - 1) );

		for (; ii + 4*kSimdSize <= _numBytes; ii += 4*kSimdSize)
		{
			simd_st(dst + ii,               value);
			simd_st(dst + ii +   kSimdSize, value);
			simd_st(dst + ii + 2*kSimdSize, value);
			simd_st(dst + ii + 3*kSimdSize, value);
		}

		for (; ii + kSimdSize <= _numBytes; ii += kSimdSize)
		{
			simd_st(dst + ii, value);
		}
	}

	void memSet(void* _dst, uint8_t _ch, size_t _numBytes)
	{
#if BX_SIMD_SUPPORTED
		memSetSimd(_dst, _ch, _numBytes);
#elif BX_CRT_NONE
		memSetRef(_dst, _ch, _numBytes);
#else
		::memset(_dst, _ch, _numBytes);
#endif // BX_*
	}

	void memSet(void* _dst, uint32_t _dstStride, uint8_t _ch, uint32_t _stride, uint32_t _num)
	{
		if (_stride == _dstStride)
		{
			memSet(_dst, _ch, _stride*_num);
			return;
		}

		uint8_t* dst = (uint8_t*)_dst;

		for (uint32_t ii = 0; ii < _num; ++ii, dst += _dstStride)
		{
			memSet(dst, _ch, _stride);
		}
	}

	int32_t memCmpRef(const void* _lhs, const void* _rhs, size_t _numBytes)
	{
		const uint8_t* lhs = (const uint8_t*)_lhs;
		const uint8_t* rhs = (const uint8_t*)_rhs;
		for (
			; 0 < _numBytes && *lhs == *rhs
			; ++lhs, ++rhs, --_numBytes
			)
		{
		}

		return 0 == _numBytes ? 0 : int32_t(*lhs) - int32_t(*rhs);
	}

	int32_t memCmpSimd(const void* _lhs, const void* _rhs, size_t _numBytes)
	{
		const uint8_t* lhs = (const uint8_t*)_lhs;
		const uint8_t* rhs = (const uint8_t*)_rhs;

		if (16 > _numBytes)
		{
			if (8 <= _numBytes)
			{
				if (*(const UnalignedU64*)lhs == *(const UnalignedU64*)rhs
				&&  *(const UnalignedU64*)(lhs + _numBytes - 8) == *(const UnalignedU64*)(rhs + _numBytes - 8) )
				{
					return 0;
				}
			}
			else if (4 <= _numBytes)
			{
				if (*(const UnalignedU32*)lhs == *(const UnalignedU32*)rhs
				&&  *(const UnalignedU32*)(lhs + _numBytes - 4) == *(const UnalignedU32*)(rhs + _numBytes - 4) )
				{
					return 0;
				}
			}

			return cmpBytes(lhs, rhs, _numBytes);
		}

		if (kSimdSize > _numBytes)
		{
			if (!memIsEqual<simd128_t>(lhs, rhs) )
			{
				return cmpBytes(lhs, rhs, 16);
			}

			const size_t last = _numBytes - 16;

			return memIsEqual<simd128_t>(lhs + last, rhs + last)
				? 0
				: cmpBytes(lhs + last, rhs + last, 16)
				;
		}

		size_t ii = 0;

		for (; ii + 2*kSimdSize <= _numBytes; ii += 2*kSimdSize)
		{
			const SimdT lhs0  = simd_ldu<SimdT>(lhs + ii);
			const SimdT rhs0  = simd_ldu<SimdT>(rhs + ii);
			const SimdT lhs1  = simd_ldu<SimdT>(lhs + ii + kSimdSize);
			const SimdT rhs1  = simd_ldu<SimdT>(rhs + ii + kSimdSize);
			const SimdT diff0 = simd_xor(lhs0, rhs0);
			const SimdT diff1 = simd_xor(lhs1, rhs1);
			const SimdT diff  = simd_or(diff0, diff1);

			if (!simd_test_zero(diff, diff) )
			{
				return cmpBytes(lhs + ii, rhs + ii, 2*kSimdSize);
			}
		}

		if (ii + kSimdSize <= _numBytes)
		{
			if (!memIsEqual<SimdT>(lhs + ii, rhs + ii) )
			{
				return cmpBytes(lhs + ii, rhs + ii, kSimdSize);
			}

			ii += kSimdSize;
		}

		if (ii < _numBytes)
		{
			const size_t last = _numBytes - kSimdSize;

			if (!memIsEqual<SimdT>(lhs + last, rhs + last) )
			{
				return cmpBytes(lhs + last, rhs + last, kSimdSize);
			}
		}

		return 0;
	}

	int32_t memCmp(const void* _lhs, const void* _rhs, size_t _numBytes)
	{
#if BX_SIMD_SUPPORTED
		return memCmpSimd(_lhs, _rhs, _numBytes);
#elif BX_CRT_NONE
		return memCmpRef(_lhs, _rhs, _numBytes);
#else
		return ::memcmp(_lhs, _rhs, _numBytes);
#endif // BX_*
	}

	///
	void gather(void* _dst, const void* _src, uint32_t _srcStride, uint32_t _stride, uint32_t _numStrides)
	{
		memMove(_dst, _stride, _src, _srcStride, _stride, _numStrides);
	}

	///
	void scatter(void* _dst, uint32_t _dstStride, const void* _src, uint32_t _stride, uint32_t _numStrides)
	{
		memMove(_dst, _dstStride, _src, _stride, _stride, _numStrides);
	}

} // namespace bx
