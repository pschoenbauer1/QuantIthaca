cmake_minimum_required(VERSION 3.25)
project(quantithaca LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# Make sure the dll's the gtest exe's are in the same directory. Otherwise fails on Windows. 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Prefer the project .venv interpreter so embedded C++ (pybind11) uses the same
# Python ABI as site-packages (numpy/pandas wheels). Stale CMake cache often keeps
# a system Python; FORCE + explicit executable avoids that.
if(EXISTS "${CMAKE_SOURCE_DIR}/.venv")
  if(WIN32)
    set(_venv_python "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
  else()
    set(_venv_python "${CMAKE_SOURCE_DIR}/.venv/bin/python")
  endif()
  if(EXISTS "${_venv_python}")
    set(Python_EXECUTABLE "${_venv_python}" CACHE FILEPATH "Python from project .venv" FORCE)
    set(Python_ROOT_DIR "${CMAKE_SOURCE_DIR}/.venv" CACHE PATH "Python root (.venv)" FORCE)
    set(Python_FIND_VIRTUALENV FIRST)
  endif()
endif()

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module Development.Embed)

message(STATUS "Python for build/tests: ${Python_EXECUTABLE} (${Python_VERSION})")
if(EXISTS "${CMAKE_SOURCE_DIR}/.venv")
  cmake_path(NORMAL_PATH Python_EXECUTABLE OUTPUT_VARIABLE _python_exe_norm)
  cmake_path(NORMAL_PATH CMAKE_SOURCE_DIR OUTPUT_VARIABLE _src_norm)
  if(NOT _python_exe_norm MATCHES "^${_src_norm}/\\.venv/")
    message(FATAL_ERROR
      "Python mismatch: expected interpreter from ${CMAKE_SOURCE_DIR}/.venv, got ${Python_EXECUTABLE}. "
      "Clear CMake cache / reconfigure so embedded Python matches .venv site-packages.")
  endif()
endif()

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
target_compile_definitions(core_bind PRIVATE PY_BRIDGE_USE_NANOBIND)

install(TARGETS core LIBRARY DESTINATION quantithaca)

include(CTest)
if(BUILD_TESTING AND NOT SKBUILD)
  # Ensure pybind11 uses CMake's already-resolved Python (from .venv/uv),
  # not its legacy finder that can pick a different system install.
  set(PYBIND11_FINDPYTHON ON CACHE BOOL "" FORCE)

  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.6
  )
  FetchContent_MakeAvailable(pybind11)

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
  target_include_directories(core_tests PRIVATE cpp/src)
  target_link_libraries(core_tests PRIVATE core utils GTest::gtest_main pybind11::embed)
  target_compile_definitions(core_tests PRIVATE PY_BRIDGE_USE_PYBIND)

  # Build-time gtest discovery executes core_tests.exe; on Windows it must find
  # python312.dll at process startup.
  if(WIN32 AND DEFINED Python_RUNTIME_LIBRARY_RELEASE)
    add_custom_command(TARGET core_tests POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Python_RUNTIME_LIBRARY_RELEASE}"
        "$<TARGET_FILE_DIR:core_tests>"
      VERBATIM
    )
  endif()

  include(GoogleTest)
  gtest_discover_tests(core_tests TEST_LIST core_tests_TESTS)

  if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
    set(_venv_site_packages "${CMAKE_SOURCE_DIR}/.venv/Lib/site-packages")
    set(_venv_scripts "${CMAKE_SOURCE_DIR}/.venv/Scripts")
    set(_core_tests_env
      "PYTHONPATH=${CMAKE_SOURCE_DIR}/python;${_venv_site_packages}"
      "PATH=${_venv_scripts};$ENV{PATH}"
    )
    set_tests_properties(${core_tests_TESTS} PROPERTIES ENVIRONMENT "${_core_tests_env}")
  endif()
endif()
  