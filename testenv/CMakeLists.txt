
cmake_minimum_required (VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

add_executable (TestEnv "src/main.cpp" "src/game.h" "src/game.cpp")

target_link_libraries(TestEnv PUBLIC C3DEngine)

add_dependencies(TestEnv C3DEngine)
