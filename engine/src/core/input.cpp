
#include "input.h"

#include "events/event.h"
#include "logger.h"
#include "platform/platform.h"
#include "services/services.h"

namespace C3D
{
	InputSystem::InputSystem() : m_logger("INPUT"), m_initialized(false), m_state() {}

	bool InputSystem::Init()
	{
		m_logger.Info("Init()");

		m_initialized = true;
		return true;
	}

	void InputSystem::Shutdown()
	{
		m_logger.Info("Shutting Down");
		m_initialized = false;
	}

	void InputSystem::Update(f64 deltaTime)
	{
		if (!m_initialized) return;

		Platform::MemCopy(&m_state.keyboardPrevious, &m_state.keyboardCurrent, sizeof KeyBoardState);
		Platform::MemCopy(&m_state.mousePrevious, &m_state.mouseCurrent, sizeof MouseState);
	}

	void InputSystem::ProcessKey(const SDL_Keycode sdlKey, const bool pressed)
	{
		u16 key;
		switch (sdlKey)
		{
			case SDLK_UP:
				key = KeyUp;
				break;
			case SDLK_DOWN:
				key = KeyDown;
				break;
			case SDLK_LALT:
				key = KeyLAlt;
				break;
			case SDLK_RALT:
				key = KeyRAlt;
				break;
			case SDLK_LSHIFT:
				key = KeyLShift;
				break;
			case SDLK_RSHIFT:
				key = KeyRShift;
				break;
			case SDLK_LCTRL:
				key = KeyLControl;
				break;
			case SDLK_RCTRL:
				key = KeyRControl;
				break;
			default:
				key = static_cast<u8>(sdlKey);
				break;
		}

		if (key >= Keys::MaxKeys)
		{
			m_logger.Warn("Key that was {} was larger than expected {}", pressed ? "pressed" : "released", key);
			return;
		}

		if (m_state.keyboardCurrent.keys[key] != pressed)
		{
			m_state.keyboardCurrent.keys[key] = pressed;

			EventContext context{};
			context.data.u16[0] = static_cast<u16>(key);
			
			const auto code = pressed ? SystemEventCode::KeyPressed : SystemEventCode::KeyReleased;
			Event.Fire(static_cast<u16>(code), nullptr, context);
		}
	}

	void InputSystem::ProcessButton(const u8 button, const bool pressed)
	{
		if (m_state.mouseCurrent.buttons[button] != pressed)
		{
			m_state.mouseCurrent.buttons[button] = pressed;

			EventContext context{};
			context.data.u16[0] = button;

			const auto code = pressed ? SystemEventCode::ButtonPressed : SystemEventCode::ButtonReleased;
			Event.Fire(ToUnderlying(code), nullptr, context);
		}
	}

	void InputSystem::ProcessMouseMove(const i32 sdlX, const i32 sdlY)
	{
		const auto x = static_cast<i16>(sdlX);
		const auto y = static_cast<i16>(sdlY);

		if (m_state.mouseCurrent.x != x || m_state.mouseCurrent.y != y)
		{
			m_state.mouseCurrent.x = x;
			m_state.mouseCurrent.y = y;

			EventContext context{};
			context.data.i16[0] = x;
			context.data.i16[1] = y;

			Services::GetEvent().Fire(ToUnderlying(SystemEventCode::MouseMoved), nullptr, context);
		}
	}

	void InputSystem::ProcessMouseWheel(const i32 delta)
	{
		EventContext context{};
		context.data.i8[0] = static_cast<i8>(delta);
		Services::GetEvent().Fire(ToUnderlying(SystemEventCode::MouseWheel), nullptr, context);
	}

	bool InputSystem::IsKeyDown(const u8 key) const
	{
		if (!m_initialized) return false;
		return m_state.keyboardCurrent.keys[key];
	}

	bool InputSystem::IsKeyUp(const u8 key) const
	{
		if (!m_initialized) return true;
		return !m_state.keyboardCurrent.keys[key];
	}

	bool InputSystem::WasKeyDown(const u8 key) const
	{
		if (!m_initialized) return false;
		return m_state.keyboardPrevious.keys[key];
	}

	bool InputSystem::WasKeyUp(const u8 key) const
	{
		if (!m_initialized) return true;
		return !m_state.keyboardPrevious.keys[key];
	}

	bool InputSystem::IsButtonDown(const Buttons button) const
	{
		if (!m_initialized) return false;
		return m_state.mouseCurrent.buttons[button];
	}

	bool InputSystem::IsButtonUp(const Buttons button) const
	{
		if (!m_initialized) return true;
		return !m_state.mouseCurrent.buttons[button];
	}

	bool InputSystem::WasButtonDown(const Buttons button) const
	{
		if (!m_initialized) return false;
		return m_state.mousePrevious.buttons[button];
	}

	bool InputSystem::WasButtonUp(const Buttons button) const
	{
		if (!m_initialized) return true;
		return !m_state.mousePrevious.buttons[button];
	}

	ivec2 InputSystem::GetMousePosition()
	{
		if (!m_initialized)
		{
			return ivec2(0);
		}
		return { m_state.mouseCurrent.x, m_state.mouseCurrent.y };
	}

	ivec2 InputSystem::GetPreviousMousePosition()
	{
		if (!m_initialized)
		{
			return ivec2(0);
		}
		return { m_state.mousePrevious.x, m_state.mousePrevious.y };
	}
}
