
cmake_minimum_required (VERSION 3.13)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE TestEnvLib_SRC "*.h" "*.cpp")

add_library(TestEnvLib SHARED ${TestEnvLib_SRC})

target_compile_definitions(TestEnvLib PUBLIC C3D_EXPORT)

# add_custom_target(GenerateVersion COMMAND VersionGen > jan.txt)

# add_dependencies(TestEnvLib GenerateVersion)

# file (READ "${CMAKE_BINARY_DIR}/testenv/lib/jan.txt" VER)

set_target_properties(TestEnvLib PROPERTIES PDB_NAME "${CMAKE_BINARY_DIR}/testenv/lib/jan.txt")

target_link_libraries(TestEnvLib PUBLIC C3DEngine)

target_include_directories(TestEnvLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")