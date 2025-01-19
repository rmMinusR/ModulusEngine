#pragma once

#include "../dllapi.h"

#include <vector>
#include <cassert>

#include "MemoryHeap.hpp"
#include "application/Application.hpp"
#include "Transform.hpp"

class ModuleTypeRegistry;
class Component;
class Level;

class GameObject
{
protected:
    Transform transform;

    std::vector<Component*> components;
    friend class Level;
    friend class Component;

    Level* level;

    void BindComponent(Component* c);
    void InvokeStart();

public:
    GameObject(Level* level);
    ~GameObject();

    inline Level* getLevel() { return level; }

    inline Transform* getTransform() { return &transform; }

    template<typename T, typename... TCtorArgs>
    inline T* CreateComponent(const TCtorArgs&... ctorArgs)
    {
        T* component;
        assert((component = GetComponent<T>()) == nullptr);
        component = level->getHeap()->create<T>(ctorArgs...);
        level->componentAddBuffer.push_back(std::pair<Component*, GameObject*>(component, this));
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
