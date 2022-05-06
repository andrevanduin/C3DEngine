
#include "ConverterState.h"

std::filesystem::path ConverterState::ConvertToExportRelative(const std::filesystem::path& path) const
{
	return path.lexically_proximate(exportPath);
}
