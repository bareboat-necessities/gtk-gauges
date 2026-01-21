cmake_minimum_required(VERSION 3.20)
project(gtk_gauges LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

add_executable(wind_demo
  src/main.cpp
  src/circular_gauge.cpp
  src/wind_instrument.cpp
)

target_include_directories(wind_demo PRIVATE src)
target_link_libraries(wind_demo PRIVATE PkgConfig::GTKMM)

# Nice warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(wind_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
