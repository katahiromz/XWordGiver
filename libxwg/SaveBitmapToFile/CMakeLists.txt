# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008"
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles"
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# enable testing
enable_testing()

# project name and language
project(SaveBitmapToFile C)

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

## set output directory (build/)
#set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
#set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

## enable Win32 resource
#enable_language(RC)

if (WIN32)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # using Clang
        set(CMAKE_C_FLAGS "-static")
        set(CMAKE_CXX_FLAGS "-static")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # using GCC
        set(CMAKE_C_FLAGS "-static -static-libgcc -static-libstdc++")
        set(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++")
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

option(BITMAP_TEST "Build bitmap test and do it" OFF)

##############################################################################

# SaveBitmapToFileW
add_library(SaveBitmapToFileW STATIC SaveBitmapToFile.c)
set_target_properties(SaveBitmapToFileW PROPERTIES COMPILE_FLAGS "-DUNICODE -D_UNICODE")

# SaveBitmapToFile
add_library(SaveBitmapToFile STATIC empty.c)
target_link_libraries(SaveBitmapToFile PRIVATE SaveBitmapToFileW)

# installation
install(TARGETS SaveBitmapToFile DESTINATION lib)
install(FILES SaveBitmapToFile.h DESTINATION include)

# tests
if (BITMAP_TEST)
    # SaveBitmapToFileTestW
    add_executable(SaveBitmapToFileTestW SaveBitmapToFileTest.c)
    target_link_libraries(SaveBitmapToFileTestW PRIVATE SaveBitmapToFile)
    set_target_properties(SaveBitmapToFileTestW PROPERTIES COMPILE_FLAGS "-DUNICODE -D_UNICODE")

    add_test(NAME SaveBitmapToFileTestW
             COMMAND $<TARGET_FILE:SaveBitmapToFileTestW>
             WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif(BITMAP_TEST)

##############################################################################
