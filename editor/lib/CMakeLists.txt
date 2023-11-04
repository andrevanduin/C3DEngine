
cmake_minimum_required (VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE EditorLib_SRC "*.h" "*.cpp")

add_library(EditorLib SHARED ${EditorLib_SRC})

if (WIN32)
    target_link_options(EditorLib PUBLIC /MANIFEST:NO)
    set_target_properties(EditorLib PROPERTIES PDB_NAME "EditorLib${RANDOM_PDB_VERSION}")
endif()

target_compile_definitions(EditorLib PUBLIC C3D_EXPORT)

target_link_libraries(EditorLib PUBLIC C3DEngine)

target_include_directories(EditorLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")