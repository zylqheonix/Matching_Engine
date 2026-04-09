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

add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  -Wshadow
  -Wconversion
  -Wsign-conversion
  -Wold-style-cast
)

if(ME_ENABLE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
elseif(ME_ENABLE_TSAN)
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread)
endif()

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.0
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(order_book_build_test tests/order_book_build_test.cpp)
target_include_directories(order_book_build_test PRIVATE include)
target_link_libraries(order_book_build_test PRIVATE GTest::gtest_main)

add_test(NAME order_book_build_test COMMAND order_book_build_test)
