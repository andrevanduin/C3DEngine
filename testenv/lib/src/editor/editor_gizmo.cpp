
#include "editor_gizmo.h"

#include <core/colors.h>
#include <renderer/renderer_frontend.h>
#include <systems/system_manager.h>

namespace
{
    constexpr u8 DISC_SEGMENTS  = 32;
    constexpr u8 DISC_SEGMENTS2 = DISC_SEGMENTS * 2;
    constexpr f32 DISC_RADIUS   = 1.0f;

    enum EditorGizmoAxis : u8
    {
        XAxis   = 0,
        YAxis   = 1,
        ZAxis   = 2,
        XYAxis  = 3,
        XZAxis  = 4,
        YZAxis  = 5,
        XYZAxis = 6,
    };

    constexpr const char* INSTANCE_NAME = "EDITOR_GIZMO";
}  // namespace

bool EditorGizmo::Create()
{
    m_mode = EditorGizmoMode::None;
    return true;
}

void EditorGizmo::Destroy()
{
    for (auto& data : m_modeData)
    {
        data.vertices.Destroy();
        data.indices.Destroy();
    }
}

bool EditorGizmo::Initialize()
{
    CreateNoneMode();
    CreateMoveMode();
    CreateScaleMode();
    CreateRotateMode();
    return true;
}

bool EditorGizmo::Load()
{
    for (auto& data : m_modeData)
    {
        if (!Renderer.CreateGeometry(data.geometry, sizeof(C3D::ColorVertex3D), data.vertices.Size(), data.vertices.GetData(), 0, 0, 0))
        {
            ERROR_LOG("Failed to create gizmo geometry.");
            return false;
        }

        if (!Renderer.UploadGeometry(data.geometry))
        {
            ERROR_LOG("Failed to upload gizmo geometry.");
            return false;
        }

        data.geometry.generation++;
    }

    return true;
}

bool EditorGizmo::Unload()
{
    for (auto& data : m_modeData)
    {
        Renderer.DestroyGeometry(data.geometry);
    }
    return true;
}

void EditorGizmo::Update() {}

void EditorGizmo::Refresh()
{
    if (m_selectedObjectTransform)
    {
        // Set the position of our transform
        m_transform.SetPosition(m_selectedObjectTransform->GetPosition());
        // If we are using local mode we set the rotation also
        if (m_orientation == EditorGizmoOrientation::Local)
        {
            m_transform.SetRotation(m_selectedObjectTransform->GetRotation());
        }
        else
        {
            m_transform.SetRotation(quat());
        }
    }
    else
    {
        // Reset the transform
        m_transform.SetPosition(vec3(0));
        m_transform.SetRotation(quat());
    }
}

void EditorGizmo::CreateNoneMode()
{
    auto& data = m_modeData[ToUnderlying(EditorGizmoMode::None)];
    data.vertices.Resize(6);

    // X
    data.vertices[0].color      = C3D::GRAY;
    data.vertices[1].color      = C3D::GRAY;
    data.vertices[1].position.x = 1.0f;

    // Y
    data.vertices[2].color      = C3D::GRAY;
    data.vertices[3].color      = C3D::GRAY;
    data.vertices[3].position.y = 1.0f;
    // Z
    data.vertices[4].color      = C3D::GRAY;
    data.vertices[5].color      = C3D::GRAY;
    data.vertices[5].position.z = 1.0f;
}

