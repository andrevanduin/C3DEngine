
#include "bitmap_font_loader.h"

#include <sstream>

#include "containers/string.h"
#include "platform/filesystem.h"
#include "systems/resource_system.h"

namespace C3D
{
	// These are in order of priority to be looked up (since we would much rather load our custom binary format)
	constexpr static SupportedBitmapFontFileType SUPPORTED_FILE_TYPES[2] =
	{
		{ ".cbf", BitmapFontFileType::CBF, true },
		{ ".fnt", BitmapFontFileType::FNT, false },
	};

	ResourceLoader<BitmapFontResource>::ResourceLoader()
		: IResourceLoader("BITMAP_FONT_LOADER", MemoryType::BitmapFont, ResourceType::BitmapFont, nullptr, "fonts")
	{}

	bool ResourceLoader<BitmapFontResource>::Load(const char* name, BitmapFontResource* outResource) const
	{
		File file;

		String filepath;
		auto type = BitmapFontFileType::NotFound;

		// Try our supported file types in order
		for (auto& supportedType : SUPPORTED_FILE_TYPES)
		{
			filepath = String::FromFormat("{}/{}/{}{}", Resources.GetBasePath(), typePath, name, supportedType.extension);
			if (File::Exists(filepath))
			{
				const u8 mode = FileModeRead | (supportedType.isBinary ? FileModeBinary : 0);
				if (file.Open(filepath, mode))
				{
					// If we find a file that exists and is readable we stop looking.
					type = supportedType.type;
					break;
				}
			}
		}

		if (type == BitmapFontFileType::NotFound)
		{
			m_logger.Error("Load() - Unable to find bitmap font of supported type called: '{}'.", name);
			return false;
		}

		outResource->fullPath = filepath;
		outResource->data.type = FontType::Bitmap;

		auto result = false;
		switch (type)
		{
			case BitmapFontFileType::FNT:
				filepath.RemoveLast(3); // Remove "fnt"
				filepath.Append("cbf");
				result = ImportFntFile(file, filepath, outResource);
				break;
			case BitmapFontFileType::CBF:
				result = ReadCbfFile(file, outResource);
				break;
			case BitmapFontFileType::NotFound:
				m_logger.Error("Load() - Unable to find bitmap font of supported type called: '{}.'", name);
				break;
		}

		file.Close();

		if (!result)
		{
			m_logger.Error("Load() - Failed to process bitmap font file: '{}'.", filepath);
			return false;
		}

		return true;
	}

	void ResourceLoader<BitmapFontResource>::Unload(BitmapFontResource* resource)
	{
		if (resource)
		{
			resource->data.glyphs.Destroy();
			resource->data.kernings.Destroy();
			resource->pages.Destroy();
			resource->loaderId = INVALID_ID;

			resource->fullPath.Destroy();
			resource->name.Destroy();
		}
	}

#define VERIFY_PARSE(lineType, lineNumber, result)														\
	if (!(result))																						\
	{																									\
		m_logger.Error("Error in file format reading type: '{}', line: {}." lineType, lineNumber);		\
		return false;																					\
	}

	bool ResourceLoader<BitmapFontResource>::ImportFntFile(File& file, const String& outCbfFilename, BitmapFontResource* data) const
	{
		String line;
		line.Reserve(512);

		u32 lineNumber = 0;
		while (file.ReadLine(line))
		{
			lineNumber++;

			// Trim line
			line.Trim();

			// Skip blank lines.
			if (line.Empty()) continue;

			switch (line.First())
			{
				case 'i':
					VERIFY_PARSE("Info", lineNumber, ParseInfoLine(line, data));
					break;
				case 'c':
					// Can be 'common', 'char' or 'chars' line
					if (line[1] == 'o') 
					{
						// Line should be common
						VERIFY_PARSE("Common", lineNumber, ParseCommonLine(line, data));
					}
					else if (line[4] == 's')
					{
						// Line should be chars
						VERIFY_PARSE("Chars", lineNumber, ParseCharsLine(line, data));
					}
					else if (line[4] == ' ')
					{
						// Line should be char
						VERIFY_PARSE("Char", lineNumber, ParseCharLine(line, data));
					}
					else
					{
						m_logger.Error("Error in file format reading: '{}'.", line);
						return false;
					}
					break;
				case 'p':
					VERIFY_PARSE("Page", lineNumber, ParsePageLine(line, data));
					break;
				case 'k':
					if (line[7] == 's')
					{
						VERIFY_PARSE("Kernings", lineNumber, ParseKerningsLine(line, data));
					}
					else
					{
						VERIFY_PARSE("Kerning", lineNumber, ParseKerningLine(line, data));
					}
					break;
				default:
					m_logger.Warn("Invalid starting sequence on line: '{}'.", line);
					break;
;			}
		}

		return WriteCbfFile(outCbfFilename, data);
	}

