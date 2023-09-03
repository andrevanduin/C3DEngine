#include "light_system.h"

namespace C3D
{
    LightSystem::LightSystem(const SystemManager* pSystemsManager) : BaseSystem(pSystemsManager, "LIGHT_SYSTEM") {}

    bool LightSystem::Init()
    {
        // NOTE: Perform some kind of config/init here?
        m_pointLights.Create(128);
        m_initialized = true;
        m_logger.Info("Init() - Successfull");
        return true;
    }

    void LightSystem::Shutdown()
    {
        // NOTE: Perform some kind of cleanup here?
        m_pointLights.Destroy();
        m_initialized = false;
        m_logger.Info("Shutdown() - Successfull");
    }

    bool LightSystem::AddDirectionalLight(const DirectionalLight& light)
    {
        if (light.name.Empty())
        {
            m_logger.Error("AddDirectionalLight() - Failed to add DirectionalLight since the name is invalid");
            return false;
        }
        m_directionalLight = light;
        return true;
    }

    bool LightSystem::RemoveDirectionalLight(const String& name)
    {
        if (name.Empty())
        {
            m_logger.Error("RemoveDirectionalLight() - Failed to remove DirectionalLight since the name is invalid");
            return false;
        }

        if (m_directionalLight.name.Empty())
        {
            m_logger.Warn("RemoveDirectionalLight() - Tried removing Directional Light that is not present");
            return true;
        }

        if (m_directionalLight.name == name)
        {
            m_directionalLight.name = "";
            return true;
        }

        m_logger.Error(
            "RemoveDirectionalLight() - Tried to remove Directional Light named: '{}' but current light is named: '{}'",
            name, m_directionalLight.name);
        return false;
    }

    bool LightSystem::AddPointLight(const PointLight& pLight)
    {
        if (pLight.name.Empty())
        {
            m_logger.Error("AddPointLight() - Failed to add Point Light since the name is not valid");
            return false;
        }

        if (m_pointLights.Has(pLight.name))
        {
            m_logger.Error("AddPointLight() - Failed to add Point Light since the there is already a light named: '{}'",
                           pLight.name);
            return false;
        }

        if (m_pointLights.Count() >= MAX_POINT_LIGHTS)
        {
            m_logger.Error("AddPointLight() - Failed to add Point Light: no more room for Point Lights (MAX = {})",
                           MAX_POINT_LIGHTS);
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
            m_logger.Error("RemovePointLight() - Failed to remove Point Light since the name is not valid");
            return false;
        }

        if (!m_pointLights.Has(name))
        {
            m_logger.Error("RemovePointLight() - Failed to remove Point Light since none exist with the name: '{}'",
                           name);
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

        m_logger.Error("GetPointLight() - No Point Light exists with the name: '{}'", name);
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
