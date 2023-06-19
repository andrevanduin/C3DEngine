
#pragma once
#include "core/defines.h"

namespace C3D
{
    enum class SceneState
    {
        /** @brief Created, but not initialized yet */
        Uninitialized,
        /** @brief Configuration is parsed and hierarchy is setup but not loaded yet */
        Initialized,
        /** @brief Loading the actual hierarchy */
        Loading,
        /** @brief Loading is done and the scene is ready to play */
        Loaded,
        /** @brief The scene is currently unloading (can't play anymore) */
        Unloading,
        /** @brief The scene is unloaded and ready to be destroyed */
        Unloaded,
    };

    class SimpleScene
    {
    public:
        /**
         * @brief Creates a new scene with the given config. No resources are allocated yet
         *
         * @return True if successfull, false otherwise
         */
        bool Create();

        /**
         * @brief Initializes the scene. Processes configuration and sets up the hierarchy
         *
         * @return True if successfull, false otherwise
         */
        bool Initialize();

        /**
         * @brief Loads the scene. Allocates the resources required to actually show the scene.
         * After calling this method the scene becomes playable
         *
         * @return True if successfull, false otherwise
         */
        bool Load();

        /**
         * @brief Unloads the scene. Deallocates the resources for the scene.
         * After calling this method the scene is in an unloaded state ready to be destroyed.
         *
         * @return True if successfull, false otherwise
         */
        bool Unload();

        bool Update(f64 deltaTime);

        /**
         * @brief Destroys the given scene. Performs an unload first if the scene is loaded
         *
         * @return True if successfull, false otherwise
         */
        bool Destroy();

        u32 GetId() const { return m_id; }
        SceneState GetState() const { return m_state; }
        bool IsEnabled() const { return m_enabled; }

    private:
        u32 m_id;
        SceneState m_state;
        bool m_enabled;
    };
}  // namespace C3D