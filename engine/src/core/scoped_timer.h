
#pragma once
#include "containers/string.h"
#include "core/defines.h"

namespace C3D
{
    class SystemManager;

    /** @brief A scoped timer that starts on creation and prints the elapsed time on destruction.
     * Requires Platform to be initialized fully in order to work properly.
     */
    class ScopedTimer
    {
    public:
        ScopedTimer(const String& scopeName, const SystemManager* pSystemsManager);
        ~ScopedTimer();

    private:
        String m_name;
        f64 m_startTime;

        const SystemManager* m_pSystemsManager;
    };
}  // namespace C3D