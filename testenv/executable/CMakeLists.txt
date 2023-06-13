
cmake_minimum_required (VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)

add_executable (TestEnv "src/main.cpp")

target_link_libraries(TestEnv PUBLIC C3DEngine)
target_link_options(TestEnv PUBLIC /MANIFEST:NO)

add_custom_target(CopyEngineDLLTestEnv
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/engine/C3DEngine.dll" "${CMAKE_BINARY_DIR}/testenv/executable"
	DEPENDS C3DEngine
)

add_custom_target(CopyVulkanRenderDLLTestEnv
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/plugins/renderers/vulkan_renderer/C3DVulkanRenderer.dll" "${CMAKE_BINARY_DIR}/testenv/executable"
	DEPENDS C3DVulkanRenderer
)

add_custom_target(CopyGameLib
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/testenv/lib/TestEnvLib.dll" "${CMAKE_BINARY_DIR}/testenv/executable"
	DEPENDS TestEnvLib
)

add_dependencies(TestEnv Shaders CopyEngineDLLTestEnv CopyVulkanRenderDLLTestEnv CopyGameLib)