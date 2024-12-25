cmake_minimum_required (VERSION 3.11)
set (CMAKE_CXX_STANDARD 17)

project(
	SanableReflection
	VERSION 0.2.0
	LANGUAGES CXX
)

include("${CMAKE_CURRENT_LIST_DIR}/target/CMakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/framework/CMakeLists.txt")
