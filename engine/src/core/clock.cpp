
#include "clock.h"
#include "platform/platform.h"

namespace C3D
{
	f64 Clock::m_elapsedTime;
	f64 Clock::m_startTime;

	void Clock::Update()
	{
		if (m_startTime != 0)
		{
			m_elapsedTime = Platform::GetAbsoluteTime() - m_startTime;
		}
	}

	void Clock::Start()
	{
		m_startTime = Platform::GetAbsoluteTime();
		m_elapsedTime = 0;
	}

	void Clock::Stop()
	{
		m_startTime = 0;
	}

	f64 Clock::GetElapsed()
	{
		return m_elapsedTime;
	}
}
