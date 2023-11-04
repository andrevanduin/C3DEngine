
cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 23)

add_executable (Editor "src/main.cpp")

target_link_libraries(Editor PUBLIC C3DEngine)

if (WIN32)
	target_link_options(Editor PUBLIC /MANIFEST:NO)
endif()

add_custom_target(CopyEngineLib
	COMMAND ${CMAKE_COMMAND} -E copy 
	"${CMAKE_BINARY_DIR}/engine/${CMAKE_SHARED_LIBRARY_PREFIX}C3DEngine${CMAKE_SHARED_LIBRARY_SUFFIX}" 
	"${CMAKE_BINARY_DIR}/editor/executable/"
	DEPENDS C3DEngine
)

add_custom_target(CopyVulkanRenderLib
	COMMAND ${CMAKE_COMMAND} -E copy 
	"${CMAKE_BINARY_DIR}/plugins/renderers/vulkan_renderer/${CMAKE_SHARED_LIBRARY_PREFIX}C3DVulkanRenderer${CMAKE_SHARED_LIBRARY_SUFFIX}" 
	"${CMAKE_BINARY_DIR}/editor/executable/"
	DEPENDS C3DVulkanRenderer
)

add_custom_target(CopyEditorLib
	COMMAND ${CMAKE_COMMAND} -E copy
	"${CMAKE_BINARY_DIR}/editor/lib/${CMAKE_SHARED_LIBRARY_PREFIX}EditorLib${CMAKE_SHARED_LIBRARY_SUFFIX}" 
	"${CMAKE_BINARY_DIR}/editor/executable/"
	DEPENDS EditorLib
)

add_dependencies(Editor CopyEngineLib CopyVulkanRenderLib CopyEditorLib)