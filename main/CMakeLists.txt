add_executable(llvmc llvmc.cpp)

target_compile_features(llvmc PRIVATE cxx_std_20)

target_link_libraries(llvmc PRIVATE llvmc_lib ${llvm_libs})