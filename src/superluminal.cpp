/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/superluminal.h>
#include <bx/file.h>
#include <bx/os.h>

namespace bx
{
	namespace
	{
		/// Finds `PerformanceAPI.dll` next to executable first, then where Superluminal is
		/// installed. Only file that exists is opened, failed `dlopen` warns.
		bool findDll(FilePath& _outPath)
		{
			_outPath.set(FilePath(Dir::Executable).getPath() );
			_outPath.join("PerformanceAPI.dll");

			FileInfo fileInfo;

			if (stat(fileInfo, _outPath) )
			{
				return true;
			}

			char programFiles[kMaxFilePath];
			uint32_t size = sizeof(programFiles);

			if (!getEnv(programFiles, &size, "ProgramFiles") )
			{
				return false;
			}

			_outPath.set(programFiles);
			_outPath.join(BX_ENABLED(BX_CPU_ARM)
				? "Superluminal/Performance/API/dll/arm64/PerformanceAPI.dll"
				: "Superluminal/Performance/API/dll/x64/PerformanceAPI.dll"
				);

			return stat(fileInfo, _outPath);
		}

	} // namespace

	void Superluminal::stubBeginEvent(const char* _id, const char* _data, uint32_t _rgba)
	{
		BX_UNUSED(_id, _data, _rgba);
	}

	Superluminal::SuppressTailCallOptimization Superluminal::stubEndEvent()
	{
		return {};
	}

	void Superluminal::stubFiber(uint64_t _fiber)
	{
		BX_UNUSED(_fiber);
	}

	void Superluminal::stubBeginFiberSwitch(uint64_t _current, uint64_t _new)
	{
		BX_UNUSED(_current, _new);
	}

	Superluminal::Superluminal()
		: m_dll(NULL)
	{
		stubAll();
	}

	Superluminal::~Superluminal()
	{
		unload();
	}

	void Superluminal::stubAll()
	{
		m_beginEvent       = stubBeginEvent;
		m_endEvent         = stubEndEvent;
		m_registerFiber    = stubFiber;
		m_unregisterFiber  = stubFiber;
		m_beginFiberSwitch = stubBeginFiberSwitch;
		m_endFiberSwitch   = stubFiber;
	}

	bool Superluminal::load()
	{
		if (!BX_ENABLED(BX_PLATFORM_WINDOWS) )
		{
			return false;
		}

		if (NULL != m_dll)
		{
			return true;
		}

		FilePath path;

		if (!findDll(path) )
		{
			return false;
		}

		void* dll = dlopen(path);

		if (NULL == dll)
		{
			return false;
		}

		typedef int (*GetApiFn)(int32_t _version, void** _outFuncs);
		GetApiFn getApi = dlsym<GetApiFn>(dll, "PerformanceAPI_GetAPI");

		// Table layout of API version 3.0.
		void* funcs[11];

		if (NULL == getApi
		||  0 == getApi(0x30000, funcs) )
		{
			BX_TRACE("Failed to obtain Superluminal's API from %s!", path.getCPtr() );
			dlclose(dll);
			return false;
		}

		m_dll              = dll;
		m_beginEvent       = (BeginEventFn      )funcs[2];
		m_endEvent         = (EndEventFn        )funcs[6];
		m_registerFiber    = (FiberFn           )funcs[7];
		m_unregisterFiber  = (FiberFn           )funcs[8];
		m_beginFiberSwitch = (BeginFiberSwitchFn)funcs[9];
		m_endFiberSwitch   = (FiberFn           )funcs[10];

		BX_TRACE("Superluminal's API is loaded from %s.", path.getCPtr() );

		return true;
	}

	void Superluminal::unload()
	{
		if (NULL != m_dll)
		{
			stubAll();
			dlclose(m_dll);
			m_dll = NULL;
		}
	}

} // namespace bx