	bool inline ParseElementAndVerify(const String& element, const char* name, DynamicArray<String>& out)
	{
		if (!element.StartsWith(name)) return false;
		out = element.Split('=');
		return out.Size() == 2;
	}

	bool ResourceLoader<BitmapFontResource>::ParseInfoLine(const String& line, BitmapFontResource* res)
	{
		auto elements = line.Split('\"');

		StringNCopy(res->data.face, elements[1].Data(), FONT_DATA_FACE_MAX_LENGTH);

		elements = elements[2].Split(' ');
		DynamicArray<String> values;

		if (!ParseElementAndVerify(elements[0], "size", values)) return false;

		res->data.size = values[1].ToU32();
		return true;
	}

	bool ResourceLoader<BitmapFontResource>::ParseCommonLine(const String& line, BitmapFontResource* res) const
	{
		const auto elements = line.Split(' ');
		DynamicArray<String> values;

		if (!ParseElementAndVerify(elements[1], "lineHeight", values)) return false;
		res->data.lineHeight = values[1].ToI32();

		if (!ParseElementAndVerify(elements[2], "base", values)) return false;
		res->data.baseline = values[1].ToI32();

		if (!ParseElementAndVerify(elements[3], "scaleW", values)) return false;
		res->data.atlasSizeX = values[1].ToU32();

		if (!ParseElementAndVerify(elements[4], "scaleH", values)) return false;
		res->data.atlasSizeY = values[1].ToU32();

		if (!ParseElementAndVerify(elements[5], "pages", values)) return false;

		const auto pageCount = values[1].ToU32();
		if (pageCount != 1)
		{
			m_logger.Error("Error in file. Page count is {} but is expected to be 1.", pageCount);
			return false;
		}

		res->pages.Reserve(pageCount);

		return true;
	}

	bool ResourceLoader<BitmapFontResource>::ParseCharsLine(const String& line, BitmapFontResource* res) const
	{
		const auto elements = line.Split(' ');
		DynamicArray<String> values;

		if (!ParseElementAndVerify(elements[1], "count", values)) return false;

		const auto glyphCount = values[1].ToU32();
		if (glyphCount == 0)
		{
			m_logger.Error("Error in file. Glyph count is 0 but is expected to be > 0");
			return false;
		}

		res->data.glyphs.Reserve(glyphCount);
		return true;
	}

	bool ResourceLoader<BitmapFontResource>::ParseCharLine(const String& line, BitmapFontResource* res)
	{
		const auto elements = line.Split(' ');
		DynamicArray<String> values;

		FontGlyph g{};

		if (!ParseElementAndVerify(elements[1], "id", values)) return false;
		g.codepoint = values[1].ToI32();

		if (!ParseElementAndVerify(elements[2], "x", values)) return false;
		g.x = values[1].ToU16();

		if (!ParseElementAndVerify(elements[3], "y", values)) return false;
		g.y = values[1].ToU16();
		
		if (!ParseElementAndVerify(elements[4], "width", values)) return false;
		g.width = values[1].ToU16();

		if (!ParseElementAndVerify(elements[5], "height", values)) return false;
		g.height = values[1].ToU16();

		if (!ParseElementAndVerify(elements[6], "xoffset", values)) return false;
		g.xOffset = values[1].ToI16();

		if (!ParseElementAndVerify(elements[7], "yoffset", values)) return false;
		g.yOffset = values[1].ToI16();

		if (!ParseElementAndVerify(elements[8], "xadvance", values)) return false;
		g.xAdvance = values[1].ToI16();

		if (!ParseElementAndVerify(elements[9], "page", values)) return false;
		g.pageId = values[1].ToU8();

		res->data.glyphs.PushBack(g);
		return true;
	}

	bool ResourceLoader<BitmapFontResource>::ParsePageLine(const String& line, BitmapFontResource* res)
	{
		const auto elements = line.Split('\"');
		DynamicArray<String> values;

		if (elements.Size() != 2) return false;
		
		const auto idElements = elements[0].Split(' ');
		if (idElements.Size() != 3) return false;

		if (!ParseElementAndVerify(idElements[1], "id", values)) return false;

		BitmapFontPage p{};
		p.id = values[1].ToI8();
		StringNCopy(p.file, elements[1].Data(), FONT_DATA_FACE_MAX_LENGTH);

		res->pages.PushBack(p);
		return true;
	}

