
cmake_minimum_required (VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)

add_executable (TestEnv "src/main.cpp" "src/game.h" "src/game.cpp")

target_link_libraries(TestEnv PUBLIC C3DEngine)

find_package(Vulkan REQUIRED)
target_link_libraries(TestEnv PRIVATE Vulkan::Vulkan)

add_custom_target(CopyDLL
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/engine/C3DEngine.dll" "${CMAKE_BINARY_DIR}/testenv"
	DEPENDS C3DEngine
)

add_dependencies(TestEnv CopyDLL)
add_dependencies(TestEnv Shaders)