/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_SUPERLUMINAL_H_HEADER_GUARD
#define BX_SUPERLUMINAL_H_HEADER_GUARD

#include "bx.h"

namespace bx
{
	/// Superluminal profiler's Performance API (https://superluminal.eu), loaded at
	/// runtime from `PerformanceAPI.dll`, without Superluminal headers, or import
	/// library. DLL is looked up next to executable, and then in Superluminal's
	/// install directory. Only Windows is supported. When API is not loaded, all
	/// functions do nothing.
	class Superluminal
	{
		BX_CLASS(Superluminal
			, NO_COPY
			);

	public:
		///
		Superluminal();

		/// Unloads API if it's loaded.
		~Superluminal();

		/// Load API.
		///
		/// @returns True if API is loaded.
		///
		bool load();

		/// Unload API.
		void unload();

		/// Returns true if API is loaded.
		bool isLoaded() const;

		/// Begin instrumentation event.
		///
		/// @param[in] _id Event id. It must be string literal, Superluminal keeps
		///   pointer to it.
		/// @param[in] _data Data that's shown with event, it can be dynamic string, or
		///   NULL.
		/// @param[in] _abgr Color in ABGR format, 0 is default color.
		///
		void beginEvent(const char* _id, const char* _data = NULL, uint32_t _abgr = 0);

		/// End instrumentation event that was most recently begun on calling thread.
		void endEvent();

		/// Register fiber. Fiber must be registered before it's switched to. Thread
		/// that switches to fibers is fiber too, it must be registered as well.
		///
		/// @param[in] _fiber Unique id of fiber.
		///
		void registerFiber(uint64_t _fiber);

		/// Unregister fiber.
		void unregisterFiber(uint64_t _fiber);

		/// Called right before switching from fiber `_current` to fiber `_new`.
		/// Instrumentation events that are open on fiber stay open while it's
		/// switched out.
		void beginFiberSwitch(uint64_t _current, uint64_t _new);

		/// Called right after switch, by fiber that called `beginFiberSwitch`, once
		/// it's switched back in. `_fiber` is that fiber's id.
		void endFiberSwitch(uint64_t _fiber);

	private:
		struct SuppressTailCallOptimization
		{
			int64_t suppressTailCall[3];
		};

		typedef void (*BeginEventFn)(const char* _id, const char* _data, uint32_t _rgba);
		typedef SuppressTailCallOptimization (*EndEventFn)();
		typedef void (*FiberFn)(uint64_t _fiber);
		typedef void (*BeginFiberSwitchFn)(uint64_t _current, uint64_t _new);

		static void stubBeginEvent(const char* _id, const char* _data, uint32_t _rgba);
		static SuppressTailCallOptimization stubEndEvent();
		static void stubFiber(uint64_t _fiber);
		static void stubBeginFiberSwitch(uint64_t _current, uint64_t _new);

		void stubAll();

		void*              m_dll;
		BeginEventFn       m_beginEvent;
		EndEventFn         m_endEvent;
		FiberFn            m_registerFiber;
		FiberFn            m_unregisterFiber;
		BeginFiberSwitchFn m_beginFiberSwitch;
		FiberFn            m_endFiberSwitch;
	};

} // namespace bx

#include "inline/superluminal.inl"

#endif // BX_SUPERLUMINAL_H_HEADER_GUARD
