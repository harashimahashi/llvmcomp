file(GLOB HEADER_LIST "${llvmc_SOURCE_DIR}/include/llvmc/*.h")
file(GLOB SRC_LIST "*.cpp")

add_library(llvmc_lib STATIC ${SRC_LIST} ${HEADER_LIST})

target_include_directories(llvmc_lib PUBLIC ../include)

target_link_libraries(llvmc_lib PRIVATE ${llvm_libs})

target_compile_features(llvmc_lib PUBLIC cxx_std_20)

string(REPLACE "." ";" COMP_VER_LIST ${CMAKE_CXX_COMPILER_VERSION})
list(GET COMP_VER_LIST 0 COMP_VER_MAJOR)

target_link_options(llvmc_lib PUBLIC
    $<$<AND:$<NOT:$<PLATFORM_ID:Windows>>,$<CXX_COMPILER_ID:AppleClang,Clang>>:-fuse-ld=lld-${COMP_VER_MAJOR}>
)

set_target_properties(
    llvmc_lib 
    PROPERTIES 
        INTERPROCEDURAL_OPTIMIZATION True
)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${HEADER_LIST}
)