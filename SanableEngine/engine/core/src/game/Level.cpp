#include "game/Level.hpp"

#include "MeshRenderer.hpp"
#include "Camera.hpp"

void Level::applyConcurrencyBuffers()
{
	for (Component* c : componentDelBuffer) destroyImmediate(c);
    componentDelBuffer.clear();

    for (GameObject* go : objectDelBuffer) destroyImmediate(go);
    objectDelBuffer.clear();

    for (GameObject* go : objectAddBuffer)
    {
        objects.push_back(go);
        go->InvokeStart();
    }
    objectAddBuffer.clear();

    for (auto& i : componentAddBuffer) i.second->BindComponent(i.first);
    for (auto& i : componentAddBuffer) i.first->onStart();
    componentAddBuffer.clear();
}

void Level::refreshCallBatchers(bool force)
{
    updateList   .ensureFresh(&heap, force);
    _3dRenderList.ensureFresh(&heap, force);
}

void Level::destroyImmediate(GameObject* go)
{
    assert(std::find(objects.cbegin(), objects.cend(), go) != objects.cend());
    objects.erase(std::find(objects.begin(), objects.end(), go));
    heap.destroy(go);
}

void Level::destroyImmediate(Component* c)
{
    assert(std::find(objects.cbegin(), objects.cend(), c->getGameObject()) != objects.cend());
    auto& l = c->getGameObject()->components;
    auto it = std::find(l.cbegin(), l.cend(), c);
    assert(it != l.cend());
    l.erase(it);
    heap.destroy(c);
}

void Level::tick()
{
    applyConcurrencyBuffers();
    updateList.memberCall(&IUpdatable::Update);
}

Level::Level(Game* game) :
    game(game)
{
    heap.getSpecificPool<GameObject>(true); //Force create GameObject pool now so it's owned by main module (avoiding nasty access violation errors)
    heap.getSpecificPool<Camera>(true); //Same with Camera
    heap.getSpecificPool<MeshRenderer>(true); //And MeshRenderer
}

Level::~Level()
{
    //Delete objects
    applyConcurrencyBuffers();
    for (GameObject* o : objects) destroy(o);
    applyConcurrencyBuffers();
}

GameObject* Level::addGameObject()
{
    GameObject* o = heap.create<GameObject>(this);
    objectAddBuffer.push_back(o);
    return o;
}

void Level::destroy(GameObject* go)
{
    objectDelBuffer.push_back(go);
}

const PoolCallBatcher<I3DRenderable>* Level::get3DRenderables() const
{
    return &_3dRenderList;
}

MemoryHeap* Level::getHeap()
{
    return &heap;
}

MemoryHeap const* Level::getHeap() const
{
    return &heap;
}

Game* Level::getGame() const
{
    return game;
}
