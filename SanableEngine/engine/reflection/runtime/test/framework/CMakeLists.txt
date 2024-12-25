cmake_minimum_required (VERSION 3.11)
set (CMAKE_CXX_STANDARD 17)

project("stix-runtime-test")

aux_source_directory("${CMAKE_CURRENT_LIST_DIR}/src" stix_test_sources)
add_executable("stix-runtime-test" ${stix_test_sources})

target_link_libraries("stix-runtime-test" PUBLIC stix stix-runtime-test-target doctest)

doctest_discover_tests(stix-runtime-test)
