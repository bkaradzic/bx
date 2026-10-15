/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/thread.h>

#if BX_CONFIG_SUPPORTS_THREADING

bx::DefaultAllocator s_allocator;
bx::MpScUnboundedBlockingQueue<void> s_mpsc(&s_allocator);

int32_t threadExit0(bx::Thread* _thread, void*)
{
	_thread->pop();

	s_mpsc.push(reinterpret_cast<void*>(uintptr_t(0x1300) ) );

	return bx::kExitSuccess;
}

int32_t threadExit1(bx::Thread* _thread, void*)
{
	BX_UNUSED(_thread);

	s_mpsc.push(reinterpret_cast<void*>(uintptr_t(0x89) ) );

	return bx::kExitFailure;
}

TEST_CASE("Semaphore", "")
{
	bx::Semaphore sem;
	REQUIRE(!sem.wait(10) );

	sem.post(1);
	REQUIRE(sem.wait() );
}

TEST_CASE("Thread", "")
{
	bx::Thread th;

	REQUIRE(!th.isRunning() );

	bool init = th.init(threadExit0, NULL, 0, NULL);
	REQUIRE(init);

	REQUIRE(th.isRunning() );
	th.push(NULL);
	th.shutdown();

	REQUIRE(!th.isRunning() );
	REQUIRE(th.getExitCode() == 0);

	th.init(threadExit1);
	REQUIRE(th.isRunning() );
	th.shutdown();

	REQUIRE(!th.isRunning() );
	REQUIRE(th.getExitCode() == 1);

	SECTION("MpScUnboundedBlockingQueue")
	{
		void* p0 = s_mpsc.pop();
		void* p1 = s_mpsc.pop();

		uintptr_t result = uintptr_t(p0) | uintptr_t(p1);

		REQUIRE(result == 0x1389);
	}
}

static constexpr uint32_t kNumTlsData = 80;

struct TlsDataCtx
{
	bx::TlsData* tls;
	uint32_t     seed;
	uint32_t     numErrors;
};

static int32_t tlsDataThread(bx::Thread* _thread, void* _userData)
{
	BX_UNUSED(_thread);

	TlsDataCtx& ctx = *(TlsDataCtx*)_userData;

	for (uint32_t ii = 0; ii < kNumTlsData; ++ii)
	{
		ctx.numErrors += NULL != ctx.tls[ii].get();
		ctx.tls[ii].set( (void*)uintptr_t(ctx.seed + ii) );
	}

	for (uint32_t jj = 0; jj < 1000; ++jj)
	{
		for (uint32_t ii = 0; ii < kNumTlsData; ++ii)
		{
			ctx.numErrors += uintptr_t(ctx.seed + ii) != uintptr_t(ctx.tls[ii].get() );
		}
	}

	return bx::kExitSuccess;
}

TEST_CASE("TlsData", "")
{
	bx::TlsData* tls = new bx::TlsData[kNumTlsData];

	for (uint32_t ii = 0; ii < kNumTlsData; ++ii)
	{
		tls[ii].set( (void*)uintptr_t(0x1000 + ii) );
	}

	TlsDataCtx ctx[4];
	bx::Thread thread[4];

	for (uint32_t ii = 0; ii < BX_COUNTOF(thread); ++ii)
	{
		ctx[ii] = { tls, (ii + 1) << 16, 0 };
		thread[ii].init(tlsDataThread, &ctx[ii]);
	}

	for (uint32_t ii = 0; ii < BX_COUNTOF(thread); ++ii)
	{
		thread[ii].shutdown();
		REQUIRE(0 == ctx[ii].numErrors);
	}

	for (uint32_t ii = 0; ii < kNumTlsData; ++ii)
	{
		REQUIRE(uintptr_t(0x1000 + ii) == uintptr_t(tls[ii].get() ) );
	}

	delete [] tls;
}

static thread_local void* s_threadLocal;

BX_NO_INLINE void* getTlsData(const bx::TlsData& _tls)
{
	return _tls.get();
}

BX_NO_INLINE void* getThreadLocal()
{
	return s_threadLocal;
}

TEST_CASE("TlsData-bench", "[.][!benchmark]")
{
	bx::TlsData tls;
	tls.set(&tls);
	s_threadLocal = &tls;

	BENCHMARK("TlsData::get")
	{
		return getTlsData(tls);
	};

	BENCHMARK("thread_local")
	{
		return getThreadLocal();
	};
}

#endif // BX_CONFIG_SUPPORTS_THREADING
