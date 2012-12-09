--
-- Copyright 2010-2012 Branimir Karadzic. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

local bxDir = (path.getabsolute("..") .. "/")

function toolchain(_buildDir, _libDir)

	newoption {
		trigger = "gcc",
		value = "GCC",
		description = "Choose GCC flavor",
		allowed = {
			{ "emscripten", "Emscripten" },
			{ "linux", "Linux" },
			{ "mingw", "MinGW" },
			{ "nacl", "Native Client" },
			{ "pnacl", "Native Client - PNaCl" },
		}
	}

	-- Avoid error when invoking premake4 --help.
	if (_ACTION == nil) then return end

	local XEDK = os.getenv("XEDK")
	if not XEDK then XEDK = "<you must install XBOX SDK>" end

	location (_buildDir .. "projects/" .. _ACTION)

	if _ACTION == "clean" then
		os.rmdir(BUILD_DIR)
	end

	if _ACTION == "gmake" then

		if nil == _OPTIONS["gcc"] then
			print("GCC flavor must be specified!")
			os.exit(1)
		end

		flags {
			"ExtraWarnings",
		}

		if "emscripten" == _OPTIONS["gcc"] then

			if not os.getenv("EMSCRIPTEN") then 
				print("Set EMSCRIPTEN enviroment variables.")
			end

			premake.gcc.cc = "$(EMSCRIPTEN)/emcc"
			premake.gcc.cxx = "$(EMSCRIPTEN)/em++"
			premake.gcc.ar = "$(EMSCRIPTEN)/emar"
			location (_buildDir .. "projects/" .. _ACTION .. "-emscripten")
		end

		if "linux" == _OPTIONS["gcc"] then
			location (_buildDir .. "projects/" .. _ACTION .. "-linux")
		end

		if "mingw" == _OPTIONS["gcc"] then
			premake.gcc.cc = "$(MINGW)/bin/mingw32-gcc"
			premake.gcc.cxx = "$(MINGW)/bin/mingw32-g++"
			premake.gcc.ar = "$(MINGW)/bin/ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-mingw")
		end

		if "nacl" == _OPTIONS["gcc"] then

			if not os.getenv("NACL") then 
				print("Set NACL enviroment variables.")
			end

			premake.gcc.cc = "$(NACL)/bin/x86_64-nacl-gcc"
			premake.gcc.cxx = "$(NACL)/bin/x86_64-nacl-g++"
			premake.gcc.ar = "$(NACL)/bin/x86_64-nacl-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-nacl")
		end

		if "pnacl" == _OPTIONS["gcc"] then

			if not os.getenv("PNACL") then 
				print("Set PNACL enviroment variables.")
			end

			premake.gcc.cc = "$(PNACL)/bin/pnacl-clang"
			premake.gcc.cxx = "$(PNACL)/bin/pnacl-clang++"
			premake.gcc.ar = "$(PNACL)/bin/pnacl-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-pnacl")
		end
	end

	flags {
		"StaticRuntime",
		"NoMinimalRebuild",
		"NoPCH",
		"NativeWChar",
		"NoRTTI",
		"NoExceptions",
		"NoEditAndContinue",
		"Symbols",
	}

	defines {
		"__STDC_LIMIT_MACROS",
		"__STDC_FORMAT_MACROS",
		"__STDC_CONSTANT_MACROS",
	}

	configuration "Debug"
		targetsuffix "Debug"

	configuration "Release"
		flags {
			"OptimizeSpeed",
		}
		targetsuffix "Release"

	configuration { "vs*" }
		includedirs { bxDir .. "include/compat/msvc" }
		defines {
			"WIN32",
			"_WIN32",
			"_HAS_EXCEPTIONS=0",
			"_HAS_ITERATOR_DEBUGGING=0",
			"_SCL_SECURE=0",
			"_SECURE_SCL=0",
			"_SCL_SECURE_NO_WARNINGS",
			"_CRT_SECURE_NO_WARNINGS",
			"_CRT_SECURE_NO_DEPRECATE",
		}
		buildoptions {
			"/Oy-", -- Suppresses creation of frame pointers on the call stack.
			"/Ob2", -- The Inline Function Expansion
		}

	configuration { "x32", "vs*" }
		targetdir (_buildDir .. "win32_" .. _ACTION .. "/bin")
		objdir (_buildDir .. "win32_" .. _ACTION .. "/obj")
		libdirs {
			_libDir .. "lib/win32_" .. _ACTION,
			"$(DXSDK_DIR)/lib/x86",
		}

	configuration { "x64", "vs*" }
		defines { "_WIN64" }
		targetdir (_buildDir .. "win64_" .. _ACTION .. "/bin")
		objdir (_buildDir .. "win64_" .. _ACTION .. "/obj")
		libdirs {
			_libDir .. "lib/win64_" .. _ACTION,
			"$(DXSDK_DIR)/lib/x64",
		}

	configuration { "mingw" }
		defines { "WIN32" }
		includedirs { bxDir .. "include/compat/mingw" }
		buildoptions {
			"-std=c++0x",
			"-U__STRICT_ANSI__",
			"-Wunused-value",
			"-fdata-sections",
			"-ffunction-sections",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration { "x32", "mingw" }
		targetdir (_buildDir .. "win32_mingw" .. "/bin")
		objdir (_buildDir .. "win32_mingw" .. "/obj")
		libdirs {
			_libDir .. "lib/win32_mingw",
			"$(DXSDK_DIR)/lib/x86",
		}
		buildoptions { "-m32" }

	configuration { "x64", "mingw" }
		targetdir (_buildDir .. "win64_mingw" .. "/bin")
		objdir (_buildDir .. "win64_mingw" .. "/obj")
		libdirs {
			_libDir .. "lib/win64_mingw",
			"$(DXSDK_DIR)/lib/x64",
		}
		buildoptions { "-m64" }

	configuration { "linux" }
		buildoptions {
			"-std=c++0x",
			"-U__STRICT_ANSI__",
			"-Wunused-value",
			"-mfpmath=sse", -- force SSE to get 32-bit and 64-bit builds deterministic.
			"-msse2",
		}
		links {
			"rt",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration { "linux", "x32" }
		targetdir (_buildDir .. "linux32_gcc" .. "/bin")
		objdir (_buildDir .. "linux32_gcc" .. "/obj")
		libdirs { _libDir .. "lib/linux32_gcc" }
		buildoptions {
			"-m32",
		}

	configuration { "linux", "x64" }
		targetdir (_buildDir .. "linux64_gcc" .. "/bin")
		objdir (_buildDir .. "linux64_gcc" .. "/obj")
		libdirs { _libDir .. "lib/linux64_gcc" }
		buildoptions {
			"-m64",
		}

	configuration { "emscripten" }
		targetdir (_buildDir .. "emscripten" .. "/bin")
		objdir (_buildDir .. "emscripten" .. "/obj")
		libdirs { _libDir .. "lib/emscripten" }
		includedirs { "$(EMSCRIPTEN)/system/include" }
		buildoptions {
			"-pthread",
		}

	configuration { "nacl" }
		defines { "_BSD_SOURCE=1", "_POSIX_C_SOURCE=199506", "_XOPEN_SOURCE=600" }
		includedirs { bxDir .. "include/compat/nacl" }
		buildoptions {
			"-std=c++0x",
			"-U__STRICT_ANSI__",
			"-pthread",
			"-fno-stack-protector",
			"-fdiagnostics-show-option",
			"-Wunused-value",
			"-fdata-sections",
			"-ffunction-sections",
			"-mfpmath=sse", -- force SSE to get 32-bit and 64-bit builds deterministic.
			"-msse2",
	--		"-fmerge-all-constants",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration { "x32", "nacl" }
		targetdir (_buildDir .. "nacl-x86" .. "/bin")
		objdir (_buildDir .. "nacl-x86" .. "/obj")
		libdirs { _libDir .. "lib/nacl-x86" }
		linkoptions { "-melf32_nacl" }

	configuration { "x64", "nacl" }
		targetdir (_buildDir .. "nacl-x64" .. "/bin")
		objdir (_buildDir .. "nacl-x64" .. "/obj")
		libdirs { _libDir .. "lib/nacl-x64" }
		linkoptions { "-melf64_nacl" }

	configuration { "pnacl" }
		defines { "_BSD_SOURCE=1", "_POSIX_C_SOURCE=199506", "_XOPEN_SOURCE=600", "__native_client__", "__LITTLE_ENDIAN__" }
		includedirs { bxDir .. "include/compat/nacl" }
		buildoptions {
			"-std=c++0x",
			"-U__STRICT_ANSI__",
			"-fno-stack-protector",
			"-fdiagnostics-show-option",
			"-Wunused-value",
			"-fdata-sections",
			"-ffunction-sections",
		}
		targetdir (_buildDir .. "pnacl" .. "/bin")
		objdir (_buildDir .. "pnacl" .. "/obj")
		libdirs { _libDir .. "lib/pnacl" }
		includedirs { "$(PNACL)/sysroot/include" }

	configuration { "Xbox360" }
		targetdir (_buildDir .. "xbox360" .. "/bin")
		objdir (_buildDir .. "xbox360" .. "/obj")
		includedirs { bxDir .. "include/compat/msvc" }
		libdirs { _libDir .. "lib/xbox360" }
		defines {
			"NOMINMAX",
			"_XBOX",
		}

	configuration {} -- reset configuration
end
