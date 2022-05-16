
target_sources(C3DEngine PRIVATE 
	"VkEngine.h" "VkEngine.cpp" "VkTextures.h" "VkTextures.cpp" "VkTypes.h"  "VkInitializers.h" "VkInitializers.cpp" 
	"VkDeletionQueue.h"  "VkFrame.h" "Logger.h" "Logger.cpp" "VkObjects.h" "Math.h" "Math.cpp"
	"Utils.h" "Utils.cpp" "VkPushBuffer.h" "VkPushBuffer.cpp" "Allocator.h" "Allocator.cpp")

add_subdirectory("shaders")
add_subdirectory("materials")
add_subdirectory("renderer")