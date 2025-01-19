#pragma once

#include <vector>
#include <optional>

#include <ReflectionSpec.hpp>
#include "MemoryHeap.hpp"
#include "StackAllocator.hpp"

#include "../dllapi.h"

#include "application/PluginManager.hpp"
#include "application/WindowBuilder.hpp"

namespace gpr460 { class System; }
class Game;

class Application
{
private:
    Game* game;

    bool isAlive;
    gpr460::System* system;
    std::optional<MemoryHeap> heap; //Optional so we can do late initialization/early destruction
    StackAllocator frameAllocator; //Temp memory that will be reset every frame
    constexpr static size_t frameAllocatorSize = 4096;
    PluginManager pluginManager;
    friend class PluginManager;

    GLSettings glSettings;
    std::vector<Window*> windows;
    friend class WindowBuilder;
    friend class Window;
    Window* mainWindow = nullptr;

    void processEvents();

public:
    bool quit = false;

    ENGINECORE_API Application();
    ENGINECORE_API ~Application();

    typedef void (*UserInitFunc)(Application*);
    ENGINECORE_API void init(Game* game, const GLSettings& glSettings, WindowBuilder& mainWindowBuilder, gpr460::System& system, UserInitFunc userInitCallback);
    ENGINECORE_API void shutdown();

    ENGINECORE_API void doMainLoop();
    ENGINECORE_API static void frameStep(void* arg);

    ENGINECORE_API Game* getGame() const;
    ENGINECORE_API gpr460::System* getSystem();
    ENGINECORE_API MemoryHeap* getHeap();
    ENGINECORE_API StackAllocator* getFrameAllocator();
    ENGINECORE_API PluginManager* getPluginManager();
    ENGINECORE_API Window* getMainWindow();

    ENGINECORE_API WindowBuilder buildWindow(const std::string& name, int width, int height);
};
