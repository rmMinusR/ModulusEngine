cmake_minimum_required (VERSION 3.14)
set (CMAKE_CXX_STANDARD 17)

project("engine-memory")

# Declare target
aux_source_directory("${CMAKE_CURRENT_LIST_DIR}/src" engine_memory_sources)
declare_dll("engine-memory" "" engine_memory_sources)
#add_library("engine-memory" STATIC ${engine_memory_sources})
target_include_directories("engine-memory" PRIVATE "${CMAKE_CURRENT_LIST_DIR}/private")

# Declare imports
target_link_libraries("engine-memory" PUBLIC stix)

# Declare exports
target_include_directories("engine-memory" PUBLIC "${CMAKE_CURRENT_LIST_DIR}/public")

# Declare install targets
install_dll("engine-memory" ".")

include("${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt")
