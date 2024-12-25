cmake_minimum_required (VERSION 3.11)
set (CMAKE_CXX_STANDARD 17)

project("engine-rtti-test-target")

aux_source_directory("${CMAKE_CURRENT_LIST_DIR}/src" engine_rtti_test_target_sources)
add_library(engine-rtti-test-target ${engine_rtti_test_target_sources})
target_include_directories("engine-rtti-test-target" PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

target_link_libraries("engine-rtti-test-target" PUBLIC engine-rtti)
stix_use_template(engine-rtti-test-target "${CMAKE_CURRENT_LIST_DIR}/rtti.template.cpp")
stix_extract_ast(engine-rtti-test-target "${CMAKE_CURRENT_LIST_DIR}")
stix_generate_reflection(engine-rtti-test-target)
