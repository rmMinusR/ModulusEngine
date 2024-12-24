cmake_minimum_required (VERSION 3.11)
set (CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)

# Declare target
aux_source_directory("${CMAKE_CURRENT_LIST_DIR}/src" engine_graphics_sources)
target_sources("engine-core" PRIVATE ${engine_graphics_sources}) # Now bundle in with engine-core
#declare_dll("engine-graphics" "" engine_graphics_sources)
target_include_directories("engine-core" PRIVATE "${CMAKE_CURRENT_LIST_DIR}/private")

# Declare imports
target_include_directories("engine-core" PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries("engine-core" PUBLIC ${SDL2_LIBRARIES} ${OPENGL_gl_LIBRARY} OpenFBX glew stb_image freetype engine-memory SDL2_image::SDL2_image)

# Declare exports
target_include_directories("engine-core" PUBLIC "${CMAKE_CURRENT_LIST_DIR}/public")
stix_extract_ast(engine-core ${CMAKE_CURRENT_LIST_DIR})

export_resource(engine-core resources/ui/shaders/font/frag.glsl)
export_resource(engine-core resources/ui/shaders/font/vert.glsl)
export_resource(engine-core resources/ui/shaders/image/frag.glsl)
export_resource(engine-core resources/ui/shaders/image/vert.glsl)
