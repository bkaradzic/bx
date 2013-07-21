--
-- Copyright 2010-2013 Branimir Karadzic. All rights reserved.
-- License: http://www.opensource.org/licenses/BSD-2-Clause
--

local bxDir = (path.getabsolute("..") .. "/")

function toolchain(_buildDir, _libDir)

	newoption {
		trigger = "gcc",
		value = "GCC",
		description = "Choose GCC flavor",
		allowed = {
			{ "android-arm", "Android - ARM" },
--			{ "emscripten-experimental", "Emscripten" },
			{ "linux", "Linux" },
			{ "mingw", "MinGW" },
			{ "nacl", "Native Client" },
			{ "nacl-arm", "Native Client - ARM" },
			{ "pnacl", "Native Client - PNaCl" },
			{ "osx", "OSX" },
			{ "ios-arm", "iOS - ARM" },
			{ "ios-simulator", "iOS - Simulator" },
			{ "qnx-arm", "QNX/Blackberry - ARM" },
		}
	}

	-- Avoid error when invoking premake4 --help.
	if (_ACTION == nil) then return end

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

		if "android-arm" == _OPTIONS["gcc"] then

			if not os.getenv("ANDROID_NDK_ARM") or not os.getenv("ANDROID_NDK_ROOT") then
				print("Set ANDROID_NDK_ARM and ANDROID_NDK_ROOT envrionment variables.")
			end

			premake.gcc.cc = "$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-gcc"
			premake.gcc.cxx = "$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-g++"
			premake.gcc.ar = "$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-android-arm")
		end

		if "emscripten-experimental" == _OPTIONS["gcc"] then

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

			if not os.getenv("NACL_SDK_ROOT") then
				print("Set NACL_SDK_ROOT enviroment variables.")
			end

			premake.gcc.cc = "$(NACL_SDK_ROOT)/toolchain/win_x86_newlib/bin/x86_64-nacl-gcc"
			premake.gcc.cxx = "$(NACL_SDK_ROOT)/toolchain/win_x86_newlib/bin/x86_64-nacl-g++"
			premake.gcc.ar = "$(NACL_SDK_ROOT)/toolchain/win_x86_newlib/bin/x86_64-nacl-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-nacl")
		end

		if "nacl-arm" == _OPTIONS["gcc"] then

			if not os.getenv("NACL_SDK_ROOT") then
				print("Set NACL_SDK_ROOT enviroment variables.")
			end

			premake.gcc.cc = "$(NACL_SDK_ROOT)/toolchain/win_arm_newlib/bin/arm-nacl-gcc"
			premake.gcc.cxx = "$(NACL_SDK_ROOT)/toolchain/win_arm_newlib/bin/arm-nacl-g++"
			premake.gcc.ar = "$(NACL_SDK_ROOT)/toolchain/win_arm_newlib/bin/arm-nacl-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-nacl-arm")
		end

		if "pnacl" == _OPTIONS["gcc"] then

			if not os.getenv("NACL_SDK_ROOT") then
				print("Set NACL_SDK_ROOT enviroment variables.")
			end

			premake.gcc.cc = "$(NACL_SDK_ROOT)/toolchain/win_x86_pnacl/newlib/bin/pnacl-clang"
			premake.gcc.cxx = "$(NACL_SDK_ROOT)/toolchain/win_x86_pnacl/newlib/bin/pnacl-clang++"
			premake.gcc.ar = "$(NACL_SDK_ROOT)/toolchain/win_x86_pnacl/newlib/bin/pnacl-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-pnacl")
		end

		if "osx" == _OPTIONS["gcc"] then
			location (_buildDir .. "projects/" .. _ACTION .. "-osx")
		end

		if "ios-arm" == _OPTIONS["gcc"] then
			premake.gcc.cc = "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/arm-apple-darwin10-llvm-gcc-4.2"
			premake.gcc.cxx = "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/arm-apple-darwin10-llvm-g++-4.2"
			premake.gcc.ar = "ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-ios-arm")
		end

		if "ios-simulator" == _OPTIONS["gcc"] then
			premake.gcc.cc = "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/i686-apple-darwin11-llvm-gcc-4.2"
			premake.gcc.cxx = "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin/i686-apple-darwin11-llvm-g++-4.2"
			premake.gcc.ar = "ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-ios-simulator")
		end

		if "qnx-arm" == _OPTIONS["gcc"] then

			if not os.getenv("QNX_HOST") then
				print("Set QNX_HOST enviroment variables.")
			end

			premake.gcc.cc = "$(QNX_HOST)/usr/bin/arm-unknown-nto-qnx8.0.0eabi-gcc"
			premake.gcc.cxx = "$(QNX_HOST)/usr/bin/arm-unknown-nto-qnx8.0.0eabi-g++"
			premake.gcc.ar = "$(QNX_HOST)/usr/bin/arm-unknown-nto-qnx8.0.0eabi-ar"
			location (_buildDir .. "projects/" .. _ACTION .. "-qnx-arm")
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
			"$(GLES_X64_DIR)",
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

	configuration { "android-arm" }
		targetdir (_buildDir .. "android-arm" .. "/bin")
		objdir (_buildDir .. "android-arm" .. "/obj")
		flags {
			"NoImportLib",
		}
		libdirs {
			_libDir .. "lib/android-arm",
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.7/libs/armeabi-v7a",
		}
		includedirs {
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.7/include",
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.7/libs/armeabi-v7a/include",
			"$(ANDROID_NDK_ROOT)/sources/android/native_app_glue",
		}
		linkoptions {
			"--sysroot=$(ANDROID_NDK_ROOT)/platforms/android-14/arch-arm",
			"-nostdlib",
			"$(ANDROID_NDK_ROOT)/platforms/android-14/arch-arm/usr/lib/crtbegin_so.o",
			"$(ANDROID_NDK_ROOT)/platforms/android-14/arch-arm/usr/lib/crtend_so.o",
			"-march=armv7-a",
--			"-Wl,-shared,-Bsymbolic",
--			"-Wl,--gc-sections",
			"-Wl,--fix-cortex-a8",
--			"-Wl,--no-undefined",
			"-static-libgcc",

			"-no-canonical-prefixes",
			"-Wl,--no-undefined",
			"-Wl,-z,noexecstack",
			"-Wl,-z,relro",
			"-Wl,-z,now",
		}
		links {
			"c",
			"m",
			"android",
			"log",
			"gnustl_static",
			"gcc",
		}
		buildoptions {
			"-std=c++0x",
			"-U__STRICT_ANSI__",
			"-Wno-psabi", -- note: the mangling of 'va_list' has changed in GCC 4.4.0
			"-fpic",
			"--sysroot=$(ANDROID_NDK_ROOT)/platforms/android-14/arch-arm",
			"-mthumb",
			"-march=armv7-a",
			"-mfloat-abi=softfp",
			"-mfpu=vfpv3-d16",
		}

	configuration { "emscripten-experimental" }
		targetdir (_buildDir .. "emscripten" .. "/bin")
		objdir (_buildDir .. "emscripten" .. "/obj")
		libdirs { _libDir .. "lib/emscripten" }
		includedirs { "$(EMSCRIPTEN)/system/include" }
		buildoptions {
			"-pthread",
		}

	configuration { "nacl or nacl-arm or pnacl" }
		includedirs { 
			"$(NACL_SDK_ROOT)/include",
			bxDir .. "include/compat/nacl",
		}

	configuration { "nacl" }
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

	configuration { "nacl-arm" }
		buildoptions {
			"-std=c++0x",
			"-U__STRICT_ANSI__",
			"-fno-stack-protector",
			"-fdiagnostics-show-option",
			"-Wunused-value",
			"-Wno-psabi", -- note: the mangling of 'va_list' has changed in GCC 4.4.0
			"-fdata-sections",
			"-ffunction-sections",
		}
		targetdir (_buildDir .. "nacl-arm" .. "/bin")
		objdir (_buildDir .. "nacl-arm" .. "/obj")
		libdirs { _libDir .. "lib/nacl-arm" }

	configuration { "pnacl" }
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

	configuration { "osx", "x32" }
		targetdir (_buildDir .. "osx32_gcc" .. "/bin")
		objdir (_buildDir .. "osx32_gcc" .. "/obj")
		libdirs { _libDir .. "lib/osx32_gcc" }
		buildoptions {
			"-m32",
		}

	configuration { "osx", "x64" }
		targetdir (_buildDir .. "osx64_gcc" .. "/bin")
		objdir (_buildDir .. "osx64_gcc" .. "/obj")
		libdirs { _libDir .. "lib/osx64_gcc" }
		buildoptions {
			"-m64",
		}

	configuration { "osx" }
		buildoptions {
			"-U__STRICT_ANSI__",
			"-Wfatal-errors",
			"-Wunused-value",
			"-msse2",
		}
		includedirs { bxDir .. "include/compat/osx" }

	configuration { "ios-arm" }
		targetdir (_buildDir .. "ios-arm" .. "/bin")
		objdir (_buildDir .. "ios-arm" .. "/obj")
		libdirs { _libDir .. "lib/ios-arm" }
		linkoptions {
			"--sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk",
			"-march=armv7-a",
			"-L/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk/usr/lib/system",
			"-F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk/System/Library/Frameworks",
			"-F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk/System/Library/PrivateFrameworks",
		}
		buildoptions {
			"--sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk",
			"-mthumb",
			"-march=armv7-a",
			"-mfloat-abi=softfp",
			"-mfpu=neon",
			"-U__STRICT_ANSI__",
			"-Wfatal-errors",
			"-Wunused-value",
		}
		includedirs { bxDir .. "include/compat/osx" }

	configuration { "ios-simulator" }
		targetdir (_buildDir .. "ios-simulator" .. "/bin")
		objdir (_buildDir .. "ios-simulator" .. "/obj")
		libdirs { _libDir .. "lib/ios-simulator" }
		linkoptions {
			"--sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.1.sdk",
			"-L/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.1.sdk/usr/lib/system",
			"-F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.1.sdk/System/Library/Frameworks",
			"-F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.1.sdk/System/Library/PrivateFrameworks",
		}
		buildoptions {
			"--sysroot=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.1.sdk",
			"-miphoneos-version-min=3.0.0",
			"-fobjc-abi-version=2",
			"-fobjc-legacy-dispatch",
			"-U__STRICT_ANSI__",
			"-Wfatal-errors",
			"-Wunused-value",
		}
		includedirs { bxDir .. "include/compat/osx" }

	configuration { "qnx-arm" }
		targetdir (_buildDir .. "qnx-arm" .. "/bin")
		objdir (_buildDir .. "qnx-arm" .. "/obj")
		libdirs { _libDir .. "lib/qnx-arm" }
