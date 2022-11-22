
#pragma once
#include "core/defines.h"
#include "memory.h"
#include "containers/string.h"

namespace C3D
{
	class Utils
	{
		inline static const char* memoryTypeStrings[static_cast<u8>(MemoryType::MaxType)] =
		{
			"Unknown          ",
			"Dynamic_Allocator",
			"Linear_Allocator ",
			"FreeList         ",
			"Array            ",
			"DynamicArray     ",
			"HashTable        ",
			"HashMap          ",
			"RingQueue        ",
			"Bst              ",
			"String           ",
			"C3DString        ",
			"Application      ",
			"ResourceLoader   ",
			"Job              ",
			"Texture          ",
			"MaterialInstance ",
			"Geometry         ",
			"RenderSystem     ",
			"Game             ",
			"Transform        ",
			"Entity           ",
			"EntityNode       ",
			"Scene            ",
			"Shader           ",
			"Resource         ",
			"Vulkan           ",
			"VulkanExternal   ",
			"Direct3D         ",
			"OpenGL           ",
			"GpuLocal         ",
			"BitmapFont       ",
			"SystemFont       ",
		};

		static String SizeToText(const u64 size)
		{
			constexpr static auto gb = GibiBytes(1);
			constexpr static auto mb = MebiBytes(1);
			constexpr static auto kb = KibiBytes(1);

			f64 amount = static_cast<f64>(size);
			if (size >= gb)
			{
				amount /= static_cast<f64>(gb);
				return String::FromFormat("{:.4}GB", amount);
			}
			if (size >= mb)
			{
				amount /= static_cast<f64>(mb);
				return String::FromFormat("{:.4}MB", amount);
			}
			if (size >= kb)
			{
				amount /= static_cast<f64>(kb);
				return String::FromFormat("{:.4}KB", amount);
			}

			return String::FromFormat("{}B", size);
		}
	public:
		static String GenerateMemoryUsageString(const MemorySystem& memorySystem)
		{
			auto taggedAllocations = memorySystem.GetTaggedAllocations();
			const auto freeSpace = memorySystem.GetFreeSpace();
			const auto totalSpace = memorySystem.GetTotalUsableSpace();

			String str;
			str.Reserve(2000);
			str.Append("System's Dynamic Memory usage:\n");
			
			for (auto i = 0; i < ToUnderlying(MemoryType::MaxType); i++)
			{
				const auto [size, count] = taggedAllocations[i];
				str.Append(String::FromFormat("  {} - ({:0>3}) {}\n", memoryTypeStrings[i], count, SizeToText(size)));
			}

			const auto usedSpace = totalSpace - freeSpace;
			const auto percentage = static_cast<f32>(usedSpace) / static_cast<f32>(totalSpace) * 100.0f;

			str += String::FromFormat("Using {} out of {} total ({:.3}% used)", SizeToText(usedSpace), SizeToText(totalSpace), percentage);

			return str;
		}
	};
}
