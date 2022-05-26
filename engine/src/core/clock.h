
#pragma once
#include "defines.h"

namespace C3D
{
	class C3D_API Clock
	{
	public:
		void Update();
		void Start();
		void Stop();

		[[nodiscard]] f64 GetElapsed() const;

	private:
		f64 m_elapsedTime = 0;
		f64 m_startTime = 0;
	};
}