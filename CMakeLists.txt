cmake_minimum_required(VERSION 3.16)
project(equ_kloku LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

find_package(raylib 6.0 QUIET CONFIG)
if(TARGET raylib)
    set(EQU_KLOKU_RAYLIB_TARGET raylib)
elseif(TARGET raylib::raylib)
    set(EQU_KLOKU_RAYLIB_TARGET raylib::raylib)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(RAYLIB QUIET IMPORTED_TARGET raylib>=6.0)
    endif()
    if(TARGET PkgConfig::RAYLIB)
        set(EQU_KLOKU_RAYLIB_TARGET PkgConfig::RAYLIB)
    else()
        message(FATAL_ERROR "raylib 6.0 is required (CMake package or pkg-config module)")
    endif()
endif()

set(EQU_KLOKU_FONT "${CMAKE_CURRENT_SOURCE_DIR}/assets/SourceHanSansCN-Regular.otf")
set(EQU_KLOKU_FONT_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/assets/OFL.txt")
configure_file("${EQU_KLOKU_FONT}"
               "${CMAKE_CURRENT_BINARY_DIR}/assets/SourceHanSansCN-Regular.otf" COPYONLY)
configure_file("${EQU_KLOKU_FONT_LICENSE}"
               "${CMAKE_CURRENT_BINARY_DIR}/assets/OFL.txt" COPYONLY)

add_library(focus_core
    src/focus_session.cc
    src/progress_store.cc
)
target_include_directories(focus_core PUBLIC src)

add_executable(equ-kloku src/main.cc src/platform_paths.cc)
target_link_libraries(equ-kloku PRIVATE focus_core ${EQU_KLOKU_RAYLIB_TARGET})

if(MSVC)
    target_compile_options(focus_core PRIVATE /W4 /permissive-)
    target_compile_options(equ-kloku PRIVATE /W4 /permissive-)
else()
    target_compile_options(focus_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(equ-kloku PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(APPLE)
    set_target_properties(equ-kloku PROPERTIES MACOSX_BUNDLE TRUE)
    set_source_files_properties("${EQU_KLOKU_FONT}" "${EQU_KLOKU_FONT_LICENSE}"
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(equ-kloku PRIVATE "${EQU_KLOKU_FONT}" "${EQU_KLOKU_FONT_LICENSE}")
endif()

install(TARGETS equ-kloku RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                           BUNDLE DESTINATION Applications)
if(NOT APPLE)
    install(FILES "${EQU_KLOKU_FONT}" "${EQU_KLOKU_FONT_LICENSE}"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/equ-kloku")
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(focus-tests tests/focus_tests.cc)
    target_link_libraries(focus-tests PRIVATE focus_core)
    if(MSVC)
        target_compile_options(focus-tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(focus-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME focus-tests COMMAND focus-tests)
endif()
