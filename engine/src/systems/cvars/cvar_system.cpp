
#include "cvar_system.h"

#include "core/engine.h"

#define IMPLEMENT_CVAR_CREATE_FUNC(TypeName)                                                       \
    bool CVarSystem::Create(const CVarName& name, TypeName value)                                  \
    {                                                                                              \
        if (Exists(name))                                                                          \
        {                                                                                          \
            INSTANCE_ERROR_LOG("Failed to create CVar. A CVar named: '{}' already exists!", name); \
            return false;                                                                          \
        }                                                                                          \
        m_cVars.Set(name, CVar(CVarType::T##TypeName, name, value));                               \
        return true;                                                                               \
    }

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "CVAR_SYSTEM";

    CVarSystem::CVarSystem(const SystemManager* pSystemsManager) : SystemWithConfig(pSystemsManager) {}

    bool CVarSystem::OnInit(const CVarSystemConfig& config)
    {
        INFO_LOG("Initializing.");

        m_config   = config;
        m_pConsole = config.pConsole;

        m_cVars.Create(config.maxCVars);

        if (!Create("vsync", true)) return false;

        m_initialized = true;
        return true;
    }

    bool CVarSystem::Remove(const CVarName& name)
    {
        if (!Exists(name))
        {
            ERROR_LOG("No CVar with name: '{}' exist!", name);
            return false;
        }

        m_cVars.Delete(name);
        return true;
    }

    bool CVarSystem::Exists(const CVarName& name) const { return m_cVars.Has(name); }

    CVar& CVarSystem::Get(const CVarName& name)
    {
        if (!Exists(name))
        {
            FATAL_LOG("Failed to find a CVar with the name: '{}'.", name);
        }
        return m_cVars.Get(name);
    }

    void CVarSystem::OnShutdown()
    {
        INFO_LOG("Shutting down.");
        m_cVars.Destroy();
        m_initialized = false;
    }

    bool CVarSystem::Print(const CVarName& name, CString<256>& output)
    {
        if (!Exists(name))
        {
            ERROR_LOG("No CVar with name: '{}' exists!", name);
            return false;
        }

        output = m_cVars.Get(name).AsString();
        return true;
    }

    String CVarSystem::PrintAll() const
    {
        String vars = "";
        for (const auto& cVar : m_cVars)
        {
            vars += cVar.AsString();
            vars += "\n";
        }
        return vars;
    }

    void CVarSystem::RegisterDefaultCommands()
    {
        m_pConsole->RegisterCommand("cvar",
                                    [this](const DynamicArray<ArgName>& args, String& output) { return OnCVarCommand(args, output); });
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
                output = String::FromFormat("The CVar '{}' does not exist!", arg2);
                return false;
            }

            const auto cVar = m_cVars.Get(arg2);
            output += cVar.AsString();
            return true;
        }

        // cvar set name value
        if (commandType == "set")
        {
            // Set command
            if (args.Size() != 4)
            {
                output = "The set command requires the name of a CVar and the value to set";
                return false;
            }

            const auto cVarName = args[2];
            if (!Exists(cVarName))
            {
                output = String::FromFormat("The CVar '{}' does not exist!", cVarName);
                return false;
            }

            auto& cVar          = Get(cVarName);
            const auto valueStr = args[3];

            switch (cVar.GetType())
            {
                case CVarType::U8:
                    cVar.SetValue(valueStr.ToU8());
                    output += cVar.AsString();
                    return true;
                case CVarType::U16:
                    cVar.SetValue(valueStr.ToU16());
                    output += cVar.AsString();
                    return true;
                case CVarType::U32:
                    cVar.SetValue(valueStr.ToU32());
                    output += cVar.AsString();
                    return true;
                case CVarType::U64:
                    cVar.SetValue(valueStr.ToU64());
                    output += cVar.AsString();
                    return true;
                case CVarType::I8:
                    cVar.SetValue(valueStr.ToI8());
                    output += cVar.AsString();
                    return true;
                case CVarType::I16:
                    cVar.SetValue(valueStr.ToI16());
                    output += cVar.AsString();
                    return true;
                case CVarType::I32:
                    cVar.SetValue(valueStr.ToI32());
                    output += cVar.AsString();
                    return true;
                case CVarType::I64:
                    cVar.SetValue(valueStr.ToI64());
                    output += cVar.AsString();
                    return true;
                case CVarType::F32:
                    cVar.SetValue(valueStr.ToF32());
                    output += cVar.AsString();
                    return true;
                case CVarType::F64:
                    cVar.SetValue(valueStr.ToF64());
                    output += cVar.AsString();
                    return true;
                case CVarType::Bool:
                    cVar.SetValue(valueStr.ToBool());
                    output += cVar.AsString();
                    return true;
                default:
                    output = String::FromFormat("Unknown type: '{}'.", ToString(cVar.GetType()));
                    return false;
            }
        }

        output = String::FromFormat("Unknown argument '{}'.", commandType);
        return false;
    }
}  // namespace C3D
