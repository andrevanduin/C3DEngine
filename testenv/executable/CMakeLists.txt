
cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 23)

add_executable (TestEnv "src/main.cpp")

target_link_libraries(TestEnv PUBLIC C3DEngine)

if (WIN32)
	target_link_options(TestEnv PUBLIC /MANIFEST:NO)
endif()

add_custom_target(CopyEngineDLL
	COMMAND ${CMAKE_COMMAND} -E copy 
	"${CMAKE_BINARY_DIR}/engine/${CMAKE_SHARED_LIBRARY_PREFIX}C3DEngine${CMAKE_SHARED_LIBRARY_SUFFIX}" 
	"${CMAKE_BINARY_DIR}/testenv/executable/"
	DEPENDS C3DEngine
)

add_custom_target(CopyVulkanRenderDLL
	COMMAND ${CMAKE_COMMAND} -E copy 
	"${CMAKE_BINARY_DIR}/plugins/renderers/vulkan_renderer/${CMAKE_SHARED_LIBRARY_PREFIX}C3DVulkanRenderer${CMAKE_SHARED_LIBRARY_SUFFIX}" 
	"${CMAKE_BINARY_DIR}/testenv/executable/"
	DEPENDS C3DVulkanRenderer
)

add_custom_target(CopyGameDLL
	COMMAND ${CMAKE_COMMAND} -E copy
	"${CMAKE_BINARY_DIR}/testenv/lib/${CMAKE_SHARED_LIBRARY_PREFIX}TestEnvLib${CMAKE_SHARED_LIBRARY_SUFFIX}" 
	"${CMAKE_BINARY_DIR}/testenv/executable/"
	DEPENDS TestEnvLib
)

add_dependencies(TestEnv CopyEngineDLL CopyVulkanRenderDLL CopyGameDLL)