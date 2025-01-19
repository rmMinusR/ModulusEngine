#pragma once

#include <SDL_scancode.h>
#include "../dllapi.h"
#include "math/Vector3.inl"

class GameWindowInputProcessor;
class Game;

class InputSystem
{
	friend class GameWindowInputProcessor;
	void onGainFocus();
	void onLoseFocus();
	char focused = 0;

	friend class Game;
	void onTick();

	mutable uint8_t const* keyboardState;
	uint32_t mouseButtonState;
	Vector3<int> mousePos;
	Vector3<int> mouseDelta;

public:
	ENGINECORE_API InputSystem();
	ENGINECORE_API virtual ~InputSystem();

	ENGINECORE_API bool getKey(SDL_Scancode key) const;
	ENGINECORE_API bool getMouseButton(uint8_t buttonID) const;
	ENGINECORE_API Vector3<int> getMousePos() const;
	ENGINECORE_API Vector3<int> getMouseDelta() const;
};
