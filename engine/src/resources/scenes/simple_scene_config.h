
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/defines.h"
#include "math/math_types.h"
#include "renderer/transform.h"
#include "resources/resource_types.h"

namespace C3D
{
    struct SimpleSceneSkyboxConfig
    {
        String name;
        String cubemapName;
    };

    struct SimpleSceneDirectionalLightConfig
    {
        String name;
        vec4 color                  = vec4(1.0);
        vec4 direction              = vec4(0);  // Ignore w (only for 16 byte alignment)
        float shadowDistance        = 200.0f;
        float shadowFadeDistance    = 20.0f;
        float shadowSplitMultiplier = 0.95f;
    };

    struct SimpleSceneTerrainConfig
    {
        String name;
        String resourceName;
        Transform transform;
    };

    struct SimpleScenePointLightConfig
    {
        String name;
        vec4 color;
        vec4 position;
        f32 constant;
        f32 linear;
        f32 quadratic;
    };

    struct SimpleSceneMeshConfig
    {
        String name;
        String resourceName;
        Transform transform;
        String parentName;  // optional
    };

    struct SimpleSceneConfig final : C3D::Resource
    {
        String description;
        SimpleSceneSkyboxConfig skyboxConfig;
        SimpleSceneDirectionalLightConfig directionalLightConfig;

        DynamicArray<SimpleScenePointLightConfig> pointLights;
        DynamicArray<SimpleSceneMeshConfig> meshes;
        DynamicArray<SimpleSceneTerrainConfig> terrains;
    };
}  // namespace C3D