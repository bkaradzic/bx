#
# Copyright 2011-2018 Branimir Karadzic. All rights reserved.
# License: https://github.com/bkaradzic/bx#license-bsd-2-clause
#

GENIE=../bx/tools/bin/$(OS)/genie

all:
	$(GENIE) vs2012
	$(GENIE) vs2013
	$(GENIE) --gcc=android-arm gmake
	$(GENIE) --gcc=android-mips gmake
	$(GENIE) --gcc=android-x86 gmake
	$(GENIE) --gcc=mingw-gcc gmake
	$(GENIE) --gcc=linux-gcc gmake
	$(GENIE) --gcc=osx gmake
	$(GENIE) --gcc=ios-arm gmake
	$(GENIE) --gcc=ios-simulator gmake
	$(GENIE) --gcc=ios-simulator64 gmake
	$(GENIE) xcode8

.build/projects/gmake-android-arm:
	$(GENIE) --gcc=android-arm gmake
android-arm-debug: .build/projects/gmake-android-arm
	make -R -C .build/projects/gmake-android-arm config=debug
android-arm-release: .build/projects/gmake-android-arm
	make -R -C .build/projects/gmake-android-arm config=release
android-arm: android-arm-debug android-arm-release

.build/projects/gmake-android-mips:
	$(GENIE) --gcc=android-mips gmake
android-mips-debug: .build/projects/gmake-android-mips
	make -R -C .build/projects/gmake-android-mips config=debug
android-mips-release: .build/projects/gmake-android-mips
	make -R -C .build/projects/gmake-android-mips config=release
android-mips: android-mips-debug android-mips-release

.build/projects/gmake-android-x86:
	$(GENIE) --gcc=android-x86 gmake
android-x86-debug: .build/projects/gmake-android-x86
	make -R -C .build/projects/gmake-android-x86 config=debug
android-x86-release: .build/projects/gmake-android-x86
	make -R -C .build/projects/gmake-android-x86 config=release
android-x86: android-x86-debug android-x86-release

.build/projects/gmake-linux:
	$(GENIE) --gcc=linux-gcc gmake
linux-debug64: .build/projects/gmake-linux
	make -R -C .build/projects/gmake-linux config=debug64
linux-release64: .build/projects/gmake-linux
	make -R -C .build/projects/gmake-linux config=release64
linux: linux-debug64 linux-release64

.build/projects/gmake-mingw-gcc:
	$(GENIE) --gcc=mingw-gcc gmake
mingw-gcc-debug32: .build/projects/gmake-mingw-gcc
	make -R -C .build/projects/gmake-mingw-gcc config=debug32
mingw-gcc-release32: .build/projects/gmake-mingw-gcc
	make -R -C .build/projects/gmake-mingw-gcc config=release32
mingw-gcc-debug64: .build/projects/gmake-mingw-gcc
	make -R -C .build/projects/gmake-mingw-gcc config=debug64
mingw-gcc-release64: .build/projects/gmake-mingw-gcc
	make -R -C .build/projects/gmake-mingw-gcc config=release64
mingw-gcc: mingw-gcc-debug32 mingw-gcc-release32 mingw-gcc-debug64 mingw-gcc-release64

.build/projects/gmake-mingw-clang:
	$(GENIE) --clang=mingw-clang gmake
mingw-clang-debug32: .build/projects/gmake-mingw-clang
	make -R -C .build/projects/gmake-mingw-clang config=debug32
mingw-clang-release32: .build/projects/gmake-mingw-clang
	make -R -C .build/projects/gmake-mingw-clang config=release32
mingw-clang-debug64: .build/projects/gmake-mingw-clang
	make -R -C .build/projects/gmake-mingw-clang config=debug64
mingw-clang-release64: .build/projects/gmake-mingw-clang
	make -R -C .build/projects/gmake-mingw-clang config=release64
mingw-clang: mingw-clang-debug32 mingw-clang-release32 mingw-clang-debug64 mingw-clang-release64

.build/projects/vs2012:
	$(GENIE) vs2012

.build/projects/vs2013:
	$(GENIE) vs2013

.build/projects/gmake-osx:
	$(GENIE) --gcc=osx gmake
osx-debug64: .build/projects/gmake-osx
	make -C .build/projects/gmake-osx config=debug64
osx-release64: .build/projects/gmake-osx
	make -C .build/projects/gmake-osx config=release64
osx: osx-debug64 osx-release64

.build/projects/gmake-ios-arm:
	$(GENIE) --gcc=ios-arm gmake
ios-arm-debug: .build/projects/gmake-ios-arm
	make -R -C .build/projects/gmake-ios-arm config=debug
ios-arm-release: .build/projects/gmake-ios-arm
	make -R -C .build/projects/gmake-ios-arm config=release
ios-arm: ios-arm-debug ios-arm-release

.build/projects/gmake-ios-simulator:
	$(GENIE) --gcc=ios-simulator gmake
