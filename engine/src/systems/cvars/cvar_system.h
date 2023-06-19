
#pragma once
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "containers/string.h"
#include "core/console/console.h"
#include "core/cvars/cvar.h"
#include "systems/system.h"

namespace C3D
{
    class UIConsole;

    struct CVarSystemConfig
    {
        u32 maxCVars = 0;
        UIConsole* pConsole = nullptr;
    };

    using ICVarChangedCallback = std::function<void(const ICVar& cVar)>;

    template <typename T>
    using CVarChangedCallback = std::function<void(const CVar<T>& cVar)>;

    class CVarSystem final : public SystemWithConfig<CVarSystemConfig>
    {
    public:
        explicit CVarSystem(const SystemManager* pSystemsManager);

        bool Init(const CVarSystemConfig& config) override;
        void Shutdown() override;

        template <typename T>
        bool Create(const CVarName& name, const T& value)
        {
            if (Exists(name))
            {
                m_logger.Error("Create() - Failed to create CVar. A CVar named: \'{}\' already exists!", name);
                return false;
            }

            const auto pCVar = Memory.New<CVar<T>>(MemoryType::CVar, name, value);
            m_cVars.Set(name, pCVar);
            return true;
        }

        bool Remove(const CVarName& name);

        [[nodiscard]] bool Exists(const CVarName& name) const;

        template <typename T>
        CVar<T>& Get(const CVarName& name) const
        {
            if (!Exists(name))
            {
                m_logger.Fatal("Get() - Failed to find a CVar with the name: \'{}\'", name);
            }
            return *reinterpret_cast<CVar<T>*>(m_cVars.Get(name));
        }

        bool Print(const CVarName& name, CString<256>& output);

        [[nodiscard]] String PrintAll() const;

        void RegisterDefaultCommands();

    private:
        bool CreateDefaultCVars();

        template <typename T>
        void SetCVar(const CString<128>& name, const T& value, String& output)
        {
            const auto cVar = reinterpret_cast<CVar<T>*>(m_cVars.Get(name));
            cVar->SetValue(value);
            output += cVar->ToString();
        }

        bool OnCVarCommand(const DynamicArray<ArgName>& args, String& output);

        HashMap<CVarName, ICVar*> m_cVars;
        DynamicArray<ICVarChangedCallback*> m_cVarChangedCallbacks;

        UIConsole* m_pConsole;
    };
}  // namespace C3D