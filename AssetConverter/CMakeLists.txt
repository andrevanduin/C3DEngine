
set(CMAKE_CXX_STANDARD 17)

add_executable(converter "main.cpp" "ConverterState.h" "ConverterState.cpp" "Gltf.h" "Gltf.cpp")

target_include_directories(converter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Add all the dependency pacakges
find_package(tinyobjloader CONFIG REQUIRED)
target_link_libraries(converter PRIVATE tinyobjloader::tinyobjloader)

target_link_libraries(converter INTERFACE stb_image::stb_image)

find_path(STB_INCLUDE_DIRS "stb.h")
target_include_directories(converter PRIVATE ${STB_INCLUDE_DIRS})

find_package(lz4 CONFIG REQUIRED)
target_link_libraries(converter PRIVATE lz4::lz4)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(converter PRIVATE nlohmann_json::nlohmann_json)

find_package(glm CONFIG REQUIRED)
target_link_libraries(converter PRIVATE glm::glm)

target_link_libraries(converter PRIVATE assetlibrary)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(converter PRIVATE ${TINYGLTF_INCLUDE_DIRS})

find_path(NVTT_INCLUDE_DIRS "nvtt.h")