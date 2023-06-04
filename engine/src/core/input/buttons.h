
#pragma once
#include <SDL2/SDL_mouse.h>

namespace C3D
{
	enum Buttons : u8
	{
		ButtonLeft = SDL_BUTTON_LEFT,
		ButtonMiddle = SDL_BUTTON_MIDDLE,
		ButtonRight = SDL_BUTTON_RIGHT,

		MaxButtons
	};
}