#include "light_system.h"

#include "resources/debug/debug_box_3d.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "LIGHT_SYSTEM";

    bool LightSystem::OnInit()
    {
        INFO_LOG("Initializing.");

        // NOTE: Perform some kind of config/init here?
        m_pointLights.Create(128);
        m_initialized = true;
        return true;
    }

    void LightSystem::OnShutdown()
    {
        INFO_LOG("Shutting down.");

        // NOTE: Perform some kind of cleanup here?
        m_pointLights.Destroy();
        m_initialized = false;
    }

    bool LightSystem::AddDirectionalLight(const DirectionalLight& light)
    {
        if (light.name.Empty())
        {
            ERROR_LOG("Failed to add DirectionalLight since the name is invalid.");
            return false;
        }
        m_directionalLight = light;
        return true;
    }

    bool LightSystem::RemoveDirectionalLight(const String& name)
    {
        if (name.Empty())
        {
            ERROR_LOG("Failed to remove DirectionalLight since the name is invalid.");
            return false;
        }

        if (m_directionalLight.name.Empty())
        {
            WARN_LOG("Tried removing Directional Light that is not present.");
            return true;
        }

        if (m_directionalLight.name == name)
        {
            m_directionalLight.name = "";
            return true;
        }

        ERROR_LOG("Tried to remove Directional Light named: '{}' but current light is named: '{}'.", name, m_directionalLight.name);
        return false;
    }

    bool LightSystem::AddPointLight(const PointLight& pLight)
    {
        if (pLight.name.Empty())
        {
            ERROR_LOG("Failed to add Point Light since the name is not valid.");
            return false;
        }

        if (m_pointLights.Has(pLight.name))
        {
            ERROR_LOG("Failed to add Point Light since the there is already a light named: '{}'.", pLight.name);
            return false;
        }

        if (m_pointLights.Count() >= MAX_POINT_LIGHTS)
        {
            ERROR_LOG("Failed to add Point Light: no more room for Point Lights (MAX = {}).", MAX_POINT_LIGHTS);
            return false;
        }

        m_pointLights.Set(pLight.name, pLight);
        m_cacheInvalid = true;
        return true;
    }

    bool LightSystem::RemovePointLight(const String& name)
    {
        if (name.Empty())
        {
            ERROR_LOG("Failed to remove Point Light since the name is not valid.");
            return false;
        }

        if (!m_pointLights.Has(name))
        {
            ERROR_LOG("Failed to remove Point Light since none exist with the name: '{}'.", name);
            return false;
        }

        m_pointLights.Delete(name);
        m_cacheInvalid = true;
        return true;
    }

    u32 LightSystem::GetPointLightCount() const { return m_pointLights.Count(); }

    PointLight* LightSystem::GetPointLight(const String& name)
    {
        if (m_pointLights.Has(name))
        {
            return &m_pointLights.Get(name);
        }

        ERROR_LOG("No Point Light exists with the name: '{}'.", name);
        return nullptr;
    }

    DynamicArray<PointLightData>& LightSystem::GetPointLights() const
    {
        if (m_cacheInvalid)
        {
            m_pointLightCache.Clear();
            for (const auto& l : m_pointLights)
            {
                m_pointLightCache.PushBack(l.data);
            }
            m_cacheInvalid = false;
        }
        return m_pointLightCache;
    }

    DirectionalLight* LightSystem::GetDirectionalLight() { return &m_directionalLight; }
}  // namespace C3D
