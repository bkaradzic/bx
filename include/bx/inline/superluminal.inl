/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_SUPERLUMINAL_H_HEADER_GUARD
#	error "Must be included from bx/superluminal.h!"
#endif // BX_SUPERLUMINAL_H_HEADER_GUARD

#include <bx/endian.h>

namespace bx
{
	inline bool Superluminal::isLoaded() const
	{
		return NULL != m_dll;
	}

	inline void Superluminal::beginEvent(const char* _id, const char* _data, uint32_t _abgr)
	{
		// Superluminal's color is RGBA, all bits set is default color.
		const uint32_t rgba = 0 == _abgr
			? UINT32_MAX
			: (endianSwap(_abgr) & 0xffffff00) | 0xff
			;

		m_beginEvent(_id, _data, rgba);
	}

	inline void Superluminal::endEvent()
	{
		m_endEvent();
	}

	inline void Superluminal::registerFiber(uint64_t _fiber)
	{
		m_registerFiber(_fiber);
	}

	inline void Superluminal::unregisterFiber(uint64_t _fiber)
	{
		m_unregisterFiber(_fiber);
	}

	inline void Superluminal::beginFiberSwitch(uint64_t _current, uint64_t _new)
	{
		m_beginFiberSwitch(_current, _new);
	}

	inline void Superluminal::endFiberSwitch(uint64_t _fiber)
	{
		m_endFiberSwitch(_fiber);
	}

} // namespace bx
