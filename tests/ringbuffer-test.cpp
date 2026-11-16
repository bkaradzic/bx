/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/ringbuffer.h>

TEST_CASE("RingBufferControl", "")
{
	constexpr uint32_t kMax = 16;

	bx::RingBufferControl control(kMax);

	REQUIRE(kMax   == control.getSize() );
	REQUIRE(0      == control.getNumUsed() );
	REQUIRE(0      == control.getNumReserved() );
	REQUIRE(kMax-1 == control.getNumEmpty() );
	REQUIRE(control.isEmpty() );

	REQUIRE(1 == control.reserve(1)  );

	REQUIRE(kMax   == control.getSize() );
	REQUIRE(0      == control.getNumUsed() );
	REQUIRE(1      == control.getNumReserved() );
	REQUIRE(kMax-2 == control.getNumEmpty() );
	REQUIRE(!control.isEmpty() );

	REQUIRE(0      == control.reserve(16, true) );
	REQUIRE(kMax-2 == control.reserve(16) );

	REQUIRE(kMax   == control.getSize() );
	REQUIRE(0      == control.getNumUsed() );
	REQUIRE(kMax-1 == control.getNumReserved() );
	REQUIRE(0      == control.getNumEmpty() );
	REQUIRE(!control.isEmpty() );

	REQUIRE(15 == control.commit(15)  );

	REQUIRE(kMax   == control.getSize() );
	REQUIRE(kMax-1 == control.getNumUsed() );
	REQUIRE(0      == control.getNumReserved() );
	REQUIRE(0      == control.getNumEmpty() );
	REQUIRE(!control.isEmpty() );

	REQUIRE(15 == control.consume(15) );

	REQUIRE(kMax   == control.getSize() );
	REQUIRE(0      == control.getNumUsed() );
	REQUIRE(0      == control.getNumReserved() );
	REQUIRE(kMax-1 == control.getNumEmpty() );
	REQUIRE(control.isEmpty() );
}

TEST_CASE("RingBufferControl resize", "")
{
	bx::RingBufferControl control(10);

	uint32_t reserved;
	uint32_t commited;
	uint32_t consumed;

	reserved = control.reserve(8);
	REQUIRE(reserved == 8);
	REQUIRE(control.m_current == 0);
	REQUIRE(control.m_write   == 8);
	REQUIRE(control.m_read    == 0);

	commited = control.commit(4);
	REQUIRE(commited == 4);
	REQUIRE(control.m_current == 4);
	REQUIRE(control.m_write   == 8);
	REQUIRE(control.m_read    == 0);

	consumed = control.consume(2);
	REQUIRE(consumed == 2);
	REQUIRE(control.m_current == 4);
	REQUIRE(control.m_write   == 8);
	REQUIRE(control.m_read    == 2);

	REQUIRE(10 == control.getSize() );

	control.resize(10);
	REQUIRE(20 == control.getSize() );

	control.reserve(8);
	REQUIRE(control.m_current == 4);
	REQUIRE(control.m_write   == 16);
	REQUIRE(control.m_read    == 2);

	control.commit(4);
	REQUIRE(control.m_current == 8);
	REQUIRE(control.m_write   == 16);
	REQUIRE(control.m_read    == 2);

	control.consume(2);
	REQUIRE(control.m_current == 8);
	REQUIRE(control.m_write   == 16);
	REQUIRE(control.m_read    == 4);

	reserved = control.reserve(4);
	REQUIRE(reserved == 4);
	commited = control.commit(4);
	REQUIRE(commited == 4);
	consumed = control.consume(6);
	REQUIRE(consumed == 6);

	REQUIRE(control.m_current == 12);
	REQUIRE(control.m_write   == 0);
	REQUIRE(control.m_read    == 10);

	REQUIRE(2  == control.getNumUsed() );
	REQUIRE(8  == control.getNumReserved() );
	REQUIRE(9  == control.getNumEmpty() );

	control.resize(-10);
	REQUIRE(11 == control.getSize() );
	REQUIRE(2  == control.getNumUsed() );
	REQUIRE(8  == control.getNumReserved() );
	REQUIRE(0  == control.getNumEmpty() );

	REQUIRE(control.m_current == 3);
	REQUIRE(control.m_write   == 0);
	REQUIRE(control.m_read    == 1);
}

