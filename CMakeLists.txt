cmake_minimum_required(VERSION 3.15)

project(
  CacheBenchmark
  VERSION 0.1
  DESCRIPTION "JSON Caching benchmark"
  LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 5.3.0)
FetchContent_MakeAvailable(fmtlib)

add_subdirectory(libs)

add_subdirectory(benchmark)
