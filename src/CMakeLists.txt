
# Add source to this project's executable.
add_executable(c3d_engine
    main.cpp
    vk_engine.cpp
    vk_engine.h
    vk_types.h
    vk_initializers.cpp
    vk_initializers.h
	vk_pipeline.cpp
	vk_pipeline.h
	vk_deletion_queue.cpp
	vk_deletion_queue.h
	vk_mesh.cpp
	vk_mesh.h
	vk_render_object.cpp
	vk_render_object.h
	vk_frame.h
	vk_texture.cpp
	vk_texture.h)

set_property(TARGET c3d_engine PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:c3d_engine>")

target_include_directories(c3d_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(c3d_engine vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(c3d_engine Vulkan::Vulkan sdl2)

add_dependencies(c3d_engine Shaders)
