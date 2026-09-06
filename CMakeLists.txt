# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# enable testing
enable_testing()

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# project name, version, and languages
project(XWordGiver VERSION 5.3.4 LANGUAGES CXX RC)

# add macro __XWORDGIVER__
add_definitions(-D__XWORDGIVER__)

##############################################################################

option(USE_QSTRING "Use QString" ON)

if (USE_QSTRING)
    add_definitions(-DUSE_QSTRING)
endif()

##############################################################################

option(DETAILED_CHECK "Enable detailed check" OFF)

if (DETAILED_CHECK)
    message(STATUS "DETAILED_CHECK is ON")
    if (CMAKE_BUILD_TYPE MATCHES "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_DEBUG -D_GLIBCXX_CONCEPT_CHECKS -D_GLIBCXX_DEBUG_PEDANTIC -Wall -Wextra -Wshadow -Wconversion -Wfloat-equal -ftrapv -fstack-protector-all -fno-omit-frame-pointer -Wno-missing-field-initializers -Wno-unused-parameter -Wno-cast-function-type")
        endif()
    endif()
endif()

##############################################################################

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Common warnings
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

    # Optimize + LTO + static linking on Release version
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_C_FLAGS_RELEASE   "-O3 -static")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -static")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-static")
    else()
        set(CMAKE_C_FLAGS_RELEASE   "-O3 -flto=auto -static")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto=auto -static")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto=auto -static")
    endif()

    # Debug version
    set(CMAKE_C_FLAGS_DEBUG   "-g")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
elseif (MSVC)
    # Multi-processor compilation
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

    # LTO on Release version only
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} /O2 /Oi /GL")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Oi /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

    # Run-time library
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

##############################################################################

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    AIHelper.cpp
    Dictionary.cpp
    GUI.cpp
    Input.cpp
    Marks.cpp
    MScrollView.cpp
    TaskbarProgress.cpp
    Utils.cpp
    XWordGiver.cpp
    XWordGiver_res.rc)

target_link_libraries(XWordGiver${BITS}
    winmm
    comctl32
    imm32
    shell32
    ole32
    uuid
    shlwapi
    SaveBitmapToFile)

if(MSVC)
    target_link_options(XWordGiver${BITS} PRIVATE /MANIFEST:NO)
endif()

##############################################################################
