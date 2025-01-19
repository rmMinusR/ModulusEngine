cmake_minimum_required (VERSION 3.14)
set (CMAKE_CXX_STANDARD 17)

#
# SemanticVM: symbolic execution engine
#

# Declare target
aux_source_directory("${CMAKE_CURRENT_LIST_DIR}/src" stix_semanticvm_sources)
aux_source_directory("${CMAKE_CURRENT_LIST_DIR}/src/platform/${TARGET_ARCH_GROUP}" stix_semanticvm_platform_sources)
target_sources("stix" PRIVATE ${stix_semanticvm_sources} ${stix_semanticvm_platform_sources})
target_include_directories("stix" PRIVATE "${CMAKE_CURRENT_LIST_DIR}/private")
target_include_directories("stix" PUBLIC "${CMAKE_CURRENT_LIST_DIR}/public")

# Declare imports
target_link_libraries("stix" PRIVATE capstone)
