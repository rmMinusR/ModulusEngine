#include "game/Game.hpp"

#include <cassert>

#include "game/GameObject.hpp"
#include "game/Component.hpp"
#include "game/InputSystem.hpp"
#include "game/Level.hpp"

Game::Game() :
    application(nullptr),
    inputSystem(nullptr),
    isAlive(false)
{
}

Game::~Game()
{
    assert(!isAlive);
}

void Game::init(Application* application)
{
    assert(!isAlive);
    isAlive = true;

    this->application = application;
    frame = 0;

    this->inputSystem = new InputSystem();

    levels = application->getHeap()->getSpecificPool<Level>(true);
}

void Game::cleanup()
{
    assert(isAlive);
    isAlive = false;

    //application->getHeap()->destroyPool<Level>(); //Don't do this, it throws incorrect warnings
    for (auto it = levels->cbegin(); it != levels->cend(); ++it) application->getHeap()->destroy(&*it);
    levels = nullptr;

    delete inputSystem;
}

void Game::applyConcurrencyBuffers()
{
    for (auto it = levels->cbegin(); it != levels->cend(); ++it)
    {
        it->applyConcurrencyBuffers();
    }
}

void Game::refreshCallBatchers(bool force)
{
    for (auto it = levels->cbegin(); it != levels->cend(); ++it)
    {
        it->refreshCallBatchers(force);
    }
}

void Game::tick()
{
    assert(isAlive);

    frame++;
    
    refreshCallBatchers();
    for (auto it = levels->cbegin(); it != levels->cend(); ++it)
    {
        it->tick();
    }
}

InputSystem* Game::getInput()
{
    return inputSystem;
}

Application* Game::getApplication() const
{
    return application;
}

void Game::visitLevels(const std::function<void(Level*)>& visitor)
{
    for (auto it = levels->cbegin(); it != levels->cend(); ++it)
    {
        visitor(&*it);
    }
}

Level* Game::getLevel(size_t which)
{
    return &*(levels->cbegin()+which);
}

size_t Game::getLevelCount() const
{
    return levels->asGeneric()->getNumAllocatedObjects();
}

Level* Game::addLevel()
{
    return levels->emplace(this);
}

void Game::removeLevel(Level* level)
{
    levels->release(level);
}
