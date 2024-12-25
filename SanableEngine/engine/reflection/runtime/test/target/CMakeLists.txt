cmake_minimum_required (VERSION 3.11)
set (CMAKE_CXX_STANDARD 17)

project("stix-runtime-test-target")

aux_source_directory("${CMAKE_CURRENT_LIST_DIR}/src" stix_test_target_sources)
add_library(stix-runtime-test-target ${stix_test_target_sources})
target_include_directories("stix-runtime-test-target" PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

target_link_libraries("stix-runtime-test-target" PUBLIC stix)
stix_use_template(stix-runtime-test-target "${CMAKE_CURRENT_LIST_DIR}/rtti.template.cpp")
stix_extract_ast(stix-runtime-test-target "${CMAKE_CURRENT_LIST_DIR}")
stix_generate_reflection(stix-runtime-test-target)
