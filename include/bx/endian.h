/*
 * Copyright 2010-2013 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_ENDIAN_H__
#define __BX_ENDIAN_H__

#include "bx.h"

namespace bx
{
	inline uint16_t endianSwap(uint16_t _in)
	{
		return (_in>>8) | (_in<<8);
	}
	
	inline uint32_t endianSwap(uint32_t _in)
	{
		return (_in>>24) | (_in<<24)
			 | ( (_in&0x00ff0000)>>8) | ( (_in&0x0000ff00)<<8)
			 ;
	}

	inline uint64_t endianSwap(uint64_t _in)
	{
		return (_in>>56) | (_in<<56)
			 | ( (_in&UINT64_C(0x00ff000000000000) )>>40) | ( (_in&UINT64_C(0x000000000000ff00) )<<40)
			 | ( (_in&UINT64_C(0x0000ff0000000000) )>>24) | ( (_in&UINT64_C(0x0000000000ff0000) )<<24)
			 | ( (_in&UINT64_C(0x000000ff00000000) )>>8)  | ( (_in&UINT64_C(0x00000000ff000000) )<<8)
			 ;
	}

	inline int16_t endianSwap(int16_t _in)
	{
		return (int16_t)endianSwap( (uint16_t)_in);
	}

	inline int32_t endianSwap(int32_t _in)
	{
		return (int32_t)endianSwap( (uint32_t)_in);
	}

	inline int64_t endianSwap(int64_t _in)
	{
		return (int64_t)endianSwap( (uint64_t)_in);
	}

	template <typename Ty>
	inline Ty littleEndian(const Ty _in)
	{
#if BX_CPU_ENDIAN_BIG
		return endianSwap(_in);
#else
		return _in;
#endif // BX_CPU_ENDIAN_BIG
	}

	template <typename Ty>
	inline Ty bigEndian(const Ty _in)
	{
#if BX_CPU_ENDIAN_LITTLE
		return endianSwap(_in);
#else
		return _in;
#endif // BX_CPU_ENDIAN_LITTLE
	}

} // namespace bx

#endif // __BX_ENDIAN_H__
