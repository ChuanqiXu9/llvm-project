cmake_minimum_required(VERSION 3.24)
project(cxx_modules_simple CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMake_TEST_MODULE_COMPILATION_RULES}")

add_executable(simple)
target_sources(simple
  PRIVATE
    main.cpp
  PRIVATE
    FILE_SET CXX_MODULES
      BASE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}"
      FILES
        importable.cppm)


# I don't know how to spcify "${CMAKE_BINARY_DIR}/CMakeFiles/simple.dir" properly
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fmodules-cache-path=${CMAKE_BINARY_DIR}/CMakeFiles/simple.dir")
target_compile_features(simple PUBLIC cxx_std_20)