void EditorGizmo::BeginInteraction(EditorGizmoInteractionType interactionType, const C3D::Camera* camera, const C3D::Ray& ray)
{
    m_interaction = interactionType;

    if (interactionType == EditorGizmoInteractionType::MouseHover)
    {
        // For hover we don't need to do anything else here
        return;
    }

    auto& data = m_modeData[ToUnderlying(m_mode)];
    if (data.currentAxisIndex > 6)
    {
        // No interaction possible because there is no current axis.
        return;
    }

    mat4 world       = m_transform.GetWorld();
    vec3 origin      = m_transform.GetPosition();
    vec3 planeNormal = {};

    if (m_interaction == EditorGizmoInteractionType::MouseDrag)
    {
        if (m_mode == EditorGizmoMode::Move || m_mode == EditorGizmoMode::Scale)
        {
            // Create the plane
            if (m_orientation == EditorGizmoOrientation::Local || m_orientation == EditorGizmoOrientation::Global)
            {
                switch (data.currentAxisIndex)
                {
                    case XAxis:
                    case XYAxis:
                        planeNormal = world * vec4(C3D::VEC3_BACKWARD, 0.0f);
                        break;
                    case YAxis:
                    case XYZAxis:
                        planeNormal = camera->GetBackward();
                        break;
                    case XZAxis:
                        planeNormal = world * vec4(C3D::VEC3_UP, 0.0f);
                        break;
                    case ZAxis:
                    case YZAxis:
                        planeNormal = world * vec4(C3D::VEC3_RIGHT, 0.0f);
                        break;
                }
            }
            else
            {
                // TODO: Other orientations
                return;
            }
        }
        else if (m_mode == EditorGizmoMode::Rotate)
        {
            switch (data.currentAxisIndex)
            {
                case XAxis:
                    planeNormal = world * vec4(C3D::VEC3_LEFT, 0.0f);
                    break;
                case YAxis:
                    planeNormal = world * vec4(C3D::VEC3_DOWN, 0.0f);
                    break;
                case ZAxis:
                    planeNormal = world * vec4(C3D::VEC3_FORWARD, 0.0f);
                    break;
            }
        }

        data.interactionPlane     = C3D::Plane3D(origin, planeNormal);
        data.interactionPlaneBack = C3D::Plane3D(origin, planeNormal * -1.0f);

        // Get the initial intersection point of the ray on the plane
        vec3 intersection = {};
        f32 distance      = 0.0f;
        if (!ray.TestAgainstPlane3D(data.interactionPlane, intersection, distance))
        {
            // Try from the other direction
            if (!ray.TestAgainstPlane3D(data.interactionPlaneBack, intersection, distance))
            {
                ERROR_LOG("RayCast could not find an intersection with the ineraction plane.");
                return;
            }
        }

        data.interactionStartPos = intersection;
        data.interactionLastPos  = intersection;
    }
}

