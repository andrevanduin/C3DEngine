
#pragma once
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "containers/string.h"
#include "core/console/console.h"
#include "core/cvars/cvar.h"
#include "systems/system.h"

#define DEFINE_CVAR_CREATE_FUNC(TypeName) bool Create(const CVarName& name, TypeName value)

namespace C3D
{
    class UIConsole;

    struct CVarSystemConfig
    {
        u32 maxCVars        = 0;
        UIConsole* pConsole = nullptr;
    };

    class CVarSystem final : public SystemWithConfig<CVarSystemConfig>
    {
    public:
        explicit CVarSystem(const SystemManager* pSystemsManager);

        bool OnInit(const CVarSystemConfig& config) override;

        /**
         * @brief Creates a new CVar with the specified name and value.
         *
         * @param name The name of the CVar
         * @param value The value that the CVar should have
         * @return True if successful; false otherwise
         */
        template <typename T>
        bool Create(const CVarName& name, T value)
        {
            if (Exists(name))
            {
                INSTANCE_ERROR_LOG("CVAR_SYSTEM", "A CVar named: '{}' already exists.", name);
                return false;
            }

            INSTANCE_INFO_LOG("CVAR_SYSTEM", "Successfully created CVar: '{}'.", name);
            m_cVars.Set(name, CVar(name, value));
            return true;
        }

        /**
         * @brief Creates a new CVar with the specified name, value and on changed callback.
         *
         * @param name The name of the CVar
         * @param value The value that the CVar should have
         * @param cb A callback function that should be called when the value of this CVar changes
         * @return True if successful; false otherwise
         */
        template <typename T>
        bool Create(const CVarName& name, T value, CVarOnChangedCallback&& cb)
        {
            if (!Create(name, value)) return false;

            auto& cvar = Get(name);
            return cvar.AddOnChangeCallback(std::forward<CVarOnChangedCallback>(cb));
        }

        bool Remove(const CVarName& name);
        bool Exists(const CVarName& name) const;

        CVar& Get(const CVarName& name);

        bool Print(const CVarName& name, CString<256>& output);

        [[nodiscard]] String PrintAll() const;

        void RegisterDefaultCommands();

        void OnShutdown() override;

    private:
        bool OnCVarCommand(const DynamicArray<ArgName>& args, String& output);

        HashMap<CVarName, CVar> m_cVars;

        UIConsole* m_pConsole;
    };
}  // namespace C3D