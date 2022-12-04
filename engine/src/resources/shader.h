
#pragma once
#include "containers/hash_table.h"
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "containers/string.h"

#include "renderer/renderer_types.h"

namespace C3D
{
	struct Texture;

	enum ShaderAttributeType : u8
	{
		Attribute_Float32, Attribute_Float32_2, Attribute_Float32_3, Attribute_Float32_4, Attribute_Matrix4,
		Attribute_Int8, Attribute_UInt8, Attribute_Int16, Attribute_UInt16,
		Attribute_Int32, Attribute_UInt32
	};

	enum ShaderUniformType : u8
	{
		Uniform_Float32, Uniform_Float32_2, Uniform_Float32_3, Uniform_Float32_4,
		Uniform_Int8, Uniform_UInt8,Uniform_Int16, Uniform_UInt16, Uniform_Int32, Uniform_UInt32,
		Uniform_Matrix4, Uniform_Sampler, Uniform_Custom = 255
	};

	/* @brief the different possible scopes in a shader. */
	enum class ShaderScope
	{
		Global,
		Instance,
		Local,
	};

	/* @brief Configuration for an attribute. */
	struct ShaderAttributeConfig
	{
		String name;

		u8 size;
		ShaderAttributeType type;
	};

	/* @brief Configuration for a uniform. */
	struct ShaderUniformConfig
	{
		String name;

		u8 size;
		u32 location;

		ShaderUniformType type;
		ShaderScope scope;
	};

	struct ShaderConfig
	{
		String name;

		/* @brief The face cull mode to be used. Default is BACK if not supplied. */
		FaceCullMode cullMode;

		DynamicArray<ShaderAttributeConfig> attributes;
		DynamicArray<ShaderUniformConfig> uniforms;

		DynamicArray<ShaderStage> stages;

		DynamicArray<String> stageNames;
		DynamicArray<String> stageFileNames;

		// TODO: Convert these bools to flags to save space
		/* @brief Indicates if depth testing should be done by this shader. */
		bool depthTest;
		/* @brief Indicates if depth writing should be done by this shader (this is ignored if depthTest = false). */
		bool depthWrite;
	};

	enum class ShaderState : u8
	{
		NotCreated, Uninitialized, Initialized
	};

	struct ShaderUniform
	{
		u64 offset;
		u16 location;
		u16 index;
		u16 size;
		u8 setIndex;
		ShaderScope scope;
		ShaderUniformType type;
	};

	struct ShaderAttribute
	{
		String name;
		ShaderAttributeType type;
		u32 size;
	};

	enum ShaderFlags
	{
		ShaderFlagNone			= 0x0,
		ShaderFlagDepthTest		= 0x1,
		ShaderFlagDepthWrite	= 0x2,
	};
	typedef u32 ShaderFlagBits;

	struct Shader
	{
		Shader()
			: id(INVALID_ID), flags(ShaderFlagNone), requiredUboAlignment(0), globalUboSize(0), globalUboStride(0),
			  globalUboOffset(0), uboSize(0), uboStride(0), attributeStride(0), instanceTextureCount(0),
			  boundScope(), boundInstanceId(INVALID_ID), boundUboOffset(0), pushConstantSize(0),
			  pushConstantRangeCount(0), pushConstantRanges{}, state(ShaderState::Uninitialized), frameNumber(INVALID_ID_U64),
			  apiSpecificData(nullptr)
		{}

		u32 id;
		String name;

		ShaderFlagBits flags;

		u64 requiredUboAlignment;
		// A running total of the size of the global uniform buffer object
		u64 globalUboSize;
		u64 globalUboStride;
		u64 globalUboOffset;

		// A running total of the size of the instance uniform buffer object
		u64 uboSize;
		u64 uboStride;
		u16 attributeStride;

		DynamicArray<TextureMap*> globalTextureMaps;
		u64 instanceTextureCount;

		ShaderScope boundScope;
		u32 boundInstanceId;
		u32 boundUboOffset;

		HashMap<String, ShaderUniform> uniforms;
		DynamicArray<ShaderAttribute> attributes;

		u64 pushConstantSize;
		// Note: this is hard-coded because the vulkan spec only guarantees a minimum 128bytes stride
		// The drivers might allocate more but this is not guaranteed on all video cards
		u64 pushConstantStride = 128;
		u8 pushConstantRangeCount;
		Range pushConstantRanges[32];

		ShaderState state;

		// @brief used to ensure the shaders globals are only updated once per frame.
		u64 frameNumber;

		// A pointer to the Renderer API specific data
		// This memory needs to be managed separately by the current rendering API backend
		void* apiSpecificData;
	};
}