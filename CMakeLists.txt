# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# project name, version, and languages
project(XWordGiver VERSION 5.4.4 LANGUAGES CXX RC)

# XWORDGIVER_VERSION_A and XWORDGIVER_VERSION_W
add_compile_definitions(XWORDGIVER_VERSION_A="${PROJECT_VERSION}")
add_compile_definitions(XWORDGIVER_VERSION_W=L"${PROJECT_VERSION}")

# option USE_CRTDLL
option(USE_CRTDLL "Use C Runtime DLL" OFF)
message(STATUS "USE_CRTDLL: ${USE_CRTDLL}")
if (NOT USE_CRTDLL)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # using Clang
        set(CMAKE_C_FLAGS "-static")
        set(CMAKE_CXX_FLAGS "-static")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # using GCC
        set(CMAKE_C_FLAGS "-static")
        set(CMAKE_CXX_FLAGS "-static")
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

# USE_GUI option
option(USE_GUI "Use GUI" ON)
message(STATUS "USE_GUI: ${USE_GUI}")

# USE_JSON option
option(USE_JSON "Use JSON" ON)
message(STATUS "USE_JSON: ${USE_JSON}")
if (USE_JSON)
    add_definitions(-DUSE_JSON)
    include_directories(json/include)
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BITS 64)
else()
    set(BITS 32)
endif()

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

##############################################################################

# json
add_subdirectory(json)

# libxwg.a
add_subdirectory(libxwg)

# testmain.exe
add_executable(testmain testmain.cpp XWordGiver_res.rc)
target_compile_definitions(testmain PRIVATE UNICODE _UNICODE)
target_include_directories(testmain PRIVATE . libxwg libxwg/SaveBitmapToFile)
target_link_libraries(testmain PRIVATE libxwg imm32 shlwapi gdiplus)

if (USE_GUI)
    # XWordGiver32.exe / XWordGiver64.exe
    add_executable(XWordGiver${BITS} WIN32 winmain.cpp XWordGiver_res.rc)
    target_compile_definitions(XWordGiver${BITS} PRIVATE UNICODE _UNICODE)
    target_include_directories(XWordGiver${BITS} PRIVATE . libxwg libxwg/SaveBitmapToFile)
    target_link_libraries(XWordGiver${BITS} PRIVATE libxwg comctl32 imm32 shlwapi gdiplus winmm ole32 userenv)
    if(MSVC)
        target_link_options(XWordGiver${BITS} PRIVATE /MANIFEST:NO)
    endif()
endif()

##############################################################################
