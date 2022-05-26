
#pragma once
#include "core/defines.h"

namespace C3D
{
	struct EventContext
	{
		union
		{
			i64 i64[2];
			u64 u64[2];
			f64 f64[2];

			i32 i32[4];
			u32 u32[4];
			f32 f32[4];

			i16 i16[8];
			u16 u16[8];

			i8 i8[16];
			u8 u8[16];

			char c[16];
		} data;
	};

	enum SystemEventCode : u16
	{
		ApplicationQuit = 0x01,
		KeyPressed = 0x02,
		KeyReleased = 0x03,
		ButtonPressed = 0x04,
		ButtonReleased = 0x05,
		MouseMoved = 0x06,
		MouseWheel = 0x07,
		Resized = 0x08,
		Minimized = 0x09,
		FocusGained = 0x10,
		MaxCode = 0xFF
	};
}