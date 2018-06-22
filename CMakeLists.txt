set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -fno-exceptions -fno-rtti")

add_library (bx STATIC 
			src/allocator.cpp
			src/bx.cpp
			src/commandline.cpp
			src/crtnone.cpp
			src/debug.cpp
			src/dtoa.cpp
			src/easing.cpp
			src/file.cpp
			src/filepath.cpp
			src/hash.cpp
			src/math.cpp
			src/mutex.cpp
			src/os.cpp
			src/process.cpp
			src/semaphore.cpp
			src/settings.cpp
			src/sort.cpp
			src/string.cpp
			src/thread.cpp
			src/timer.cpp
			src/url.cpp)

target_include_directories(bx PRIVATE
                           ./include
						   ./3rdparty)