void EditorGizmo::HandleInteraction(const C3D::Ray& ray)
{
    if (m_mode == EditorGizmoMode::None || m_mode >= EditorGizmoMode::Max) return;

    auto& data = m_modeData[ToUnderlying(m_mode)];

    if (m_interaction == EditorGizmoInteractionType::MouseDrag)
    {
        if (data.currentAxisIndex == INVALID_ID_U8)
        {
            // NOTE: Don't handle any interactions if we don't have a current axis
            return;
        }

        // Drag state
        mat4 world = m_transform.GetWorld();
        // Get the initial intersection point of the ray on the plane
        vec3 intersection = {};
        f32 distance      = 0.0f;

        if (m_mode == EditorGizmoMode::Move)
        {
            if (!ray.TestAgainstPlane3D(data.interactionPlane, intersection, distance))
            {
                // Try from the other direction
                if (!ray.TestAgainstPlane3D(data.interactionPlaneBack, intersection, distance))
                {
                    ERROR_LOG("RayCast could not find an intersection with the ineraction plane.");
                    return;
                }
            }

            // Get the delta between the current intersection and the previous one
            vec3 delta = intersection - data.interactionLastPos;

            vec3 direction   = {};
            vec3 translation = {};

            if (m_orientation == EditorGizmoOrientation::Local || m_orientation == EditorGizmoOrientation::Global)
            {
                switch (data.currentAxisIndex)
                {
                    case XAxis:
                        direction = world * vec4(C3D::VEC3_RIGHT, 0.0f);
                        // Project delta onto direction
                        translation = direction * glm::dot(delta, direction);
                        break;
                    case YAxis:
                        direction = world * vec4(C3D::VEC3_UP, 0.0f);
                        // Project delta onto direction
                        translation = direction * glm::dot(delta, direction);
                        break;
                    case ZAxis:
                        direction = world * vec4(C3D::VEC3_FORWARD, 0.0f);
                        // Project delta onto direction
                        translation = direction * glm::dot(delta, direction);
                        break;
                    case XYAxis:
                    case XZAxis:
                    case YZAxis:
                    case XYZAxis:
                        translation = delta;
                        break;
                }
            }
            else
            {
                // TODO: Other orientations
                return;
            }

            // Apply the translation to our gizmo's transform
            m_transform.Translate(translation);
            data.interactionLastPos = intersection;
            // If we have access to the selected object's transform we also apply the translation there
            if (m_selectedObjectTransform)
            {
                m_selectedObjectTransform->Translate(translation);
            }
        }
        else if (m_mode == EditorGizmoMode::Scale)
        {
            if (!ray.TestAgainstPlane3D(data.interactionPlane, intersection, distance))
            {
                // Try from the other direction
                if (!ray.TestAgainstPlane3D(data.interactionPlaneBack, intersection, distance))
                {
                    ERROR_LOG("RayCast could not find an intersection with the ineraction plane.");
                    return;
                }
            }

            vec3 direction = {};
            vec3 scale     = {};
            vec3 origin    = m_transform.GetPosition();

            // Scale along the current axis's line in local space.
            // We will transform this to global later if it's needed
            switch (data.currentAxisIndex)
            {
                case XAxis:
                    direction = C3D::VEC3_RIGHT;
                    break;
                case YAxis:
                    direction = C3D::VEC3_UP;
                    break;
                case ZAxis:
                    direction = C3D::VEC3_FORWARD;
                    break;
                case XYAxis:
                    // Combine the two axis and scale along both of them
                    direction = glm::normalize((C3D::VEC3_RIGHT + C3D::VEC3_UP) * 0.5f);
                    break;
                case XZAxis:
                    // Combine the two axis and scale along both of them
                    direction = glm::normalize((C3D::VEC3_RIGHT + C3D::VEC3_BACKWARD) * 0.5f);
                    break;
                case YZAxis:
                    // Combine the two axis and scale along both of them
                    direction = glm::normalize((C3D::VEC3_UP + C3D::VEC3_BACKWARD) * 0.5f);
                    break;
                case XYZAxis:
                    direction = glm::normalize(vec3(1.0f));
                    break;
            }

            // Get the distance from the last interaction pos to the current intersection (interaction pos)
            // This wil determinte the magnitude by which we will scale
            f32 dist = glm::distance(data.interactionLastPos, intersection);

            // Get the direction of the intersection from the origin
            vec3 directionFromLastPos = glm::normalize(intersection - data.interactionLastPos);

            // Get the transformed direction
            vec3 transformedDirection = {};
            if (m_orientation == EditorGizmoOrientation::Local)
            {
                // If we are scaling in the local orientation we transform our direction by the gizmo's world matrix
                if (data.currentAxisIndex < 6)
                {
                    transformedDirection = world * vec4(direction, 0.0f);
                }
                else
                {
                    // If we are scaling in all directions we can simply take the local up vector
                    transformedDirection = world * vec4(C3D::VEC3_UP, 0.0f);
                }
            }
            else if (m_orientation == EditorGizmoOrientation::Global)
            {
                // When we are scaling in global orientation we just take the direction as is
                transformedDirection = direction;
            }
            else
            {
                // TODO: Other directions
                return;
            }

            // Determine the sign of the magnitude
            f32 d = C3D::Sign(glm::dot(transformedDirection, directionFromLastPos));

            // Calculate the scale difference
            scale = direction * (d * dist);

            // For global transforms, we get the inverse of the rotation and apply that
            // to the scale to make sure we are scaling based on global (absolute) axis instead of the local ones.
            if (m_orientation == EditorGizmoOrientation::Global)
            {
                if (m_selectedObjectTransform)
                {
                    quat q = glm::inverse(m_selectedObjectTransform->GetRotation());
                    scale  = q * scale;
                }
            }

            INFO_LOG("scale (diff) = [{:.4f}, {:.4f}, {:.4f}].", scale.x, scale.y, scale.z);

            // Apply the scale to the selected object
            if (m_selectedObjectTransform)
            {
                for (u8 i = 0; i < 3; i++)
                {
                    // Our scale contains the delta we add 1.0f so we can multiply it with the current scale to get our final scaled result
                    scale[i] += 1.0f;
                }

                INFO_LOG("Applying scale: [{:.4f}, {:.4f}, {:.4f}].", scale.x, scale.y, scale.z);
                m_selectedObjectTransform->Scale(scale);
            }
            data.interactionLastPos = intersection;
        }
        else if (m_mode == EditorGizmoMode::Rotate)
        {
            vec3 origin         = m_transform.GetPosition();
            vec3 interactionPos = {};
            f32 distance        = 0.0f;

            if (!ray.TestAgainstPlane3D(data.interactionPlane, interactionPos, distance))
            {
                // Try from the other direction
                if (!ray.TestAgainstPlane3D(data.interactionPlaneBack, interactionPos, distance))
                {
                    return;
                }
            }

            vec3 direction = {};

            // We get the difference in angle between this interaction and the previous one
            vec3 v0 = data.interactionLastPos - origin;
            vec3 v1 = interactionPos - origin;

            f32 angle = C3D::ACos(glm::dot(glm::normalize(v0), glm::normalize(v1)));
            if (C3D::EpsilonEqual(angle, 0.0f) || C3D::IsNaN(angle))
            {
                // If our result == 0.0f or it's not a number we don't need to rotate
                return;
            }

            vec3 cross = glm::cross(v0, v1);
            if (glm::dot(data.interactionPlane.normal, cross) > 0)
            {
                angle = -angle;
            }

            switch (data.currentAxisIndex)
            {
                case XAxis:
                    direction = C3D::GetRight(world);
                    break;
                case YAxis:
                    direction = C3D::GetUp(world);
                    break;
                case ZAxis:
                    direction = C3D::GetBackward(world);
                    break;
            }

            // Get the final rotation as a quaternion
            quat rotation = glm::normalize(glm::angleAxis(angle, direction));
            // Apply the rotation to the gizmo
            m_transform.Rotate(rotation);
            data.interactionLastPos = interactionPos;

            // Apply the rotation to the selected object
            if (m_selectedObjectTransform)
            {
                m_selectedObjectTransform->Rotate(rotation);
            }
        }
    }
    else if (m_interaction == EditorGizmoInteractionType::MouseHover)
    {
        // Hover state
        f32 dist;
        mat4 model = m_transform.GetWorld();
        u8 hitAxis = INVALID_ID_U8;

        if (m_mode == EditorGizmoMode::Move || m_mode == EditorGizmoMode::Scale)
        {
            // Loop through each axis and axis combo.
            // We loop backwards to give priority to the combination axis since their hit-boxes are smaller
            for (i32 i = 6; i >= 0; i--)
            {
                if (ray.TestAgainstExtents(data.extents[i], model, dist))
                {
                    hitAxis = i;
                    break;
                }
            }

            if (hitAxis != data.currentAxisIndex)
            {
                data.currentAxisIndex = hitAxis;

                for (u32 i = 0; i < 3; i++)
                {
                    if (i == hitAxis)
                    {
                        data.vertices[(i * 2) + 0].color = C3D::YELLOW;
                        data.vertices[(i * 2) + 1].color = C3D::YELLOW;
                    }
                    else
                    {
                        // Set non-hit axis back to their original colors
                        data.vertices[(i * 2) + 0].color    = C3D::BLACK;
                        data.vertices[(i * 2) + 0].color[i] = 1.0f;
                        data.vertices[(i * 2) + 1].color    = C3D::BLACK;
                        data.vertices[(i * 2) + 1].color[i] = 1.0f;
                    }
                }

                if (m_mode == EditorGizmoMode::Move)
                {
                    // XYZ
                    if (hitAxis == XYZAxis)
                    {
                        // Turn them all yellow
                        for (auto& vertex : data.vertices)
                        {
                            vertex.color = C3D::YELLOW;
                        }
                    }
                    else
                    {
                        if (hitAxis == XYAxis)
                        {
                            // X/Y
                            // 6/7, 12/13
                            data.vertices[6].color  = C3D::YELLOW;
                            data.vertices[7].color  = C3D::YELLOW;
                            data.vertices[12].color = C3D::YELLOW;
                            data.vertices[13].color = C3D::YELLOW;
                        }
                        else
                        {
                            data.vertices[6].color  = C3D::RED;
                            data.vertices[7].color  = C3D::RED;
                            data.vertices[12].color = C3D::GREEN;
                            data.vertices[13].color = C3D::GREEN;
                        }

                        if (hitAxis == XZAxis)
                        {
                            // X/Z
                            // 8/9, 16/17
                            data.vertices[8].color  = C3D::YELLOW;
                            data.vertices[9].color  = C3D::YELLOW;
                            data.vertices[16].color = C3D::YELLOW;
                            data.vertices[17].color = C3D::YELLOW;
                        }
                        else
                        {
                            data.vertices[8].color  = C3D::RED;
                            data.vertices[9].color  = C3D::RED;
                            data.vertices[16].color = C3D::BLUE;
                            data.vertices[17].color = C3D::BLUE;
                        }

                        if (hitAxis == YZAxis)
                        {
                            // Y/Z
                            // 10/11, 14/15
                            data.vertices[10].color = C3D::YELLOW;
                            data.vertices[11].color = C3D::YELLOW;
                            data.vertices[14].color = C3D::YELLOW;
                            data.vertices[15].color = C3D::YELLOW;
                        }
                        else
                        {
                            data.vertices[10].color = C3D::GREEN;
                            data.vertices[11].color = C3D::GREEN;
                            data.vertices[14].color = C3D::BLUE;
                            data.vertices[15].color = C3D::BLUE;
                        }
                    }
                }
                else if (m_mode == EditorGizmoMode::Scale)
                {
                    // XYZ
                    if (hitAxis == XYZAxis)
                    {
                        // Turn them all yellow
                        for (auto& vertex : data.vertices)
                        {
                            vertex.color = C3D::YELLOW;
                        }
                    }
                    else
                    {
                        // X/Y 6/7
                        if (hitAxis == XYAxis)
                        {
                            data.vertices[6].color = C3D::YELLOW;
                            data.vertices[7].color = C3D::YELLOW;
                        }
                        else
                        {
                            data.vertices[6].color = C3D::RED;
                            data.vertices[7].color = C3D::GREEN;
                        }

                        // X/Z 10/11
                        if (hitAxis == XZAxis)
                        {
                            data.vertices[10].color = C3D::YELLOW;
                            data.vertices[11].color = C3D::YELLOW;
                        }
                        else
                        {
                            data.vertices[10].color = C3D::RED;
                            data.vertices[11].color = C3D::BLUE;
                        }

                        // Y/Z 8/9
                        if (hitAxis == YZAxis)
                        {
                            data.vertices[8].color = C3D::YELLOW;
                            data.vertices[9].color = C3D::YELLOW;
                        }
                        else
                        {
                            data.vertices[8].color = C3D::BLUE;
                            data.vertices[9].color = C3D::GREEN;
                        }
                    }
                }

                Renderer.UpdateGeometryVertices(data.geometry, 0, data.vertices.Size(), data.vertices.GetData());
            }
        }
        else if (m_mode == EditorGizmoMode::Rotate)
        {
            vec3 point;

            // Loop through each axis
            for (i32 i = 0; i < 3; i++)
            {
                vec3 aaNormal = {};
                aaNormal[i]   = 1.0f;
                aaNormal      = model * vec4(aaNormal, 0.0f);
                vec3 center   = m_transform.GetPosition();

                C3D::Disc3D disc = { center, aaNormal, DISC_RADIUS + 0.05f, DISC_RADIUS - 0.05f };
                if (ray.TestAgainstDisc3D(disc, point, dist))
                {
                    hitAxis = i;
                    break;
                }
                else
                {
                    // If we have no hit we try from the other side
                    disc.normal *= -1.0f;
                    if (ray.TestAgainstDisc3D(disc, point, dist))
                    {
                        hitAxis = i;
                        break;
                    }
                }
            }

            if (data.currentAxisIndex != hitAxis)
            {
                data.currentAxisIndex = hitAxis;

                // Main axis colors
                for (u32 i = 0; i < 3; i++)
                {
                    vec4 setColor = C3D::BLACK;
                    // Yellow for hit axis; otherwise original colour.
                    if (i == hitAxis)
                    {
                        setColor.r = 1.0f;
                        setColor.g = 1.0f;
                    }
                    else
                    {
                        setColor[i] = 1.0f;
                    }

                    // Main axis in center.
                    data.vertices[(i * 2) + 0].color = setColor;
                    data.vertices[(i * 2) + 1].color = setColor;

                    // Ring
                    u32 ringOffset = 6 + (DISC_SEGMENTS2 * i);
                    for (u32 j = 0; j < DISC_SEGMENTS; j++)
                    {
                        data.vertices[ringOffset + (j * 2) + 0].color = setColor;
                        data.vertices[ringOffset + (j * 2) + 1].color = setColor;
                    }
                }
            }

            Renderer.UpdateGeometryVertices(data.geometry, 0, data.vertices.Size(), data.vertices.GetData());
        }
    }
}

