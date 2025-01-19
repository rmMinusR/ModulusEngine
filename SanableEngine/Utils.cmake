# Detect 32/64-bit architecture
if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    set(ARCH_NAME x64)
elseif (${CMAKE_SIZEOF_VOID_P} MATCHES 4)
    set(ARCH_NAME x86)
else()
    message(ERROR " > Unknown architecture: ${CMAKE_SIZEOF_VOID_P}-byte")
endif ()

# Detect platform
if (WIN32)
    set(PLATFORM_DLL_EXTENSION ".dll")
elseif(EMSCRIPTEN)
    set(PLATFORM_DLL_EXTENSION ".wasm")
else()
    message(ERROR " > Could not determine plugin extension: Unknown platform")
endif()
add_compile_definitions(PLATFORM_DLL_EXTENSION="${PLATFORM_DLL_EXTENSION}")

if (EMSCRIPTEN)
    add_compile_options("-fPIC") # Emscripten side modules require PIC. Might as well turn it on globally.
endif()

# Set debug flags
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    message(" > Detected debug build. Names will be left in.")
    #add_compile_options(-g3)
    #add_link_options(-g3)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    message(" > Detected release build. Output will be optimized.")
    add_compile_options(-O2)
    add_link_options(-O2)
else()
    message(ERROR " > Unknown build type ${CMAKE_BUILD_TYPE}")
endif()


function(export_dll dll project)
	if (WIN32)
        add_custom_command(
            TARGET ${project} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${dll}>" "$<TARGET_FILE_DIR:${project}>"
            VERBATIM
        )
    endif()
endfunction()

function(set_linkage_static project)
    if (EMSCRIPTEN)
        target_link_options(${project} PRIVATE -sMAIN_MODULE)
    endif()
endfunction()

function(set_linkage_shared project)
    if (EMSCRIPTEN)
        target_link_options(${project} PRIVATE -sSIDE_MODULE)
    endif()
endfunction()

function(install_dll name dest)
    if (WIN32)
        install(TARGETS "${name}"
	        RUNTIME DESTINATION "${dest}" COMPONENT Runtime
        )
    elseif (EMSCRIPTEN)
        install(FILES "$<TARGET_FILE_DIR:${name}>/${name}.wasm" DESTINATION "${dest}")
    else()
        message(ERROR "Unknown platform, don't know how to install DLL")
    endif()
endfunction()

function(declare_dll name exportRelpath sources_var)
    add_library(${name} SHARED ${${sources_var}})
    set_target_properties(${name} PROPERTIES PREFIX "") # Stop prefixing everything with lib!
    set_linkage_shared(${name})
    set_target_properties(${name} PROPERTIES
	    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${exportRelpath}"
	    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${exportRelpath}"
	    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${exportRelpath}"
    )

    if (EMSCRIPTEN)
        # This is actually an .a archive file (output of the "ar" command), which needs to be turned into a WASM binary
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_CXX_COMPILER} -shared -sSIDE_MODULE -o $<TARGET_FILE_DIR:${name}>/${name}.wasm $<TARGET_FILE:${name}>
            COMMENT "Packaging ${name}: .a -> .wasm"
        )
        set_property(TARGET ${name} APPEND PROPERTY ADDITIONAL_CLEAN_FILES "$<TARGET_FILE_DIR:${name}>/${name}.wasm") # Ensure it gets cleaned up
    endif()
endfunction()

SET(sanableAllPlugins "")
function(declare_plugin name sources_var)
    declare_dll(${name} "plugins/${name}" ${sources_var})
    install_dll(${name} "plugins/${name}")
    SET(sanableAllPlugins ${sanableAllPlugins} ${name} PARENT_SCOPE)

    stix_extract_ast(${name} ${CMAKE_CURRENT_LIST_DIR})
    stix_generate_reflection(${name})
endfunction()

function(export_resource libTarget fileRelPath)
    add_custom_command(
        TARGET ${libTarget} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_LIST_DIR}/${fileRelPath}
                ${CMAKE_CURRENT_BINARY_DIR}/${fileRelPath}
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${fileRelPath}
    )
    install(FILES "${CMAKE_CURRENT_LIST_DIR}/${fileRelPath}" DESTINATION "${fileRelPath}")
endfunction()