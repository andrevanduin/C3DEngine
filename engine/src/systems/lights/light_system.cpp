#include "light_system.h"

namespace C3D
{
    LightSystem::LightSystem(const Engine* engine) : BaseSystem(engine, "LIGHT_SYSTEM") {}

    bool LightSystem::Init()
    {
        // NOTE: Perform some kind of config/init here?
        m_logger.Info("Init() - Successfull");
        m_initialized = true;
        return true;
    }

    void LightSystem::Shutdown()
    {
        // NOTE: Perform some kind of cleanup here?
        m_logger.Info("Shutdown() - Successfull");
        m_initialized = false;
    }

    bool LightSystem::AddDirectionalLight(DirectionalLight* light)
    {
        if (!light)
        {
            m_logger.Error("AddDirectionalLight() - Failed to add DirectionalLight since the pointer is not valid");
            return false;
        }

        m_directionalLight = light;
        return true;
    }

    bool LightSystem::RemoveDirectionalLight(DirectionalLight* light)
    {
        if (light == m_directionalLight)
        {
            m_directionalLight = nullptr;
            return true;
        }

        m_logger.Error("RemoveDirectionalLight() - Failed to remove Directional Light");
        return false;
    }

    bool LightSystem::AddPointLight(PointLight* pLight)
    {
        if (!pLight)
        {
            m_logger.Error("AddPointLight() - Failed to add Point Light since the pointer is not valid");
            return false;
        }

        for (auto& light : m_pointLights)
        {
            if (!light)
            {
                light = pLight;
                return true;
            }
        }

        m_logger.Error("AddPointLight() - Failed to add Point Light: no more room for Point Lights (MAX = {})",
                       MAX_POINT_LIGHTS);
        return false;
    }

    bool LightSystem::RemovePointLight(PointLight* pLight)
    {
        if (!pLight)
        {
            m_logger.Error("AddPointLight() - Failed to remove Point Light since the pointer is not valid");
            return false;
        }

        for (auto light : m_pointLights)
        {
            if (light == pLight)
            {
                light = nullptr;
                return true;
            }
        }

        m_logger.Error("RemovePointLight() - Failed to remove Point Light");
        return false;
    }
    u32 LightSystem::GetPointLightCount()
    {
        u32 counter = 0;
        for (auto l : m_pointLights)
        {
            if (l) counter++;
        }
        return counter;
    }

    DynamicArray<PointLight>& LightSystem::GetPointLights()
    {
        m_pointLightCache.Clear();

        for (auto l : m_pointLights)
        {
            if (l) m_pointLightCache.PushBack(*l);
        }

        return m_pointLightCache;
    }

    DirectionalLight* LightSystem::GetDirectionalLight() { return m_directionalLight; }
}  // namespace C3D
