
#include "transform_system.h"

#include "cson/cson_reader.h"
#include "memory/global_memory_system.h"

namespace C3D
{
    bool TransformSystem::OnInit(const CSONObject& config)
    {
        INFO_LOG("Initializing.");

        // Parse the user provided config
        for (const auto& prop : config.properties)
        {
            if (prop.name.IEquals("initialTransforms"))
            {
                m_config.initialTransforms = prop.GetI64();
            }
        }

        if (m_config.initialTransforms < 32)
        {
            WARN_LOG("Initial transforms < 32, which is not recommended. Defaulting to 32.");
            m_config.initialTransforms = 32;
        }

        if (!Allocate(m_config.initialTransforms))
        {
            ERROR_LOG("Failed to create HandleTable.");
            return false;
        }

        return true;
    }

    void TransformSystem::OnShutdown()
    {
        // If we have some transforms we should free the memory
        if (m_numberOfTransforms > 0)
        {
            Memory.Free(m_positions);
            Memory.Free(m_scales);
            Memory.Free(m_rotations);
            Memory.Free(m_determinants);
            Memory.Free(m_localMatrices);
            Memory.Free(m_worldMatrices);
            Memory.Free(m_uuids);
            Memory.Free(m_isDirtyFlags);
        }
    }

    Handle<Transform> TransformSystem::Acquire()
    {
        auto handle = CreateHandle();
        auto i      = handle.index;

        m_positions[i]     = vec3(0);
        m_scales[i]        = vec3(1.0f);
        m_rotations[i]     = quat(1.0f, 0, 0, 0);
        m_localMatrices[i] = mat4(1.0f);
        m_worldMatrices[i] = mat4(1.0f);

        // NOTE: We don't add the isDirty flag since with everything defaulted our matrices evaluate to identity matrices

        return handle;
    }

    Handle<Transform> TransformSystem::Acquire(const vec3& position)
    {
        auto handle = CreateHandle();
        auto i      = handle.index;

        m_positions[i]     = position;
        m_scales[i]        = vec3(1.0f);
        m_rotations[i]     = quat(1.0f, 0, 0, 0);
        m_localMatrices[i] = mat4(1.0f);
        m_worldMatrices[i] = mat4(1.0f);
        // Mark as dirty since we have to calculate local matrix
        m_isDirtyFlags[i] = true;

        return handle;
    }

    Handle<Transform> TransformSystem::Acquire(const quat& rotation)
    {
        auto handle = CreateHandle();
        auto i      = handle.index;

        m_positions[i]     = vec3(0);
        m_scales[i]        = vec3(1.0f);
        m_rotations[i]     = rotation;
        m_localMatrices[i] = mat4(1.0f);
        m_worldMatrices[i] = mat4(1.0f);
        // Mark as dirty since we have to calculate local matrix
        m_isDirtyFlags[i] = true;

        return handle;
    }

    Handle<Transform> TransformSystem::Acquire(const vec3& position, const quat& rotation)
    {
        auto handle = CreateHandle();
        auto i      = handle.index;

        m_positions[i]     = position;
        m_scales[i]        = vec3(1.0f);
        m_rotations[i]     = rotation;
        m_localMatrices[i] = mat4(1.0f);
        m_worldMatrices[i] = mat4(1.0f);
        // Mark as dirty since we have to calculate local matrix
        m_isDirtyFlags[i] = true;

        return handle;
    }

    Handle<Transform> TransformSystem::Acquire(const vec3& position, const quat& rotation, const vec3& scale)
    {
        auto handle = CreateHandle();
        auto i      = handle.index;

        m_positions[i]     = position;
        m_scales[i]        = scale;
        m_rotations[i]     = rotation;
        m_localMatrices[i] = mat4(1.0f);
        m_worldMatrices[i] = mat4(1.0f);
        // Mark as dirty since we have to calculate local matrix
        m_isDirtyFlags[i] = true;

        return handle;
    }

    bool TransformSystem::Translate(Handle<Transform> handle, const vec3& translation)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        m_positions[handle.index] += translation;
        m_isDirtyFlags[handle.index] = true;

