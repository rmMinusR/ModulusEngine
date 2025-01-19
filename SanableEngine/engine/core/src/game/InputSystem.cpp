#include "game/InputSystem.hpp"

#include <SDL_keyboard.h>
#include <SDL_mouse.h>

InputSystem::InputSystem()
{
	int sz;
	keyboardState = SDL_GetKeyboardState(&sz); //Managed by SDL, do not free
}

InputSystem::~InputSystem()
{
}

void InputSystem::onGainFocus()
{
	focused++;
	assert(focused <= 2);
}

void InputSystem::onLoseFocus()
{
	focused--;
	assert(focused >= 0);
}

void InputSystem::onTick()
{
	Vector3<int> oldMousePos = mousePos;
	mouseButtonState = SDL_GetMouseState(&mousePos.x, &mousePos.y);
	mouseDelta = mousePos-oldMousePos;
}

bool InputSystem::getKey(SDL_Scancode key) const
{
	return focused && keyboardState[key];
}

bool InputSystem::getMouseButton(uint8_t buttonID) const
{
	return focused && (mouseButtonState & (1<<buttonID));
}

Vector3<int> InputSystem::getMousePos() const
{
	return mousePos;
}

Vector3<int> InputSystem::getMouseDelta() const
{
	return mouseDelta;
}
