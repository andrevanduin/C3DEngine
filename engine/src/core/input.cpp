
#include "input.h"

#include "event.h"
#include "logger.h"
#include "memory.h"

namespace C3D
{
	InputSystem::InputState InputSystem::m_state;
	bool InputSystem::m_initialized = false;


	bool InputSystem::Init()
	{
		Logger::PrefixInfo("INPUT", "Init()");

		Memory::Zero(&m_state, sizeof m_state);
		m_initialized = true;
		return true;
	}

	void InputSystem::Shutdown()
	{
		Logger::PrefixInfo("INPUT", "Shutting Down");
		m_initialized = false;
	}

	void InputSystem::Update(f64 deltaTime)
	{
		if (!m_initialized) return;

		Memory::Copy(&m_state.keyboardPrevious, &m_state.keyboardCurrent, sizeof KeyBoardState);
		Memory::Copy(&m_state.mousePrevious, &m_state.mouseCurrent, sizeof MouseState);
	}

	void InputSystem::ProcessKey(const SDL_Keycode key, const bool pressed)
	{
		if (m_state.keyboardCurrent.keys[key] != pressed)
		{
			m_state.keyboardCurrent.keys[key] = pressed;

			EventContext context{};
			context.data.u16[0] = static_cast<u16>(key);


			auto code = pressed ? SystemEventCode::KeyPressed : SystemEventCode::KeyReleased;
			EventSystem::Fire(static_cast<u16>(code), nullptr, context);
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
			EventSystem::Fire(ToUnderlying(code), nullptr, context);
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

			EventSystem::Fire(ToUnderlying(SystemEventCode::MouseMoved), nullptr, context);
		}
	}

	void InputSystem::ProcessMouseWheel(const i32 delta)
	{
		EventContext context{};
		context.data.i8[0] = static_cast<i8>(delta);
		EventSystem::Fire(ToUnderlying(SystemEventCode::MouseWheel), nullptr, context);
	}

	bool InputSystem::IsKeyDown(const Keys key)
	{
		if (!m_initialized) return false;
		return m_state.keyboardCurrent.keys[ToUnderlying(key)];
	}

	bool InputSystem::IsKeyUp(const Keys key)
	{
		if (!m_initialized) return true;
		return !m_state.keyboardCurrent.keys[ToUnderlying(key)];
	}

	bool InputSystem::WasKeyDown(const Keys key)
	{
		if (!m_initialized) return false;
		return m_state.keyboardPrevious.keys[ToUnderlying(key)];
	}

	bool InputSystem::WasKeyUp(const Keys key)
	{
		if (!m_initialized) return true;
		return !m_state.keyboardPrevious.keys[ToUnderlying(key)];
	}

	bool InputSystem::IsButtonDown(const Buttons button)
	{
		if (!m_initialized) return false;
		return m_state.mouseCurrent.buttons[ToUnderlying(button)];
	}

	bool InputSystem::IsButtonUp(const Buttons button)
	{
		if (!m_initialized) return true;
		return !m_state.mouseCurrent.buttons[ToUnderlying(button)];
	}

	bool InputSystem::WasButtonDown(const Buttons button)
	{
		if (!m_initialized) return false;
		return m_state.mousePrevious.buttons[ToUnderlying(button)];
	}

	bool InputSystem::WasButtonUp(const Buttons button)
	{
		if (!m_initialized) return true;
		return !m_state.mousePrevious.buttons[ToUnderlying(button)];
	}

	void InputSystem::GetMousePosition(i16* x, i16* y)
	{
		if (!m_initialized)
		{
			*x = 0;
			*y = 0;
			return;
		}
		*x = m_state.mouseCurrent.x;
		*y = m_state.mouseCurrent.y;
	}

	void InputSystem::GetPreviousMousePosition(i16* x, i16* y)
	{
		if (!m_initialized)
		{
			*x = 0;
			*y = 0;
			return;
		}
		*x = m_state.mousePrevious.x;
		*y = m_state.mousePrevious.y;
	}
}
