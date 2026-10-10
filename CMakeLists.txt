cmake_minimum_required(VERSION 3.16)
project(kondor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(SDL2 CONFIG QUIET)
if(NOT TARGET SDL2::SDL2)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
  add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
endif()

add_executable(kondor
  src/KONDOR.CPP
  src/compat/borland.cpp
  src/platform/platform_sdl.cpp)
target_include_directories(kondor PRIVATE src)
target_link_libraries(kondor PRIVATE SDL2::SDL2)
target_compile_options(kondor PRIVATE -Wall -Wextra -Wno-unused-parameter)
install(TARGETS kondor RUNTIME DESTINATION bin)
