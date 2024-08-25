
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/defines.h"
#include "math/math_types.h"
#include "renderer/transform.h"
#include "resources/resource_types.h"

namespace C3D
{
    struct SceneSkyboxConfig
    {
        String name;
        String cubemapName;
    };

    struct SceneDirectionalLightConfig
    {
        String name;
        vec4 color                  = vec4(1.0);
        vec4 direction              = vec4(0);  // Ignore w (only for 16 byte alignment)
        float shadowDistance        = 200.0f;
        float shadowFadeDistance    = 20.0f;
        float shadowSplitMultiplier = 0.95f;
    };

    struct SceneTerrainConfig
    {
        String name;
        String resourceName;
        Transform transform;
    };

    struct ScenePointLightConfig
    {
        String name;
        vec4 color;
        vec4 position;
        f32 constant;
        f32 linear;
        f32 quadratic;
    };

    struct SceneMeshConfig
    {
        String name;
        String resourceName;
        Transform transform;
        String parentName;  // optional
    };

    struct SceneConfig final : public IResource
    {
        SceneConfig() : IResource(ResourceType::Scene) {}

        String description;
        SceneSkyboxConfig skyboxConfig;
        SceneDirectionalLightConfig directionalLightConfig;

        DynamicArray<ScenePointLightConfig> pointLights;
        DynamicArray<SceneMeshConfig> meshes;
        DynamicArray<SceneTerrainConfig> terrains;
    };
}  // namespace C3D