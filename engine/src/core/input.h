
#pragma once
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>

#include "defines.h"

namespace C3D
{
    enum Keys : u8
    {
        /** @Brief The Backspace Key. */
        KeyBackspace = 0x08,
        /** @Brief The Enter Key. */
        KeyEnter = 0x0d,
        /** @Brief The Tab Key. */
        KeyTab = 0x09,
        /** @Brief The Shift Key. */
        KeyShift = 0x10,
        /** @Brief The Control/Ctrl Key. */
        KeyControl = 0x11,

        /** @Brief The Pause Key. */
        KeyPause = 0x13,
        /** @Brief The Caps Lock Key. */
        KeyCapital = 0x14,

        /** @Brief The Escape Key. */
        KeyEscape = 0x1b,

        KeyConvert = 0x1c,
        KeyNonConvert = 0x1d,
        KeyAccept = 0x1e,
        KeyModeChange = 0x1f,

        /** @Brief The SpaceBar Key. */
        KeySpace = 0x20,
        KeyPrior = 0x21,
        KeyNext = 0x22,
        /** @Brief The End Key. */
        KeyEnd = 0x23,
        /** @Brief The Home Key. */
        KeyHome = 0x24,
        /** @Brief The Left Arrow Key. */
        KeyLeft = 0x25,
        /** @Brief The Up Arrow Key. */
        KeyUp = 0x26,
        /** @Brief The Right Arrow Key. */
        KeyRight = 0x27,
        /** @Brief The Down Arrow Key. */
        KeyDown = 0x28,
        KeySelect = 0x29,
        KeyPrint = 0x2a,
        KeyExecute = 0x2b,
        KeySnapshot = 0x2c,
        /** @Brief The Insert Key. */
        KeyInsert = 0x2d,
        /** @Brief The Delete Key. */
        KeyDelete = 0x2e,
        KeyHelp = 0x2f,

        /** @Brief The 0 Key */
        Key0 = 0x30,
        /** @Brief The 1 Key */
        Key1 = 0x31,
        /** @Brief The 2 Key */
        Key2 = 0x32,
        /** @Brief The 3 Key */
        Key3 = 0x33,
        /** @Brief The 4 Key */
        Key4 = 0x34,
        /** @Brief The 5 Key */
        Key5 = 0x35,
        /** @Brief The 6 Key */
        Key6 = 0x36,
        /** @Brief The 7 Key */
        Key7 = 0x37,
        /** @Brief The 8 Key */
        Key8 = 0x38,
        /** @Brief The 9 Key */
        Key9 = 0x39,

        /** @Brief The A Key. */
        KeyA = 'a',
        /** @Brief The B Key. */
        KeyB = 'b',
        /** @Brief The C Key. */
        KeyC = 'c',
        /** @Brief The D Key. */
        KeyD = 'd',
        /** @Brief The E Key. */
        KeyE = 'e',
        /** @Brief The F Key. */
        KeyF = 'f',
        /** @Brief The G Key. */
        KeyG = 'g',
        /** @Brief The H Key. */
        KeyH = 'h',
        /** @Brief The I Key. */
        KeyI = 'i',
        /** @Brief The J Key. */
        KeyJ = 'j',
        /** @Brief The K Key. */
        KeyK = 'k',
        /** @Brief The L Key. */
        KeyL = 'l',
        /** @Brief The M Key. */
        KeyM = 'm',
        /** @Brief The N Key. */
        KeyN = 'n',
        /** @Brief The O Key. */
        KeyO = 'o',
        /** @Brief The P Key. */
        KeyP = 'p',
        /** @Brief The Q Key. */
        KeyQ = 'q',
        /** @Brief The R Key. */
        KeyR = 'r',
        /** @Brief The S Key. */
        KeyS = 's',
        /** @Brief The T Key. */
        KeyT = 't',
        /** @Brief The U Key. */
        KeyU = 'u',
        /** @Brief The V Key. */
        KeyV = 'v',
        /** @Brief The W Key. */
        KeyW = 'w',
        /** @Brief The X Key. */
        KeyX = 'x',
        /** @Brief The Y Key. */
        KeyY = 'y',
        /** @Brief The Z Key. */
        KeyZ = 'z',

        /** @Brief The Left Windows/Super Key. */
        KeyLWin = 0x5b,
        /** @Brief The Right Windows/Super Key. */
        KeyRWin = 0x5c,
        KeyApps = 0x5d,

        /** @Brief The Sleep Key. */
        KeySleep = 0x5f,

        /** @Brief The NumberPad 0 Key. */
        KeyNumpad0 = 0x60,
        /** @Brief The NumberPad 1 Key. */
        KeyNumpad1 = 0x61,
        /** @Brief The NumberPad 2 Key. */
        KeyNumpad2 = 0x62,
        /** @Brief The NumberPad 3 Key. */
        KeyNumpad3 = 0x63,
        /** @Brief The NumberPad 4 Key. */
        KeyNumpad4 = 0x64,
        /** @Brief The NumberPad 5 Key. */
        KeyNumpad5 = 0x65,
        /** @Brief The NumberPad 6 Key. */
        KeyNumpad6 = 0x66,
        /** @Brief The NumberPad 7 Key. */
        KeyNumpad7 = 0x67,
        /** @Brief The NumberPad 8 Key. */
        KeyNumpad8 = 0x68,
        /** @Brief The NumberPad 9 Key. */
        KeyNumpad9 = 0x69,
        /** @Brief The NumberPad Multiply Key. */
        KeyMultiply = 0x6a,
        /** @Brief The NumberPad Add Key. */
        KeyAdd = 0x6b,
        /** @Brief The NumberPad Separator Key. */
        KeySeparator = 0x6c,
        /** @Brief The NumberPad Subtract Key. */
        KeySubtract = 0x6d,
        /** @Brief The NumberPad Decimal Key. */
        KeyDecimal = 0x6e,
        /** @Brief The NumberPad Divide Key. */
        KeyDivide = 0x6f,

