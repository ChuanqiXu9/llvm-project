set(STD_PARTITIONS_SOURCES
    std-coroutine.cppm
    std-type_traits.cppm
    std-utility.cppm
    std-vector.cppm
    std-exception.cppm)

file(GLOB STD_SOURCES "*.cppm")

set_property(SOURCE ${STD_SOURCES} PROPERTY LANGUAGE CXX)

add_library(std_partitions_interfaces OBJECT ${STD_PARTITIONS_SOURCES})

# NOTE:
#  Since CMake can't recognize C++20 Modules well now.
#  We need to implement the two phase compilation manually.
#
#  Since CMake don't know anything about the .pcm files and its infrastructure
#  is based on .o files.
#  So we generate the .pcm files as .o files at first. Then we treat these .pcm
#  files as source files and we compile these .pcm files to .o files and link them to
#  libstd_modules (necessary).
#
#  The pcm files are located at ${CMAKE_CURRENT_BINARY_DIR}. And we can find a better
#  place for it.

set(MODULES_INTERFACES_COMPILE_FLAGS "${LIBCXX_COMPILE_FLAGS} -std=c++2b -pthread --precompile")

set_target_properties(std_partitions_interfaces
                      PROPERTIES
                         LINKER_LANGUAGE CXX
                         COMPILE_FLAGS "${MODULES_INTERFACES_COMPILE_FLAGS}"
                         LINK_FLAGS    "${LIBCXX_LINK_FLAGS}")
target_include_directories(std_partitions_interfaces
                           PRIVATE ${LIBCXX_GENERATED_INCLUDE_DIR}
                                   ${LIBCXX_GENERATED_INCLUDE_TARGET_DIR})
target_add_compile_flags_if_supported(std_partitions_interfaces PUBLIC -nostdinc++)

add_custom_target(generate_module_interface ALL
                  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/std_partitions_interfaces.dir/*.cppm.o ${CMAKE_CURRENT_BINARY_DIR}/. &&
                          rename .cppm.o .pcm ${CMAKE_CURRENT_BINARY_DIR}/*.cppm.o
                  DEPENDS std_partitions_interfaces)

add_library(std_module_interface OBJECT std.cppm)
add_dependencies(std_module_interface generate_module_interface)
set_target_properties(std_module_interface
                      PROPERTIES
                         LINKER_LANGUAGE CXX
                         COMPILE_FLAGS "${MODULES_INTERFACES_COMPILE_FLAGS} -fprebuilt-module-path=${CMAKE_CURRENT_BINARY_DIR}")

add_custom_target(generate_std_module_interface ALL
                  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/std_module_interface.dir/std.cppm.o ${CMAKE_CURRENT_BINARY_DIR}/std.pcm
                  DEPENDS std_module_interface)

file(GLOB STD_FAKE_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/*.pcm")
set_property(SOURCE ${STD_FAKE_SOURCES} PROPERTY LANGUAGE CXX)

add_library(std_modules STATIC ${STD_FAKE_SOURCES})
set_target_properties(std_modules
                      PROPERTIES
                         LINKER_LANGUAGE CXX
                         COMPILE_FLAGS "-std=c++2b -pthread"
                         LINK_FLAGS    "${LIBCXX_LINK_FLAGS}")
