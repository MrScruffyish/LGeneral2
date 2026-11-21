cmake_minimum_required(VERSION 3.16)

project(lgeneral VERSION 1.4.4 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

option(LGENERAL_TRACE "Build with deployment/turn trace logging" OFF)
option(LGENERAL_SOUND "Build with SDL_mixer audio support" ON)
option(LGENERAL_NLS "Build with gettext translations" ON)
option(LGENERAL_INSTALL "Use installed data paths" OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)

find_library(MATH_LIBRARY m)
if(NOT MATH_LIBRARY)
    message(FATAL_ERROR "math library is needed")
endif()

set(WITH_SOUND 0)
set(HAVE_LIBSDL_MIXER 0)
if(LGENERAL_SOUND)
    pkg_check_modules(SDL3_MIXER QUIET IMPORTED_TARGET sdl3-mixer)
    if(SDL3_MIXER_FOUND)
        set(WITH_SOUND 1)
        set(HAVE_LIBSDL_MIXER 1)
    else()
        message(STATUS "SDL3_mixer not found; audio will be disabled")
    endif()
endif()

set(ENABLE_NLS 0)
set(HAVE_GETTEXT 0)
set(HAVE_DCGETTEXT 0)
if(LGENERAL_NLS)
    find_program(MSGFMT msgfmt)
    if(NOT MSGFMT)
        message(FATAL_ERROR "msgfmt is needed for gettext translations")
    endif()
    include(CheckIncludeFile)
    check_include_file(libintl.h HAVE_LIBINTL_H)
    if(NOT HAVE_LIBINTL_H)
        message(FATAL_ERROR "libintl.h is needed for gettext translations")
    endif()
    set(ENABLE_NLS 1)
    include(CheckFunctionExists)
    check_function_exists(gettext HAVE_GETTEXT)
    check_function_exists(dcgettext HAVE_DCGETTEXT)
endif()

include(CheckSymbolExists)
set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(setenv stdlib.h HAVE_SETENV)
check_symbol_exists(strcasestr string.h HAVE_STRCASESTR)
unset(CMAKE_REQUIRED_DEFINITIONS)

if(LGENERAL_INSTALL)
    set(LGENERAL_LOCALEDIR "${CMAKE_INSTALL_PREFIX}/share/locale")
    set(LGENERAL_INSTALLDIR "${CMAKE_INSTALL_PREFIX}/lgeneral")
    set(LGENERAL_INSTALL_DEFINITION INSTALLDIR="${LGENERAL_INSTALLDIR}")
else()
    set(LGENERAL_LOCALEDIR "share/locale")
    set(LGENERAL_INSTALL_DEFINITION "")
endif()

set(PACKAGE "lgeneral")
set(PACKAGE_NAME "lgeneral")
set(PACKAGE_TARNAME "lgeneral")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(DATADIR "${CMAKE_INSTALL_PREFIX}/share")
set(HAVE_BROKEN_MKDIR 0)
set(LGENERAL_PATHS_GENERIC 1)
configure_file(cmake/config.h.cmake config.h @ONLY)

add_compile_definitions(HAVE_CONFIG_H=1 VERSION="${PROJECT_VERSION}")
if(LGENERAL_TRACE)
    add_compile_definitions(LG_TRACE=1)
endif()
if(WITH_SOUND)
    add_compile_definitions(WITH_SOUND=1)
endif()
if(LGENERAL_INSTALL)
    add_compile_definitions(${LGENERAL_INSTALL_DEFINITION})
endif()
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(LGENERAL_UTIL_SOURCES
    util/localize.cpp util/localize.h util/hash-string.h
    util/hashtable.cpp util/hashtable.h util/hashtable_private.h
    util/hashtable_itr.cpp util/hashtable_itr.h
    util/hashtable_utility.cpp util/hashtable_utility.h
    util/paths.cpp util/paths.h util/portability.cpp util/paths-linux.cpp
)

add_library(util STATIC ${LGENERAL_UTIL_SOURCES})
target_include_directories(util PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)
target_compile_definitions(util PUBLIC DATADIR="${DATADIR}" PREFIX="${PREFIX}")
set_target_properties(util PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/util
)

set(LGENERAL_GAME_SOURCES
    src/main.cpp
    src/config.cpp src/date.cpp
    src/file.cpp src/map.cpp src/nation.cpp
    src/player.cpp src/scenario.cpp src/lg-sdl.cpp src/misc.cpp src/unit.cpp
    src/audio.cpp src/parser.cpp src/list.cpp src/unit_lib.cpp
    src/terrain.cpp src/image.cpp src/slot.cpp src/gui.cpp src/purchase_dlg.cpp
    src/event.cpp src/windows.cpp src/action.cpp src/campaign.cpp
    src/strat_map.cpp src/ai.cpp src/engine.cpp src/ai_group.cpp src/ai_tools.cpp
)

add_executable(lgeneral ${LGENERAL_GAME_SOURCES})
target_include_directories(lgeneral PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)
target_link_libraries(lgeneral PRIVATE util PkgConfig::SDL3 ${MATH_LIBRARY})
if(WITH_SOUND)
    target_link_libraries(lgeneral PRIVATE PkgConfig::SDL3_MIXER)
endif()
set_target_properties(lgeneral PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src
)

set(LGENERAL_CONVERTER_SOURCES
    lgc-pg/main.cpp lgc-pg/units.cpp lgc-pg/shp.cpp lgc-pg/nations.cpp
    lgc-pg/terrain.cpp lgc-pg/maps.cpp lgc-pg/scenarios.cpp
    lgc-pg/list.cpp lgc-pg/misc.cpp lgc-pg/parser.cpp
)

add_executable(lgc-pg ${LGENERAL_CONVERTER_SOURCES})
target_include_directories(lgc-pg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lgc-pg
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)
target_link_libraries(lgc-pg PRIVATE util PkgConfig::SDL3 ${MATH_LIBRARY})
set_target_properties(lgc-pg PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lgc-pg
)

add_executable(shptool lgc-pg/shptool.cpp lgc-pg/misc.cpp lgc-pg/shp.cpp)
target_compile_definitions(shptool PRIVATE SHPTOOL=1)
target_include_directories(shptool PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lgc-pg
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)
target_link_libraries(shptool PRIVATE util PkgConfig::SDL3 ${MATH_LIBRARY})
set_target_properties(shptool PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lgc-pg
)

add_executable(lged
    lged/lged.cpp lged/misc.cpp lged/parser.cpp lged/list.cpp
)
target_compile_definitions(lged PRIVATE
    CMDLINE_ONLY=1
    TOP_SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_include_directories(lged PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lged
    ${CMAKE_CURRENT_SOURCE_DIR}/lgeneral-redit/src
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)
target_link_libraries(lged PRIVATE util ${MATH_LIBRARY})
set_target_properties(lged PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lged
)

add_executable(ltrextract
    tools/ltrextract/ltrextract.cpp tools/ltrextract/util.cpp
    tools/ltrextract/util.h tools/ltrextract/terrain.cpp
    tools/ltrextract/terrain.h tools/ltrextract/map.cpp
    tools/ltrextract/map.h tools/ltrextract/nation.cpp
    tools/ltrextract/nation.h tools/ltrextract/unit_lib.cpp
    tools/ltrextract/unit_lib.h tools/ltrextract/scenario.cpp
    tools/ltrextract/scenario.h tools/ltrextract/campaign.cpp
    tools/ltrextract/campaign.h src/parser.cpp src/list.cpp
)
target_compile_definitions(ltrextract PRIVATE LTREXTRACT=1)
target_include_directories(ltrextract PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/ltrextract
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)
target_link_libraries(ltrextract PRIVATE util ${MATH_LIBRARY})
set_target_properties(ltrextract PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tools/ltrextract
)

if(LGENERAL_NLS)
    set(LGENERAL_CATALOGS)
    foreach(domain IN ITEMS lgeneral pg)
        foreach(lang IN ITEMS en de)
            set(po_file ${CMAKE_CURRENT_SOURCE_DIR}/po/${domain}/${lang}.po)
            set(gmo_file ${CMAKE_CURRENT_BINARY_DIR}/po/${domain}/${lang}.gmo)
            add_custom_command(
                OUTPUT ${gmo_file}
                COMMAND ${CMAKE_COMMAND} -E make_directory
                        ${CMAKE_CURRENT_BINARY_DIR}/po/${domain}
                COMMAND ${MSGFMT} -c -o ${gmo_file} ${po_file}
                DEPENDS ${po_file}
                VERBATIM
            )
            list(APPEND LGENERAL_CATALOGS ${gmo_file})
        endforeach()
    endforeach()
    add_custom_target(lgeneral-translations ALL DEPENDS ${LGENERAL_CATALOGS})
endif()
