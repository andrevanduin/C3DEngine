
#include <glm/glm.hpp>

namespace C3D
{
	struct CullParams
	{
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		bool occlusionCull;
		bool frustumCull;
		bool aabb;

		float drawDistance;

		glm::vec3 aabbMin;
		glm::vec3 aabbMax;
	};
}