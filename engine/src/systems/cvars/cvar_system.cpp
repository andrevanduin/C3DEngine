
#include "cvar_system.h"

#include "core/engine.h"

namespace C3D
{
    CVarSystem::CVarSystem(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager, "CVAR_SYSTEM") {}

    bool CVarSystem::Init(const CVarSystemConfig& config)
    {
        m_logger.Info("Init()");

        m_config = config;
        m_pConsole = config.pConsole;

        m_cVars.Create(config.maxCVars);

        if (!CreateDefaultCVars())
        {
            m_logger.Error("Init() - Failed to create default CVars");
            return false;
        }

        m_initialized = true;
        return true;
    }

    void CVarSystem::Shutdown()
    {
        m_logger.Info("Shutdown()");

        for (const auto cVar : m_cVars)
        {
            Remove(cVar->name);
        }
        m_cVars.Destroy();
        m_initialized = false;
    }

    bool CVarSystem::Remove(const CVarName& name)
    {
        if (!Exists(name))
        {
            m_logger.Error("Remove() - No CVar with name: \'{}\' exist!", name);
            return false;
        }

        const auto cVar = m_cVars.Get(name);
        m_cVars.Delete(name);

        Memory.Delete(MemoryType::CVar, cVar);
        return true;
    }

    bool CVarSystem::Exists(const CVarName& name) const { return m_cVars.Has(name); }

    bool CVarSystem::Print(const CVarName& name, CString<256>& output)
    {
        if (!Exists(name))
        {
            m_logger.Error("Print() - No CVar with name: \'{}\' exists!", name);
            return false;
        }

        output = m_cVars.Get(name)->ToString();
        return true;
    }

    String CVarSystem::PrintAll() const
    {
        String vars = "";
        for (const auto cVar : m_cVars)
        {
            vars += cVar->ToString();
            vars += "\n";
        }
        return vars;
    }

    void CVarSystem::RegisterDefaultCommands()
    {
        m_pConsole->RegisterCommand(
            "cvar", [this](const DynamicArray<ArgName>& args, String& output) { return OnCVarCommand(args, output); });
    }

    bool CVarSystem::CreateDefaultCVars()
    {
        if (!Create("vsync", true)) return false;

        return true;
    }

    bool CVarSystem::OnCVarCommand(const DynamicArray<ArgName>& args, String& output)
    {
        if (args.Size() <= 1)
        {
            output = "Not enough arguments provided";
            return false;
        }

        const auto commandType = args[1];

        // cvar print [ all | name ]
        if (commandType == "print")
        {
            // Print command
            if (args.Size() != 3)
            {
                output = "The print command requires the name of a CVar or the 'all' argument";
                return false;
            }

            const auto arg2 = args[2];
            if (arg2 == "all")
            {
                // Print all command
                output = PrintAll();
                return true;
            }

            // Assume arg2 is the name of a CVar
            if (!Exists(arg2))
            {
                output = String::FromFormat("The CVar \'{}\' does not exist!", arg2);
                return false;
            }

            const auto cVar = m_cVars.Get(arg2);
            output += cVar->ToString();
            return true;
        }

        // cvar set name type value
        if (commandType == "set")
        {
            // Set command
            if (args.Size() != 5)
            {
                output = "The set command requires the name of a CVar, the type and a value to set";
                return false;
            }

            const auto cVarName = args[2];
            if (!Exists(cVarName))
            {
                output = String::FromFormat("The CVar \'{}\' does not exist!", cVarName);
                return false;
            }

            const auto type = args[3];
            const auto valueStr = args[4];

            if (type == "i32" || type == "int")
            {
                SetCVar(cVarName, valueStr.ToI32(), output);
                return true;
            }

            if (type == "u32")
            {
                SetCVar(cVarName, valueStr.ToU32(), output);
                return true;
            }

            if (type == "bool")
            {
                SetCVar(cVarName, valueStr.ToBool(), output);
                return true;
            }

            output = String::FromFormat("Unknown type: \'{}\'", type);
            return false;
        }

        output = String::FromFormat("Unknown argument \'{}\'", commandType);
        return false;
    }
}  // namespace C3D