	bool ResourceLoader<BitmapFontResource>::ParseKerningsLine(const String& line, BitmapFontResource* res) const
	{
		const auto elements = line.Split(' ');
		DynamicArray<String> values;

		if (!ParseElementAndVerify(elements[1], "count", values)) return false;

		const auto kerningCount = values[1].ToU32();
		if (kerningCount == 0)
		{
			m_logger.Error("Error in file. Kerning count is 0 but is expected to be > 0");
			return false;
		}

		res->data.kernings.Reserve(kerningCount);
		return true;
	}

	bool ResourceLoader<BitmapFontResource>::ParseKerningLine(const String& line, BitmapFontResource* res)
	{
		const auto elements = line.Split(' ');
		DynamicArray<String> values;

		FontKerning k{};

		if (!ParseElementAndVerify(elements[1], "first", values)) return false;
		k.codepoint0 = values[1].ToI32();

		if (!ParseElementAndVerify(elements[2], "second", values)) return false;
		k.codepoint1 = values[1].ToI32();

		if (!ParseElementAndVerify(elements[3], "amount", values)) return false;
		k.amount = values[1].ToI16();

		res->data.kernings.PushBack(k);
		return true;
	}

	bool ResourceLoader<BitmapFontResource>::ReadCbfFile(File& file, BitmapFontResource* res) const
	{
		ResourceHeader header = {};
		file.Read(&header);

		if (header.magicNumber != BINARY_RESOURCE_FILE_MAGIC_NUMBER || header.resourceType != ToUnderlying(ResourceType::BitmapFont))
		{
			m_logger.Error("CBF file header is invalid. The file can not be properly read");
			return false;
		}

		// TODO: Read/Process the file version

		// Face string
		u64 faceLength = 0;
		file.Read(&faceLength);
		file.Read(res->data.face, faceLength + 1);
		// Ensure null terminator
		res->data.face[faceLength] = '\0';

		// Font size
		file.Read(&res->data.size);

		// Line height
		file.Read(&res->data.lineHeight);

		// Baseline
		file.Read(&res->data.baseline);

		// SizeX
		file.Read(&res->data.atlasSizeX);

		// SizeY
		file.Read(&res->data.atlasSizeY);

		// Pages
		u64 pageCount;
		file.Read(&pageCount);
		res->pages.Reserve(pageCount);
		for (u64 i = 0; i < pageCount; i++)
		{
			BitmapFontPage page{};

			// Page id
			file.Read(&page.id);

			// Page file name
			u64 pageFilenameLength;
			// Get the length of the name
			file.Read(&pageFilenameLength);
			// Actually read the name
			file.Read(page.file, pageFilenameLength + 1);
			// Ensure '\0' termination
			page.file[pageFilenameLength] = '\0';

			res->pages.PushBack(page);
		}

		// Both of these are only simple data so we can read them directly
		// Glyphs
		file.Read(res->data.glyphs);
		// Kernings
		file.Read(res->data.kernings);

		file.Close();
		return true;
	}

	bool ResourceLoader<BitmapFontResource>::WriteCbfFile(const String& path, const BitmapFontResource* res) const
	{
		File file;
		if (!file.Open(path, FileModeWrite | FileModeBinary))
		{
			m_logger.Error("Failed to open file for writing: '{}'.", path);
			return false;
		}

		ResourceHeader header = {};
		header.magicNumber = BINARY_RESOURCE_FILE_MAGIC_NUMBER;
		header.resourceType = ToUnderlying(ResourceType::BitmapFont);
		header.version = 0x01;

		// Header
		file.Write(&header);
		// Face string
		const u64 length = StringLength(res->data.face);
		file.Write(&length);
		file.Write(res->data.face, length + 1); // Account for the '\0' character
		// Font size
		file.Write(&res->data.size);
		// Line height
		file.Write(&res->data.lineHeight);
		// Baseline
		file.Write(&res->data.baseline);
		// SizeX
		file.Write(&res->data.atlasSizeX);
		// SizeY
		file.Write(&res->data.atlasSizeY);
		// Page count
		const auto pageCount = res->pages.Size();
		file.Write(&pageCount);

		// The pages
		for (auto& page : res->pages)
		{
			// Page id
			file.Write(&page.id);
			// File name
			const u64 pageFileLength = StringLength(page.file);
			file.Write(&pageFileLength);
			file.Write(page.file, pageFileLength + 1); // Account for the '\0' character
		}

		// Write out the glyphs which we can do directly since it's just simple data
		file.Write(res->data.glyphs);

		// Write out the kernings which we can do directly since it's just simple data
		file.Write(res->data.kernings);

		file.Close();
		return true;
	}
}
