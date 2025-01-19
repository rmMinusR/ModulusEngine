cmake_minimum_required (VERSION 3.14)
set (CMAKE_CXX_STANDARD 17)

option(STIX_RTTIGEN_PROFILING "Enable profiling during reflection generation time" FALSE)
set(STIX_DEFAULT_TEMPLATE_FILE "${CMAKE_CURRENT_LIST_DIR}/rtti.template.cpp" CACHE STRING "Default template file containing binders")

# Optional property setters. Must be set before calling stix_generate_reflection.

function(stix_use_template targetName template)
	set_target_properties(${targetName} PROPERTIES STIX_TEMPLATE_FILE ${template})
endfunction()

function(stix_default_image_capture_backend targetName backend)
	set_target_properties(${targetName} PROPERTIES STIX_DEFAULT_IMAGE_CAPTURE_BACKEND ${backend})
endfunction()

function(stix_default_image_capture_status targetName status)
	set_target_properties(${targetName} PROPERTIES STIX_DEFAULT_IMAGE_CAPTURE_STATUS ${status})
endfunction()


# Main integration

set(STIX_AST_COMMAND python "${CMAKE_CURRENT_LIST_DIR}/cx_ast_clang_reader.py")
function(stix_extract_ast targetName rootDir)
	# Add rootDir to STIX_ROOT_DIRS (allows more complex directory structure)
	get_target_property(ROOT_DIRS ${targetName} STIX_ROOT_DIRS)
	if (ROOT_DIRS STREQUAL "ROOT_DIRS-NOTFOUND")
		set(ROOT_DIRS ${rootDir})
	else()
		list(APPEND ROOT_DIRS ${rootDir})
	endif()
	set_property(TARGET ${targetName} PROPERTY STIX_ROOT_DIRS ${ROOT_DIRS})

	# Add RTTI generator target if none exists
	if (NOT TARGET ${targetName}_STIX_AST)
		get_target_property(OUTPUT_FILE_DIR ${targetName} BINARY_DIR)
		set(AST_FILE "${OUTPUT_FILE_DIR}/generated-rtti/${targetName}.stix-ast")

		if (STIX_RTTIGEN_PROFILING)
			set(PROFILING_ARGS --task-profiling --call-profiling)
		else()
			set(PROFILING_ARGS )
		endif()

		add_custom_target(
			${targetName}_STIX_AST
			BYPRODUCTS "${AST_FILE}"
 			COMMAND ${STIX_AST_COMMAND}
						--target $<TARGET_PROPERTY:${targetName},STIX_ROOT_DIRS>
						--output "${AST_FILE}"
 						--include $<TARGET_PROPERTY:${targetName},INCLUDE_DIRECTORIES>
 						--define $<TARGET_PROPERTY:${targetName},COMPILE_DEFINITIONS>
 						${PROFILING_ARGS}
 						#-- $<TARGET_PROPERTY:${targetName},COMPILE_OPTIONS> $<TARGET_PROPERTY:${targetName},COMPILE_FLAGS> ${COMPILE_OPTIONS} ${COMPILE_FLAGS}
			VERBATIM
			USES_TERMINAL
 			COMMENT "Extracting AST for ${targetName}"
 		)
	endif()
endfunction()

set(STIX_RTTI_CODEGEN_COMMAND python "${CMAKE_CURRENT_LIST_DIR}/cx_ast_rttigen_writer.py")
function(stix_generate_reflection targetName)
	# Requires stix_extract_ast() to be run before this
	if (NOT TARGET ${targetName}_STIX_AST)
		message(FATAL_ERROR "Missing AST data to generate RTTI binders for ${targetName} - call stix_extract_ast before stix_generate_reflection")
	endif()

	# Resolve optional properties
	get_target_property(TEMPLATE_FILE ${targetName} STIX_TEMPLATE_FILE)
	if (TEMPLATE_FILE STREQUAL "TEMPLATE_FILE-NOTFOUND")
		set(TEMPLATE_ARG --template ${STIX_DEFAULT_TEMPLATE_FILE})
	else()
		set(TEMPLATE_ARG --template ${TEMPLATE_FILE})
	endif()
	get_target_property(DEFAULT_IMAGE_CAPTURE_BACKEND ${targetName} STIX_DEFAULT_IMAGE_CAPTURE_BACKEND)
	if (DEFAULT_IMAGE_CAPTURE_BACKEND STREQUAL "DEFAULT_IMAGE_CAPTURE_BACKEND-NOTFOUND")
		set(DEFAULT_IMAGE_CAPTURE_BACKEND_ARG )
	else()
		set(DEFAULT_IMAGE_CAPTURE_BACKEND_ARG --default-image-capture-backend ${DEFAULT_IMAGE_CAPTURE_BACKEND})
	endif()
	get_target_property(DEFAULT_IMAGE_CAPTURE_STATUS ${targetName} STIX_DEFAULT_IMAGE_CAPTURE_STATUS)
	if (DEFAULT_IMAGE_CAPTURE_STATUS STREQUAL "DEFAULT_IMAGE_CAPTURE_STATUS-NOTFOUND")
		set(DEFAULT_IMAGE_CAPTURE_STATUS_ARG )
	else()
		set(DEFAULT_IMAGE_CAPTURE_STATUS_ARG --default-image-capture-status ${DEFAULT_IMAGE_CAPTURE_STATUS})
	endif()

	# Add RTTI generator target if none exists
	if (NOT TARGET ${targetName}_STIX_RTTI)
		get_target_property(OUTPUT_FILE_DIR ${targetName} BINARY_DIR)
		set(AST_FILE "${OUTPUT_FILE_DIR}/generated-rtti/${targetName}.stix-ast")
		set(OUTPUT_FILE "${OUTPUT_FILE_DIR}/generated-rtti/${targetName}_rtti.generated.cpp")

		if (STIX_RTTIGEN_PROFILING)
			set(PROFILING_ARGS --task-profiling --call-profiling)
		else()
			set(PROFILING_ARGS )
		endif()

		add_custom_target(
			${targetName}_STIX_RTTI
			BYPRODUCTS "${OUTPUT_FILE}"
 			COMMAND ${STIX_RTTI_CODEGEN_COMMAND}
						--input "${AST_FILE}"
						--output "${OUTPUT_FILE}"
 						${TEMPLATE_ARG}
						${DEFAULT_IMAGE_CAPTURE_BACKEND_ARG}
						${DEFAULT_IMAGE_CAPTURE_STATUS_ARG}
						${PROFILING_ARGS}
			VERBATIM
			USES_TERMINAL
			DEPENDS ${targetName}_STIX_AST
 			COMMENT "Generating embedded RTTI for ${targetName}"
 		)

		add_dependencies(${targetName} ${targetName}_STIX_RTTI)

		target_sources(${targetName} PRIVATE ${OUTPUT_FILE})
	endif()
endfunction()


# TODO: register unit tests
