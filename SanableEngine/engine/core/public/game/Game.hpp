#pragma once

#include <vector>
#include <optional>
#include <functional>
#include "../dllapi.h"
#include "Level.hpp"

class Application;
class PluginManager;
class GameObject;
class InputSystem;

class Game
{
    Application* application;
    InputSystem* inputSystem;

    std::optional<Level> level;
    void applyConcurrencyBuffers(); //Passthrough for now
    
    friend class PluginManager;
    void refreshCallBatchers(bool force = false);

    //TODO allow accumulation from multiple levels
    //PoolCallBatcher<IUpdatable> updateList;
    //PoolCallBatcher<I3DRenderable> _3dRenderList;

    friend class Application;
    void init(Application* application);
    void cleanup();
    void tick();

    bool isAlive;
public:
    ENGINECORE_API Game();
    ENGINECORE_API ~Game();

    int frame = 0;

	ENGINECORE_API InputSystem* getInput();
    ENGINECORE_API Application* getApplication() const;

	ENGINECORE_API void visitLevels(const std::function<void(Level*)>& visitor);
	ENGINECORE_API Level* getLevel(size_t which);
	ENGINECORE_API size_t getLevelCount() const;
};
