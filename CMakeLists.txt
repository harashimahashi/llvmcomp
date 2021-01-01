cmake_minimum_required(VERSION 3.16)
project(llvmc VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader)

file(GLOB LLVMC_SRC src/*.cpp)

add_executable(llvmc ${LLVMC_SRC})
target_compile_options(llvmc PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>: /W3 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: -Wall -Wextra>
)
target_link_libraries(llvmc ${llvm_libs})

set_property(TARGET llvmc PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
