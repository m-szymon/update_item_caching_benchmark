set(KEEP_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")

FetchContent_Declare(
  pcre-src
  URL https://downloads.sourceforge.net/project/pcre/pcre/8.42/pcre-8.42.tar.bz2
)
FetchContent_Populate(pcre-src)
set(PCRE_SOURCE ${pcre-src_SOURCE_DIR})

FetchContent_Declare(
  hyperscan
  GIT_REPOSITORY https://github.com/intel/hyperscan.git)

FetchContent_Populate(hyperscan)
execute_process(
  COMMAND patch -p1 -i ${CMAKE_CURRENT_SOURCE_DIR}/hyperscan.patch
  WORKING_DIRECTORY ${hyperscan_SOURCE_DIR}
)
add_subdirectory(${hyperscan_SOURCE_DIR} ${hyperscan_BINARY_DIR})
target_include_directories(hs PUBLIC "${hyperscan_SOURCE_DIR}/src")
target_include_directories(hs PUBLIC "${hyperscan_SOURCE_DIR}/chimera")

set(RAPIDJSON_BUILD_TESTS OFF)
include(CheckCXXCompilerFlag)

# Check for SSE2
check_cxx_compiler_flag("-msse2" HAS_SSE2)
# Check for SSE4.2
check_cxx_compiler_flag("-msse4.2" HAS_SSE42)
# Check for NEON (for ARM)
check_cxx_compiler_flag("-mfpu=neon" HAS_NEON)
if (HAS_SSE42)
    add_definitions(-DRAPIDJSON_SSE42)
elseif (HAS_SSE2)
    add_definitions(-DRAPIDJSON_SSE2)
elseif (HAS_NEON)
    add_definitions(-DRAPIDJSON_NEON)
endif()
FetchContent_Declare(
  rapidjson
  GIT_REPOSITORY https://github.com/Tencent/rapidjson.git)
FetchContent_MakeAvailable(rapidjson)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(simdjson)

FetchContent_Declare(
  yyjson
  GIT_REPOSITORY https://github.com/ibireme/yyjson.git)
FetchContent_MakeAvailable(yyjson)

FetchContent_Declare(
  jsmn
  GIT_REPOSITORY https://github.com/zserge/jsmn.git)
FetchContent_MakeAvailable(jsmn)
add_library(jsmn INTERFACE)
target_include_directories(jsmn INTERFACE "${jsmn_SOURCE_DIR}")

set(ABSL_ENABLE_INSTALL ON)
FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp
  GIT_TAG 20250127.1)
FetchContent_MakeAvailable(absl)

FetchContent_Declare(
  re2
  GIT_REPOSITORY https://github.com/google/re2.git
  GIT_TAG main)
FetchContent_MakeAvailable(re2)

set(CMAKE_CXX_FLAGS "${KEEP_CMAKE_CXX_FLAGS}")