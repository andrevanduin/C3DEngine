
#include "input_system.h"

#include "core/engine.h"
#include "core/logger.h"
#include "platform/platform.h"
#include "systems/events/event_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    InputSystem::InputSystem(const SystemManager* pSystemsManager)
        : BaseSystem(pSystemsManager, "INPUT"), m_keyboardCurrent(), m_keyboardPrevious(), m_mouseCurrent(), m_mousePrevious()
    {}

    void InputSystem::Update(const FrameData& frameData)
    {
        if (!m_initialized) return;

        m_keyboardPrevious = m_keyboardCurrent;
        m_mousePrevious    = m_mouseCurrent;
    }

    void InputSystem::ProcessKey(const Keys key, const InputState state)
    {
        if (key >= Keys::MaxKeys)
        {
            m_logger.Warn("Key{} keycode was larger than expected '{}'.", state == InputState::Down ? "Down" : "Up", ToUnderlying(key));
            return;
        }

        auto& currentKeyState = m_keyboardCurrent.keys[key];
        if (state == InputState::Down)
        {
            if (currentKeyState.state == InputState::Up)
            {
                /* Up -> Down fire a KeyDown event and set to the Down state. */
                EventContext context{};
                context.data.u16[0] = key;
                Event.Fire(EventCodeKeyDown, nullptr, context);

                currentKeyState.state = InputState::Down;
            }
            else if (currentKeyState.state == InputState::Down)
            {
                /* Down -> Down Key is still down meaning we should start counting to change towards Held state. */
                currentKeyState.downCount++;
                if (currentKeyState.downCount > KEY_HELD_DELAY)
                {
                    currentKeyState.state = InputState::Held;
                }
            }

            /* Held -> Down means we do nothing. */
        }
        /* state == InputState::Up */
        else if (currentKeyState.state == InputState::Down || currentKeyState.state == InputState::Held)
        {
            /* Down -> Up fire a KeyUp event and set to the Up state. */
            EventContext context{};
            context.data.u16[0] = key;
            Event.Fire(EventCodeKeyUp, nullptr, context);

            currentKeyState.state = InputState::Up;
        }
    }

    void InputSystem::ProcessButton(const u8 button, const InputState state)
    {
        auto& currentButtonState = m_mouseCurrent.buttons[button];

        EventContext context{};
        context.data.u16[0] = button;
        context.data.i16[1] = m_mouseCurrent.x;
        context.data.i16[2] = m_mouseCurrent.y;

        if (state == InputState::Down)
        {
            if (currentButtonState.state == InputState::Up)
            {
                /* Up -> Down fire a ButtonDown event and set to the Down state. */
                Event.Fire(EventCodeButtonDown, nullptr, context);
                currentButtonState.state = InputState::Down;
            }
            else if (currentButtonState.state == InputState::Down)
            {
                /* Down -> Down Key is still down meaning we should start counting to change towards Held state. */
                currentButtonState.downCount++;
                if (currentButtonState.downCount > BUTTON_HELD_DELAY)
                {
                    currentButtonState.state = InputState::Held;
                }
            }

            /* Held -> Down means we do nothing. */
        }
        /* state == InputState::Up */
        else if (currentButtonState.state == InputState::Down || currentButtonState.state == InputState::Held)
        {
            /* Down -> Up fire a ButtonUp event and set to the Up state. */
            Event.Fire(EventCodeButtonUp, nullptr, context);
            currentButtonState.state = InputState::Up;
        }
    }

    void InputSystem::ProcessMouseMove(const i32 xPos, const i32 yPos)
    {
        const auto x = static_cast<i16>(xPos);
        const auto y = static_cast<i16>(yPos);

        if (m_mouseCurrent.x != x || m_mouseCurrent.y != y)
        {
            m_mouseCurrent.x = x;
            m_mouseCurrent.y = y;

            EventContext context{};
            context.data.i16[0] = x;
            context.data.i16[1] = y;

            Event.Fire(ToUnderlying(EventCodeMouseMoved), nullptr, context);
        }
    }

    void InputSystem::ProcessMouseWheel(const i32 delta) const
    {
        EventContext context{};
        context.data.i8[0] = static_cast<i8>(delta);
        Event.Fire(ToUnderlying(EventCodeMouseScrolled), nullptr, context);
    }

    bool InputSystem::IsKeyDown(const u8 key) const
    {
        if (!m_initialized) return false;
        return m_keyboardCurrent.keys[key].state > InputState::Up; /* Down or Held */
    }

    bool InputSystem::IsKeyUp(const u8 key) const
    {
        if (!m_initialized) return true;
        return m_keyboardCurrent.keys[key].state == InputState::Up;
    }

    bool InputSystem::IsKeyPressed(const u8 key) const
    {
        if (!m_initialized) return false;
        return m_keyboardCurrent.keys[key].state == InputState::Down && m_keyboardPrevious.keys[key].state == InputState::Up;
    }

    bool InputSystem::WasKeyDown(const u8 key) const
    {
        if (!m_initialized) return false;
        return m_keyboardPrevious.keys[key].state > InputState::Up; /* Down or Held */
    }

    bool InputSystem::WasKeyUp(const u8 key) const
    {
        if (!m_initialized) return true;
        return m_keyboardPrevious.keys[key].state == InputState::Up;
    }

    bool InputSystem::IsButtonDown(const Buttons button) const
    {
        if (!m_initialized) return false;
        return m_mouseCurrent.buttons[button].state > InputState::Up; /* Down or Held */
    }

    bool InputSystem::IsButtonUp(const Buttons button) const
    {
        if (!m_initialized) return true;
        return m_mouseCurrent.buttons[button].state == InputState::Up;
    }

    bool InputSystem::IsButtonPressed(const Buttons button) const
    {
        if (!m_initialized) return false;
        return m_mouseCurrent.buttons[button].state == InputState::Down && m_mousePrevious.buttons[button].state == InputState::Up;
    }

    bool InputSystem::WasButtonDown(const Buttons button) const
    {
        if (!m_initialized) return false;
        return m_mousePrevious.buttons[button].state > InputState::Up; /* Down or Held */
    }

    bool InputSystem::WasButtonUp(const Buttons button) const
    {
        if (!m_initialized) return true;
        return m_mousePrevious.buttons[button].state == InputState::Up;
    }

    bool InputSystem::IsShiftHeld() const
    {
        if (!m_initialized) return false;
        return m_keyboardCurrent.keys[KeyShift].state > InputState::Up || m_keyboardCurrent.keys[KeyLShift].state > InputState::Up ||
               m_keyboardCurrent.keys[KeyRShift].state > InputState::Up;
    }

    ivec2 InputSystem::GetMousePosition()
    {
        if (!m_initialized)
        {
            return ivec2(0);
        }
        return { m_mouseCurrent.x, m_mouseCurrent.y };
    }

    ivec2 InputSystem::GetPreviousMousePosition()
    {
        if (!m_initialized)
        {
            return ivec2(0);
        }
        return { m_mousePrevious.x, m_mousePrevious.y };
    }
}  // namespace C3D
