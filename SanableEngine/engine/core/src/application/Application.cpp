#include "application/Application.hpp"

#include <cassert>
#include <iostream>

#include <SDL.h>

#include "System.hpp"
#include "GlobalTypeRegistry.hpp"
#include "application/Window.hpp"
#include "game/Game.hpp"
#include "MeshRenderer.hpp"
#include "Camera.hpp"

void Application::processEvents()
{
    assert(isAlive);
    
    SDL_Event event;

    //Utility functions
    auto forwardToWindow = [&](SDL_Window* windowHandle)
    {
        auto it = std::find_if(windows.begin(), windows.end(), [=](Window* w) { return w->handle == windowHandle; });
        if (it != windows.end()) (*it)->handleEvent(event);
    };

    while (SDL_PollEvent(&event))
    {
        //Old testing stuff, should prob be refactored
        if (event.type == SDL_QUIT) quit = true;

        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE) quit = true;
            if (event.key.keysym.sym == SDLK_F5) pluginManager.reloadAll();
        }

        //Foward events to appropriate windows
        switch (event.type)
        {
        case SDL_WINDOWEVENT:
            forwardToWindow(SDL_GetWindowFromID(event.window.windowID));
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            forwardToWindow(SDL_GetKeyboardFocus());
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEMOTION:
            forwardToWindow(SDL_GetMouseFocus());
            break;

        }
    }
}

Application::Application() :
    isAlive(false),
    system(nullptr),
    pluginManager(this),
    mainWindow(nullptr)
{
}

Application::~Application()
{
    assert(!isAlive);
}

void engine_reportTypes(ModuleTypeRegistry* registry);

void Application::init(Game* game, const GLSettings& glSettings, WindowBuilder& mainWindowBuilder, gpr460::System& _system, UserInitFunc userInitCallback)
{
    assert(!isAlive);
    isAlive = true;
    quit = false;

    frameAllocator.resize(frameAllocatorSize);

    this->system = &_system;
    system->Init(this);

    //Prepare RTTI
    GlobalTypeRegistry::clear();
    GlobalTypeRegistry::loadIntrinsics();
    {
        ModuleTypeRegistry m;
        engine_reportTypes(&m);
        GlobalTypeRegistry::loadModule("Application", m);
    }

    levelHeap.emplace();
    levelHeap.value().getSpecificPool<GameObject>(true); //Force create GameObject pool now so it's owned by main module (avoiding nasty access violation errors)
    levelHeap.value().getSpecificPool<Camera>(true); //Same with Camera
    levelHeap.value().getSpecificPool<MeshRenderer>(true); //And MeshRenderer
    levelHeap.value().ensureFresh();

    this->game = game;
    game->init(this);

    this->glSettings = glSettings;
    mainWindow = mainWindowBuilder.build();

    pluginManager.discoverAll(system->GetBaseDir()/"plugins");
    std::cout << "Discovered " << pluginManager.plugins.size() << " plugins" << std::endl;
    for (Plugin const* p : pluginManager.plugins) std::cout << " - " << std::filesystem::relative( p->getPath(), system->GetBaseDir() ).string() << std::endl;
    pluginManager.loadAll();
    pluginManager.hookAll();

    if (userInitCallback) (*userInitCallback)(this);
    levelHeap.value().ensureFresh();
    game->refreshCallBatchers();
}

void Application::shutdown()
{
    assert(isAlive);
    isAlive = false;

    game->applyConcurrencyBuffers();
    pluginManager.unhookAll(true); //FIXME: Pools destroyed automatically here, but Component and GameObject need to interface with Game
    game->applyConcurrencyBuffers();
    game->cleanup();
    game->applyConcurrencyBuffers();

    //If any plugins didn't clean up their window, do it for them
    while (!windows.empty()) delete windows[windows.size()-1]; //Destructor will automatically erase the tail
    mainWindow = nullptr;
    
    levelHeap.value().destroyPool<GameObject>(); //Clean up memory, GameObject pool first so remaining components are released
    pluginManager.unloadAll(); //Unload plugin code, handling destructors of globals in module

    //RTTI and plugin info shouldn't appear on the leaks report
    GlobalTypeRegistry::clear();
    pluginManager.forgetAll();

    levelHeap.reset(); //Finish cleaning up memory
    system->Shutdown();
}

void Application::doMainLoop()
{
    system->DoMainLoop();
}

void Application::frameStep(void* arg)
{
    Application* engine = (Application*)arg;

    engine->frameAllocator.restoreCheckpoint(StackAllocator::Checkpoint());

    engine->game->refreshCallBatchers(false);
    engine->processEvents();
    engine->game->refreshCallBatchers(false);
    engine->game->tick();
    engine->game->refreshCallBatchers(false);
    for (Window* w : engine->windows) w->draw();

    if (engine->pluginManager.executeCommandBuffer() != 0) engine->levelHeap.value().ensureFresh();
}

Game* Application::getGame() const
{
    return game;
}

gpr460::System* Application::getSystem()
{
    return system;
}

MemoryHeap* Application::getLevelHeap()
{
    return &levelHeap.value();
}

StackAllocator* Application::getFrameAllocator()
{
    return &frameAllocator;
}

PluginManager* Application::getPluginManager()
{
    return &pluginManager;
}

Window* Application::getMainWindow()
{
    return !windows.empty() ? windows[0] : nullptr; //FIXME hacky
}

WindowBuilder Application::buildWindow(const std::string& name, int width, int height)
{
    return WindowBuilder(this, name, width, height, glSettings);
}
