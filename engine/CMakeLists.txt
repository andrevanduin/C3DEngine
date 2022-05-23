
cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE C3DEngine_SRC "*.hpp" "*.cpp")

add_library(C3DEngine SHARED ${C3DEngine_SRC})

add_compile_definitions(CEXPORT)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(C3DEngine PUBLIC spdlog::spdlog spdlog::spdlog_header_only)

target_include_directories(C3DEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")