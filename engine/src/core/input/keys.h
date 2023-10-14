
#pragma once
#include "core/defines.h"

namespace C3D
{
    enum Keys : u16
    {
        /** @brief The Backspace Key. */
        KeyBackspace = 0x08,
        /** @brief The Enter Key. */
        KeyEnter = 0x0d,
        /** @brief The Tab Key. */
        KeyTab = 0x09,
        /** @brief The Shift Key. */
        KeyShift = 0x10,
        /** @brief The Control/Ctrl Key. */
        KeyControl = 0x11,

        /** @brief The Pause Key. */
        KeyPause = 0x13,
        /** @brief The Caps Lock Key. */
        KeyCapital = 0x14,

        /** @brief The Escape Key. */
        KeyEscape = 0x1b,

        KeyConvert    = 0x1c,
        KeyNonConvert = 0x1d,
        KeyAccept     = 0x1e,
        KeyModeChange = 0x1f,

        /** @brief The SpaceBar Key. */
        KeySpace = 0x20,
        KeyPrior = 0x21,
        KeyNext  = 0x22,
        /** @brief The End Key. */
        KeyEnd = 0x23,
        /** @brief The Home Key. */
        KeyHome = 0x24,
        /** @brief The Left Arrow Key. */
        KeyArrowLeft = 0x25,
        /** @brief The Up Arrow Key. */
        KeyArrowUp = 0x26,
        /** @brief The Right Arrow Key. */
        KeyArrowRight = 0x27,
        /** @brief The Down Arrow Key. */
        KeyArrowDown = 0x28,
        KeySelect    = 0x29,
        KeyPrint     = 0x2a,
        KeyExecute   = 0x2b,
        KeySnapshot  = 0x2c,
        /** @brief The Delete Key. */
        KeyDelete = 0x2e,
        KeyHelp   = 0x2f,
        /** @brief The 0 Key */
        Key0 = 0x30,
        /** @brief The 1 Key */
        Key1 = 0x31,
        /** @brief The 2 Key */
        Key2 = 0x32,
        /** @brief The 3 Key */
        Key3 = 0x33,
        /** @brief The 4 Key */
        Key4 = 0x34,
        /** @brief The 5 Key */
        Key5 = 0x35,
        /** @brief The 6 Key */
        Key6 = 0x36,
        /** @brief The 7 Key */
        Key7 = 0x37,
        /** @brief The 8 Key */
        Key8 = 0x38,
        /** @brief The 9 Key */
        Key9 = 0x39,
        /** @brief The A Key. */
        KeyA = 'A',
        /** @brief The B Key. */
        KeyB = 'B',
        /** @brief The C Key. */
        KeyC = 'C',
        /** @brief The D Key. */
        KeyD = 'D',
        /** @brief The E Key. */
        KeyE = 'E',
        /** @brief The F Key. */
        KeyF = 'F',
        /** @brief The G Key. */
        KeyG = 'G',
        /** @brief The H Key. */
        KeyH = 'H',
        /** @brief The I Key. */
        KeyI = 'I',
        /** @brief The J Key. */
        KeyJ = 'J',
        /** @brief The K Key. */
        KeyK = 'K',
        /** @brief The L Key. */
        KeyL = 'L',
        /** @brief The M Key. */
        KeyM = 'M',
        /** @brief The N Key. */
        KeyN = 'N',
        /** @brief The O Key. */
        KeyO = 'O',
        /** @brief The P Key. */
        KeyP = 'P',
        /** @brief The Q Key. */
        KeyQ = 'Q',
        /** @brief The R Key. */
        KeyR = 'R',
        /** @brief The S Key. */
        KeyS = 'S',
        /** @brief The T Key. */
        KeyT = 'T',
        /** @brief The U Key. */
        KeyU = 'U',
        /** @brief The V Key. */
        KeyV = 'V',
        /** @brief The W Key. */
        KeyW = 'W',
        /** @brief The X Key. */
        KeyX = 'X',
        /** @brief The Y Key. */
        KeyY = 'Y',
        /** @brief The Z Key. */
        KeyZ = 'Z',

        /** @brief The Left Windows/Super Key. */
        KeyLWin = 0x5b,
        /** @brief The Right Windows/Super Key. */
        KeyRWin = 0x5c,
        KeyApps = 0x5d,