void EditorGizmo::EndInteraction()
{
    if (m_interaction == EditorGizmoInteractionType::MouseDrag && m_mode == EditorGizmoMode::Rotate)
    {
        INFO_LOG("For ROTATE interaction.");
        if (m_orientation == EditorGizmoOrientation::Global)
        {
            // Reset our orientation when we are in global orientation mode
            m_transform.SetRotation(quat());
        }
    }

    m_interaction = EditorGizmoInteractionType::None;
}

void EditorGizmo::SetOrientation(const EditorGizmoOrientation orientation)
{
    m_orientation = orientation;
    Refresh();
}

void EditorGizmo::SetSelectedObjectTransform(C3D::Transform* selected)
{
    m_selectedObjectTransform = selected;
    Refresh();
}

void EditorGizmo::CreateMoveMode()
{
    auto& data = m_modeData[ToUnderlying(EditorGizmoMode::Move)];
    data.vertices.Resize(18);

    // X
    data.vertices[0].color      = C3D::RED;
    data.vertices[0].position.x = 0.2f;
    data.vertices[1].color      = C3D::RED;
    data.vertices[1].position.x = 2.0f;

    // Y
    data.vertices[2].color      = C3D::GREEN;
    data.vertices[2].position.y = 0.2f;
    data.vertices[3].color      = C3D::GREEN;
    data.vertices[3].position.y = 2.0f;

    // Z
    data.vertices[4].color      = C3D::BLUE;
    data.vertices[4].position.z = 0.2f;
    data.vertices[5].color      = C3D::BLUE;
    data.vertices[5].position.z = 2.0f;

    // X Box lines
    data.vertices[6].color      = C3D::RED;
    data.vertices[6].position.x = 0.4f;
    data.vertices[7].color      = C3D::RED;
    data.vertices[7].position.x = 0.4f;
    data.vertices[7].position.y = 0.4f;

    data.vertices[8].color      = C3D::RED;
    data.vertices[8].position.x = 0.4f;
    data.vertices[9].color      = C3D::RED;
    data.vertices[9].position.x = 0.4f;
    data.vertices[9].position.z = 0.4f;

    // Y Box lines
    data.vertices[10].color      = C3D::GREEN;
    data.vertices[10].position.y = 0.4f;
    data.vertices[11].color      = C3D::GREEN;
    data.vertices[11].position.y = 0.4f;
    data.vertices[11].position.z = 0.4f;

    data.vertices[12].color      = C3D::GREEN;
    data.vertices[12].position.y = 0.4f;
    data.vertices[13].color      = C3D::GREEN;
    data.vertices[13].position.y = 0.4f;
    data.vertices[13].position.x = 0.4f;

    // Z Box lines
    data.vertices[14].color      = C3D::BLUE;
    data.vertices[14].position.z = 0.4f;
    data.vertices[15].color      = C3D::BLUE;
    data.vertices[15].position.z = 0.4f;
    data.vertices[15].position.y = 0.4f;

    data.vertices[16].color      = C3D::BLUE;
    data.vertices[16].position.z = 0.4f;
    data.vertices[17].color      = C3D::BLUE;
    data.vertices[17].position.z = 0.4f;
    data.vertices[17].position.x = 0.4f;

    // One for each axis (3) + one for each combination axis (3) + one for all axis together (1)
    data.extents.Resize(7);

    // Create a box for each axis
    // X
    data.extents[0].min = vec3(0.4f, -0.2f, -0.2f);
    data.extents[0].max = vec3(2.1f, 0.2f, 0.2f);

    // Y
    data.extents[1].min = vec3(-0.2f, 0.4f, -0.2f);
    data.extents[1].max = vec3(0.2f, 2.1f, 0.2f);

    // Z
    data.extents[2].min = vec3(-0.2f, -0.2f, 0.4f);
    data.extents[2].max = vec3(0.2f, 0.2f, 2.1f);

    // Create a box for the combination axis
    // X/Y
    data.extents[3].min = vec3(0.1f, 0.1f, -0.05f);
    data.extents[3].max = vec3(0.5f, 0.5f, 0.05f);

    // X/Z
    data.extents[4].min = vec3(0.1f, -0.05f, 0.1f);
    data.extents[4].max = vec3(0.5f, 0.05f, 0.5f);

    // Y/Z
    data.extents[5].min = vec3(-0.05f, 0.1f, 0.1f);
    data.extents[5].max = vec3(0.05f, 0.5f, 0.5f);

    // XYZ
    data.extents[6].min = vec3(-0.1f, -0.1f, -0.1f);
    data.extents[6].max = vec3(0.1f, 0.1f, 0.1f);
}

