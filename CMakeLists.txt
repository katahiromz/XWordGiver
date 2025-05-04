# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# enable testing
enable_testing()

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# project name and languages
project(XWordGiver CXX RC)

##############################################################################

option(USE_QSTRING "Use QString" ON)

if (USE_QSTRING)
    add_definitions(-DUSE_QSTRING)
endif()

##############################################################################

option(DETAILED_CHECK "Enable detailed check" OFF)

if (DETAILED_CHECK)
    message(STATUS "DETAILED_CHECK is ON")
    if (uppercase_CMAKE_BUILD_TYPE MATCHES "DEBUG")
        if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG -D_GLIBCXX_CONCEPT_CHECKS -D_GLIBCXX_DEBUG_PEDANTIC -Wall -Wextra -Wshadow -Wconversion -Wfloat-equal -ftrapv -fstack-protector-all -fno-omit-frame-pointer -Wno-missing-field-initializers -Wno-unused-parameter -Wno-cast-function-type")
        endif()
    endif()
endif()

##############################################################################

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # using Clang
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_C_FLAGS "-static -O3 -Wall -pedantic")
        set(CMAKE_CXX_FLAGS "-static -O3 -Wall -pedantic")
    else()
        set(CMAKE_C_FLAGS "-static -g -Wall -pedantic")
        set(CMAKE_CXX_FLAGS "-static -g -Wall -pedantic")
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_C_FLAGS "-s -static -O3 -Wall -pedantic")
        set(CMAKE_CXX_FLAGS "-s -static -O3 -Wall -pedantic")
    else()
        set(CMAKE_C_FLAGS "-static -g -Wall -pedantic")
        set(CMAKE_CXX_FLAGS "-static -g -Wall -pedantic")
    endif()
elseif (MSVC AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
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

##############################################################################

# C++17
set(CMAKE_CXX_STANDARD 17)

# UTF-8
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

include_directories(. json/include SaveBitmapToFile)

add_definitions(-DUNICODE -D_UNICODE)

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

# XWordGiver32.exe and XWordGiver64.exe
add_executable(XWordGiver${BITS} WIN32
    Dictionary.cpp
    GUI.cpp
    Input.cpp
    Marks.cpp
    MScrollView.cpp
    TaskbarProgress.cpp
    Utils.cpp
    XWordGiver.cpp
    XWordGiver_res.rc)
target_link_libraries(XWordGiver${BITS} winmm comctl32 imm32 shell32 ole32 uuid shlwapi SaveBitmapToFile)
if(MSVC)
    target_link_options(XWordGiver${BITS} PRIVATE /MANIFEST:NO)
endif()

##############################################################################
