cmake_minimum_required(VERSION 3.20)
project(minitv LANGUAGES C)

# Presets also pass C++ settings; this is a C-only project
unset(CMAKE_CXX_COMPILER)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0 glib-2.0)

# Public headers shared by every feature
add_library(ncl_interface INTERFACE)
target_include_directories(ncl_interface INTERFACE ${CMAKE_SOURCE_DIR}/interface)

add_subdirectory(feature/config)
add_subdirectory(feature/player)
add_subdirectory(feature/control)

add_executable(minitv main.c)
target_link_libraries(minitv PRIVATE ncl_config ncl_player ncl_control)

install(TARGETS minitv RUNTIME DESTINATION bin)
install(FILES conf/minitv.conf DESTINATION /etc/minitv)
