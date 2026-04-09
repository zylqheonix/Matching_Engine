cmake_minimum_required(VERSION 3.20)

project(matching_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release on single-config generators when unspecified.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(ME_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ME_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ME_ENABLE_ASAN AND ME_ENABLE_TSAN)
  message(FATAL_ERROR "ME_ENABLE_ASAN and ME_ENABLE_TSAN are mutually exclusive.")
endif()

add_library(me_project_options INTERFACE)
link_libraries(me_project_options)

target_compile_options(me_project_options INTERFACE
  -Wall
  -Wextra
  -Wpedantic
  -Wshadow
  -Wconversion
  -Wsign-conversion
  -Wold-style-cast
)

if(ME_ENABLE_ASAN)
  target_compile_options(me_project_options INTERFACE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(me_project_options INTERFACE -fsanitize=address)
elseif(ME_ENABLE_TSAN)
  target_compile_options(me_project_options INTERFACE -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(me_project_options INTERFACE -fsanitize=thread)
endif()

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.0
)
FetchContent_MakeAvailable(googlebenchmark)
