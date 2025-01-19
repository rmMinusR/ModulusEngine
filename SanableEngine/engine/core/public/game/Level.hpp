#pragma once

#include <vector>

#include "MemoryHeap.hpp"
#include "PoolCallBatcher.hpp"
#include "Component.hpp"

class GameObject;
class Game;

class Level
{
public:
    ENGINECORE_API Level(Game* game);
    ENGINECORE_API ~Level();

    ENGINECORE_API GameObject* addGameObject();
    ENGINECORE_API void destroy(GameObject* go);

    ENGINECORE_API const PoolCallBatcher<I3DRenderable>* get3DRenderables() const;

    ENGINECORE_API MemoryHeap* getHeap();
    ENGINECORE_API MemoryHeap const* getHeap() const;

    ENGINECORE_API Game* getGame() const;
    
private:
    MemoryHeap heap;

	std::vector<GameObject*> objects;
    void applyConcurrencyBuffers();
    std::vector<GameObject*> objectAddBuffer;
    std::vector<GameObject*> objectDelBuffer;
    std::vector<std::pair<Component*, GameObject*>> componentAddBuffer;
    std::vector<Component*> componentDelBuffer;
    friend class GameObject;
    friend class Game;

    PoolCallBatcher<IUpdatable> updateList;
    PoolCallBatcher<I3DRenderable> _3dRenderList;

    void refreshCallBatchers(bool force = false);

    void destroyImmediate(GameObject* go);
    void destroyImmediate(Component* c);

    void tick();

    Game* game;
};
