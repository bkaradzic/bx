--
-- Copyright 2010-2014 Branimir Karadzic. All rights reserved.
-- License: https://github.com/bkaradzic/bx#license-bsd-2-clause
--

local bxDir = (path.getabsolute("..") .. "/")
local naclToolchain = ""

function toolchain(_buildDir, _libDir)

	newoption {
		trigger = "gcc",
		value = "GCC",
		description = "Choose GCC flavor",
		allowed = {
			{ "android-arm",   "Android - ARM"          },
			{ "android-mips",  "Android - MIPS"         },
			{ "android-x86",   "Android - x86"          },
			{ "asmjs",         "Emscripten/asm.js"      },
			{ "freebsd",       "FreeBSD"                },
			{ "linux-gcc",     "Linux (GCC compiler)"   },
			{ "linux-clang",   "Linux (Clang compiler)" },
			{ "ios-arm",       "iOS - ARM"              },
			{ "ios-simulator", "iOS - Simulator"        },
			{ "mingw-gcc",     "MinGW"                  },
			{ "mingw-clang",   "MinGW (clang compiler)" },
			{ "nacl",          "Native Client"          },
			{ "nacl-arm",      "Native Client - ARM"    },
			{ "osx",           "OSX"                    },
			{ "pnacl",         "Native Client - PNaCl"  },
			{ "qnx-arm",       "QNX/Blackberry - ARM"   },
			{ "rpi",           "RaspberryPi"            },
		},
	}

	newoption {
		trigger = "vs",
		value = "toolset",
		description = "Choose VS toolset",
		allowed = {
			{ "vs2012-clang",  "Clang 3.6"         },
			{ "vs2013-clang",  "Clang 3.6"         },
			{ "winphone8",     "Windows Phone 8.0" },
			{ "winphone81",    "Windows Phone 8.1" },
		},
	}

	newoption {
		trigger = "with-android",
		value   = "#",
		description = "Set Android platform version (default: android-14).",
	}

	newoption {
		trigger = "with-ios",
		value   = "#",
		description = "Set iOS target version (default: 8.0).",
	}

	-- Avoid error when invoking genie --help.
	if (_ACTION == nil) then return false end

	location (_buildDir .. "projects/" .. _ACTION)

	if _ACTION == "clean" then
		os.rmdir(BUILD_DIR)
	end

	local androidPlatform = "android-14"
	if _OPTIONS["with-android"] then
		androidPlatform = "android-" .. _OPTIONS["with-android"]
	end

	local iosPlatform = ""
	if _OPTIONS["with-ios"] then
		iosPlatform = _OPTIONS["with-ios"]
	end

	if _ACTION == "gmake" then

		if nil == _OPTIONS["gcc"] then
			print("GCC flavor must be specified!")
			os.exit(1)
		end

		flags {
			"ExtraWarnings",
		}

		if "android-arm" == _OPTIONS["gcc"] then

			if not os.getenv("ANDROID_NDK_ARM") or not os.getenv("ANDROID_NDK_ROOT") then
				print("Set ANDROID_NDK_ARM and ANDROID_NDK_ROOT envrionment variables.")
			end

			premake.gcc.cc  = "$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-gcc"
			premake.gcc.cxx = "$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-g++"
			premake.gcc.ar  = "$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-android-arm")
		end

		if "android-mips" == _OPTIONS["gcc"] then

			if not os.getenv("ANDROID_NDK_MIPS") or not os.getenv("ANDROID_NDK_ROOT") then
				print("Set ANDROID_NDK_MIPS and ANDROID_NDK_ROOT envrionment variables.")
			end

			premake.gcc.cc  = "$(ANDROID_NDK_MIPS)/bin/mipsel-linux-android-gcc"
			premake.gcc.cxx = "$(ANDROID_NDK_MIPS)/bin/mipsel-linux-android-g++"
			premake.gcc.ar  = "$(ANDROID_NDK_MIPS)/bin/mipsel-linux-android-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-android-mips")
		end

		if "android-x86" == _OPTIONS["gcc"] then

			if not os.getenv("ANDROID_NDK_X86") or not os.getenv("ANDROID_NDK_ROOT") then
				print("Set ANDROID_NDK_X86 and ANDROID_NDK_ROOT envrionment variables.")
			end

			premake.gcc.cc  = "$(ANDROID_NDK_X86)/bin/i686-linux-android-gcc"
			premake.gcc.cxx = "$(ANDROID_NDK_X86)/bin/i686-linux-android-g++"
			premake.gcc.ar  = "$(ANDROID_NDK_X86)/bin/i686-linux-android-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-android-x86")
		end

		if "asmjs" == _OPTIONS["gcc"] then

			if not os.getenv("EMSCRIPTEN") then
				print("Set EMSCRIPTEN enviroment variables.")
			end

			premake.gcc.cc   = "$(EMSCRIPTEN)/emcc"
			premake.gcc.cxx  = "$(EMSCRIPTEN)/em++"
			premake.gcc.ar   = "$(EMSCRIPTEN)/emar"
			premake.gcc.llvm = true
			location (_buildDir .. "projects/" .. _ACTION .. "-asmjs")
		end

		if "freebsd" == _OPTIONS["gcc"] then
			location (_buildDir .. "projects/" .. _ACTION .. "-freebsd")
		end

		if "ios-arm" == _OPTIONS["gcc"] then
			premake.gcc.cc  = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang"
			premake.gcc.cxx = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"
			premake.gcc.ar  = "ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-ios-arm")
		end

		if "ios-simulator" == _OPTIONS["gcc"] then
			premake.gcc.cc  = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang"
			premake.gcc.cxx = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"
			premake.gcc.ar  = "ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-ios-simulator")
		end

		if "linux-gcc" == _OPTIONS["gcc"] then
			location (_buildDir .. "projects/" .. _ACTION .. "-linux")
		end

		if "linux-clang" == _OPTIONS["gcc"] then
			premake.gcc.cc  = "clang"
			premake.gcc.cxx = "clang++"
			premake.gcc.ar  = "ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-linux-clang")
		end

		if "mingw-gcc" == _OPTIONS["gcc"] then
			premake.gcc.cc  = "$(MINGW)/bin/x86_64-w64-mingw32-gcc"
			premake.gcc.cxx = "$(MINGW)/bin/x86_64-w64-mingw32-g++"
			premake.gcc.ar  = "$(MINGW)/bin/ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-mingw-gcc")
		end

		if "mingw-clang" == _OPTIONS["gcc"] then
			premake.gcc.cc   = "$(CLANG)/bin/clang"
			premake.gcc.cxx  = "$(CLANG)/bin/clang++"
			premake.gcc.ar   = "$(MINGW)/bin/ar"
