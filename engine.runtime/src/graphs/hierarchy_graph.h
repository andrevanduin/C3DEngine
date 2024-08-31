
#pragma once
#include "containers/dynamic_array.h"
#include "defines.h"
#include "identifiers/handle.h"
#include "math/math_types.h"

namespace C3D
{
    struct Transform;

    struct HierarchyGraphNode
    {
        /** @brief The unique id for this node. Will be invalid when this node is not acquired. */
        UUID uuid;
        /** @brief An optional handle to a transform for this node. Will be an invalid handle if no transform is attached. */
        Handle<Transform> transform;
        /** @brief The index to the parent of this node. Is set to INVALID_ID when this is a root node. */
        u32 parent = INVALID_ID;
        /** @brief An array of indices to the children of this node. */
        DynamicArray<u32> children;
    };

    class C3D_API HierarchyGraph
    {
    public:
        bool Create(u32 initialCapacity);
        void Destroy();

        Handle<HierarchyGraphNode> AddNode(Handle<Transform> transform = Handle<Transform>());
        bool AddChild(Handle<HierarchyGraphNode> parentHandle, Handle<HierarchyGraphNode> childHandle);

        bool Update();

        Handle<Transform> GetTransform(Handle<HierarchyGraphNode> node) const;

        bool Release(Handle<HierarchyGraphNode> handle, bool releaseTransform);

    private:
        u32 CreateNode();

        void UpdateNode(u32 nodeIndex, mat4& world, bool worldNeedsUpdate);

        /** @brief An array of indices to root nodes. */
        DynamicArray<u32> m_rootIndices;
        /** @brief An array of the nodes that are part of this graph. */
        DynamicArray<HierarchyGraphNode> m_nodes;
    };
}  // namespace C3D