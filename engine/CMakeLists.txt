
cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE C3DEngine_SRC "*.hpp" "*.cpp")

add_library(C3DEngine SHARED ${C3DEngine_SRC})

add_custom_target(CopyDLL
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/engine/C3DEngine.dll" "${CMAKE_BINARY_DIR}/testenv"
)

add_dependencies(C3DEngine CopyDLL)

add_compile_definitions(C3D_EXPORT)
add_compile_definitions(C3D_PLATFORM_WINDOWS)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(C3DEngine PUBLIC spdlog::spdlog spdlog::spdlog_header_only)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE SDL2::SDL2 SDL2::SDL2main)

target_include_directories(C3DEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")