# CMakeList.txt : CMake project for C3DEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

find_package(Vulkan REQUIRED)

# Add source to this project's executable.
add_executable (C3DEngine "main.cpp" "core/VkEngine.h" "core/VkEngine.cpp" "core/VkTextures.h" "core/VkTextures.cpp" "core/VkTypes.h" "core/VkInitializers.h" "core/VkInitializers.cpp" "core/VkPipeline.h" "core/VkPipeline.cpp" "core/VkDeletionQueue.h"  "core/VkMesh.h" "core/VkMesh.cpp" "core/VkRenderObject.h"  "core/VkFrame.h" "core/Logger.h" "core/Logger.cpp" "core/VkObjects.h" "core/Utils.h" "core/Utils.cpp")

# Add all the dependency pacakges
find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE SDL2::SDL2 SDL2::SDL2main)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE imgui::imgui)

find_package(glm CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE glm::glm)

find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE unofficial::vulkan-memory-allocator::vulkan-memory-allocator)

find_package(vk-bootstrap CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE vk-bootstrap::vk-bootstrap)

find_package(tinyobjloader CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE tinyobjloader::tinyobjloader)

target_link_libraries(C3DEngine INTERFACE stb_image::stb_image)

find_path(STB_INCLUDE_DIRS "stb.h")
target_include_directories(C3DEngine PRIVATE ${STB_INCLUDE_DIRS})

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(C3DEngine PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

add_dependencies(C3DEngine Shaders)