        /** @brief The Sleep Key. */
        KeySleep = 0x5f,
        /** @brief The NumberPad 0 Key. */
        KeyNumpad0 = 0x60,
        /** @brief The NumberPad 1 Key. */
        KeyNumpad1 = 0x61,
        /** @brief The NumberPad 2 Key. */
        KeyNumpad2 = 0x62,
        /** @brief The NumberPad 3 Key. */
        KeyNumpad3 = 0x63,
        /** @brief The NumberPad 4 Key. */
        KeyNumpad4 = 0x64,
        /** @brief The NumberPad 5 Key. */
        KeyNumpad5 = 0x65,
        /** @brief The NumberPad 6 Key. */
        KeyNumpad6 = 0x66,
        /** @brief The NumberPad 7 Key. */
        KeyNumpad7 = 0x67,
        /** @brief The NumberPad 8 Key. */
        KeyNumpad8 = 0x68,
        /** @brief The NumberPad 9 Key. */
        KeyNumpad9 = 0x69,
        /** @brief The NumberPad Multiply Key. */
        KeyMultiply = 0x6a,
        /** @brief The NumberPad Add Key. */
        KeyAdd = 0x6b,
        /** @brief The NumberPad Separator Key. */
        KeySeparator = 0x6c,
        /** @brief The NumberPad Subtract Key. */
        KeySubtract = 0x6d,
        /** @brief The NumberPad Decimal Key. */
        KeyDecimal = 0x6e,
        /** @brief The NumberPad Divide Key. */
        KeyDivide = 0x6f,

        /** @brief The F1 Key. */
        KeyF1 = 0x70,
        /** @brief The F2 Key. */
        KeyF2 = 0x71,
        /** @brief The F3 Key. */
        KeyF3 = 0x72,
        /** @brief The F4 Key. */
        KeyF4 = 0x73,
        /** @brief The F5 Key. */
        KeyF5 = 0x74,
        /** @brief The F6 Key. */
        KeyF6 = 0x75,
        /** @brief The F7 Key. */
        KeyF7 = 0x76,
        /** @brief The F8 Key. */
        KeyF8 = 0x77,
        /** @brief The F9 Key. */
        KeyF9 = 0x78,
        /** @brief The F10 Key. */
        KeyF10 = 0x79,
        /** @brief The F11 Key. */
        KeyF11 = 0x7a,
        /** @brief The F12 Key. */
        KeyF12 = 0x7b,
        /** @brief The F13 Key. */
        KeyF13 = 0x7c,
        /** @brief The F14 Key. */
        KeyF14 = 0x7d,
        /** @brief The F15 Key. */
        KeyF15 = 0x7e,
        /** @brief The F16 Key. */
        KeyF16 = 0x7f,
        /** @brief The F17 Key. */
        KeyF17 = 0x80,
        /** @brief The F18 Key. */
        KeyF18 = 0x81,
        /** @brief The F19 Key. */
        KeyF19 = 0x82,
        /** @brief The F20 Key. */
        KeyF20 = 0x83,
        /** @brief The F21 Key. */
        KeyF21 = 0x84,
        /** @brief The F22 Key. */
        KeyF22 = 0x85,
        /** @brief The F23 Key. */
        KeyF23 = 0x86,
        /** @brief The F24 Key. */
        KeyF24 = 0x87,

        /** @brief The Number Lock Key. */
        KeyNumLock = 0x90,

        /** @brief The Scroll Lock Key. */
        KeyScroll = 0x91,

        /** @brief The NumberPad Equal Key. */
        KeyNumpadEqual = 0x92,

        /** @brief The Left Shift Key. */
        KeyLShift = 0xa0,
        /** @brief The Right Shift Key. */
        KeyRShift = 0xa1,
        /** @brief The Left Control Key. */
        KeyLControl = 0xa2,
        /** @brief The Right Control Key. */
        KeyRControl = 0xa3,
        /** @brief The Left Alt Key. */
        KeyLAlt = 0xa4,
        /** @brief The Right Alt Key. */
        KeyRAlt = 0xa5,

        /** @brief The Semicolon Key. */
        KeySemicolon = 0xba,
        /** @brief The Equals (=) Key. */
        KeyEquals = 0xbb,
        /** @brief The Comma Key. */
        KeyComma = 0xbc,
        /** @brief The minus (-) Key. */
        KeyMinus = 0xbd,
        /** @brief The Period Key. */
        KeyPeriod = 0xbe,
        /** @brief The Slash Key. */
        KeySlash = 0xbf,
        /** @brief The Grave key. */
        KeyGrave = 0xc0,
        MaxKeys,
    };
}