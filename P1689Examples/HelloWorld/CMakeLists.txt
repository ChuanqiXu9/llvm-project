cmake_minimum_required(VERSION 3.24)
project(cxx_modules_Hello CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMake_TEST_MODULE_COMPILATION_RULES}")

add_executable(Hello)
target_sources(Hello
  PRIVATE
    Use.cpp Impl.cpp
  PRIVATE
    FILE_SET CXX_MODULES
      BASE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}"
      FILES
        Hello.cppm Hello-impl_part.cppm Hello-interface_part.cppm)

# I don't know how to spcify "${CMAKE_BINARY_DIR}/CMakeFiles/Hello.dir" properly
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fmodules-cache-path=${CMAKE_BINARY_DIR}/CMakeFiles/Hello.dir")
target_compile_features(Hello PUBLIC cxx_std_20)
