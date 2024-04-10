
#include "textbox.h"

#include "core/input/keys.h"
#include "defaults.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/input/input_system.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";
    constexpr auto CORNER_SIZE          = u16vec2(1, 1);
    constexpr auto BLINK_DELAY          = 0.8;

    Component Textbox::Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator)
    {
        Component component(systemsManager);

        component.MakeInternal<InternalData>(pAllocator);
        component.onDestroy = &Destroy;
        component.onUpdate  = &OnUpdate;
        component.onRender  = &OnRender;
        component.onResize  = &OnResize;
        component.onKeyDown = &OnKeyDown;
        component.onClick   = &DefaultMethods::OnClick;

        return component;
    }

    bool Textbox::Initialize(Component& self, const u16vec2& pos, const u16vec2& size, const String& text, FontHandle font)
    {
        self.Initialize(pos, size, ComponentTypeTextbox);
        auto& data = self.GetInternal<InternalData>();
        data.textComponent.Initialize(self, font, text);
        data.textComponent.offsetX = 4;
        data.textComponent.offsetY = 4;

        data.nineSlice.Initialize(self, "TextboxNineSlice", AtlasIDTextboxNineSlice, size, CORNER_SIZE);
        data.cursor.Initialize(self, "TextboxCursor", AtlasIDTextboxCursor, u16vec2(2, size.y - 8));

        CalculateCursorOffset(data);
        return true;
    }

    void Textbox::OnUpdate(Component& self)
    {
        if (self.IsFlagSet(FlagActive))
        {
            auto& data = self.GetInternal<InternalData>();
            auto& os   = self.GetSystem<Platform>();

            auto currentTime = os.GetAbsoluteTime();
            if (currentTime >= data.nextBlink)
            {
                data.nextBlink = currentTime + BLINK_DELAY;
                data.showCursor ^= true;
            }
        }
    }

    void Textbox::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& data = self.GetInternal<InternalData>();
        data.nineSlice.OnRender(self, frameData, locations);
        data.textComponent.OnRender(self, frameData, locations);
        if (data.showCursor && self.IsFlagSet(FlagActive))
        {
            data.cursor.OnRender(self, frameData, locations);
        }
    }

    void Textbox::OnResize(Component& self)
    {
        auto& data = self.GetInternal<InternalData>();
        auto size  = self.GetSize();

        data.nineSlice.OnResize(self, size);
        data.cursor.OnResize(self, u16vec2(4, size.y - 2));
    }

    void Textbox::SetText(Component& self, const char* text)
    {
        auto& data = self.GetInternal<Textbox::InternalData>();
        data.textComponent.SetText(self, text);
        CalculateCursorOffset(data);
    }

    void Textbox::CalculateCursorOffset(InternalData& data)
    {
        data.cursor.offsetY = 4;
        data.cursor.offsetX = data.textComponent.maxX + 6;
    }

    void Textbox::OnTextChanged(Component& self)
    {
        auto& data = self.GetInternal<InternalData>();
        CalculateCursorOffset(data);

        if (self.pUserHandlers && self.pUserHandlers->onTextChangedHandler)
        {
            // Notify the user of the text changing
            self.pUserHandlers->onTextChangedHandler(data.textComponent.text);
        }
    }

    bool Textbox::OnKeyDown(Component& self, const KeyEventContext& ctx)
    {
        auto& data        = self.GetInternal<InternalData>();
        auto& uiSystem    = self.GetSystem<UI2DSystem>();
        auto& inputSystem = self.GetSystem<InputSystem>();
        auto& text        = data.textComponent.text;

        const u16 keyCode = ctx.keyCode;

        if (keyCode == KeyEnter)
        {
            // Deactivate this component
            uiSystem.SetActive(self.GetID(), false);
            if (self.pUserHandlers && self.pUserHandlers->onTextInputEndHandler)
            {
                // Notify the user of this event
                self.pUserHandlers->onTextInputEndHandler(keyCode, text);
            }
            return true;
        }

        if (keyCode == KeyBackspace)
        {
            if (!text.Empty())
            {
                data.textComponent.RemoveLast(self);
                OnTextChanged(self);
            }
            return true;
        }

        const auto shiftHeld = inputSystem.IsShiftDown();
        char typedChar;

        if (keyCode >= KeyA && keyCode <= KeyZ)
        {
            // Ignore '`' key so we don't type that into the console immediatly after opening it
            if (keyCode == KeyGrave) return false;

            // Characters
            typedChar = static_cast<char>(keyCode);
            // If shift is not held we want don't want capital letters
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

        data.textComponent.Append(self, typedChar);
        OnTextChanged(self);
        return true;
    }

    void Textbox::Destroy(Component& self, const DynamicAllocator* pAllocator)
    {
        auto& data = self.GetInternal<InternalData>();
        data.nineSlice.Destroy(self);
        data.textComponent.Destroy(self);
        data.cursor.Destroy(self);
        self.DestroyInternal(pAllocator);
    }
}  // namespace C3D::UI_2D