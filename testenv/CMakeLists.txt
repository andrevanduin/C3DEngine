
cmake_minimum_required (VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

add_executable (TestEnv "src/main.cpp")

target_link_libraries(TestEnv PUBLIC C3DEngine)

add_custom_target(CopyDLL
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/engine/C3DEngine.dll" "${CMAKE_BINARY_DIR}/testenv"
)

add_dependencies(TestEnv CopyDLL)