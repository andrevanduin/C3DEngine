
#pragma once
#include "core/defines.h"
#include "core/input/buttons.h"
#include "core/input/keys.h"
#include "core/logger.h"
#include "math/math_types.h"
#include "systems/system.h"

namespace C3D
{
    /** @brief How many KeyDown events we expect before switching to HELD state. **/
    constexpr u8 KEY_HELD_DELAY = 10;
    /** @brief How many ButtonDown events we expect before switching to HELD state. **/
    constexpr u8 BUTTON_HELD_DELAY = 10;

    enum InputState : u8
    {
        Up   = 0,
        Down = 1,
        Held = 2
    };

    struct KeyState
    {
        InputState state;
        u8 downCount;
    };
    using ButtonState = KeyState;

    struct KeyBoardState
    {
        KeyState keys[static_cast<u8>(Keys::MaxKeys)];
    };

    struct MouseState
    {
        i16 x, y;
        ButtonState buttons[static_cast<u8>(Buttons::MaxButtons)];
    };

    class InputSystem final : public BaseSystem
    {
    public:
        explicit InputSystem(const SystemManager* pSystemsManager);

        void Update(const FrameData& frameData) override;

        void ProcessKey(Keys key, InputState state);
        void ProcessButton(u8 button, InputState state);
        void ProcessMouseMove(i32 xPos, i32 yPos);
        void ProcessMouseWheel(i32 delta) const;

        C3D_API [[nodiscard]] bool IsKeyDown(u8 key) const;
        C3D_API [[nodiscard]] bool IsKeyUp(u8 key) const;
        C3D_API [[nodiscard]] bool IsKeyPressed(u8 key) const;

        C3D_API [[nodiscard]] bool WasKeyDown(u8 key) const;
        C3D_API [[nodiscard]] bool WasKeyUp(u8 key) const;

        C3D_API [[nodiscard]] bool IsButtonDown(Buttons button) const;
        C3D_API [[nodiscard]] bool IsButtonUp(Buttons button) const;
        C3D_API [[nodiscard]] bool IsButtonPressed(Buttons button) const;

        C3D_API [[nodiscard]] bool WasButtonDown(Buttons button) const;
        C3D_API [[nodiscard]] bool WasButtonUp(Buttons button) const;

        C3D_API [[nodiscard]] bool IsShiftHeld() const;

        C3D_API ivec2 GetMousePosition();
        C3D_API ivec2 GetPreviousMousePosition();

    private:
        KeyBoardState m_keyboardCurrent, m_keyboardPrevious;
        MouseState m_mouseCurrent, m_mousePrevious;
    };
}  // namespace C3D
