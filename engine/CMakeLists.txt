
cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE C3DEngine_SRC "*.h" "*.cpp")

add_library(C3DEngine SHARED ${C3DEngine_SRC})

add_compile_definitions(C3D_EXPORT)
add_compile_definitions(C3D_PLATFORM_WINDOWS)
add_compile_definitions(C3D_MEMORY_METRICS)
add_compile_definitions(C3D_MEMORY_METRICS_POINTERS)
add_compile_definitions(C3D_MEMORY_METRICS_MALLOC)
# add_compile_definitions(C3D_TRACE_ALLOCS)

find_package(Vulkan REQUIRED)
target_link_libraries(C3DEngine PUBLIC Vulkan::Vulkan)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(C3DEngine PUBLIC spdlog::spdlog spdlog::spdlog_header_only)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE SDL2::SDL2 SDL2::SDL2main)

find_package(Vulkan REQUIRED FATAL_ERROR)
target_link_libraries(C3DEngine PRIVATE Vulkan::Vulkan)

find_package(vk-bootstrap CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE vk-bootstrap::vk-bootstrap)

find_package(glm CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE glm::glm)

find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")
target_include_directories(C3DEngine PRIVATE ${STB_INCLUDE_DIRS})

target_include_directories(C3DEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")