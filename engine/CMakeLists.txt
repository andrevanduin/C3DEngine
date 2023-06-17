
cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_TOOLCHAIN_FILE "E:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake")

file(GLOB_RECURSE C3DEngine_SRC "*.h" "*.cpp")

add_library(C3DEngine SHARED ${C3DEngine_SRC})

target_compile_definitions(C3DEngine PUBLIC C3D_EXPORT)
target_compile_options(C3DEngine PRIVATE /EHsc /wd4002)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(C3DEngine PUBLIC spdlog::spdlog spdlog::spdlog_header_only)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE SDL2::SDL2 SDL2::SDL2main)

find_package(glm CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE glm::glm)

find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")
target_include_directories(C3DEngine PRIVATE ${STB_INCLUDE_DIRS})

target_include_directories(C3DEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
