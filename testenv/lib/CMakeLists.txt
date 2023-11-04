
cmake_minimum_required (VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE TestEnvLib_SRC "*.h" "*.cpp")

add_library(TestEnvLib SHARED ${TestEnvLib_SRC})

if (WIN32)
    target_link_options(TestEnvLib PUBLIC /MANIFEST:NO)
    set_target_properties(TestEnvLib PROPERTIES PDB_NAME "TestEnvLib${RANDOM_PDB_VERSION}")
endif()

target_compile_definitions(TestEnvLib PUBLIC C3D_EXPORT)

target_link_libraries(TestEnvLib PUBLIC C3DEngine)

target_include_directories(TestEnvLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")