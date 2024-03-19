include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.zip)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(CTest)
add_executable(ts_stack_test 
    ts_stack.cc
    ts_queue.cc
    ts_map.cc)
target_compile_features(ts_stack_test PRIVATE cxx_std_17)
target_link_libraries(
    ts_stack_test
    GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(ts_stack_test)