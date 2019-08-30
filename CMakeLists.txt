# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008" .
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 2.4)

# project name and language
project(xword CXX)

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
    # enable Win32 resource
    enable_language(RC)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # using Clang
        if (uppercase_CMAKE_BUILD_TYPE MATCHES "^RELEASE$")
            set(CMAKE_C_FLAGS "-static -s -O9 -mtune=skylake")
            set(CMAKE_CXX_FLAGS "-static -s -O9 -mtune=skylake")
        else()
            set(CMAKE_C_FLAGS "-static -g")
            set(CMAKE_CXX_FLAGS "-static -g")
        endif()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # using GCC
        if (uppercase_CMAKE_BUILD_TYPE MATCHES "^RELEASE$")
            set(CMAKE_C_FLAGS "-static -s -O9 -mtune=skylake")
            set(CMAKE_CXX_FLAGS "-static -s -O9 -mtune=skylake")
        else()
            set(CMAKE_C_FLAGS "-static -g")
            set(CMAKE_CXX_FLAGS "-static -g")
        endif()
    elseif (MSVC)
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

include_directories(.)

add_definitions(-DMZC_NO_SHAREWARE -DUNICODE -D_UNICODE)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BITS 64)
else()
    set(BITS 32)
endif()

if (MSVC)
    add_definitions(-DMSVC)
endif()

# xword.exe
add_executable(xword${BITS} WIN32
    GUI.cpp
    Input.cpp
    Marks.cpp
    PointSizeRect.cpp
    ScrollView.cpp
    UndoBuffer.cpp
    Utils.cpp
    WordData.cpp
    xword.cpp
    xword_res.rc)
target_link_libraries(xword${BITS} comctl32 imm32 shell32 ole32 uuid shlwapi)

##############################################################################
