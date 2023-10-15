
cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE C3DEngine_SRC "src/*.h" "src/*.cpp")

add_library(C3DEngine SHARED ${C3DEngine_SRC})

target_compile_definitions(C3DEngine PUBLIC C3D_EXPORT)
target_compile_options(C3DEngine PRIVATE /EHsc /wd4002)

Include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(spdlog glm)


target_link_libraries(C3DEngine PRIVATE spdlog::spdlog spdlog glm::glm)

target_include_directories(C3DEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb" "${spdlog_SOURCE_DIR}/include" "${glm_SOURCE_DIR}")
