# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd26812")

project ("C3DEngine")

# Define our compile definitions that are relevant for all targets
add_compile_definitions(C3D_PLATFORM_WINDOWS)
add_compile_definitions(C3D_MEMORY_METRICS)
# add_compile_definitions(C3D_MEMORY_METRICS_POINTERS)
add_compile_definitions(C3D_MEMORY_METRICS_MALLOC)

# Include sub-projects.
add_subdirectory ("tools")
add_subdirectory ("engine")
add_subdirectory ("plugins")
add_subdirectory ("testenv")
add_subdirectory ("tests")

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/assets/shaders/*.comp"
    )

message(STATUS "Building Shaders")
message(STATUS ${GLSL_VALIDATOR})

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  message(STATUS "Building ${FILE_NAME}")

  set(SPIRV "${PROJECT_SOURCE_DIR}/assets/shaders/${FILE_NAME}.spv")

  ##execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})