
cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE C3DEngine_SRC "src/*.h" "src/*.cpp")

add_library(C3DEngine SHARED ${C3DEngine_SRC})

target_compile_definitions(C3DEngine PUBLIC C3D_EXPORT)
target_compile_options(C3DEngine PRIVATE /EHsc /wd4002)

target_link_libraries(C3DEngine PUBLIC fmt::fmt spdlog::spdlog glm::glm)

target_include_directories(C3DEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb" "${spdlog_SOURCE_DIR}/include" "${fmt_SOURCE_DIR}/include" "${glm_SOURCE_DIR}")
