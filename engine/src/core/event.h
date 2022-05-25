
#pragma once
#include <vector>

#include "defines.h"

namespace C3D
{
	constexpr auto MAX_MESSAGE_CODES = 16384;

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

	enum class SystemEventCode: u16
	{
		ApplicationQuit = 0x01,
		KeyPressed = 0x02,
		KeyReleased = 0x03,
		ButtonPressed = 0x04,
		ButtonReleased = 0x05,
		MouseMoved = 0x06,
		MouseWheel = 0x07,
		Resized = 0x08,
		MaxCode = 0xFF
	};

	typedef bool (*pOnEvent)(u16 code, void* sender, void* listenerInstance, EventContext data);

	class C3D_API EventSystem
	{
		struct RegisteredEvent
		{
			void*	 listener;
			pOnEvent callback;
		};

		struct EventCodeEntry
		{
			std::vector<RegisteredEvent> events;
		};

		struct EventSystemState
		{
			EventCodeEntry registered[MAX_MESSAGE_CODES];
		};

	public:
		static bool Init();
		static void Shutdown();

		static bool Register(u16 code, void* listener, pOnEvent onEvent);
		static bool UnRegister(u16 code, const void* listener, pOnEvent onEvent);

		static bool Fire(u16 code, void* sender, EventContext data);

	private:
		static bool m_initialized;
		static EventSystemState m_state;
	};
}