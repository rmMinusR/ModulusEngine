#include "game/Component.hpp"

#include <cassert>

#include "application/Application.hpp"
#include "game/GameObject.hpp"
#include "game/Level.hpp"
#include "Material.hpp"

void Component::BindToGameObject(GameObject* obj)
{
	assert(gameObject == nullptr);
	gameObject = obj;
}

Game* Component::getEngine() const
{
	return gameObject->getLevel()->getGame();
}

GameObject* Component::getGameObject() const
{
	return gameObject;
}

Component::Component() :
	gameObject(nullptr)
{
}

Component::~Component()
{
	//TODO ensure gameObject knows to remove this
}

void Component::onStart()
{
}

IUpdatable::~IUpdatable()
{
}

const ShaderProgram* I3DRenderable::getShader() const
{
	const Material* material = getMaterial();
	return material ? material->getShader() : nullptr;
}

I3DRenderable::~I3DRenderable()
{
}
