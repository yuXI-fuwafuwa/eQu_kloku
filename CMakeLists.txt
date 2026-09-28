cmake_minimum_required(VERSION 3.16)
project(equ_kloku LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)
include(GNUInstallDirs)

add_library(focus_core
    src/focus_session.cc
    src/progress_store.cc
)
target_include_directories(focus_core PUBLIC src)
target_compile_options(focus_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(equ-kloku src/main.cc)
target_link_libraries(equ-kloku PRIVATE focus_core PkgConfig::RAYLIB)
target_compile_options(equ-kloku PRIVATE -Wall -Wextra -Wpedantic)
install(TARGETS equ-kloku RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
    add_executable(focus-tests tests/focus_tests.cc)
    target_link_libraries(focus-tests PRIVATE focus_core)
    target_compile_options(focus-tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME focus-tests COMMAND focus-tests)
endif()
