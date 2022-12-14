#include "EngineCore.hpp"

#include <SDL.h>

#include <cassert>
#include <iostream>

#include "GameObject.hpp"
#include "Component.hpp"
#include "System.hpp"

uint32_t GetTicks()
{
    return SDL_GetTicks();
}

void EngineCore::applyConcurrencyBuffers()
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

void EngineCore::processEvents()
{
    assert(isAlive);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT) quit = true;

        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE) quit = true;
            if (event.key.keysym.sym == SDLK_F5) reloadPlugins();
        }
    }
}

void EngineCore::reloadPlugins()
{
    std::cout << "Hot Reload Started\n";

    std::cout << "Removing plugins hooks...\n";
    pluginManager.unhookAll();
    applyConcurrencyBuffers();

    std::cout << "Unloading plugin code...\n";
    pluginManager.unloadAll();

    std::cout << "Loading plugin code...\n";
    pluginManager.discoverAll(system->GetBaseDir()/"plugins");

    std::cout << "Refreshing pointers... (vtables)\n";
    pluginManager.refreshVtablePointers();

    std::cout << "Applying plugin hooks...\n";
    pluginManager.hookAll();
    
    std::cout << "Refreshing pointers... (call batchers)\n";
    refreshCallBatchers();

    std::cout << "Hot Reload Complete\n";
}

void EngineCore::refreshCallBatchers()
{
    updateList.clear();
    renderList.clear();

    for (GameObject* go : objects)
    {
        for (GameObject::ComponentRecord& c : go->components)
        {
            IUpdatable* u = dynamic_cast<IUpdatable*>(c.ptr);
            if (u) updateList.add(u);

            IRenderable* r = dynamic_cast<IRenderable*>(c.ptr);
            if (r) renderList.add(r);
        }
    }
}

EngineCore::EngineCore() :
    isAlive(false),
    system(nullptr),
    window(nullptr),
    renderer(nullptr),
    pluginManager(this)
{
}

EngineCore::~EngineCore()
{
    assert(!isAlive);
}

void EngineCore::init(char const* windowName, int windowWidth, int windowHeight, SystemFactoryFunc createSystem, UserInitFunc userInitCallback)
{
    assert(!isAlive);
    isAlive = true;

    system = createSystem();
    system->Init(this);
    memoryManager.init();

    quit = false;
    window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    frame = 0;
    frameStart = GetTicks();

    pluginManager.discoverAll(system->GetBaseDir()/"plugins");

    if (userInitCallback) (*userInitCallback)(this);
}

void EngineCore::shutdown()
{
    assert(isAlive);
    isAlive = false;

    for (GameObject* o : objects) memoryManager.destroy(o);
    objects.clear();

    pluginManager.unhookAll();
    pluginManager.unloadAll();

    SDL_DestroyRenderer(renderer);
    renderer = nullptr;

    SDL_DestroyWindow(window);
    window = nullptr;

    memoryManager.cleanup();
    system->Shutdown();
    delete system;
}

GameObject* EngineCore::addGameObject()
{
    GameObject* o = memoryManager.create<GameObject>(this);
    objectAddBuffer.push_back(o);
    return o;
}

void EngineCore::destroy(GameObject* go)
{
    objectDelBuffer.push_back(go);
}

void EngineCore::destroyImmediate(GameObject* go)
{
    assert(std::find(objects.cbegin(), objects.cend(), go) != objects.cend());
    objects.erase(std::find(objects.begin(), objects.end(), go));
    memoryManager.destroy(go);
}

void EngineCore::destroyImmediate(Component* c)
{
    assert(std::find(objects.cbegin(), objects.cend(), c->getGameObject()) != objects.cend());
    auto& l = c->getGameObject()->components;
    auto it = std::find_if(l.cbegin(), l.cend(), [&](GameObject::ComponentRecord r) { return r.ptr == c; });
    assert(it != l.cend());
    l.erase(it);
    memoryManager.destroy(c);
}

void EngineCore::doMainLoop()
{
    system->DoMainLoop();
}

void EngineCore::frameStep(void* arg)
{
    EngineCore* engine = (EngineCore*)arg;

    engine->tick();
    engine->draw();
}

void EngineCore::tick()
{
    assert(isAlive);

    frame++;
    frameStart = GetTicks();

    processEvents();

    applyConcurrencyBuffers();

    updateList.memberCall(&IUpdatable::Update);
}

void EngineCore::draw()
{
    assert(isAlive);

    //Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    //Draw objects
    renderList.memberCall(&IRenderable::Render);

    SDL_RenderPresent(renderer);
}
