# libxwg CMakeLists.txt

# Use GUI?
if (USE_GUI)
    file(GLOB GUI_SOURCES "gui/*.cpp")
endif()
file(GLOB LIBXWG_SOURCES "*.cpp")

add_subdirectory(SaveBitmapToFile)

add_library(libxwg STATIC ${LIBXWG_SOURCES} ${GUI_SOURCES})
set_target_properties(libxwg PROPERTIES PREFIX "")
target_compile_definitions(libxwg PRIVATE UNICODE _UNICODE)
target_include_directories(libxwg PRIVATE . .. SaveBitmapToFile)
target_link_libraries(libxwg SaveBitmapToFile winhttp)

if (MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    target_precompile_headers(libxwg PRIVATE xwg.h)
    if (USE_GUI)
        target_precompile_headers(libxwg PRIVATE gui/MWindowBase.h)
    endif()
endif()
