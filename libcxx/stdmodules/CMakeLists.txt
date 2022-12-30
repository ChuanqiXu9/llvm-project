set(PREBUILT_MODULE_PATH ${CMAKE_BINARY_DIR}/libcxx/modules)
file(MAKE_DIRECTORY ${PREBUILT_MODULE_PATH})

function(module name)
  set(input ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cppm)
  set(output ${PREBUILT_MODULE_PATH}/${name}.pcm)

  add_custom_command(
    OUTPUT ${output}
    COMMAND
      ${CMAKE_CXX_COMPILER}
      ${LIBCXX_COMPILE_FLAGS}
      # These arguments need to match what is used in the tests
      -std=c++2b
      -pthread
      --target=x86_64-unknown-linux-gnu
      -stdlib=libc++
      -fprebuilt-module-path=${PREBUILT_MODULE_PATH}
      --precompile
      ${input}
      -o ${output}
    DEPENDS
      ${input}
  )

  add_custom_target(${name}
    DEPENDS ${output}
  )
endfunction()

function(partition name)
  module(${name})
  add_dependencies(partitions ${name})
endfunction()

add_custom_target(partitions)

partition(std-coroutine)
partition(std-exception)
partition(std-type_traits)
partition(std-utility)
partition(std-vector)
module(std)
add_dependencies(std partitions)
