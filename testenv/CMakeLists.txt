
cmake_minimum_required (VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)

add_executable (TestEnv "src/main.cpp" "src/game.h" "src/game.cpp")

target_link_libraries(TestEnv PUBLIC C3DEngine)
target_link_libraries(TestEnv PUBLIC C3DVulkanRenderer)
target_link_options(TestEnv PUBLIC /MANIFEST:NO)

add_custom_target(CopyEngineDLLTestEnv
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/engine/C3DEngine.dll" "${CMAKE_BINARY_DIR}/testenv"
	DEPENDS C3DEngine
)

add_custom_target(CopyVulkanRenderDLLTestEnv
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/vulkan_renderer/C3DVulkanRenderer.dll" "${CMAKE_BINARY_DIR}/testenv"
	DEPENDS C3DVulkanRenderer
)

add_dependencies(TestEnv Shaders)
add_dependencies(TestEnv CopyEngineDLLTestEnv)
add_dependencies(TestEnv CopyVulkanRenderDLLTestEnv)