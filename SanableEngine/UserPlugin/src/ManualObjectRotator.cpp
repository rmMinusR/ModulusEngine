#include "ManualObjectRotator.hpp"

#include <SDL_keyboard.h>

ManualObjectRotator::ManualObjectRotator() :
	ManualObjectRotator(0.02f)
{
}

ManualObjectRotator::ManualObjectRotator(float rotationSpeed) :
	rotationSpeed(0.02f)
{
}

void ManualObjectRotator::Update()
{
	Vector3<float> axis;
	
	int sz;
	Uint8 const* keys = SDL_GetKeyboardState(&sz); //Managed by SDL, do not free
	
	//Calc input
	if (keys[SDL_SCANCODE_K]) axis += Vector3<float>(-1, 0, 0);
	if (keys[SDL_SCANCODE_I]) axis += Vector3<float>( 1, 0, 0);
	if (keys[SDL_SCANCODE_J]) axis += Vector3<float>(0, -1, 0);
	if (keys[SDL_SCANCODE_L]) axis += Vector3<float>(0,  1, 0);
	if (keys[SDL_SCANCODE_U]) axis += Vector3<float>(0, 0, -1);
	if (keys[SDL_SCANCODE_O]) axis += Vector3<float>(0, 0,  1);

	//Fast mode
	if (keys[SDL_SCANCODE_RSHIFT]) axis *= 20;

	//Apply
	if (axis.mgnSq() > 0)
	{
		glm::quat spin = glm::angleAxis(axis.mgn()*rotationSpeed, (glm::vec3)axis.normalized());
		Transform* t = gameObject->getTransform();
		t->setRotation(spin * t->getRotation());
	}
}
