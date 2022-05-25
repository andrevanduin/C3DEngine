
#pragma once
#include "defines.h"

namespace C3D
{
	class Clock
	{
	public:
		static void Update();
		static void Start();
		static void Stop();

		static f64 GetElapsed();

	private:
		static f64 m_elapsedTime;
		static f64 m_startTime;
	};
}