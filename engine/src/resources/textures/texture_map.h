
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

        /** @brief Pointer to the corresponding texture. */
        Texture* texture = nullptr;
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
        /** @brief A pointer to internal, render API-specific data. Typically the internal sampler. */
        void* internalData = nullptr;
    };
}  // namespace C3D