--		includedirs { bxDir .. "include/compat/qnx" }
		buildoptions {
			"-std=c++0x",
			"-U__STRICT_ANSI__",
			"-Wno-psabi", -- note: the mangling of 'va_list' has changed in GCC 4.4.0
		}

	configuration {} -- reset configuration
end

function strip()

	configuration { "android*", "Release" }
		postbuildcommands {
			"@echo Stripping symbols.",
			"@$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-strip -s \"$(TARGET)\""
		}

	configuration { "linux", "Release" }
		postbuildcommands {
			"@echo Stripping symbols.",
			"@strip -s \"$(TARGET)\""
		}

	configuration { "mingw", "Release" }
		postbuildcommands {
			"@echo Stripping symbols.",
			"@$(MINGW)/bin/strip -s \"$(TARGET)\""
		}

	configuration { "nacl", "Release" }
		postbuildcommands {
			"@echo Stripping symbols.",
			"@$(NACL_SDK_ROOT)/toolchain/win_x86_newlib/bin/x86_64-nacl-strip -s \"$(TARGET)\""
		}

	configuration { "nacl-arm", "Release" }
		postbuildcommands {
			"@echo Stripping symbols.",
			"@$(NACL_SDK_ROOT)/toolchain/win_arm_newlib/bin/arm-nacl-strip -s \"$(TARGET)\""
		}

	configuration { "pnacl", "Release" }
		postbuildcommands {
			"@echo Stripping symbols.",
			"@$(NACL_SDK_ROOT)/toolchain/win_x86_pnacl/newlib/bin/pnacl-strip -s \"$(TARGET)\""
		}

	configuration {} -- reset configuration
end
