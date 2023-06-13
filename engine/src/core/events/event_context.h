
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
		EventCodeApplicationQuit = 0x01,
		EventCodeKeyDown = 0x02,
		EventCodeKeyUp = 0x03,
		EventCodeButtonDown = 0x04,
		EventCodeButtonUp = 0x05,
		EventCodeMouseMoved = 0x06,
		EventCodeMouseScrolled = 0x07,
		EventCodeResized = 0x08,
		EventCodeMinimized = 0x09,
		EventCodeFocusGained = 0x0a,
		EventCodeSetRenderMode = 0x0b,

		EventCodeDebug0 = 0x10,
		EventCodeDebug1 = 0x11,
		EventCodeDebug2 = 0x12,
		EventCodeDebug3 = 0x13,
		EventCodeDebug4 = 0x14,

		EventCodeObjectHoverIdChanged = 0x15,
		EventCodeDefaultRenderTargetRefreshRequired = 0x16,
		EventCodeWatchedFileChanged = 0x17,
		EventCodeWatchedFileRemoved = 0x18,

		EventCodeMaxCode = 0xFF
	};
}