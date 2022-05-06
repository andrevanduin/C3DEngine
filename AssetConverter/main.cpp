
#include <iostream>
#include <filesystem>

#include "Gltf.h"

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "No Path to info file was provided" << std::endl;
		return -1;
	}

	GltfConverter converter;

	const std::filesystem::path directory{ argv[1] };
	const auto exportedDir = directory.parent_path() / "exportedAssets";

	std::cout << "Loaded asset directory at: " << directory << std::endl;

	ConverterState state = { directory, exportedDir };

	for (auto& p : std::filesystem::recursive_directory_iterator(directory))
	{
		std::cout << "File: " << p << std::endl;

		auto relative = p.path().lexically_proximate(directory);
		auto exportPath = exportedDir / relative;

		if (!is_directory(exportPath.parent_path()))
		{
			create_directory(exportPath.parent_path());
		}

		if (p.path().extension() == ".gltf")
		{
			converter.LoadFromAscii(p, exportPath);
		}
	}

	return 0;
}
