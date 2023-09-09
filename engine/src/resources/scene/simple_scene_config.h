
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
        vec4 color;
        vec4 direction;
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

    struct SimpleSceneConfig final : Resource
    {
        String description;
        SimpleSceneSkyboxConfig skyboxConfig;
        SimpleSceneDirectionalLightConfig directionalLightConfig;

        DynamicArray<SimpleScenePointLightConfig> pointLights;
        DynamicArray<SimpleSceneMeshConfig> meshes;
        DynamicArray<SimpleSceneTerrainConfig> terrains;
    };
}  // namespace C3D