        return true;
    }

    bool TransformSystem::Scale(Handle<Transform> handle, const vec3& scale)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        m_scales[handle.index] *= scale;
        m_isDirtyFlags[handle.index] = true;

        return true;
    }

    bool TransformSystem::Rotate(Handle<Transform> handle, const quat& rotation)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        m_rotations[handle.index] *= rotation;
        m_isDirtyFlags[handle.index] = true;

        return true;
    }

    const mat4& TransformSystem::GetLocal(Handle<Transform> handle) const
    {
        if (!handle.IsValid())
        {
            FATAL_LOG("Provided handle is invalid.");
        }
        return m_localMatrices[handle.index];
    }

    const mat4& TransformSystem::GetWorld(Handle<Transform> handle) const
    {
        if (!handle.IsValid())
        {
            FATAL_LOG("Provided handle is invalid.");
        }
        return m_worldMatrices[handle.index];
    }

    void TransformSystem::SetWorld(Handle<Transform> handle, const mat4& world)
    {
        if (handle.IsValid())
        {
            m_worldMatrices[handle.index] = world;
            m_determinants[handle.index]  = glm::determinant(world);
        }
    }

    f32 TransformSystem::GetDeterminant(Handle<Transform> handle) const
    {
        if (!handle.IsValid())
        {
            FATAL_LOG("Provided handle is invalid.");
        }
        return m_determinants[handle.index];
    }

    const vec3& TransformSystem::GetPosition(Handle<Transform> handle) const
    {
        if (!handle.IsValid())
        {
            FATAL_LOG("Provided handle is invalid.");
        }
        return m_positions[handle.index];
    }

    bool TransformSystem::SetPosition(Handle<Transform> handle, const vec3& position)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        m_positions[handle.index]    = position;
        m_isDirtyFlags[handle.index] = true;

        return true;
    }

    bool TransformSystem::SetX(Handle<Transform> handle, f32 x)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        m_positions[handle.index].x  = x;
        m_isDirtyFlags[handle.index] = true;

        return true;
    }

    bool TransformSystem::SetY(Handle<Transform> handle, f32 y)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        m_positions[handle.index].y  = y;
        m_isDirtyFlags[handle.index] = true;

        return true;
    }

    bool TransformSystem::SetZ(Handle<Transform> handle, f32 z)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        m_positions[handle.index].z  = z;
        m_isDirtyFlags[handle.index] = true;

        return true;
    }

    const quat& TransformSystem::GetRotation(Handle<Transform> handle) const
    {
        if (!handle.IsValid())
        {
            FATAL_LOG("Provided handle is invalid.");
        }
        return m_rotations[handle.index];
    }

    bool TransformSystem::SetRotation(Handle<Transform> handle, const quat& rotation)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        m_rotations[handle.index]    = rotation;
        m_isDirtyFlags[handle.index] = true;

        return true;
    }

    bool TransformSystem::IsDirty(Handle<Transform> handle) const
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }
        return m_isDirtyFlags[handle.index];
    }

    bool TransformSystem::UpdateLocal(Handle<Transform> handle)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        if (m_isDirtyFlags[handle.index])
        {
            mat4 t = glm::translate(m_positions[handle.index]);
            mat4 r = glm::mat4_cast(m_rotations[handle.index]);
            mat4 s = glm::scale(m_scales[handle.index]);

            m_isDirtyFlags[handle.index]  = false;
            m_localMatrices[handle.index] = t * r * s;
            return true;
        }

        return false;
    }

    bool TransformSystem::Release(Handle<Transform>& handle)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Provided handle is invalid. Nothing was done.");
            return false;
        }

        // Invalidate the UUID at the handle's index
        m_uuids[handle.index].Invalidate();
        // Invalidate the handle itself
        handle.Invalidate();

        return true;
    }

    bool TransformSystem::Allocate(u64 newNumberOfTransforms)
    {
        // If we have some transforms already we should free the memory first
        if (m_numberOfTransforms > 0)
        {
            {
                // Allocate new positions
                auto newPositions = Memory.Allocate<vec3>(MemoryType::Transform, newNumberOfTransforms);
                // Copy over the old ones
                std::copy_n(m_positions, m_numberOfTransforms, newPositions);
                // Free the old ones
                Memory.Free(m_positions);
                // Take over the new pointer
                m_positions = newPositions;
            }

            {
                // Allocate new scales
                auto newScales = Memory.Allocate<vec3>(MemoryType::Transform, newNumberOfTransforms);
                // Copy over the old ones
                std::copy_n(m_scales, m_numberOfTransforms, newScales);
                // Free the old ones
                Memory.Free(m_scales);
                // Take over the new pointer
                m_scales = newScales;
            }

            {
                // Allocate new rotations
                auto newRotations = Memory.Allocate<quat>(MemoryType::Transform, newNumberOfTransforms);
                // Copy over the old ones
                std::copy_n(m_rotations, m_numberOfTransforms, newRotations);
                // Free the old ones
                Memory.Free(m_rotations);
                // Take over the new pointer
                m_rotations = newRotations;
            }

            {
                // Allocate new determinants
                auto newDeterminants = Memory.Allocate<f32>(MemoryType::Transform, newNumberOfTransforms);
                // Copy over the old ones
                std::copy_n(m_determinants, m_numberOfTransforms, newDeterminants);
                // Free the old ones
                Memory.Free(m_determinants);
                // Take over the new pointer
                m_determinants = newDeterminants;
            }

            {
                // Allocate new local matrices
                auto newLocalMatrices = Memory.Allocate<mat4>(MemoryType::Transform, newNumberOfTransforms);
                // Copy over the old ones
                std::copy_n(m_localMatrices, m_numberOfTransforms, newLocalMatrices);
                // Free the old ones
                Memory.Free(m_localMatrices);
                // Take over the new pointer
                m_localMatrices = newLocalMatrices;
            }

            {
                // Allocate new world matrices
                auto newWorldMatrices = Memory.Allocate<mat4>(MemoryType::Transform, newNumberOfTransforms);
                // Copy over the old ones
                std::copy_n(m_worldMatrices, m_numberOfTransforms, newWorldMatrices);
                // Free the old ones
                Memory.Free(m_worldMatrices);
                // Take over the new pointer
                m_worldMatrices = newWorldMatrices;
            }

            {
                // Allocate new uuids
                auto newUuids = Memory.Allocate<UUID>(MemoryType::Transform, newNumberOfTransforms);
                // Copy over the old ones
                std::copy_n(m_uuids, m_numberOfTransforms, newUuids);
                // Free the old ones
                Memory.Free(m_uuids);
                // Take over the new pointer
                m_uuids = newUuids;

                // Invalidate all new entries
                for (u32 i = m_numberOfTransforms; i < newNumberOfTransforms; ++i)
                {
                    m_uuids[i].Invalidate();
                }
            }

            {
                // Allocate new is dirty flags
                auto newIsDirtyFlags = Memory.Allocate<bool>(MemoryType::Transform, newNumberOfTransforms);
                // Copy over the old ones
                std::copy_n(m_isDirtyFlags, m_numberOfTransforms, newIsDirtyFlags);
                // Free the old ones
                Memory.Free(m_isDirtyFlags);
                // Take over the new pointer
                m_isDirtyFlags = newIsDirtyFlags;
            }
        }
        else
        {
            // TODO: Alignment for doing SIMD instructions
            m_positions     = Memory.Allocate<vec3>(MemoryType::Transform, newNumberOfTransforms);
            m_scales        = Memory.Allocate<vec3>(MemoryType::Transform, newNumberOfTransforms);
            m_rotations     = Memory.Allocate<quat>(MemoryType::Transform, newNumberOfTransforms);
            m_determinants  = Memory.Allocate<f32>(MemoryType::Transform, newNumberOfTransforms);
            m_localMatrices = Memory.Allocate<mat4>(MemoryType::Transform, newNumberOfTransforms);
            m_worldMatrices = Memory.Allocate<mat4>(MemoryType::Transform, newNumberOfTransforms);
            m_uuids         = Memory.Allocate<UUID>(MemoryType::Transform, newNumberOfTransforms);
            m_isDirtyFlags  = Memory.Allocate<bool>(MemoryType::Transform, newNumberOfTransforms);

            // Invalidate all initial entries
            for (u32 i = 0; i < newNumberOfTransforms; ++i)
            {
                m_uuids[i].Invalidate();
            }
        }

        // Keep track of how many transforms we have
        m_numberOfTransforms = newNumberOfTransforms;
        return true;
    }

    Handle<Transform> TransformSystem::CreateHandle()
    {
        for (u64 i = 0; i < m_numberOfTransforms; ++i)
        {
            if (!m_uuids[i].IsValid())
            {
                // We have found an empty slot
                m_uuids[i].Generate();
                // Return our handle with the found index and the generated uuid
                return Handle<Transform>(i, m_uuids[i]);
            }
        }

        // If we reach this point it means we have run out of transforms and we need to reallocate

        // Keep track of the number of transforms before reallocation since this will be our index
        auto index = m_numberOfTransforms;
        // Allocate twice as many transforms as before
        Allocate(m_numberOfTransforms * 2);
        // Create our handle based on the index we store off before reallocation
        m_uuids[index].Generate();
        // Return our handle
        return Handle<Transform>(index, m_uuids[index]);
    }
}  // namespace C3D