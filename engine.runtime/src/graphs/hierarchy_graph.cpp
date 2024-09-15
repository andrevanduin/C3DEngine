
#include "hierarchy_graph.h"

#include "systems/system_manager.h"
#include "systems/transforms/transform_system.h"

namespace C3D
{
    bool HierarchyGraph::Create(u32 initialCapacity)
    {
        m_nodes.Reserve(initialCapacity);
        return true;
    }

    void HierarchyGraph::Destroy()
    {
        for (auto& node : m_nodes)
        {
            if (node.transform)
            {
                // If we have a valid transform assigned to this node we should release it
                if (!Transforms.Release(node.transform))
                {
                    WARN_LOG("Failed to release transform for node: '{}'.", node.uuid);
                }
            }
        }

        m_nodes.Destroy();
    }

    Handle<HierarchyGraphNode> HierarchyGraph::AddNode(Handle<Transform> transform)
    {
        // Create a node an get it's index
        auto index = CreateNode();
        // Get a reference to the node
        auto& node = m_nodes[index];
        // Set the user-provided transform
        node.transform = transform;
        // By default a node is root
        m_rootIndices.PushBack(index);
        // Return a handle to the node
        return Handle<HierarchyGraphNode>(index, node.uuid);
    }

    bool HierarchyGraph::AddChild(Handle<HierarchyGraphNode> parentHandle, Handle<HierarchyGraphNode> childHandle)
    {
        if (!parentHandle.IsValid())
        {
            ERROR_LOG("Invalid parent handle provided. Nothing was done.");
            return false;
        }

        if (!childHandle.IsValid())
        {
            ERROR_LOG("Invalid child handle provided. Nothing was done.");
            return false;
        }

        // Get a reference to the parent and child
        auto& parent = m_nodes[parentHandle.index];
        auto& child  = m_nodes[childHandle.index];
        // Add this child to the children list of the parent
        parent.children.PushBack(childHandle.index);
        // Add the index of the parent to the child
        child.parent = parentHandle.index;
        // Remove the child from the root indices list
        m_rootIndices.Remove(childHandle.index);
        // Return success
        return true;
    }

    bool HierarchyGraph::Update()
    {
        for (auto rootIndex : m_rootIndices)
        {
            // Update the node with a identity matrix since the root nodes don't have a parent
            auto identity = mat4(1.0f);
            UpdateNode(rootIndex, identity, false);
        }

        return true;
    }

    Handle<Transform> HierarchyGraph::GetTransform(Handle<HierarchyGraphNode> handle) const
    {
        if (!handle.IsValid())
        {
            FATAL_LOG("Invalid handle provided.");
        }
        return m_nodes[handle.index].transform;
    }

    bool HierarchyGraph::Release(Handle<HierarchyGraphNode> handle, bool releaseTransform)
    {
        if (!handle.IsValid())
        {
            ERROR_LOG("Invalid handle provided. Nothing was done.");
            return false;
        }

        // Get the node at the provided index
        auto& node = m_nodes[handle.index];
        // Invalidate the node
        node.uuid.Invalidate();

        if (releaseTransform)
        {
            if (!Transforms.Release(node.transform))
            {
                ERROR_LOG("Failed to release transform as requested.");
                return false;
            }
        }

        return true;
    }

    u32 HierarchyGraph::CreateNode()
    {
        for (u32 i = 0; i < m_nodes.Size(); ++i)
        {
            auto& node = m_nodes[i];
            if (!node.uuid)
            {
                // This node is invalid (empty)
                // Generate a uuid
                node.uuid.Generate();
                // Return the index to this node
                return i;
            }
        }

        // We did not find an empty slot so let's create a new node and append it to the end

        // The index is equal to the m_nodes.Size() since we will append a new node
        auto index = m_nodes.Size();
        // Create a new node
        HierarchyGraphNode node;
        node.uuid.Generate();
        // Append the node to the end
        m_nodes.PushBack(node);
        // Return the index to the node
        return index;
    }

    void HierarchyGraph::UpdateNode(u32 nodeIndex, mat4& parentWorld, bool worldNeedsUpdate)
    {
        // Get the node
        auto& node = m_nodes[nodeIndex];
        // Stop at invalid nodes
        if (!node.uuid) return;
        // Check if this node has a transform
        if (!node.transform)
        {
            // We can immediatly update this node's children since this node has no transform
            for (auto index : node.children)
            {
                // The world matrix remains unchanged for it's children
                UpdateNode(index, parentWorld, worldNeedsUpdate);
            }
        }
        else
        {
            // Update the node's local matrix if needed
            if (Transforms.UpdateLocal(node.transform) || worldNeedsUpdate)
            {
                // If any local changed then all children need to update their world matrix
                worldNeedsUpdate = true;
                // Get the local matrix
                const auto& local = Transforms.GetLocal(node.transform);
                // Calculate this node's world
                parentWorld *= local;
                // Set this node's world
                Transforms.SetWorld(node.transform, parentWorld);
            }

            // Also update all this node's children
            for (auto index : node.children)
            {
                // This world matrix is now updated
                UpdateNode(index, parentWorld, worldNeedsUpdate);
            }
        }
    }

}  // namespace C3D