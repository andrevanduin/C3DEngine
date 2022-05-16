
#include "../VkTypes.h"

namespace C3D
{
	struct DepthPyramid
	{
		AllocatedImage image;
		VkImageView mips[16] = {};

		uint32_t width, height, levels;
	};
}