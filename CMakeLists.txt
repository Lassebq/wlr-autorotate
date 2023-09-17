cmake_minimum_required(VERSION 3.27)
project("wlr-autorotate"
    DESCRIPTION "Automatically changes screen orientation in wlroots based compositors based on the state of accelerometer"
    LANGUAGES C
)

add_compile_options(-lm -lcs50 -g -Wall -Wextra -Wno-unused-parameter -Wno-unused-value -Wno-missing-field-initializers -Wno-narrowing)
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0 gio-2.0 wayland-client)
set(CMAKE_C_STANDARD 99)

file(GLOB_RECURSE SOURCES "src/*.c")

add_executable("wlr-autorotate"
    ${CMAKE_SOURCE_DIR}/protocols/wlr-output-management-unstable-v1-protocol.o    
    ${SOURCES}
)
target_include_directories("wlr-autorotate" PRIVATE ${GLIB_INCLUDE_DIRS})
target_include_directories("wlr-autorotate" PRIVATE src protocols)
target_link_libraries("wlr-autorotate"
    PkgConfig::deps
)