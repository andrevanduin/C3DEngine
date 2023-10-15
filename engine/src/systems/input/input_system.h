
#pragma once
#include "core/defines.h"
#include "core/input/buttons.h"
#include "core/input/keys.h"
#include "core/logger.h"
#include "math/math_types.h"
#include "systems/system.h"

namespace C3D
{
    /** @brief How many update ticks we expect before switching to HELD state for keys. **/
    constexpr u8 KEY_HELD_DELAY = 10;
    /** @brief How many update ticks we expect before switching to HELD state for buttons. **/
    constexpr u8 BUTTON_HELD_DELAY = 10;
    /** @brief Maximum amount of keys that can be held. */
    constexpr u8 MAX_HELD_KEYS = 8;
    /** @brief Maximum amount of buttnos that can be held. */
    constexpr u8 MAX_HELD_BUTTONS = 3;

    enum InputState : u8
    {
        Up   = 0,
        Down = 1,
        Held = 2
    };

    struct KeyState
    {
        InputState state = InputState::Up;
        u8 downCount     = 0;
    };

    struct ButtonState
    {
        InputState state = InputState::Up;
        u8 downCount     = 0;
        bool inDrag      = false;
    };

    struct KeyBoardState
    {
        KeyState keys[static_cast<u8>(Keys::MaxKeys)];
    };

    struct MouseState
    {
        ivec2 pos;
        ButtonState buttons[static_cast<u8>(Buttons::MaxButtons)];
    };

    class InputSystem final : public BaseSystem
    {
    public:
        explicit InputSystem(const SystemManager* pSystemsManager);

        bool Init() override;

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

        C3D_API [[nodiscard]] bool IsShiftDown() const;
        C3D_API [[nodiscard]] bool IsCtrlDown() const;
        C3D_API [[nodiscard]] bool IsAltDown() const;

        C3D_API const ivec2& GetMousePosition() const;
        C3D_API const ivec2& GetPreviousMousePosition() const;

    private:
        KeyBoardState m_keyboardCurrent, m_keyboardPrevious;
        MouseState m_mouseCurrent, m_mousePrevious;

        u8 m_downKeys[MAX_HELD_KEYS], m_downButtons[MAX_HELD_BUTTONS];
    };
}  // namespace C3D
