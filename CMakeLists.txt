cmake_minimum_required(VERSION 3.25)
project(quantithaca LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Make sure the dll's the gtest exe's are in the same directory. Otherwise fails on Windows. 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

FetchContent_Declare(
  nanobind
  GIT_REPOSITORY https://github.com/wjakob/nanobind.git
  GIT_TAG v2.2.0
)
FetchContent_MakeAvailable(nanobind)

add_subdirectory(cpp/src/context)
add_subdirectory(cpp/src/utils)

add_library(core STATIC
  cpp/src/core.cpp
)
target_include_directories(core PUBLIC cpp/include)
target_link_libraries(core PUBLIC utils context)

nanobind_add_module(core_bind cpp/src/bindings.cpp)
target_link_libraries(core_bind PRIVATE core utils)

install(TARGETS core LIBRARY DESTINATION quantithaca)

include(CTest)
if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(core_tests
    tests/cpp/test_core.cpp
    tests/cpp/test_graph.cpp
    tests/cpp/test_utils.cpp
  )
  target_link_libraries(core_tests PRIVATE core utils GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(core_tests)
endif()
