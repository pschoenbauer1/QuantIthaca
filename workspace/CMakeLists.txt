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

set(WITH_GFLAGS OFF CACHE BOOL "glog: skip gflags (optional)" FORCE)
set(WITH_GTEST OFF CACHE BOOL "glog: skip GTest (project supplies its own)" FORCE)
set(WITH_UNWIND none CACHE STRING "glog: unwind backend" FORCE)
FetchContent_Declare(
  glog
  GIT_REPOSITORY https://github.com/google/glog.git
  GIT_TAG v0.7.1
)
# glog defaults to SHARED; a separate glog.dll is then required at runtime. Windows
# gtest_discover_tests runs core_tests.exe during the build — if glog.dll is not next to
# the exe, load fails with 0xc0000135 (STATUS_DLL_NOT_FOUND). Static glog avoids that.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build glog as static (see comment above)" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF)
FetchContent_MakeAvailable(glog)
set(BUILD_TESTING ON CACHE BOOL "" FORCE)
set(BUILD_TESTING ON)

# ------------------------ Core ------------------------


add_library(core STATIC
  cpp/src/core.cpp
  cpp/src/context/graph.cpp
  cpp/src/context_obj/dummy_obj.cpp
  cpp/src/context/py_obj.cpp
  cpp/src/utils/threads.cpp
)
set_target_properties(core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(core PUBLIC cpp/include cpp/src)
install(TARGETS core LIBRARY DESTINATION quantithaca)


# ------------------------ Nanobind ------------------------


nanobind_add_module(core_bind cpp/src/py/bindings.cpp cpp/src/py/graph_py_factory.cpp)
target_include_directories(core_bind PUBLIC cpp/src)
target_link_libraries(core_bind PRIVATE core)
target_compile_definitions(core_bind PRIVATE PY_BRIDGE_USE_NANOBIND)


# ------------------------ Test ------------------------


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
    cpp/src/py/graph_py_factory.cpp
  )
  target_include_directories(core_tests PRIVATE cpp/src)
  target_link_libraries(core_tests PRIVATE
    core
    GTest::gtest_main
    pybind11::embed
    glog::glog
  )
  # Embedded gtests must call Python via pybind11 — nanobind does not support embedding.
  target_compile_definitions(core_tests PRIVATE PY_BRIDGE_USE_PYBIND PY_EMBED_PYTHON)
  add_dependencies(core_tests core_bind)

  # Build-time gtest discovery executes core_tests.exe; on Windows it must find
  # python312.dll (and its deps, e.g. zlib.dll) next to the exe — discovery runs
  # with a minimal PATH, so copying is required (load fails with 0xc0000135 otherwise).
  if(WIN32 AND DEFINED Python_RUNTIME_LIBRARY_RELEASE)
    get_filename_component(_python_runtime_dir "${Python_RUNTIME_LIBRARY_RELEASE}" DIRECTORY)
    add_custom_command(TARGET core_tests POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${Python_RUNTIME_LIBRARY_RELEASE}"
        "$<TARGET_FILE_DIR:core_tests>"
      VERBATIM
    )
    if(EXISTS "${_python_runtime_dir}/zlib.dll")
      add_custom_command(TARGET core_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_python_runtime_dir}/zlib.dll"
          "$<TARGET_FILE_DIR:core_tests>"
        VERBATIM
      )
    elseif(EXISTS "${_python_runtime_dir}/Library/bin/zlib.dll")
      add_custom_command(TARGET core_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_python_runtime_dir}/Library/bin/zlib.dll"
          "$<TARGET_FILE_DIR:core_tests>"
        VERBATIM
      )
    endif()
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
  