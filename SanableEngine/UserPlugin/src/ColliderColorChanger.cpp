#include "ColliderColorChanger.hpp"

#include "application/Application.hpp"
#include "game/Game.hpp"
#include "game/GameObject.hpp"
#include "RectangleCollider.hpp"
#include "RectangleRenderer.hpp"

void ColliderColorChanger::BindToGameObject(GameObject* obj)
{
	Component::BindToGameObject(obj);

	collider = obj->GetComponent<RectangleCollider>();
	renderer = obj->GetComponent<RectangleRenderer>();

	assert(collider);
	assert(renderer);
}

ColliderColorChanger::ColliderColorChanger(SDL_Color normalColor, SDL_Color overlapColor) :
	Component(),
	normalColor(normalColor),
	overlapColor(overlapColor),
	collider(nullptr),
	renderer(nullptr)
{
}

ColliderColorChanger::~ColliderColorChanger()
{
}

void ColliderColorChanger::Update()
{
	int nHits = collider->GetCollisions(nullptr);
	RectangleCollider** hits = getEngine()->getApplication()->getFrameAllocator()->alloc<RectangleCollider*>(nHits);
	collider->GetCollisions(hits);

	renderer->SetColor(nHits>0 ? overlapColor : normalColor);

	for (int i = 0; i < nHits; ++i) printf("0x%p is colliding with 0x%p\n", this, hits[i]);
}
