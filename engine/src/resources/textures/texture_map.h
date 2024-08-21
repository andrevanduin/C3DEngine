
#pragma once
#include "resources/materials/material_types.h"
#include "texture_types.h"

namespace C3D
{
    struct Texture;

    struct TextureMap
    {
        TextureMap() = default;

        TextureMap(TextureFilter filter, TextureRepeat repeat)
            : minifyFilter(filter), magnifyFilter(filter), repeatU(repeat), repeatV(repeat), repeatW(repeat)
        {}

        TextureMap(const MaterialConfigMap& config)
            : minifyFilter(config.minifyFilter),
              magnifyFilter(config.magnifyFilter),
              repeatU(config.repeatU),
              repeatV(config.repeatV),
              repeatW(config.repeatW)
        {}

        /** @brief A handle to the corresponding texture. */
        TextureHandle texture = INVALID_ID;
        /** @brief Texture filtering mode for minification. */
        TextureFilter minifyFilter = TextureFilter::ModeLinear;
        /** @brief Texture filtering mode for magnification. */
        TextureFilter magnifyFilter = TextureFilter::ModeLinear;
        /** @brief Texture repeat mode on the U axis. */
        TextureRepeat repeatU = TextureRepeat::Repeat;
        /** @brief Texture repeat mode on the V axis. */
        TextureRepeat repeatV = TextureRepeat::Repeat;
        /** @brief Texture repeat mode on the W axis. */
        TextureRepeat repeatW = TextureRepeat::Repeat;
        /** @brief An id for internal render API-specific data. Typically the internal sampler. */
        u32 internalId = 0;
        /** @brief The amount of mip levels for this texture map.
         * This value should always be atleast 1 since we will always have atleast the base image.
         */
        u8 mipLevels = 1;
        /** @brief The generation for the assigned texture. Used to determine if we need to regenerate resources for this texture map.
         * For example when the mip level changes. */
        u32 generation = INVALID_ID;
    };
}  // namespace C3D