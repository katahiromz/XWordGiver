# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008" .
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 2.8.12)

# enable testing
enable_testing()

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# project name and languages
project(XWordGiver CXX RC)

# check build type
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# make uppercase string of build type
string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)

# check build type again
if (NOT uppercase_CMAKE_BUILD_TYPE MATCHES "^(DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL)$")
    message(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

if (WIN32)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # using Clang
        if (uppercase_CMAKE_BUILD_TYPE MATCHES "^RELEASE$")
            set(CMAKE_C_FLAGS "-static -O3 -Wall -pedantic")
            set(CMAKE_CXX_FLAGS "-static -O3 -Wall -pedantic")
        else()
            set(CMAKE_C_FLAGS "-static -g -Wall -pedantic")
            set(CMAKE_CXX_FLAGS "-static -g -Wall -pedantic")
        endif()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # using GCC
        if (uppercase_CMAKE_BUILD_TYPE MATCHES "^RELEASE$")
            set(CMAKE_C_FLAGS "-static -s -O3 -mtune=skylake -Wall -pedantic")
            set(CMAKE_CXX_FLAGS "-static -s -O3 -mtune=skylake -Wall -pedantic")
        else()
            set(CMAKE_C_FLAGS "-static -g -Wall -pedantic")
            set(CMAKE_CXX_FLAGS "-static -g -Wall -pedantic")
        endif()
    elseif (MSVC AND NOT uppercase_CMAKE_BUILD_TYPE MATCHES "^DEBUG$")
        # replace "/MD" with "/MT" (building without runtime DLLs)
        set(CompilerFlags
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        foreach(CompilerFlags ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlags} "${${CompilerFlags}}")
        endforeach()
    endif()
endif()

##############################################################################

# C++17
set(CMAKE_CXX_STANDARD 17)

include_directories(. json/include SaveBitmapToFile)

add_definitions(-DMZC_NO_SHAREWARE -DUNICODE -D_UNICODE)

# SINGLE_THREAD_MODE option
option(SINGLE_THREAD_MODE "Single thread mode" OFF)
if(SINGLE_THREAD_MODE)
    add_definitions(-DSINGLE_THREAD_MODE)
endif()

# NO_RANDOM option
option(NO_RANDOM "Non-random mode" OFF)
if(NO_RANDOM)
    add_definitions(-DNO_RANDOM)
endif()

subdirs(SaveBitmapToFile dict_analyze)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BITS 64)
else()
    set(BITS 32)
endif()

if (MSVC)
    add_definitions(-DMSVC)
    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
endif()

# XWordGiver32.exe and XWordGiver64.exe
add_executable(XWordGiver${BITS} WIN32
    Dictionary.cpp
    GUI.cpp
    Input.cpp
    Marks.cpp
    MScrollView.cpp
    Utils.cpp
    XG_UndoBuffer.cpp
    XG_Window.cpp
    XWordGiver.cpp
    XWordGiver_res.rc)
target_link_libraries(XWordGiver${BITS} comctl32 imm32 shell32 ole32 uuid shlwapi SaveBitmapToFile)

##############################################################################