ios-simulator-debug: .build/projects/gmake-ios-simulator
	make -R -C .build/projects/gmake-ios-simulator config=debug
ios-simulator-release: .build/projects/gmake-ios-simulator
	make -R -C .build/projects/gmake-ios-simulator config=release
ios-simulator: ios-simulator-debug ios-simulator-release

.build/projects/gmake-ios-simulator64:
	$(GENIE) --gcc=ios-simulator64 gmake
ios-simulator64-debug: .build/projects/gmake-ios-simulator64
	make -R -C .build/projects/gmake-ios-simulator64 config=debug
ios-simulator64-release: .build/projects/gmake-ios-simulator64
	make -R -C .build/projects/gmake-ios-simulator64 config=release
ios-simulator64: ios-simulator64-debug ios-simulator64-release

rebuild-shaders:
	make -R -C examples rebuild

analyze:
	cppcheck src/
	cppcheck examples/

docs:
	doxygen scripts/bgfx.doxygen
	markdown README.md > .build/docs/readme.html

clean:
	@echo Cleaning...
	-@rm -rf .build
	@mkdir .build

###

SILENT ?= @

UNAME := $(shell uname)
ifeq ($(UNAME),$(filter $(UNAME),Linux GNU Darwin))
ifeq ($(UNAME),$(filter $(UNAME),Darwin))
OS=darwin
BUILD_PROJECT_DIR=gmake-osx
BUILD_OUTPUT_DIR=osx64_clang
BUILD_TOOLS_CONFIG=release64
EXE=
else
OS=linux
BUILD_PROJECT_DIR=gmake-linux
BUILD_OUTPUT_DIR=linux64_gcc
BUILD_TOOLS_CONFIG=release64
EXE=
endif
else
OS=windows
BUILD_PROJECT_DIR=gmake-mingw-gcc
BUILD_OUTPUT_DIR=win32_mingw-gcc
BUILD_TOOLS_CONFIG=release64
EXE=.exe
endif

# bin2c
.build/osx64_clang/bin/bin2cRelease: .build/projects/gmake-osx
	$(SILENT) make -C .build/projects/gmake-osx bin2c config=$(BUILD_TOOLS_CONFIG)

tools/bin/darwin/bin2c: .build/osx64_clang/bin/bin2cRelease
	$(SILENT) cp $(<) $(@)

.build/linux64_gcc/bin/bin2cRelease: .build/projects/gmake-linux
	$(SILENT) make -C .build/projects/gmake-linux bin2c config=$(BUILD_TOOLS_CONFIG)

tools/bin/linux/bin2c: .build/linux64_gcc/bin/bin2cRelease
	$(SILENT) cp $(<) $(@)

.build/win64_mingw-gcc/bin/bin2cRelease.exe: .build/projects/gmake-mingw-gcc
	$(SILENT) make -C .build/projects/gmake-mingw-gcc bin2c config=$(BUILD_TOOLS_CONFIG)

tools/bin/windows/bin2c.exe: .build/win64_mingw-gcc/bin/bin2cRelease.exe
	$(SILENT) cp $(<) $(@)

bin2c: tools/bin/$(OS)/bin2c$(EXE)

# lemon
.build/osx64_clang/bin/lemonRelease: .build/projects/gmake-osx
	$(SILENT) make -C .build/projects/gmake-osx lemon config=$(BUILD_TOOLS_CONFIG)

tools/bin/darwin/lemon: .build/osx64_clang/bin/lemonRelease
	$(SILENT) cp $(<) $(@)

.build/linux64_gcc/bin/lemonRelease: .build/projects/gmake-linux
	$(SILENT) make -C .build/projects/gmake-linux lemon config=$(BUILD_TOOLS_CONFIG)

tools/bin/linux/lemon: .build/linux64_gcc/bin/lemonRelease
	$(SILENT) cp $(<) $(@)

.build/win64_mingw-gcc/bin/lemonRelease.exe: .build/projects/gmake-mingw-gcc
	$(SILENT) make -C .build/projects/gmake-mingw-gcc lemon config=$(BUILD_TOOLS_CONFIG)

tools/bin/windows/lemon.exe: .build/win64_mingw-gcc/bin/lemonRelease.exe
	$(SILENT) cp $(<) $(@)

tools/bin/$(OS)/lempar.c: tools/lemon/lempar.c
	$(SILENT) cp $(<) $(@)

lemon: tools/bin/$(OS)/lemon$(EXE) tools/bin/$(OS)/lempar.c

tools: bin2c lemon

dist: tools/bin/darwin/bin2c tools/bin/linux/bin2c tools/bin/windows/bin2c.exe

.build/$(BUILD_OUTPUT_DIR)/bin/bx.testRelease$(EXE): .build/projects/$(BUILD_PROJECT_DIR)
	$(SILENT) make -C .build/projects/$(BUILD_PROJECT_DIR) bx.test config=$(BUILD_TOOLS_CONFIG)

test: .build/$(BUILD_OUTPUT_DIR)/bin/bx.testRelease$(EXE)
