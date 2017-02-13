/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include <bx/bx.h>
#include <bx/readerwriter.h>
#include <bx/debug.h>

#if !BX_CRT_NONE
#	include <string.h> // memcpy, memmove, memset
#endif // !BX_CRT_NONE

namespace bx
{
	void xchg(void* _a, void* _b, size_t _numBytes)
	{
		uint8_t* lhs = (uint8_t*)_a;
		uint8_t* rhs = (uint8_t*)_b;
		const uint8_t* end = rhs + _numBytes;
		while (rhs != end)
		{
			xchg(*lhs++, *rhs++);
		}
	}

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

	void memCopy(void* _dst, const void* _src, size_t _numBytes)
	{
#if BX_CRT_NONE
		memCopyRef(_dst, _src, _numBytes);
#else
		::memcpy(_dst, _src, _numBytes);
#endif // BX_CRT_NONE
	}

	void memCopy(void* _dst, const void* _src, uint32_t _size, uint32_t _num, uint32_t _srcPitch, uint32_t _dstPitch)
	{
		const uint8_t* src = (const uint8_t*)_src;
		uint8_t* dst = (uint8_t*)_dst;

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			memCopy(dst, src, _size);
			src += _srcPitch;
			dst += _dstPitch;
		}
	}

	///
	void gather(void* _dst, const void* _src, uint32_t _size, uint32_t _num, uint32_t _srcPitch)
	{
		memCopy(_dst, _src, _size, _num, _srcPitch, _size);
	}

	///
	void scatter(void* _dst, const void* _src, uint32_t _size, uint32_t _num, uint32_t _dstPitch)
	{
		memCopy(_dst, _src, _size, _num, _size, _dstPitch);
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

	void memMove(void* _dst, const void* _src, size_t _numBytes)
	{
#if BX_CRT_NONE
		memMoveRef(_dst, _src, _numBytes);
#else
		::memmove(_dst, _src, _numBytes);
#endif // BX_CRT_NONE
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

	void memSet(void* _dst, uint8_t _ch, size_t _numBytes)
	{
#if BX_CRT_NONE
		memSetRef(_dst, _ch, _numBytes);
#else
		::memset(_dst, _ch, _numBytes);
#endif // BX_CRT_NONE
	}

	int32_t memCmpRef(const void* _lhs, const void* _rhs, size_t _numBytes)
	{
		const char* lhs = (const char*)_lhs;
		const char* rhs = (const char*)_rhs;
		for (
			; 0 < _numBytes && *lhs == *rhs
			; ++lhs, ++rhs, --_numBytes
			)
		{
		}

		return 0 == _numBytes ? 0 : *lhs - *rhs;
	}

	int32_t memCmp(const void* _lhs, const void* _rhs, size_t _numBytes)
	{
#if BX_CRT_NONE
		return memCmpRef(_lhs, _rhs, _numBytes);
#else
		return ::memcmp(_lhs, _rhs, _numBytes);
#endif // BX_CRT_NONE
	}

	namespace
	{
		struct Param
		{
			Param()
				: width(0)
				, base(10)
				, prec(6)
				, fill(' ')
				, bits(0)
				, left(false)
				, upper(false)
				, spec(false)
				, sign(false)
			{
			}

			int32_t width;
			uint32_t base;
			uint32_t prec;
			char fill;
			uint8_t bits;
			bool left;
			bool upper;
			bool spec;
			bool sign;
		};

		static int32_t write(WriterI* _writer, const char* _str, int32_t _len, const Param& _param, Error* _err)
		{
			int32_t size = 0;
			int32_t len = (int32_t)strnlen(_str, _len);
			int32_t padding = _param.width > len ? _param.width - len : 0;
			bool sign = _param.sign && len > 1 && _str[0] != '-';
			padding = padding > 0 ? padding - sign : 0;

			if (!_param.left)
			{
				size += writeRep(_writer, _param.fill, padding, _err);
			}

			if (NULL == _str)
			{
				size += write(_writer, "(null)", 6, _err);
			}
			else if (_param.upper)
			{
				for (int32_t ii = 0; ii < len; ++ii)
				{
					size += write(_writer, toUpper(_str[ii]), _err);
				}
			}
			else if (sign)
			{
				size += write(_writer, '+', _err);
				size += write(_writer, _str, len, _err);
			}
			else
			{
				size += write(_writer, _str, len, _err);
			}

			if (_param.left)
			{
				size += writeRep(_writer, _param.fill, padding, _err);
			}

			return size;
		}

		static int32_t write(WriterI* _writer, char _ch, const Param& _param, Error* _err)
		{
			return write(_writer, &_ch, 1, _param, _err);
		}

		static int32_t write(WriterI* _writer, const char* _str, const Param& _param, Error* _err)
		{
			return write(_writer, _str, INT32_MAX, _param, _err);
		}

		static int32_t write(WriterI* _writer, int32_t _i, const Param& _param, Error* _err)
		{
			char str[33];
			int32_t len = toString(str, sizeof(str), _i, _param.base);

			if (len == 0)
			{
				return 0;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, int64_t _i, const Param& _param, Error* _err)
		{
			char str[33];
			int32_t len = toString(str, sizeof(str), _i, _param.base);

			if (len == 0)
			{
				return 0;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, uint32_t _u, const Param& _param, Error* _err)
		{
			char str[33];
			int32_t len = toString(str, sizeof(str), _u, _param.base);

			if (len == 0)
			{
				return 0;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, uint64_t _u, const Param& _param, Error* _err)
		{
			char str[33];
			int32_t len = toString(str, sizeof(str), _u, _param.base);

			if (len == 0)
			{
				return 0;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, double _d, const Param& _param, Error* _err)
		{
			char str[1024];
			int32_t len = toString(str, sizeof(str), _d);

			if (len == 0)
			{
				return 0;
			}

			if (_param.upper)
			{
				toUpperUnsafe(str, len);
			}

			const char* dot = strnchr(str, '.');
			if (NULL != dot)
			{
				const int32_t precLen = int32_t(
						dot
						+ uint32_min(_param.prec + _param.spec, 1)
						+ _param.prec
						- str
						);
				if (precLen > len)
				{
					for (int32_t ii = len; ii < precLen; ++ii)
					{
						str[ii] = '0';
					}
					str[precLen] = '\0';
				}
				len = precLen;
			}

			return write(_writer, str, len, _param, _err);
		}

		static int32_t write(WriterI* _writer, const void* _ptr, const Param& _param, Error* _err)
		{
			char str[35] = "0x";
			int32_t len = toString(str + 2, sizeof(str) - 2, uint32_t(uintptr_t(_ptr) ), 16);

			if (len == 0)
			{
				return 0;
			}

			len += 2;
			return write(_writer, str, len, _param, _err);
		}
	} // anonymous namespace

	int32_t write(WriterI* _writer, const char* _format, va_list _argList, Error* _err)
	{
		MemoryReader reader(_format, uint32_t(strnlen(_format) ) );

		int32_t size = 0;

		while (_err->isOk() )
		{
			char ch = '\0';
			read(&reader, ch, _err);

			if (!_err->isOk() )
			{
				break;
			}
			else if ('%' == ch)
			{
				// %[flags][width][.precision][length sub-specifier]specifier
				read(&reader, ch);

				Param param;

				// flags
				while (' ' == ch
				||     '-' == ch
				||     '+' == ch
				||     '0' == ch
				||     '#' == ch)
				{
					switch (ch)
					{
						default:
						case ' ': param.fill = ' ';  break;
						case '-': param.left = true; break;
						case '+': param.sign = true; break;
						case '0': param.fill = '0';  break;
						case '#': param.spec = true; break;
					}

					read(&reader, ch);
				}

				if (param.left)
				{
					param.fill = ' ';
				}

				// width
				if ('*' == ch)
				{
					read(&reader, ch);
					param.width = va_arg(_argList, int32_t);

					if (0 > param.width)
					{
						param.left  = true;
						param.width = -param.width;
					}

				}
				else
				{
					while (isNumeric(ch) )
					{
						param.width = param.width * 10 + ch - '0';
						read(&reader, ch);
					}
				}

				// .precision
				if ('.' == ch)
				{
					read(&reader, ch);

					if ('*' == ch)
					{
						read(&reader, ch);
						param.prec = va_arg(_argList, int32_t);
					}
					else
					{
						param.prec = 0;
						while (isNumeric(ch) )
						{
							param.prec = param.prec * 10 + ch - '0';
							read(&reader, ch);
						}
					}
				}

				// length sub-specifier
				while ('h' == ch
				||     'I' == ch
				||     'l' == ch
				||     'j' == ch
				||     't' == ch
				||     'z' == ch)
				{
					switch (ch)
					{
						default: break;

						case 'j': param.bits = sizeof(intmax_t )*8; break;
						case 't': param.bits = sizeof(size_t   )*8; break;
						case 'z': param.bits = sizeof(ptrdiff_t)*8; break;

						case 'h': case 'I': case 'l':
							switch (ch)
							{
								case 'h': param.bits = sizeof(short int)*8; break;
								case 'l': param.bits = sizeof(long int )*8; break;
								default: break;
							}

							read(&reader, ch);
							switch (ch)
							{
								case 'h': param.bits = sizeof(signed char  )*8; break;
								case 'l': param.bits = sizeof(long long int)*8; break;
								case '3':
								case '6':
									read(&reader, ch);
									switch (ch)
									{
										case '2': param.bits = sizeof(int32_t)*8; break;
										case '4': param.bits = sizeof(int64_t)*8; break;
										default: break;
									}
									break;

								default: seek(&reader, -1); break;
							}
							break;
					}

					read(&reader, ch);
				}

				// specifier
				switch (toLower(ch) )
				{
					case 'c':
						size += write(_writer, char(va_arg(_argList, int32_t) ), param, _err);
						break;

					case 's':
						size += write(_writer, va_arg(_argList, const char*), param, _err);
						break;

					case 'o':
						param.base = 8;
						switch (param.bits)
						{
						default: size += write(_writer, va_arg(_argList, int32_t), param, _err); break;
						case 64: size += write(_writer, va_arg(_argList, int64_t), param, _err); break;
						}
						break;

					case 'i':
					case 'd':
						param.base = 10;
						switch (param.bits)
						{
						default: size += write(_writer, va_arg(_argList, int32_t), param, _err); break;
						case 64: size += write(_writer, va_arg(_argList, int64_t), param, _err); break;
						};
						break;

					case 'f':
						param.upper = isUpper(ch);
						size += write(_writer, va_arg(_argList, double), param, _err);
						break;

					case 'p':
						size += write(_writer, va_arg(_argList, void*), param, _err);
						break;

					case 'x':
						param.base  = 16;
						param.upper = isUpper(ch);
						switch (param.bits)
						{
						default: size += write(_writer, va_arg(_argList, uint32_t), param, _err); break;
						case 64: size += write(_writer, va_arg(_argList, uint64_t), param, _err); break;
						}
						break;

					case 'u':
						param.base = 10;
						switch (param.bits)
						{
						default: size += write(_writer, va_arg(_argList, uint32_t), param, _err); break;
						case 64: size += write(_writer, va_arg(_argList, uint64_t), param, _err); break;
						}
						break;

					default:
						size += write(_writer, ch, _err);
						break;
				}
			}
			else
			{
				size += write(_writer, ch, _err);
			}
		}

		size += write(_writer, '\0', _err);

		return size;
	}

	int32_t write(WriterI* _writer, Error* _err, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		int32_t size = write(_writer, _format, argList, _err);
		va_end(argList);
		return size;
	}

} // namespace bx

#if BX_CRT_NONE
extern "C" void* memcpy(void* _dst, const void* _src, size_t _numBytes)
{
	bx::memCopy(_dst, _src, _numBytes);
	return _dst;
}

extern "C" void* memmove(void* _dst, const void* _src, size_t _numBytes)
{
	bx::memMove(_dst, _src, _numBytes);
	return _dst;
}

extern "C" void* memset(void* _dst, int _ch, size_t _numBytes)
{
	bx::memSet(_dst, uint8_t(_ch), _numBytes);
	return _dst;
}

extern "C" int32_t memcmp(const void* _lhs, const void* _rhs, size_t _numBytes)
{
	return bx::memCmp(_lhs, _rhs, _numBytes);
}
#endif // BX_CRT_NONE
