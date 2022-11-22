
#pragma once
#include "containers/hash_table.h"
#include "containers/dynamic_array.h"
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
		u8 nameLength;
		char* name;
		u8 size;
		ShaderAttributeType type;
	};

	/* @brief Configuration for a uniform. */
	struct ShaderUniformConfig
	{
		u8 nameLength;
		char* name;
		u8 size;
		u32 location;
		ShaderUniformType type;
		ShaderScope scope;
	};

	struct ShaderConfig
	{
		char* name;

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
		char* name;
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
		u32 id;
		char* name;

		ShaderFlagBits flags;

		u64 requiredUboAlignment;
		u64 globalUboSize;
		u64 globalUboStride;
		u64 globalUboOffset;

		u64 uboSize;
		u64 uboStride;
		u16 attributeStride;

		DynamicArray<TextureMap*> globalTextureMaps;
		u64 instanceTextureCount;

		ShaderScope boundScope;
		u32 boundInstanceId;
		u32 boundUboOffset;

		HashTable<u16> uniformLookup;

		DynamicArray<ShaderUniform> uniforms;
		DynamicArray<ShaderAttribute> attributes;

		u64 pushConstantSize;
		u64 pushConstantStride;
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