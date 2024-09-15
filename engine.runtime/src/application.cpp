
#include "application.h"

#include "parsers/cson_parser.h"

namespace C3D
{
    void Application::ParseWindowConfig(const CSONObject& config)
    {
        auto windowConfig = WindowConfig();
        for (const auto& prop : config.properties)
        {
            if (prop.name.IEquals("name"))
            {
                windowConfig.name = prop.GetString();
            }
            else if (prop.name.IEquals("width"))
            {
                windowConfig.width = prop.GetI64();
            }
            else if (prop.name.IEquals("height"))
            {
                windowConfig.height = prop.GetI64();
            }
            else if (prop.name.IEquals("title"))
            {
                windowConfig.title = prop.GetString();
            }
            else if (prop.name.IEquals("position"))
            {
                const auto& pos = prop.GetString();
                if (pos.IEquals("center"))
                {
                    windowConfig.flags |= WindowFlag::WindowFlagCenter;
                }
            }
            else if (prop.name.IEquals("fullscreen"))
            {
                if (prop.GetBool())
                {
                    windowConfig.flags |= WindowFlag::WindowFlagFullScreen;
                }
            }
        }
        m_appConfig.windowConfigs.PushBack(windowConfig);
    }

    Application::Application(ApplicationState* state)
    {
        CSONParser parser;
        auto config = parser.ParseFile("../../../testenv/assets/application_config.cson");

        // Initialize our SystemConfigs HashMap
        m_appConfig.systemConfigs.Create();

        for (const auto& property : config.properties)
        {
            if (property.name.IEquals("applicationname"))
            {
                m_appConfig.name = property.GetString();
            }
            else if (property.name.IEquals("frameallocatorsize"))
            {
                m_appConfig.frameAllocatorSize = MebiBytes(property.GetI64());
            }
            else if (property.name.IEquals("windows"))
            {
                const auto& windowConfigs = property.GetArray();
                for (const auto& config : windowConfigs.properties)
                {
                    ParseWindowConfig(config.GetObject());
                }
            }
            else if (property.name.IEquals("systemconfigs"))
            {
                const auto& systemConfigs = property.GetArray();
                for (const auto& systemConfig : systemConfigs.properties)
                {
                    // Get the object containing name and config properties
                    const auto& obj = systemConfig.GetObject();
                    // The first property should be the name
                    const auto& name = obj.properties[0].GetString();
                    // The sceond property should be the config object
                    const auto& config = obj.properties[1].GetObject();
                    // Store the config object in our HashMap
                    m_appConfig.systemConfigs.Set(name, config);
                }
            }
        }
    }
}  // namespace C3D