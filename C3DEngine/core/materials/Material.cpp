
#include "Material.h"

namespace C3D
{
	bool MaterialData::operator==(const MaterialData& other) const
	{
		if (other.baseTemplate != baseTemplate || other.parameters != parameters || other.textures.size() != textures.size())
		{
			return false;
		}

		// Compare the actual binary data
		return memcmp(other.textures.data(), textures.data(), textures.size() * sizeof(textures[0])) == 0;
	}

	size_t MaterialData::Hash() const
	{
		auto result = std::hash<std::string>()(baseTemplate);
		for (auto& [sampler, view] : textures)
		{
			size_t textureHash = 
				(std::hash<size_t>()(reinterpret_cast<size_t>(sampler)) << 3) &
				(std::hash<size_t>()(reinterpret_cast<size_t>(view)) >> 7);

			result ^= std::hash<size_t>()(textureHash);
		}
		return result;
	}
}
