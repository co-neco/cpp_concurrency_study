cmake_minimum_required(VERSION 3.10)

project(ConcurrentClass VERSION 1.0)

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_17)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

target_compile_options(compiler_flags INTERFACE
    "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>")

enable_testing()
add_subdirectory(tests)

add_executable(ConcurrentClass main.cpp)
target_link_libraries(
    ConcurrentClass PUBLIC 
    compiler_flags)