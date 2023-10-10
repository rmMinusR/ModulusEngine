#include <iostream>

#include "PluginCore.hpp"
#include "Plugin.hpp"

#include <SDL.h>
#include "GameObject.hpp"
#include "RectangleRenderer.hpp"
#include "RectangleCollider.hpp"
#include "ColliderColorChanger.hpp"
#include "PlayerController.hpp"
#include "ObjectSpinner.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "MeshRenderer.hpp"
#include "ShaderProgram.hpp"
#include "Material.hpp"

EngineCore* engine;

PLUGIN_C_API(bool) plugin_preInit(Plugin* const context, PluginReportedData* report, EngineCore* engine)
{
    std::cout << "UserPlugin: plugin_preInit() called" << std::endl;
    ::engine = engine;

    report->name = "UserPlugin";


    return true;
}

GameObject* camera;
GameObject* player;
GameObject* obstacle;
GameObject* staticObj;

Mesh* mesh;
ShaderProgram* shader;
Material* material;

PLUGIN_C_API(bool) plugin_init(bool firstRun)
{
    std::cout << "UserPlugin: plugin_init() called" << std::endl;

    if (firstRun) {
        camera = engine->addGameObject();
        Camera* cc = camera->CreateComponent<Camera>();
        cc->zFar = 100;
        //cc->setGUIProj();
        //cc->setOrtho(400);
        //cc->setOrtho(1);
        cc->setPersp(90);
        //camera->getTransform()->setRotation(glm::angleAxis(glm::radians(30.0f), glm::vec3(0, 0, 1)));
        camera->CreateComponent<PlayerController>(0.01f);

        mesh = new Mesh();
        //mesh->load("resources/bunny.fbx");
        mesh->load("resources/dragon.fbx");

        shader = new ShaderProgram("resources/shaders/fresnel");
        shader->load();
        
        material = new Material(shader);

        for (Vector3f pos(-0.5f, -0.5f, -0.4f); pos.y < 0.5f; pos.y += 0.2f) for (pos.x = -0.5f; pos.x < 0.5f; pos.x += 0.2f)
        {
            GameObject* o = engine->addGameObject();
            o->getTransform()->setPosition(pos);
            o->CreateComponent<MeshRenderer>(mesh, material);

            Vector3f axis;
            axis.x = (float(rand())/RAND_MAX)*2 - 1;
            axis.y = (float(rand())/RAND_MAX)*2 - 1;
            axis.z = (float(rand())/RAND_MAX)*2 - 1;
            float angle = ((float(rand())/RAND_MAX)*2 - 1) * 0.02f;
            o->CreateComponent<ObjectSpinner>(glm::angleAxis(angle, (glm::vec3)axis));
        }

        player = engine->addGameObject();
        player->getTransform()->setPosition(Vector3<float>(50, 50, -10));
        //player->CreateComponent<PlayerController>(1);
        player->CreateComponent<RectangleCollider>(10, 10);
        player->CreateComponent<RectangleRenderer>(10, 10, SDL_Color{ 255, 0, 0, 255 });
        player->CreateComponent<ColliderColorChanger>(SDL_Color{ 255, 0, 0, 255 }, SDL_Color{ 0, 0, 255, 255 });

        staticObj = engine->addGameObject();
        staticObj->getTransform()->setPosition(Vector3<float>(350, 210, -20));
        staticObj->CreateComponent<RectangleRenderer>(510, 120, SDL_Color{ 0, 127, 0, 255 });

        obstacle = engine->addGameObject();
        obstacle->getTransform()->setPosition(Vector3<float>(225, 225, -15));
        obstacle->CreateComponent<RectangleCollider>(50, 50);
        obstacle->CreateComponent<RectangleRenderer>(50, 50, SDL_Color{ 127, 63, 0, 255 });
    }

    return true;
}

PLUGIN_C_API(void) plugin_cleanup(bool shutdown)
{
    std::cout << "UserPlugin: plugin_cleanup() called" << std::endl;

    if (shutdown)
    {
        engine->destroy(camera);
        engine->destroy(player);
        engine->destroy(obstacle);
        engine->destroy(staticObj);

        delete mesh;

        engine->getMemoryManager()->destroyPool<PlayerController>();
        engine->getMemoryManager()->destroyPool<ColliderColorChanger>();
    }
}
