
#include "textbox.h"

#include "core/input/keys.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/fonts/font_system.h"
#include "systems/input/input_system.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";
    constexpr auto CORNER_SIZE          = u16vec2(1, 1);
    constexpr auto BLINK_DELAY          = 0.7;

    constexpr auto TEXT_PADDING = 4.f;

    constexpr auto CLIP_PADDING = 4;

    constexpr auto CURSOR_WIDTH   = 2;
    constexpr auto CURSOR_PADDING = 3.f;

    constexpr char SHIFT_NUMBER_KEY_MAP[10] = { ')', '!', '@', '#', '$', '%', '^', '&', '*', '(' };

    Component Textbox::Create(const SystemManager* systemsManager, const DynamicAllocator* pAllocator)
    {
        Component component(systemsManager);

        component.MakeInternal<InternalData>(pAllocator);
        component.onInitialize = &Initialize;
        component.onDestroy    = &Destroy;
        component.onUpdate     = &OnUpdate;
        component.onRender     = &OnRender;
        component.onResize     = &OnResize;
        component.onKeyDown    = &OnKeyDown;
        component.onClick      = &OnClick;

        return component;
    }

    bool Textbox::Initialize(Component& self, const Config& config)
    {
        auto& data = self.GetInternal<InternalData>();
        data.textComponent.Initialize(self, config);
        data.textComponent.offsetX = TEXT_PADDING;
        data.textComponent.offsetY = TEXT_PADDING;

        data.characterIndexCurrent = config.text.Size();
        data.characterIndexStart   = config.text.Size();
        data.characterIndexEnd     = config.text.Size();

        data.nineSlice.Initialize(self, "TextboxNineSlice", AtlasIDTextboxNineSlice, config.size, config.cornerSize,
                                  config.backgroundColor);
        data.cursor.Initialize(self, "TextboxCursor", AtlasIDTextboxCursor, u16vec2(CURSOR_WIDTH, config.size.y - 8));

        data.clip.Initialize(self, "TextboxClippingMask", u16vec2(config.size.x - (2 * CLIP_PADDING), config.size.y));
        data.clip.offsetX = CLIP_PADDING;

        data.highlight.Initialize(self, "TextboxHighlight", AtlasIDTextboxHighlight, u16vec2(10, config.size.y - 8), config.highlightColor);
        data.highlight.offsetX = CLIP_PADDING;

        CalculateCursorOffset(self);
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
                if (data.flags & FlagCursor)
                {
                    data.flags &= ~FlagCursor;
                }
                else
                {
                    data.flags |= FlagCursor;
                }
            }
        }
    }

    void Textbox::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& data = self.GetInternal<InternalData>();
        // Render the background
        data.nineSlice.OnRender(self, frameData, locations);
        // Render the clipping mask
        data.clip.OnRender(self, frameData, locations);
        // Render our text
        data.textComponent.OnRender(self, frameData, locations);
        if (self.IsFlagSet(FlagActive))
        {
            if (data.flags & FlagHighlight)
            {
                // Render the highlight when we need to show it
                data.highlight.OnRender(self, frameData, locations);
            }
            if (data.flags & FlagCursor)
            {
                // Render the cursor when we need to show it
                data.cursor.OnRender(self, frameData, locations);
            }
        }
        // Reset our clipping mask
        data.clip.ResetClipping(self);
    }

    void Textbox::OnResize(Component& self)
    {
        auto& data = self.GetInternal<InternalData>();
        auto size  = self.GetSize();

        data.nineSlice.OnResize(self, size);
        data.cursor.OnResize(self, u16vec2(CURSOR_WIDTH, size.y - 8));
        data.clip.OnResize(self, u16vec2(size.x - (CLIP_PADDING * 2), size.y));
    }

    void Textbox::SetText(Component& self, const char* text)
    {
        auto& data = self.GetInternal<InternalData>();
        data.textComponent.SetText(self, text);

        data.characterIndexStart   = data.textComponent.text.Size();
        data.characterIndexEnd     = data.textComponent.text.Size();
        data.characterIndexCurrent = data.textComponent.text.Size();

        CalculateCursorOffset(self);
    }

    void Textbox::CalculateCursorOffset(Component& self)
    {
        auto& fontSystem = self.GetSystem<FontSystem>();
        auto& data       = self.GetInternal<InternalData>();

        auto textSize = fontSystem.MeasureString(data.textComponent.font, data.textComponent.text, data.characterIndexCurrent);

        data.cursor.offsetY = TEXT_PADDING;
        data.cursor.offsetX = C3D::Clamp(textSize.x + CURSOR_PADDING, 0.f, (f32)data.clip.size.x);

        data.textComponent.offsetX = -C3D::Max(textSize.x - data.clip.size.x, -TEXT_PADDING);
    }

    void Textbox::CalculateHighlight(Component& self, bool shiftDown)
    {
        auto& data = self.GetInternal<InternalData>();

        if (shiftDown)
        {
            data.flags |= FlagHighlight;

            auto& fontSystem = self.GetSystem<FontSystem>();

            auto start = fontSystem.MeasureString(data.textComponent.font, data.textComponent.text, data.characterIndexStart);
            auto end   = fontSystem.MeasureString(data.textComponent.font, data.textComponent.text, data.characterIndexEnd);

            auto width = end.x - start.x;
            data.highlight.OnResize(self, u16vec2(width, data.highlight.size.y));

            data.highlight.offsetY = TEXT_PADDING;
            data.highlight.offsetX = start.x + data.textComponent.offsetX;
        }
        else
        {
            data.flags &= ~FlagHighlight;
        }
    }

    void Textbox::OnTextChanged(Component& self)
    {
        CalculateCursorOffset(self);

        if (self.pUserHandlers && self.pUserHandlers->onTextChangedHandler)
        {
            // Notify the user of the text changing
            auto& data = self.GetInternal<InternalData>();
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
                if (data.characterIndexStart == data.characterIndexEnd)
                {
                    // Delete character behind the cursor
                    data.textComponent.RemoveAt(self, data.characterIndexCurrent - 1);
                    data.characterIndexStart--;
                }
                else
                {
                    // Delete range
                    data.textComponent.RemoveRange(self, data.characterIndexStart, data.characterIndexEnd);
                }

                data.characterIndexEnd     = data.characterIndexStart;
                data.characterIndexCurrent = data.characterIndexStart;

                OnTextChanged(self);
                CalculateHighlight(self, false);
            }
            return true;
        }

        if (keyCode == KeyDelete)
        {
            if (!text.Empty())
            {
                if (data.characterIndexStart == data.characterIndexEnd)
                {
                    // Delete character in front the cursor
                    if (data.characterIndexCurrent < text.Size())
                    {
                        data.textComponent.RemoveAt(self, data.characterIndexCurrent);
                    }
                }
                else
                {
                    // Delete range
                    data.textComponent.RemoveRange(self, data.characterIndexStart, data.characterIndexEnd);
                }

                data.characterIndexEnd     = data.characterIndexStart;
                data.characterIndexCurrent = data.characterIndexStart;

                OnTextChanged(self);
                CalculateHighlight(self, false);
            }
            return true;
        }

        const auto shiftHeld = inputSystem.IsShiftDown();

        if (keyCode == KeyArrowLeft)
        {
            if (data.characterIndexCurrent > data.characterIndexStart)
            {
                // Moving right currently
                if (shiftHeld)
                {
                    // We are highlighting
                    data.characterIndexEnd--;
                    data.characterIndexCurrent = data.characterIndexEnd;
                }
                else
                {
                    // Shift is not held and we are highlighting so we should stop and move to the start of the highlight
                    data.characterIndexEnd     = data.characterIndexStart;
                    data.characterIndexCurrent = data.characterIndexStart;
                }
            }
            else if (data.characterIndexStart > 0)
            {
                // Moving left currently
                if (shiftHeld)
                {
                    // We are highlighting
                    data.characterIndexStart--;
                    data.characterIndexCurrent = data.characterIndexStart;
                }
                else
                {
                    // Shift is not held
                    if (data.characterIndexStart != data.characterIndexEnd)
                    {
                        // We are highlighting and we should stop
                        data.characterIndexEnd     = data.characterIndexStart;
                        data.characterIndexCurrent = data.characterIndexStart;
                    }
                    else
                    {
                        // Not highlighting
                        data.characterIndexStart--;
                        data.characterIndexEnd     = data.characterIndexStart;
                        data.characterIndexCurrent = data.characterIndexStart;
                    }
                }
            }

            CalculateCursorOffset(self);
            CalculateHighlight(self, shiftHeld);
            return true;
        }

        if (keyCode == KeyArrowRight)
        {
            if (data.characterIndexCurrent < data.characterIndexEnd)
            {
                // Moving left currently
                if (shiftHeld)
                {
                    // We are highlighting
                    data.characterIndexStart++;
                    data.characterIndexCurrent = data.characterIndexStart;
                }
                else
                {
                    // Shift is not held and we are highlighting so we should stop and move to the end of the highlight
                    data.characterIndexStart   = data.characterIndexEnd;
                    data.characterIndexCurrent = data.characterIndexEnd;
                }
            }
            else if (data.characterIndexEnd < text.Size())
            {
                // Moving right currently
                if (shiftHeld)
                {
                    // We are highlighting
                    data.characterIndexEnd++;
                    data.characterIndexCurrent = data.characterIndexEnd;
                }
                else
                {
                    // Shift is not held
                    if (data.characterIndexStart != data.characterIndexEnd)
                    {
                        // We are highlighting and we should stop
                        data.characterIndexStart   = data.characterIndexEnd;
                        data.characterIndexCurrent = data.characterIndexEnd;
                    }
                    else
                    {
                        // Not highlighting
                        data.characterIndexStart++;
                        data.characterIndexEnd     = data.characterIndexStart;
                        data.characterIndexCurrent = data.characterIndexStart;
                    }
                }
            }

            CalculateCursorOffset(self);
            CalculateHighlight(self, shiftHeld);

            return true;
        }

        if (keyCode == KeyHome)
        {
            data.characterIndexStart   = 0;
            data.characterIndexCurrent = data.characterIndexStart;
            if (!shiftHeld)
            {
                data.characterIndexEnd = data.characterIndexStart;
            }

            CalculateCursorOffset(self);
            CalculateHighlight(self, shiftHeld);
            return true;
        }

        if (keyCode == KeyEnd)
        {
            // 123456
            data.characterIndexEnd     = text.Size();
            data.characterIndexCurrent = data.characterIndexEnd;
            if (!shiftHeld)
            {
                data.characterIndexStart = data.characterIndexEnd;
            }

            CalculateCursorOffset(self);
            CalculateHighlight(self, shiftHeld);
            return true;
        }

        const auto capsLockActive = inputSystem.IsCapslockActive();

        char typedChar;

        if (keyCode >= KeyA && keyCode <= KeyZ)
        {
            // Ignore '`' key so we don't type that into the console immediatly after opening it
            if (keyCode == KeyGrave) return false;

            // Characters
            typedChar = static_cast<char>(keyCode);
            // If shift is not held we want don't want capital letters
            if (!shiftHeld && !capsLockActive) typedChar += 32;
        }
        else if (keyCode == KeySpace)
        {
            typedChar = static_cast<char>(keyCode);
        }
        else if (keyCode >= KeyEquals && keyCode <= KeySlash)
        {
            switch (keyCode)
            {
                case KeyEquals:
                    typedChar = shiftHeld ? '+' : '=';
                    break;
                case KeyComma:
                    typedChar = shiftHeld ? '<' : ',';
                    break;
                case KeyMinus:
                    typedChar = shiftHeld ? '_' : '-';
                    break;
                case KeyPeriod:
                    typedChar = shiftHeld ? '>' : '.';
                    break;
                case KeySlash:
                    typedChar = shiftHeld ? '?' : '/';
                    break;
                default:
                    FATAL_LOG("Unknown char found while trying to parse key for console '{}'.", keyCode);
                    break;
            }
        }
        else if (keyCode >= Key0 && keyCode <= Key9)
        {
            // Numbers
            typedChar = shiftHeld ? SHIFT_NUMBER_KEY_MAP[keyCode - Key0] : static_cast<char>(keyCode);
        }
        else
        {
            // A key was pressed that we don't care about
            return false;
        }

        if (data.characterIndexStart != data.characterIndexEnd)
        {
            // We first remove the highlighted area from the string (and skip regeneration since we will do it in the next step)
            data.textComponent.RemoveRange(self, data.characterIndexStart, data.characterIndexEnd, false);
            // Set our current index to the start of the highlighted area since we just removed it
            data.characterIndexCurrent = data.characterIndexStart;
            CalculateHighlight(self, false);
        }

        // Then at that position insert our new char
        data.textComponent.Insert(self, data.characterIndexCurrent, typedChar);
        // And reset our cursor indices
        data.characterIndexStart++;
        data.characterIndexEnd     = data.characterIndexStart;
        data.characterIndexCurrent = data.characterIndexStart;

        OnTextChanged(self);
        return true;
    }

    bool Textbox::OnClick(Component& self, const MouseButtonEventContext& ctx)
    {
        // Set the textbox to active
        auto& uiSystem = self.GetSystem<UI2DSystem>();
        uiSystem.SetActive(self.GetID(), true);

        // Determine cursor location
        CalculateCursorOffset(self);

        // Handle the optional user provided onClick handler method
        if (self.pUserHandlers && self.pUserHandlers->onClickHandler)
        {
            return self.pUserHandlers->onClickHandler(ctx);
        }
        return true;
    }

    void Textbox::Destroy(Component& self, const DynamicAllocator* pAllocator)
    {
        auto& data = self.GetInternal<InternalData>();

        data.nineSlice.Destroy(self);
        data.textComponent.Destroy(self);
        data.cursor.Destroy(self);
        data.clip.Destroy(self);
        data.highlight.Destroy(self);

        self.DestroyInternal(pAllocator);
    }
}  // namespace C3D::UI_2D