
cmake_minimum_required (VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)

include_directories(TextureTools "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

add_executable (TextureTools "src/main.cpp")

target_link_libraries(TextureTools PUBLIC C3DEngine)

target_include_directories(TextureTools PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

add_custom_target(CopyEngineDLLTextureTools
	COMMAND ${CMAKE_COMMAND} -E copy 
	"${CMAKE_BINARY_DIR}/engine/${CMAKE_SHARED_LIBRARY_PREFIX}C3DEngine${CMAKE_SHARED_LIBRARY_SUFFIX}" 
	"${CMAKE_BINARY_DIR}/tools/texture_tools/"
	DEPENDS C3DEngine
)

add_dependencies(TextureTools CopyEngineDLLTextureTools)