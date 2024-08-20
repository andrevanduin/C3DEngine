
#pragma once
#include <containers/dynamic_array.h>
#include <containers/string.h>
#include <core/defines.h>
#include <math/math_types.h>
#include <renderer/transform.h>
#include <resources/resource_types.h>

struct SimpleSceneSkyboxConfig
{
    C3D::String name;
    C3D::String cubemapName;
};

struct SimpleSceneDirectionalLightConfig
{
    C3D::String name;
    vec4 color                  = vec4(1.0);
    vec4 direction              = vec4(0);  // Ignore w (only for 16 byte alignment)
    float shadowDistance        = 200.0f;
    float shadowFadeDistance    = 20.0f;
    float shadowSplitMultiplier = 0.95f;
};

struct SimpleSceneTerrainConfig
{
    C3D::String name;
    C3D::String resourceName;
    C3D::Transform transform;
};

struct SimpleScenePointLightConfig
{
    C3D::String name;
    vec4 color;
    vec4 position;
    f32 constant;
    f32 linear;
    f32 quadratic;
};

struct SimpleSceneMeshConfig
{
    C3D::String name;
    C3D::String resourceName;
    C3D::Transform transform;
    C3D::String parentName;  // optional
};

struct SimpleSceneConfig final : C3D::Resource
{
    C3D::String description;
    SimpleSceneSkyboxConfig skyboxConfig;
    SimpleSceneDirectionalLightConfig directionalLightConfig;

    C3D::DynamicArray<SimpleScenePointLightConfig> pointLights;
    C3D::DynamicArray<SimpleSceneMeshConfig> meshes;
    C3D::DynamicArray<SimpleSceneTerrainConfig> terrains;
};