cmake_minimum_required (VERSION 3.14)
set (CMAKE_CXX_STANDARD 17)

#
# C++ runtime reflection
#

# Declare target
aux_source_directory("${CMAKE_CURRENT_LIST_DIR}/src" stix_sources)
declare_dll("stix" "" stix_sources)
#add_library("stix" STATIC ${stix_sources})
target_include_directories("stix" PRIVATE "${CMAKE_CURRENT_LIST_DIR}/private")

# Declare exports
target_include_directories("stix" PUBLIC "${CMAKE_CURRENT_LIST_DIR}/public")

include("${CMAKE_CURRENT_LIST_DIR}/SemanticVM/CMakeLists.txt")

# Declare install targets
install_dll("stix" ".")


# Register runtime unit tests
include("${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt")