void EditorGizmo::CreateScaleMode()
{
    auto& data = m_modeData[ToUnderlying(EditorGizmoMode::Scale)];

    data.vertices.Resize(12);

    // X
    data.vertices[0].color      = C3D::RED;
    data.vertices[1].color      = C3D::RED;
    data.vertices[1].position.x = 2.0f;

    // Y
    data.vertices[2].color      = C3D::GREEN;
    data.vertices[3].color      = C3D::GREEN;
    data.vertices[3].position.y = 2.0f;

    // Z
    data.vertices[4].color      = C3D::BLUE;
    data.vertices[5].color      = C3D::BLUE;
    data.vertices[5].position.z = 2.0f;

    // X/Y outer line
    data.vertices[6].position.x = 0.8f;
    data.vertices[6].color      = C3D::RED;
    data.vertices[7].position.y = 0.8f;
    data.vertices[7].color      = C3D::GREEN;

    // Z/Y outer line
    data.vertices[8].position.z = 0.8f;
    data.vertices[8].color      = C3D::BLUE;
    data.vertices[9].position.y = 0.8f;
    data.vertices[9].color      = C3D::GREEN;

    // X/Z outer line
    data.vertices[10].position.x = 0.8f;
    data.vertices[10].color      = C3D::RED;
    data.vertices[11].position.z = 0.8f;
    data.vertices[11].color      = C3D::BLUE;

    // One for each axis (3) + one for each combination axis (3) + one for all axis together (1)
    data.extents.Resize(7);

    // Create a box for each axis
    // X
    data.extents[0].min = vec3(0.4f, -0.2f, -0.2f);
    data.extents[0].max = vec3(2.1f, 0.2f, 0.2f);

    // Y
    data.extents[1].min = vec3(-0.2f, 0.4f, -0.2f);
    data.extents[1].max = vec3(0.2f, 2.1f, 0.2f);

    // Z
    data.extents[2].min = vec3(-0.2f, -0.2f, 0.4f);
    data.extents[2].max = vec3(0.2f, 0.2f, 2.1f);

    // Create a box for the combination axis
    // X/Y
    data.extents[3].min = vec3(0.1f, 0.1f, -0.05f);
    data.extents[3].max = vec3(0.5f, 0.5f, 0.05f);

    // X/Z
    data.extents[4].min = vec3(0.1f, -0.05f, 0.1f);
    data.extents[4].max = vec3(0.5f, 0.05f, 0.5f);

    // Y/Z
    data.extents[5].min = vec3(-0.05f, 0.1f, 0.1f);
    data.extents[5].max = vec3(0.05f, 0.5f, 0.5f);

    // XYZ
    data.extents[6].min = vec3(-0.1f, -0.1f, -0.1f);
    data.extents[6].max = vec3(0.1f, 0.1f, 0.1f);
}

