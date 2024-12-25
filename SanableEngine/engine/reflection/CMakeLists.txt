cmake_minimum_required(VERSION 3.11)
set(CMAKE_CXX_STANDARD 17)

project(
	STIX
	VERSION 0.3.0
	LANGUAGES CXX
	DESCRIPTION "Sanable Type Interrogation eXtensions - a RTTI solution for C++"
)

include("${CMAKE_CURRENT_LIST_DIR}/tools/CMakeLists.txt")
include("${CMAKE_CURRENT_LIST_DIR}/runtime/CMakeLists.txt")
