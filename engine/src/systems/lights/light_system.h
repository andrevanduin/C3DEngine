
#pragma once
#include "containers/array.h"
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "math/math_types.h"
#include "systems/system.h"

namespace C3D
{
    constexpr auto MAX_POINT_LIGHTS = 10;

    struct DirectionalLight
    {
        vec4 color;
        vec4 direction;  // Ignore w (only for 16 byte alignment)
    };

    struct PointLight
    {
        vec4 color;
        vec4 position;  // Ignore w (only for 16 byte alignment)
        // Usually 1, make sure denominator never gets smaller than 1
        f32 fConstant;
        // Reduces light intensity linearly
        f32 linear;
        // Makes the light fall of slower at longer dinstances
        f32 quadratic;
        f32 padding;
    };

    class C3D_API LightSystem final : public BaseSystem
    {
    public:
        LightSystem(const Engine* engine);

        bool Init() override;
        void Shutdown() override;

        /**
         * @brief Adds a Directional Light to the scene
         *
         * @param light The Directional Light you want to add
         * @return True if successfully added, false otherwise
         */
        bool AddDirectionalLight(DirectionalLight* light);

        /**
         * @brief Removes a Direction Light from the scene
         *
         * @param light The Directional Light you want to remove
         * @return True if successfully removed, false otherwise
         */
        bool RemoveDirectionalLight(DirectionalLight* light);

        /**
         * @brief Adds a Point Light to the scene
         *
         * @param light The Point Light you want to add
         * @return True if successfully added, false otherwise
         */
        bool AddPointLight(PointLight* pLight);

        /**
         * @brief Removes a Point Light from the scene
         *
         * @param light The Point Light you want to remove
         * @return True if successfully removed, false otherwise
         */
        bool RemovePointLight(PointLight* pLight);

        /**
         * @brief Get amount of Point Lights we currently have in our scene
         *
         * @return The amount of Point Lights
         */
        u32 GetPointLightCount();

        /**
         * @brief Gets the Point Lights currently in the scene
         *
         * @return A reference to a DynamicArray that contains all the Point Lights currently in the scene
         */
        DynamicArray<PointLight>& GetPointLights();

        /**
         * @brief Get the Directional Light that is currently in the scene
         *
         * @return A pointer to the directional light
         */
        DirectionalLight* GetDirectionalLight();

    private:
        DirectionalLight* m_directionalLight;
        Array<PointLight*, MAX_POINT_LIGHTS> m_pointLights;

        DynamicArray<PointLight> m_pointLightCache;
    };
}  // namespace C3D