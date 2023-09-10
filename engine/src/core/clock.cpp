
#include "clock.h"

#include "platform/platform.h"
#include "systems/system_manager.h"

namespace C3D
{
    Clock::Clock(const Platform* os) : m_elapsedTime(0), m_startTime(0), m_operatingSystem(os) {}

    void Clock::Update()
    {
        if (m_startTime != 0.0)
        {
            m_elapsedTime = m_operatingSystem->GetAbsoluteTime() - m_startTime;
        }
    }

    void Clock::Start()
    {
        m_startTime   = m_operatingSystem->GetAbsoluteTime();
        m_elapsedTime = 0;
    }

    void Clock::Stop() { m_startTime = 0; }

    /** @brief Returns the elepased time in seconds since clock.Start(). */
    f64 Clock::GetElapsed()
    {
        Update();
        return m_elapsedTime;
    }

    /** @brief Returns the elepased time in milliseconds since clock.Start(). */
    f64 Clock::GetElapsedMs()
    {
        Update();
        return m_elapsedTime * 1000;
    }
}  // namespace C3D