--			premake.gcc.ar   = "$(CLANG)/bin/llvm-ar"
--			premake.gcc.llvm = true
			location (_buildDir .. "projects/" .. _ACTION .. "-mingw-clang")
		end

		if "nacl" == _OPTIONS["gcc"] then

			if not os.getenv("NACL_SDK_ROOT") then
				print("Set NACL_SDK_ROOT enviroment variables.")
			end

			naclToolchain = "$(NACL_SDK_ROOT)/toolchain/win_x86_newlib/bin/x86_64-nacl-"
			if os.is("macosx") then
				naclToolchain = "$(NACL_SDK_ROOT)/toolchain/mac_x86_newlib/bin/x86_64-nacl-"
			elseif os.is("linux") then
				naclToolchain = "$(NACL_SDK_ROOT)/toolchain/linux_x86_newlib/bin/x86_64-nacl-"
			end

			premake.gcc.cc  = naclToolchain .. "gcc"
			premake.gcc.cxx = naclToolchain .. "g++"
			premake.gcc.ar  = naclToolchain .. "ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-nacl")
		end

		if "nacl-arm" == _OPTIONS["gcc"] then

			if not os.getenv("NACL_SDK_ROOT") then
				print("Set NACL_SDK_ROOT enviroment variables.")
			end

			naclToolchain = "$(NACL_SDK_ROOT)/toolchain/win_arm_newlib/bin/arm-nacl-"
			if os.is("macosx") then
				naclToolchain = "$(NACL_SDK_ROOT)/toolchain/mac_arm_newlib/bin/arm-nacl-"
			elseif os.is("linux") then
				naclToolchain = "$(NACL_SDK_ROOT)/toolchain/linux_arm_newlib/bin/arm-nacl-"
			end

			premake.gcc.cc  = naclToolchain .. "gcc"
			premake.gcc.cxx = naclToolchain .. "g++"
			premake.gcc.ar  = naclToolchain .. "ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-nacl-arm")
		end

		if "osx" == _OPTIONS["gcc"] then
			if os.is("linux") then
				local osxToolchain = "x86_64-apple-darwin13-"
				premake.gcc.cc  = osxToolchain .. "clang"
				premake.gcc.cxx = osxToolchain .. "clang++"
				premake.gcc.ar  = osxToolchain .. "ar"
			end
			location (_buildDir .. "projects/" .. _ACTION .. "-osx")
		end

		if "pnacl" == _OPTIONS["gcc"] then

			if not os.getenv("NACL_SDK_ROOT") then
				print("Set NACL_SDK_ROOT enviroment variables.")
			end

			naclToolchain = "$(NACL_SDK_ROOT)/toolchain/win_pnacl/bin/pnacl-"
			if os.is("macosx") then
				naclToolchain = "$(NACL_SDK_ROOT)/toolchain/mac_pnacl/bin/pnacl-"
			elseif os.is("linux") then
				naclToolchain = "$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-"
			end

			premake.gcc.cc  = naclToolchain .. "clang"
			premake.gcc.cxx = naclToolchain .. "clang++"
			premake.gcc.ar  = naclToolchain .. "ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-pnacl")
		end

		if "qnx-arm" == _OPTIONS["gcc"] then

			if not os.getenv("QNX_HOST") then
				print("Set QNX_HOST enviroment variables.")
			end

			premake.gcc.cc  = "$(QNX_HOST)/usr/bin/arm-unknown-nto-qnx8.0.0eabi-gcc"
			premake.gcc.cxx = "$(QNX_HOST)/usr/bin/arm-unknown-nto-qnx8.0.0eabi-g++"
			premake.gcc.ar  = "$(QNX_HOST)/usr/bin/arm-unknown-nto-qnx8.0.0eabi-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-qnx-arm")
		end

		if "rpi" == _OPTIONS["gcc"] then
			location (_buildDir .. "projects/" .. _ACTION .. "-rpi")
		end
	elseif _ACTION == "vs2012" or _ACTION == "vs2013" or _ACTION == "vs2015" then

		if (_ACTION .. "-clang") == _OPTIONS["vs"] then
			premake.vstudio.toolset = ("LLVM-" .. _ACTION)
			location (_buildDir .. "projects/" .. _ACTION .. "-clang")
		end

		if "winphone8" == _OPTIONS["vs"] then
			premake.vstudio.toolset = "v110_wp80"
			location (_buildDir .. "projects/" .. _ACTION .. "-winphone8")
		end

		if "winphone81" == _OPTIONS["vs"] then
			premake.vstudio.toolset = "v120_wp81"
			platforms { "ARM" }
			location (_buildDir .. "projects/" .. _ACTION .. "-winphone81")
		end
	end

	flags {
		"StaticRuntime",
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

	configuration { "Debug" }
		targetsuffix "Debug"

	configuration { "Release" }
		flags {
			"OptimizeSpeed",
		}
		targetsuffix "Release"

	configuration { "vs*", "x86" }
		flags {
			"EnableSSE2",
		}

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
		linkoptions {
			"/ignore:4221", -- LNK4221: This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library
		}

	configuration { "vs2008" }
		includedirs { bxDir .. "include/compat/msvc/pre1600" }

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

	configuration { "ARM", "vs*" }
		targetdir (_buildDir .. "arm_" .. _ACTION .. "/bin")
		objdir (_buildDir .. "arm_" .. _ACTION .. "/obj")

	configuration { "vs*-clang" }
		buildoptions {
			"-Qunused-arguments",
		}

	configuration { "x32", "vs*-clang" }
		targetdir (_buildDir .. "win32_" .. _ACTION .. "-clang/bin")
		objdir (_buildDir .. "win32_" .. _ACTION .. "-clang/obj")

	configuration { "x64", "vs*-clang" }
		targetdir (_buildDir .. "win64_" .. _ACTION .. "-clang/bin")
		objdir (_buildDir .. "win64_" .. _ACTION .. "-clang/obj")

	configuration { "winphone8*" }
		removeflags {
			"StaticRuntime",
			"NoExceptions",
		}

	configuration { "mingw-*" }
		defines { "WIN32" }
		includedirs { bxDir .. "include/compat/mingw" }
		buildoptions {
			"-std=c++11",
			"-Wunused-value",
			"-fdata-sections",
			"-ffunction-sections",
			"-msse2",
			"-Wunused-value",
			"-Wundef",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration { "x32", "mingw-gcc" }
		targetdir (_buildDir .. "win32_mingw-gcc" .. "/bin")
		objdir (_buildDir .. "win32_mingw-gcc" .. "/obj")
		libdirs {
			_libDir .. "lib/win32_mingw-gcc",
			"$(DXSDK_DIR)/lib/x86",
		}
		buildoptions { "-m32" }

	configuration { "x64", "mingw-gcc" }
		targetdir (_buildDir .. "win64_mingw-gcc" .. "/bin")
		objdir (_buildDir .. "win64_mingw-gcc" .. "/obj")
		libdirs {
			_libDir .. "lib/win64_mingw-gcc",
			"$(DXSDK_DIR)/lib/x64",
			"$(GLES_X64_DIR)",
		}
		buildoptions { "-m64" }

	configuration { "mingw-clang" }
		buildoptions {
			"-isystem$(MINGW)/lib/gcc/x86_64-w64-mingw32/4.8.1/include/c++",
			"-isystem$(MINGW)/lib/gcc/x86_64-w64-mingw32/4.8.1/include/c++/x86_64-w64-mingw32",
			"-isystem$(MINGW)/x86_64-w64-mingw32/include",
		}
		linkoptions {
			"-Qunused-arguments",
			"-Wno-error=unused-command-line-argument-hard-error-in-future",
		}

	configuration { "x32", "mingw-clang" }
		targetdir (_buildDir .. "win32_mingw-clang" .. "/bin")
		objdir (_buildDir .. "win32_mingw-clang" .. "/obj")
		libdirs {
			_libDir .. "lib/win32_mingw-clang",
			"$(DXSDK_DIR)/lib/x86",
		}
		buildoptions { "-m32" }

	configuration { "x64", "mingw-clang" }
		targetdir (_buildDir .. "win64_mingw-clang" .. "/bin")
		objdir (_buildDir .. "win64_mingw-clang" .. "/obj")
		libdirs {
			_libDir .. "lib/win64_mingw-clang",
			"$(DXSDK_DIR)/lib/x64",
			"$(GLES_X64_DIR)",
		}
		buildoptions { "-m64" }

	configuration { "linux-gcc and not linux-clang" }
		buildoptions {
			"-mfpmath=sse", -- force SSE to get 32-bit and 64-bit builds deterministic.
		}

	configuration { "linux-clang" }

	configuration { "linux-*" }
		buildoptions {
			"-std=c++0x",
			"-msse2",
			"-Wunused-value",
			"-Wundef",
		}
		links {
			"rt",
			"dl",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration { "linux-gcc", "x32" }
		targetdir (_buildDir .. "linux32_gcc" .. "/bin")
		objdir (_buildDir .. "linux32_gcc" .. "/obj")
		libdirs { _libDir .. "lib/linux32_gcc" }
		buildoptions {
			"-m32",
		}

	configuration { "linux-gcc", "x64" }
		targetdir (_buildDir .. "linux64_gcc" .. "/bin")
		objdir (_buildDir .. "linux64_gcc" .. "/obj")
		libdirs { _libDir .. "lib/linux64_gcc" }
		buildoptions {
			"-m64",
		}

	configuration { "linux-clang", "x32" }
		targetdir (_buildDir .. "linux32_clang" .. "/bin")
		objdir (_buildDir .. "linux32_clang" .. "/obj")
		libdirs { _libDir .. "lib/linux32_clang" }
		buildoptions {
			"-m32",
		}

	configuration { "linux-clang", "x64" }
		targetdir (_buildDir .. "linux64_clang" .. "/bin")
		objdir (_buildDir .. "linux64_clang" .. "/obj")
		libdirs { _libDir .. "lib/linux64_clang" }
		buildoptions {
			"-m64",
		}

	configuration { "android-*" }
		flags {
			"NoImportLib",
		}
		includedirs {
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/include",
			"$(ANDROID_NDK_ROOT)/sources/android/native_app_glue",
		}
		linkoptions {
			"-nostdlib",
			"-static-libgcc",
		}
		links {
			"c",
			"dl",
			"m",
			"android",
			"log",
			"gnustl_static",
			"gcc",
		}
		buildoptions {
			"-fPIC",
			"-std=c++0x",
			"-no-canonical-prefixes",
			"-Wa,--noexecstack",
			"-fstack-protector",
			"-ffunction-sections",
			"-Wno-psabi", -- note: the mangling of 'va_list' has changed in GCC 4.4.0
			"-Wunused-value",
			"-Wundef",
		}
		linkoptions {
			"-no-canonical-prefixes",
			"-Wl,--no-undefined",
			"-Wl,-z,noexecstack",
			"-Wl,-z,relro",
			"-Wl,-z,now",
		}

	configuration { "android-arm" }
		targetdir (_buildDir .. "android-arm" .. "/bin")
		objdir (_buildDir .. "android-arm" .. "/obj")
		libdirs {
			_libDir .. "lib/android-arm",
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi-v7a",
		}
		includedirs {
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi-v7a/include",
		}
		buildoptions {
			"--sysroot=$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-arm",
			"-mthumb",
			"-march=armv7-a",
			"-mfloat-abi=softfp",
			"-mfpu=neon",
			"-Wunused-value",
			"-Wundef",
		}
		linkoptions {
			"--sysroot=$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-arm",
			"$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-arm/usr/lib/crtbegin_so.o",
			"$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-arm/usr/lib/crtend_so.o",
			"-march=armv7-a",
			"-Wl,--fix-cortex-a8",
		}

	configuration { "android-mips" }
		targetdir (_buildDir .. "android-mips" .. "/bin")
		objdir (_buildDir .. "android-mips" .. "/obj")
		libdirs {
			_libDir .. "lib/android-mips",
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/mips",
		}
		includedirs {
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/mips/include",
		}
		buildoptions {
			"--sysroot=$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-mips",
			"-Wunused-value",
			"-Wundef",
		}
		linkoptions {
			"--sysroot=$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-mips",
			"$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-mips/usr/lib/crtbegin_so.o",
			"$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-mips/usr/lib/crtend_so.o",
		}

	configuration { "android-x86" }
		targetdir (_buildDir .. "android-x86" .. "/bin")
		objdir (_buildDir .. "android-x86" .. "/obj")
		libdirs {
			_libDir .. "lib/android-x86",
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/x86",
		}
		includedirs {
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/x86/include",
		}
		buildoptions {
			"--sysroot=$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-x86",
			"-march=i686",
			"-mtune=atom",
			"-mstackrealign",
			"-msse3",
			"-mfpmath=sse",
			"-Wunused-value",
			"-Wundef",
		}
		linkoptions {
			"--sysroot=$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-x86",
			"$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-x86/usr/lib/crtbegin_so.o",
			"$(ANDROID_NDK_ROOT)/platforms/" .. androidPlatform .. "/arch-x86/usr/lib/crtend_so.o",
		}

	configuration { "asmjs" }
		targetdir (_buildDir .. "asmjs" .. "/bin")
		objdir (_buildDir .. "asmjs" .. "/obj")
		libdirs { _libDir .. "lib/asmjs" }
		buildoptions {
			"-isystem$(EMSCRIPTEN)/system/include",
			"-isystem$(EMSCRIPTEN)/system/include/libc",
			"-Wunused-value",
			"-Wundef",
		}

	configuration { "freebsd" }
		targetdir (_buildDir .. "freebsd" .. "/bin")
		objdir (_buildDir .. "freebsd" .. "/obj")
		libdirs { _libDir .. "lib/freebsd" }
		includedirs {
			bxDir .. "include/compat/freebsd",
		}

	configuration { "nacl or nacl-arm or pnacl" }
		buildoptions {
			"-std=c++0x",
			"-U__STRICT_ANSI__", -- strcasecmp, setenv, unsetenv,...
			"-fno-stack-protector",
			"-fdiagnostics-show-option",
			"-fdata-sections",
			"-ffunction-sections",
			"-Wunused-value",
			"-Wundef",
		}
		includedirs {
			"$(NACL_SDK_ROOT)/include",
			bxDir .. "include/compat/nacl",
		}

	configuration { "nacl" }
		buildoptions {
			"-pthread",
			"-mfpmath=sse", -- force SSE to get 32-bit and 64-bit builds deterministic.
			"-msse2",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration { "x32", "nacl" }
		targetdir (_buildDir .. "nacl-x86" .. "/bin")
		objdir (_buildDir .. "nacl-x86" .. "/obj")
		libdirs { _libDir .. "lib/nacl-x86" }
		linkoptions { "-melf32_nacl" }

	configuration { "x32", "nacl", "Debug" }
		libdirs { "$(NACL_SDK_ROOT)/lib/newlib_x86_32/Debug" }

	configuration { "x32", "nacl", "Release" }
		libdirs { "$(NACL_SDK_ROOT)/lib/newlib_x86_32/Release" }

	configuration { "x64", "nacl" }
		targetdir (_buildDir .. "nacl-x64" .. "/bin")
		objdir (_buildDir .. "nacl-x64" .. "/obj")
		libdirs { _libDir .. "lib/nacl-x64" }
		linkoptions { "-melf64_nacl" }

	configuration { "x64", "nacl", "Debug" }
		libdirs { "$(NACL_SDK_ROOT)/lib/newlib_x86_64/Debug" }

	configuration { "x64", "nacl", "Release" }
		libdirs { "$(NACL_SDK_ROOT)/lib/newlib_x86_64/Release" }

	configuration { "nacl-arm" }
		buildoptions {
			"-Wno-psabi", -- note: the mangling of 'va_list' has changed in GCC 4.4.0
		}
		targetdir (_buildDir .. "nacl-arm" .. "/bin")
		objdir (_buildDir .. "nacl-arm" .. "/obj")
		libdirs { _libDir .. "lib/nacl-arm" }

	configuration { "nacl-arm", "Debug" }
		libdirs { "$(NACL_SDK_ROOT)/lib/newlib_arm/Debug" }

	configuration { "nacl-arm", "Release" }
		libdirs { "$(NACL_SDK_ROOT)/lib/newlib_arm/Release" }

	configuration { "pnacl" }
		targetdir (_buildDir .. "pnacl" .. "/bin")
		objdir (_buildDir .. "pnacl" .. "/obj")
		libdirs { _libDir .. "lib/pnacl" }

	configuration { "pnacl", "Debug" }
		libdirs { "$(NACL_SDK_ROOT)/lib/pnacl/Debug" }

	configuration { "pnacl", "Release" }
		libdirs { "$(NACL_SDK_ROOT)/lib/pnacl/Release" }

	configuration { "Xbox360" }
		targetdir (_buildDir .. "xbox360" .. "/bin")
		objdir (_buildDir .. "xbox360" .. "/obj")
		includedirs { bxDir .. "include/compat/msvc" }
		libdirs { _libDir .. "lib/xbox360" }
		defines {
			"NOMINMAX",
			"_XBOX",
		}

	configuration { "osx", "x32" }
		targetdir (_buildDir .. "osx32_clang" .. "/bin")
		objdir (_buildDir .. "osx32_clang" .. "/obj")
		libdirs { _libDir .. "lib/osx32_clang" }
		buildoptions {
			"-m32",
		}

	configuration { "osx", "x64" }
		targetdir (_buildDir .. "osx64_clang" .. "/bin")
		objdir (_buildDir .. "osx64_clang" .. "/obj")
		libdirs { _libDir .. "lib/osx64_clang" }
		buildoptions {
			"-m64",
		}

	configuration { "osx" }
		buildoptions {
			"-Wfatal-errors",
			"-msse2",
			"-Wunused-value",
			"-Wundef",
		}
		includedirs { bxDir .. "include/compat/osx" }

	configuration { "ios-*" }
		linkoptions {
			"-lc++",
		}
		buildoptions {
			"-Wfatal-errors",
			"-Wunused-value",
			"-Wundef",
		}
		includedirs { bxDir .. "include/compat/ios" }

	configuration { "ios-arm" }
		targetdir (_buildDir .. "ios-arm" .. "/bin")
		objdir (_buildDir .. "ios-arm" .. "/obj")
		libdirs { _libDir .. "lib/ios-arm" }
		linkoptions {
			"-miphoneos-version-min=7.0",
			"-arch armv7",
			"--sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS" ..iosPlatform .. ".sdk",
			"-L/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS" ..iosPlatform .. ".sdk/usr/lib/system",
			"-F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS" ..iosPlatform .. ".sdk/System/Library/Frameworks",
			"-F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS" ..iosPlatform .. ".sdk/System/Library/PrivateFrameworks",
		}
		buildoptions {
			"-miphoneos-version-min=7.0",
			"-arch armv7",
			"--sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS" ..iosPlatform .. ".sdk",
		}

	configuration { "ios-simulator" }
		targetdir (_buildDir .. "ios-simulator" .. "/bin")
		objdir (_buildDir .. "ios-simulator" .. "/obj")
		libdirs { _libDir .. "lib/ios-simulator" }
		linkoptions {
			"-mios-simulator-version-min=7.0",
			"-arch i386",
			"--sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator" ..iosPlatform .. ".sdk",
			"-L/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator" ..iosPlatform .. ".sdk/usr/lib/system",
			"-F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator" ..iosPlatform .. ".sdk/System/Library/Frameworks",
			"-F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator" ..iosPlatform .. ".sdk/System/Library/PrivateFrameworks",
		}
		buildoptions {
			"-mios-simulator-version-min=7.0",
			"-arch i386",
			"--sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator" ..iosPlatform .. ".sdk",
		}

	configuration { "qnx-arm" }
		targetdir (_buildDir .. "qnx-arm" .. "/bin")
		objdir (_buildDir .. "qnx-arm" .. "/obj")
		libdirs { _libDir .. "lib/qnx-arm" }
--		includedirs { bxDir .. "include/compat/qnx" }
		buildoptions {
			"-std=c++0x",
			"-Wno-psabi", -- note: the mangling of 'va_list' has changed in GCC 4.4.0
			"-Wunused-value",
			"-Wundef",
		}

	configuration { "rpi" }
		targetdir (_buildDir .. "rpi" .. "/bin")
		objdir (_buildDir .. "rpi" .. "/obj")
		libdirs {
			_libDir .. "lib/rpi",
			"/opt/vc/lib",
		}
		defines {
			"__VCCOREVER__=0x04000000", -- There is no special prefedined compiler symbol to detect RaspberryPi, faking it.
			"__STDC_VERSION__=199901L",
		}
		buildoptions {
			"-std=c++0x",
			"-Wunused-value",
			"-Wundef",
		}
		includedirs {
			"/opt/vc/include",
			"/opt/vc/include/interface/vcos/pthreads",
			"/opt/vc/include/interface/vmcs_host/linux",
		}
		links {
			"rt",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration {} -- reset configuration

	return true
end

function strip()

	configuration { "android-arm", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) $(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-strip -s \"$(TARGET)\""
		}

	configuration { "android-mips", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) $(ANDROID_NDK_MIPS)/bin/mipsel-linux-android-strip -s \"$(TARGET)\""
		}

	configuration { "android-x86", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) $(ANDROID_NDK_X86)/bin/i686-linux-android-strip -s \"$(TARGET)\""
		}

	configuration { "linux-* or rpi", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) strip -s \"$(TARGET)\""
		}

	configuration { "mingw*", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) $(MINGW)/bin/strip -s \"$(TARGET)\""
		}

	configuration { "pnacl" }
		postbuildcommands {
			"$(SILENT) echo Running pnacl-finalize.",
			"$(SILENT) " .. naclToolchain .. "finalize \"$(TARGET)\""
		}

	configuration { "*nacl*", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) " .. naclToolchain .. "strip -s \"$(TARGET)\""
		}

	configuration { "asmjs" }
		postbuildcommands {
			"$(SILENT) echo Running asmjs finalize.",
			"$(SILENT) $(EMSCRIPTEN)/emcc -O2 -s TOTAL_MEMORY=268435456 \"$(TARGET)\" -o \"$(TARGET)\".html"
			-- ALLOW_MEMORY_GROWTH
		}

	configuration {} -- reset configuration
end
