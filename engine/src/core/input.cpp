
#include "input.h"

#include "events/event.h"
#include "logger.h"
#include "platform/platform.h"
#include "systems/system_manager.h"

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

	void InputSystem::ProcessKey(const SDL_Keycode sdlKey, const bool down)
	{
		u16 key;
		switch (sdlKey)
		{
			case SDLK_UP:
				key = KeyArrowUp;
				break;
			case SDLK_DOWN:
				key = KeyArrowDown;
				break;
			case SDLK_LEFT:
				key = KeyArrowLeft;
				break;
			case SDLK_RIGHT:
				key = KeyArrowRight;
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
			m_logger.Warn("Key{} keycode was larger than expected {}", down ? "Down" : "Up", key);
			return;
		}

		if (m_state.keyboardCurrent.keys[key] != down)
		{
			m_state.keyboardCurrent.keys[key] = down;

			EventContext context{};
			context.data.u16[0] = static_cast<u16>(key);
			
			const auto code = down ? SystemEventCode::KeyDown : SystemEventCode::KeyUp;
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

			const auto code = pressed ? SystemEventCode::ButtonDown : SystemEventCode::ButtonUp;
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

			Event.Fire(ToUnderlying(SystemEventCode::MouseMoved), nullptr, context);
		}
	}

	void InputSystem::ProcessMouseWheel(const i32 delta)
	{
		EventContext context{};
		context.data.i8[0] = static_cast<i8>(delta);
		Event.Fire(ToUnderlying(SystemEventCode::MouseScrolled), nullptr, context);
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

	bool InputSystem::IsKeyPressed(const u8 key) const
	{
		if (!m_initialized) return false;
		return m_state.keyboardCurrent.keys[key] && !m_state.keyboardPrevious.keys[key];
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

	bool InputSystem::IsButtonPressed(const Buttons button) const
	{
		if (!m_initialized) return false;
		return m_state.mouseCurrent.buttons[button] && !m_state.mousePrevious.buttons[button];
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

	bool InputSystem::IsShiftHeld() const
	{
		if (!m_initialized) return false;
		return m_state.keyboardCurrent.keys[KeyShift] || m_state.keyboardCurrent.keys[KeyLShift] || m_state.keyboardCurrent.keys[KeyRShift];
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
