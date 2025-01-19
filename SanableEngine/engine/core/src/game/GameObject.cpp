#include "game/GameObject.hpp"

#include <utility>
#include <cassert>

#include "application/Application.hpp"
#include "game/Game.hpp"
#include "game/Component.hpp"

void GameObject::BindComponent(Component* c)
{
	assert(c->gameObject == nullptr || c->gameObject == this);
	assert(std::find(components.cbegin(), components.cend(), c) == components.cend());

	components.push_back(c);
	c->BindToGameObject(this);

	c->onStart();
}

void GameObject::InvokeStart()
{
	for (Component* c : components) c->onStart();
}

GameObject::GameObject(Level* level) :
	level(level)
{
}

GameObject::~GameObject()
{
	if (components.size() != 0)
	{
		for (Component* c : components) level->getHeap()->destroy(c);
		components.clear();
	}
}
