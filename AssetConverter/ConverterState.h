
#pragma once
#include <filesystem>

struct ConverterState
{
	std::filesystem::path assetPath;
	std::filesystem::path exportPath;

	[[nodiscard]] std::filesystem::path ConvertToExportRelative(const std::filesystem::path& path) const;
};