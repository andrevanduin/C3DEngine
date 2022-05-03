
set(CMAKE_CXX_STANDARD 17)

add_library(assetlibrary STATIC "MeshAsset.h" "MeshAsset.cpp"  "AssetLoader.h" "AssetLoader.cpp" "TextureAsset.h" "TextureAsset.cpp" "MaterialAsset.h" "MaterialAsset.cpp" "PrefabAsset.h" "PrefabAsset.cpp")

target_include_directories(assetlibrary PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(lz4 CONFIG REQUIRED)
target_link_libraries(assetlibrary PRIVATE lz4::lz4)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(assetlibrary PRIVATE nlohmann_json::nlohmann_json)