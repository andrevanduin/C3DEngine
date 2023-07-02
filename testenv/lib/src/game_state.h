
#pragma once
#include <core/application.h>
#include <core/frame_data.h>
#include <math/frustum.h>
#include <resources/mesh.h>
#include <resources/simple_scene.h>
#include <resources/ui_text.h>
#include <systems/lights/light_system.h>

namespace C3D
{
    class Camera;
}

struct GameState final : C3D::ApplicationState
{
    C3D::Camera* camera = nullptr;

    C3D::Frustum cameraFrustum;

    // TEMP
    C3D::SimpleScene simpleScene;

    C3D::Skybox skybox;

    C3D::Mesh meshes[10];
    C3D::Mesh* carMesh = nullptr;
    C3D::Mesh* sponzaMesh = nullptr;

    C3D::DirectionalLight dirLight;
    C3D::PointLight pLights[3];

    bool modelsLoaded = false;

    C3D::UIMesh uiMeshes[10];
    C3D::UIText testText;

    u32 hoveredObjectId = INVALID_ID;

    C3D::DynamicArray<C3D::RegisteredEventCallback> m_registeredCallbacks;
    // TEMP
};

struct GameFrameData final : C3D::ApplicationFrameData
{
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> worldGeometries;
};