TEST_CASE("RingBufferControl resize without reserved slots", "")
{
	{
		bx::RingBufferControl control(8);

		REQUIRE(5 == control.reserve(5) );
		REQUIRE(5 == control.commit(5) );
		REQUIRE(2 == control.consume(2) );

		control.resize(8);
		REQUIRE(16 == control.getSize() );
		REQUIRE(3  == control.getNumUsed() );
		REQUIRE(0  == control.getNumReserved() );
		REQUIRE(12 == control.getNumEmpty() );

		REQUIRE(control.m_current == 5);
		REQUIRE(control.m_write   == 5);
		REQUIRE(control.m_read    == 2);
	}

	{
		bx::RingBufferControl control(8);

		REQUIRE(6 == control.reserve(6) );
		REQUIRE(6 == control.commit(6) );
		REQUIRE(5 == control.consume(5) );
		REQUIRE(4 == control.reserve(4) );
		REQUIRE(4 == control.commit(4) );

		REQUIRE(control.m_current == 2);
		REQUIRE(control.m_write   == 2);
		REQUIRE(control.m_read    == 5);

		control.resize(8);
		REQUIRE(16 == control.getSize() );
		REQUIRE(5  == control.getNumUsed() );
		REQUIRE(0  == control.getNumReserved() );
		REQUIRE(10 == control.getNumEmpty() );

		REQUIRE(control.m_current == 2);
		REQUIRE(control.m_write   == 2);
		REQUIRE(control.m_read    == 13);

		control.resize(-8);
		REQUIRE(8 == control.getSize() );
		REQUIRE(5 == control.getNumUsed() );
		REQUIRE(0 == control.getNumReserved() );
		REQUIRE(2 == control.getNumEmpty() );

		REQUIRE(control.m_current == 2);
		REQUIRE(control.m_write   == 2);
		REQUIRE(control.m_read    == 5);
	}

	{
		bx::RingBufferControl control(8);

		REQUIRE(3 == control.reserve(3) );
		REQUIRE(3 == control.commit(3) );
		REQUIRE(3 == control.consume(3) );

		control.resize(8);
		REQUIRE(16 == control.getSize() );
		REQUIRE(0  == control.getNumUsed() );
		REQUIRE(0  == control.getNumReserved() );
		REQUIRE(15 == control.getNumEmpty() );
		REQUIRE(control.isEmpty() );

		REQUIRE(control.m_current == 3);
		REQUIRE(control.m_write   == 3);
		REQUIRE(control.m_read    == 3);
	}
}

TEST_CASE("RingBufferControl shrink past the end of buffer", "")
{
	{
		bx::RingBufferControl control(16);

		REQUIRE(10 == control.reserve(10) );
		REQUIRE(10 == control.commit(10) );
		REQUIRE(8  == control.consume(8) );

		REQUIRE(13 == control.getNumEmpty() );

		control.resize(-13);
		REQUIRE(3 == control.getSize() );
		REQUIRE(2 == control.getNumUsed() );
		REQUIRE(0 == control.getNumReserved() );
		REQUIRE(0 == control.getNumEmpty() );

		REQUIRE(control.m_current == 0);
		REQUIRE(control.m_write   == 0);
		REQUIRE(control.m_read    == 1);
	}

	{
		bx::RingBufferControl control(32);

		REQUIRE(29 == control.reserve(29) );
		REQUIRE(29 == control.commit(29) );
		REQUIRE(18 == control.consume(18) );

		REQUIRE(20 == control.getNumEmpty() );

		control.resize(-4);
		REQUIRE(28 == control.getSize() );
		REQUIRE(11 == control.getNumUsed() );
		REQUIRE(0  == control.getNumReserved() );
		REQUIRE(16 == control.getNumEmpty() );

		REQUIRE(control.m_current == 0);
		REQUIRE(control.m_write   == 0);
		REQUIRE(control.m_read    == 17);
	}
}
