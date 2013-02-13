/*
 * Copyright 2010-2012 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#ifndef __BX_RINGBUFFER_H__
#define __BX_RINGBUFFER_H__

#include "bx.h"
#include "cpu.h"
#include "uint32_t.h"

namespace bx
{
	class RingBufferControl
	{
	public:
		RingBufferControl(uint32_t _size)
			: m_size(_size)
			, m_current(0)
			, m_write(0)
			, m_read(0)
		{
		}

		~RingBufferControl()
		{
		}

		uint32_t available() const
		{
			return distance(m_read, m_current);
		}

		uint32_t consume(uint32_t _size) // consumer only
		{
			const uint32_t maxSize    = distance(m_read, m_current);
			const uint32_t sizeNoSign = uint32_and(_size, 0x7FFFFFFF);
			const uint32_t test       = uint32_sub(sizeNoSign, maxSize);
			const uint32_t size       = uint32_sels(test, _size, maxSize);
			const uint32_t advance    = uint32_add(m_read, size);
			const uint32_t read       = uint32_mod(advance, m_size);
			m_read = read;
			return size;
		}

		uint32_t reserve(uint32_t _size) // producer only
		{
			const uint32_t dist       = distance(m_write, m_read)-1;
			const uint32_t maxSize    = uint32_sels(dist, m_size-1, dist);
			const uint32_t sizeNoSign = uint32_and(_size, 0x7FFFFFFF);
			const uint32_t test       = uint32_sub(sizeNoSign, maxSize);
			const uint32_t size       = uint32_sels(test, _size, maxSize);
			const uint32_t advance    = uint32_add(m_write, size);
			const uint32_t write      = uint32_mod(advance, m_size);
			m_write = write;
			return size;
		}

		uint32_t commit(uint32_t _size) // producer only
		{
			const uint32_t maxSize    = distance(m_current, m_write);
			const uint32_t sizeNoSign = uint32_and(_size, 0x7FFFFFFF);
			const uint32_t test       = uint32_sub(sizeNoSign, maxSize);
			const uint32_t size       = uint32_sels(test, _size, maxSize);
			const uint32_t advance    = uint32_add(m_current, size);
			const uint32_t current    = uint32_mod(advance, m_size);
			m_current = current;
			return size;
		}

		uint32_t distance(uint32_t _from, uint32_t _to) const // both
		{
			const uint32_t diff   = uint32_sub(_to, _from);
			const uint32_t le     = uint32_add(m_size, diff);
			const uint32_t result = uint32_sels(diff, le, diff);

			return result;
		}

		const uint32_t m_size;
		uint32_t m_current;
		uint32_t m_write;
		uint32_t m_read;

	private:
		RingBufferControl(const RingBufferControl&);
		void operator=(const RingBufferControl&);
	};

	class SpScRingBufferControl
	{
	public:
		SpScRingBufferControl(uint32_t _size)
			: m_size(_size)
			, m_current(0)
			, m_write(0)
			, m_read(0)
		{
		}

		~SpScRingBufferControl()
		{
		}

		uint32_t available() const
		{
			return distance(m_read, m_current);
		}

		uint32_t consume(uint32_t _size) // consumer only
		{
			const uint32_t maxSize    = distance(m_read, m_current);
			const uint32_t sizeNoSign = uint32_and(_size, 0x7FFFFFFF);
			const uint32_t test       = uint32_sub(sizeNoSign, maxSize);
			const uint32_t size       = uint32_sels(test, _size, maxSize);
			const uint32_t advance    = uint32_add(m_read, size);
			const uint32_t read       = uint32_mod(advance, m_size);
			m_read = read;
			return size;
		}

		uint32_t reserve(uint32_t _size) // producer only
		{
			const uint32_t dist       = distance(m_write, m_read)-1;
			const uint32_t maxSize    = uint32_sels(dist, m_size-1, dist);
			const uint32_t sizeNoSign = uint32_and(_size, 0x7FFFFFFF);
			const uint32_t test       = uint32_sub(sizeNoSign, maxSize);
			const uint32_t size       = uint32_sels(test, _size, maxSize);
			const uint32_t advance    = uint32_add(m_write, size);
			const uint32_t write      = uint32_mod(advance, m_size);
			m_write = write;
			return size;
		}

		uint32_t commit(uint32_t _size) // producer only
		{
			const uint32_t maxSize    = distance(m_current, m_write);
			const uint32_t sizeNoSign = uint32_and(_size, 0x7FFFFFFF);
			const uint32_t test       = uint32_sub(sizeNoSign, maxSize);
			const uint32_t size       = uint32_sels(test, _size, maxSize);
			const uint32_t advance    = uint32_add(m_current, size);
			const uint32_t current    = uint32_mod(advance, m_size);

			// must commit all memory writes before moving m_current pointer
			// once m_current pointer moves data is used by consumer thread
			memoryBarrier();
			m_current = current;
			return size;
		}

		uint32_t distance(uint32_t _from, uint32_t _to) const // both
		{
			const uint32_t diff   = uint32_sub(_to, _from);
			const uint32_t le     = uint32_add(m_size, diff);
			const uint32_t result = uint32_sels(diff, le, diff);

			return result;
		}

		const uint32_t m_size;
		uint32_t m_current;
		uint32_t m_write;
		uint32_t m_read;

	private:
		SpScRingBufferControl(const SpScRingBufferControl&);
		void operator=(const SpScRingBufferControl&);
	};

	template <typename Control>
	class ReadRingBufferT
	{
	public:
		ReadRingBufferT(Control& _control, const char* _buffer, uint32_t _size)
			: m_control(_control)
			, m_read(_control.m_read)
			, m_end(m_read+_size)
			, m_size(_size)
			, m_buffer(_buffer)
		{
			BX_CHECK(_control.available() >= _size, "%d >= %d", _control.available(), _size);
		}

		~ReadRingBufferT()
		{
		}

		void end()
		{
			m_control.consume(m_size);
		}

		void read(char* _data, uint32_t _len)
		{
			const uint32_t end = (m_read + _len) % m_control.m_size;
			uint32_t wrap = 0;
			const char* from = &m_buffer[m_read];

			if (end < m_read)
			{
				wrap = m_control.m_size - m_read;
				memcpy(_data, from, wrap);
				_data += wrap;
				from = (const char*)&m_buffer[0];
			}

			memcpy(_data, from, _len-wrap);

			m_read = end;
		}

		void skip(uint32_t _len)
		{
			m_read += _len;
			m_read %= m_control.m_size;
		}

	private:
		template <typename Ty>
		friend class WriteRingBufferT;

		ReadRingBufferT();
		ReadRingBufferT(const Control&);
		void operator=(const Control&);

		Control& m_control;
		uint32_t m_read;
		uint32_t m_end;
		const uint32_t m_size;
		const char* m_buffer;
	};

	typedef ReadRingBufferT<RingBufferControl> ReadRingBuffer;
	typedef ReadRingBufferT<SpScRingBufferControl> SpScReadRingBuffer;

	template <typename Control>
	class WriteRingBufferT
	{
	public:
		WriteRingBufferT(Control& _control, char* _buffer, uint32_t _size)
			: m_control(_control)
			, m_size(_size)
			, m_buffer(_buffer)
		{
			uint32_t size = m_control.reserve(_size);
			BX_UNUSED(size);
			BX_CHECK(size == _size, "%d == %d", size, _size);
			m_write = m_control.m_current;
			m_end = m_write+_size;
		}

		~WriteRingBufferT()
		{
		}

		void end()
		{
			m_control.commit(m_size);
		}

		void write(const char* _data, uint32_t _len)
		{
			const uint32_t end = (m_write + _len) % m_control.m_size;
			uint32_t wrap = 0;
			char* to = &m_buffer[m_write];

			if (end < m_write)
			{
				wrap = m_control.m_size - m_write;
				memcpy(to, _data, wrap);
				_data += wrap;
				to = (char*)&m_buffer[0];
			}

			memcpy(to, _data, _len-wrap);

			m_write = end;
		}

		void write(ReadRingBufferT<Control>& _read, uint32_t _len)
		{
			const uint32_t end = (_read.m_read + _len) % _read.m_control.m_size;
			uint32_t wrap = 0;
			const char* from = &_read.m_buffer[_read.m_read];

			if (end < _read.m_read)
			{
				wrap = _read.m_control.m_size - _read.m_read;
				write(from, wrap);
				from = (const char*)&_read.m_buffer[0];
			}

			write(from, _len-wrap);

			_read.m_read = end;
		}

		void skip(uint32_t _len)
		{
			m_write += _len;
			m_write %= m_control.m_size;
		}

	private:
		WriteRingBufferT();
		WriteRingBufferT(const WriteRingBufferT<Control>&);
		void operator=(const WriteRingBufferT<Control>&);

		Control& m_control;
		uint32_t m_write;
		uint32_t m_end;
		const uint32_t m_size;
		char* m_buffer;
	};

	typedef WriteRingBufferT<RingBufferControl> WriteRingBuffer;
	typedef WriteRingBufferT<SpScRingBufferControl> SpScWriteRingBuffer;

} // namespace bx

#endif // __BX_RINGBUFFER_H__
