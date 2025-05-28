file(GLOB TESTS_LIST CONFIGURE_DEPENDS "tests/*.cpp")

add_executable(benchmark benchmark.cpp "tests/tests.h" ${TESTS_LIST})
target_compile_features(benchmark PUBLIC cxx_std_20)

target_link_libraries(benchmark PUBLIC fmt::fmt hs chimera re2 RapidJSON simdjson yyjson jsmn)
