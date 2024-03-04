#pragma once

#include "../dllapi.h"

#include <vector>
#include <cassert>

#include "MemoryManager.hpp"
#include "application/Application.hpp"
#include "Transform.hpp"

class ModuleTypeRegistry;
class Component;
class Game;
struct HotswapTypeData;

class GameObject
{
protected:
    Transform transform;

    std::vector<Component*> components;
    friend class Game;
    friend class Component;

    Game* const engine;

    void BindComponent(Component* c);
    void InvokeStart();

public:
    GameObject(Game* engine);
    ~GameObject();

    inline Game* getContext() { return engine; }

    inline Transform* getTransform() { return &transform; }
    inline EngineCore* getEngine() { return engine; }

    template<typename T, typename... TCtorArgs>
    inline T* CreateComponent(const TCtorArgs&... ctorArgs)
    {
        T* component;
        assert((component = GetComponent<T>()) == nullptr);
        component = engine->getApplication()->getMemoryManager()->create<T>(ctorArgs...);
        engine->componentAddBuffer.push_back(std::pair<Component*, GameObject*>(component, this));
        return component;
    }

    template<typename T>
    inline T* GetComponent()
    {
        T* out = nullptr;
        for (Component* c : components) if (( out = dynamic_cast<T*>(c) )) return out;
        return nullptr;
    }
};
