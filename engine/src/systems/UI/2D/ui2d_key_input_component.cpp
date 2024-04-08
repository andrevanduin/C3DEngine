
#include "core/ecs/entity_view.h"
#include "core/input/keys.h"
#include "systems/input/input_system.h"
#include "systems/system_manager.h"
#include "ui2d_system.h"

namespace C3D
{
    /*
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    using namespace UI_2D;

    bool UI2DSystem::OnKeyDown(const EventContext& context)
    {
        auto ctx = KeyEventContext(context.data.u16[0]);

        // Entities with TextInput component should handle this keydown event as the user inputting text (if active)
        for (auto entity : EntityView<FlagsComponent, TextInputComponent>(m_ecs))
        {
            auto& fComponent = m_ecs.GetComponent<FlagsComponent>(entity);
            if (fComponent.flags & FlagActive)
            {
                // If we have found our active component we return since there can only be one active component taking input at the time
                return KeyDownTextInput(entity, ctx);
            }
        }

        // Ensure other handlers can also handle the key down event
        return false;
    }

    bool UI2DSystem::AddOnEndTextInputHandler(Entity entity, const UI_2D::OnEndTextInputEventHandler& handler)
    {
        ASSERT_VALID(entity);

        auto& component          = m_ecs.GetComponent<TextInputComponent>(entity);
        component.onEndTextInput = handler;

        return true;
    }

    bool UI2DSystem::KeyDownTextInput(Entity entity, const UI_2D::KeyEventContext& ctx)
    {
        const u16 keyCode = ctx.keyCode;

        const auto shiftHeld = Input.IsShiftDown();
        char typedChar;

        if (keyCode == KeyEnter)
        {
            // Set this entity to inactive
            SetActive(entity, false);

            // Enter is typed
            auto& component = m_ecs.GetComponent<TextInputComponent>(entity);
            if (component.onEndTextInput)
            {
                // Raise the text input end event
                auto& tComponent = m_ecs.GetComponent<TextComponent>(entity);
                component.onEndTextInput(keyCode, tComponent.text);
            }

            return true;
        }
        else if (keyCode == KeyBackspace)
        {
            typedChar = KeyBackspace;
        }
        else if (keyCode >= KeyA && keyCode <= KeyZ)
        {
            // Ingore '`' key so we don't type that into the console immediatly after opening it
            if (keyCode == KeyGrave) return false;

            // Characters
            typedChar = static_cast<char>(keyCode);
            // If shift is not held we don't want capital letters
            if (!shiftHeld) typedChar += 32;
        }
        else if (keyCode == KeySpace)
        {
            typedChar = static_cast<char>(keyCode);
        }
        else if (keyCode == KeyMinus || keyCode == KeyEquals)
        {
            switch (keyCode)
            {
                case KeyMinus:
                    typedChar = shiftHeld ? '_' : '-';
                    break;
                case KeyEquals:
                    typedChar = shiftHeld ? '+' : '=';
                    break;
                default:
                    FATAL_LOG("Unknown char found while trying to parse key for console '{}'.", keyCode);
                    break;
            }
        }
        else if (keyCode >= Key0 && keyCode <= Key9)
        {
            // Numbers
            typedChar = static_cast<char>(keyCode);
            if (shiftHeld)
            {
                switch (keyCode)
                {
                    case Key0:
                        typedChar = ')';
                        break;
                    case Key1:
                        typedChar = '!';
                        break;
                    case Key2:
                        typedChar = '@';
                        break;
                    case Key3:
                        typedChar = '#';
                        break;
                    case Key4:
                        typedChar = '$';
                        break;
                    case Key5:
                        typedChar = '%';
                        break;
                    case Key6:
                        typedChar = '^';
                        break;
                    case Key7:
                        typedChar = '&';
                        break;
                    case Key8:
                        typedChar = '*';
                        break;
                    case Key9:
                        typedChar = '(';
                        break;
                    default:
                        FATAL_LOG("Unknown char found while trying to parse numbers for console '{}'.", keyCode);
                        break;
                }
            }
        }
        else
        {
            // A key was pressed that we don't care about
            return false;
        }

        // Update our text component to use the new text
        auto& tComponent = m_ecs.GetComponent<TextComponent>(entity);
        if (typedChar == KeyBackspace)
        {
            tComponent.text.RemoveLast();
        }
        else
        {
            tComponent.text += typedChar;
        }

        RegenerateTextComponent(entity);

        auto& component = m_ecs.GetComponent<TextInputComponent>(entity);
        // If the component wants to handle onTextChanged events we notify the user of the text changing
        if (component.onTextChanged)
        {
            component.onTextChanged(tComponent.text);
        }

        // This event is handled, no need for other handlers to do anything
        return true;
    }
*/
}  // namespace C3D