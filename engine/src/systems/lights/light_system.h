
#pragma once
#include "containers/array.h"
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "core/defines.h"
#include "math/math_types.h"
#include "systems/system.h"

namespace C3D
{
    constexpr auto MAX_POINT_LIGHTS = 10;

    /** @brief Shader data required for a directional light */
    struct DirectionalLightData
    {
        vec4 color     = vec4(1.0);
        vec4 direction = vec4(0);  // Ignore w (only for 16 byte alignment)
    };

    /** @brief Structure for a directional light (typically used to emulate a sun) */
    struct DirectionalLight
    {
        /** @brief The name of this Directional Light. */
        String name;
        /** @brief The Shader data for this Directional Light. */
        DirectionalLightData data;
        /** @brief User-defined debug data. */
        void* debugData;

        DirectionalLight() = default;
    };

    /** @brief Shader data required for a Point Light */
    struct PointLightData
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

    /* @brief Structure for a Point Light */
    struct PointLight
    {
        /** @brief The name of this Point Light. */
        String name;
        /** @brief The Shader data for this Point Light. */
        PointLightData data;
        /** @brief User-defined debug data. */
        void* debugData;
    };

    class C3D_API LightSystem final : public BaseSystem
    {
    public:
        explicit LightSystem(const SystemManager* pSystemsManager);

        bool OnInit() override;
        void OnShutdown() override;

        /**
         * @brief Adds a Directional Light to the scene
         *
         * @param light The Directional Light you want to add
         * @return True if successfully added, false otherwise
         */
        bool AddDirectionalLight(const DirectionalLight& light);

        /**
         * @brief Removes a Direction Light from the scene
         *
         * @param name The name of the Directional Light you want to remove
         * @return True if successfully removed, false otherwise
         */
        bool RemoveDirectionalLight(const String& name);

        /**
         * @brief Adds a Point Light to the scene
         *
         * @param pLight The Point Light you want to add
         * @return True if successfully added, false otherwise
         */
        bool AddPointLight(const PointLight& pLight);

        /**
         * @brief Removes a Point Light from the scene
         *
         * @param name The name of the Point Light you want to remove
         * @return True  if successfully removed, false otherwise
         */
        bool RemovePointLight(const String& name);

        /**
         * @brief Get amount of Point Lights we currently have in our scene
         *
         * @return The amount of Point Lights
         */
        u32 GetPointLightCount() const;

        /**
         * @brief Get a Point Light object
         *
         * @param name The name of the point light you want to get
         *
         * @return Pointer to the Point light
         */
        PointLight* GetPointLight(const String& name);

        /**
         * @brief Gets the Point Lights currently in the scene
         *
         * @return A reference to a DynamicArray that contains all the Point Lights currently in the scene
         */
        DynamicArray<PointLightData>& GetPointLights() const;

        /**
         * @brief Get the Directional Light that is currently in the scene
         *
         * @return A pointer to the directional light
         */
        DirectionalLight* GetDirectionalLight();

        void InvalidatePointLightCache() const { m_cacheInvalid = true; }

    private:
        DirectionalLight m_directionalLight;

        HashMap<String, PointLight> m_pointLights;

        mutable DynamicArray<PointLightData> m_pointLightCache;
        mutable bool m_cacheInvalid = true;
    };
}  // namespace C3D