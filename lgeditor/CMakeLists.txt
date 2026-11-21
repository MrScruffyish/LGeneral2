cmake_minimum_required(VERSION 3.16)

project(lgeditor VERSION 0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT DEFINED LGENDIR OR LGENDIR STREQUAL "")
    message(FATAL_ERROR "LGENDIR is required; pass -DLGENDIR=/path/to/lgeneral/data")
endif()
if(NOT IS_DIRECTORY "${LGENDIR}")
    message(FATAL_ERROR "LGENDIR does not exist: ${LGENDIR}")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2 SDL2_image SDL2_ttf)

find_library(MATH_LIBRARY m)
if(NOT MATH_LIBRARY)
    message(FATAL_ERROR "math library is needed")
endif()

add_compile_definitions(
    DATADIR="../data/"
    LGENDIR="${LGENDIR}"
    _GNU_SOURCE=1
)

add_executable(lgeditor
    src/mylist.cpp src/parser.cpp src/data.cpp src/widgets.cpp
    src/mapview.cpp src/gui.cpp src/main.cpp
    src/mapview.h src/widgets.h src/gui.h src/parser.h src/mylist.h src/data.h
)
target_link_libraries(lgeditor PRIVATE PkgConfig::SDL2 ${MATH_LIBRARY})
set_target_properties(lgeditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src
)