void EditorGizmo::CreateRotateMode()
{
    auto& data = m_modeData[ToUnderlying(EditorGizmoMode::Rotate)];
    data.vertices.Resize(12 + (DISC_SEGMENTS * 2 * 3));

    // Start with the center where we draw small axis
    // X
    data.vertices[0].color      = C3D::RED;
    data.vertices[1].color      = C3D::RED;
    data.vertices[1].position.x = 0.2f;

    // Y
    data.vertices[2].color      = C3D::GREEN;
    data.vertices[3].color      = C3D::GREEN;
    data.vertices[3].position.y = 0.2f;

    // Z
    data.vertices[4].color      = C3D::BLUE;
    data.vertices[5].color      = C3D::BLUE;
    data.vertices[5].position.z = 0.2f;

    // Starting index
    u32 j = 6;

    // x
    for (u32 i = 0; i < DISC_SEGMENTS; i++, j += 2)
    {
        // Two at a time to form a line.
        auto theta                  = static_cast<f32>(i) / DISC_SEGMENTS * C3D::PI_2;
        data.vertices[j].position.y = DISC_RADIUS * C3D::Cos(theta);
        data.vertices[j].position.z = DISC_RADIUS * C3D::Sin(theta);
        data.vertices[j].color      = C3D::RED;

        theta                           = static_cast<f32>((i + 1) % DISC_SEGMENTS) / DISC_SEGMENTS * C3D::PI_2;
        data.vertices[j + 1].position.y = DISC_RADIUS * C3D::Cos(theta);
        data.vertices[j + 1].position.z = DISC_RADIUS * C3D::Sin(theta);
        data.vertices[j + 1].color      = C3D::RED;
    }

    // y
    for (u32 i = 0; i < DISC_SEGMENTS; i++, j += 2)
    {
        // Two at a time to form a line.
        auto theta                  = static_cast<f32>(i) / DISC_SEGMENTS * C3D::PI_2;
        data.vertices[j].position.x = DISC_RADIUS * C3D::Cos(theta);
        data.vertices[j].position.z = DISC_RADIUS * C3D::Sin(theta);
        data.vertices[j].color      = C3D::GREEN;

        theta                           = static_cast<f32>((i + 1) % DISC_SEGMENTS) / DISC_SEGMENTS * C3D::PI_2;
        data.vertices[j + 1].position.x = DISC_RADIUS * C3D::Cos(theta);
        data.vertices[j + 1].position.z = DISC_RADIUS * C3D::Sin(theta);
        data.vertices[j + 1].color      = C3D::GREEN;
    }

    // z
    for (u32 i = 0; i < DISC_SEGMENTS; i++, j += 2)
    {
        // Two at a time to form a line.
        auto theta                  = static_cast<f32>(i) / DISC_SEGMENTS * C3D::PI_2;
        data.vertices[j].position.x = DISC_RADIUS * C3D::Cos(theta);
        data.vertices[j].position.y = DISC_RADIUS * C3D::Sin(theta);
        data.vertices[j].color      = C3D::BLUE;

        theta                           = static_cast<f32>((i + 1) % DISC_SEGMENTS) / DISC_SEGMENTS * C3D::PI_2;
        data.vertices[j + 1].position.x = DISC_RADIUS * C3D::Cos(theta);
        data.vertices[j + 1].position.y = DISC_RADIUS * C3D::Sin(theta);
        data.vertices[j + 1].color      = C3D::BLUE;
    }

    // NOTE: We do not need extents for the rotate mode since we are using discs
}

C3D::UUID EditorGizmo::GetId() const { return m_id; }

C3D::Geometry* EditorGizmo::GetGeometry() { return &m_modeData[ToUnderlying(m_mode)].geometry; }

vec3 EditorGizmo::GetPosition() const { return m_transform.GetPosition(); }

mat4 EditorGizmo::GetModel() const { return m_transform.GetWorld(); }