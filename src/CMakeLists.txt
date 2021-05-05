file(GLOB HEADER_LIST "${llvmc_SOURCE_DIR}/include/llvmc/*.h")
file(GLOB SRC_LIST "*.cpp")

add_library(llvmc_lib STATIC ${SRC_LIST} ${HEADER_LIST})

target_include_directories(llvmc_lib PUBLIC ../include)

target_link_libraries(llvmc_lib PRIVATE ${llvm_libs})

target_compile_features(llvmc_lib PUBLIC cxx_std_20)

target_compile_options(llvmc_lib PRIVATE
    $<$<NOT:$<OR:$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:MSVC>>>: -Wall -Wpedantic -Wno-switch>
)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${HEADER_LIST}
)