        /** @Brief The F1 Key. */
        KeyF1 = 0x70,
        /** @Brief The F2 Key. */
        KeyF2 = 0x71,
        /** @Brief The F3 Key. */
        KeyF3 = 0x72,
        /** @Brief The F4 Key. */
        KeyF4 = 0x73,
        /** @Brief The F5 Key. */
        KeyF5 = 0x74,
        /** @Brief The F6 Key. */
        KeyF6 = 0x75,
        /** @Brief The F7 Key. */
        KeyF7 = 0x76,
        /** @Brief The F8 Key. */
        KeyF8 = 0x77,
        /** @Brief The F9 Key. */
        KeyF9 = 0x78,
        /** @Brief The F10 Key. */
        KeyF10 = 0x79,
        /** @Brief The F11 Key. */
        KeyF11 = 0x7a,
        /** @Brief The F12 Key. */
        KeyF12 = 0x7b,
        /** @Brief The F13 Key. */
        KeyF13 = 0x7c,
        /** @Brief The F14 Key. */
        KeyF14 = 0x7d,
        /** @Brief The F15 Key. */
        KeyF15 = 0x7e,
        /** @Brief The F16 Key. */
        KeyF16 = 0x7f,
        /** @Brief The F17 Key. */
        KeyF17 = 0x80,
        /** @Brief The F18 Key. */
        KeyF18 = 0x81,
        /** @Brief The F19 Key. */
        KeyF19 = 0x82,
        /** @Brief The F20 Key. */
        KeyF20 = 0x83,
        /** @Brief The F21 Key. */
        KeyF21 = 0x84,
        /** @Brief The F22 Key. */
        KeyF22 = 0x85,
        /** @Brief The F23 Key. */
        KeyF23 = 0x86,
        /** @Brief The F24 Key. */
        KeyF24 = 0x87,

        /** @Brief The Number Lock Key. */
        KeyNumLock = 0x90,

        /** @Brief The Scroll Lock Key. */
        KeyScroll = 0x91,

        /** @Brief The NumberPad Equal Key. */
        KeyNumpadEqual = 0x92,

        /** @Brief The Left Shift Key. */
        KeyLShift = 0xa0,
        /** @Brief The Right Shift Key. */
        KeyRShift = 0xa1,
        /** @Brief The Left Control Key. */
        KeyLControl = 0xa2,
        /** @Brief The Right Control Key. */
        KeyRControl = 0xa3,
        /** @Brief The Left Alt Key. */
        KeyLAlt = 0xa4,
        /** @Brief The Right Alt Key. */
        KeyRAlt = 0xa5,

        /** @Brief The Semicolon Key. */
        KeySemicolon = 0xba,
        /** @Brief The Plus Key. */
        KeyPlus = 0xbb,
        /** @Brief The Comma Key. */
        KeyComma = 0xbc,
        /** @Brief The Minus Key. */
        KeyMinus = 0xbd,
        /** @Brief The Period Key. */
        KeyPeriod = 0xbe,
        /** @Brief The Slash Key. */
        KeySlash = 0xbf,

        /** @Brief The Grave Key. */
        KeyGrave = 0xc0,

        MaxKeys
	};

	enum Buttons : u8
	{
		ButtonLeft = SDL_BUTTON_LEFT,
        ButtonMiddle = SDL_BUTTON_MIDDLE,
		ButtonRight = SDL_BUTTON_RIGHT,

        MaxButtons
	};

	class InputSystem
	{
		struct KeyBoardState
		{
			bool keys[static_cast<u8>(Keys::MaxKeys)];
		};

		struct MouseState
		{
			i16 x, y;
			bool buttons[static_cast<u8>(Buttons::MaxButtons)];
		};

		struct InputState
		{
			KeyBoardState keyboardCurrent;
			KeyBoardState keyboardPrevious;

			MouseState mouseCurrent;
			MouseState mousePrevious;
		};
	public:
		bool Init();
		void Shutdown();

		void Update(f64 deltaTime);

		void ProcessKey(SDL_Keycode sdlKey, bool pressed);
		void ProcessButton(u8 button, bool pressed);
		void ProcessMouseMove(i32 sdlX, i32 sdlY);
		void ProcessMouseWheel(i32 delta);

		C3D_API [[nodiscard]] bool IsKeyDown(u8 key) const;
        C3D_API [[nodiscard]] bool IsKeyUp(u8 key) const;

        C3D_API [[nodiscard]] bool WasKeyDown(u8 key) const;
        C3D_API [[nodiscard]] bool WasKeyUp(u8 key) const;

        C3D_API [[nodiscard]] bool IsButtonDown(Buttons button) const;
        C3D_API [[nodiscard]] bool IsButtonUp(Buttons button) const;

        C3D_API [[nodiscard]] bool WasButtonDown(Buttons button) const;
        C3D_API [[nodiscard]] bool WasButtonUp(Buttons button) const;

        C3D_API void GetMousePosition(i16* x, i16* y);
        C3D_API void GetPreviousMousePosition(i16* x, i16* y);
	private:
        bool m_initialized = false;
		InputState m_state{};
	};
}
