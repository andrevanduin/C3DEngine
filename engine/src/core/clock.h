
#pragma once
#include "defines.h"
#include "platform/platform.h"

namespace C3D
{
	class C3D_API Clock
	{
	public:
		explicit Clock(const Platform* os);

		void Update();
		void Start();
		void Stop();

		[[nodiscard]] f64 GetElapsed() const;

	private:
		f64 m_elapsedTime;
		f64 m_startTime;

		const Platform* m_operatingSystem;
	};
}
