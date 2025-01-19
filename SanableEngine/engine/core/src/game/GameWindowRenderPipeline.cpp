#include "game/GameWindowRenderPipeline.hpp"

#include <unordered_map>
#include <GL/glew.h>
#include "game/Game.hpp"
#include "game/Level.hpp"
#include "application/Window.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include "ShaderProgram.hpp"
#include "MemoryRoot.hpp"

GameWindowRenderPipeline::GameWindowRenderPipeline(Game* game) :
	game(game),
	hud(game->getApplication())
{
}

GameWindowRenderPipeline::~GameWindowRenderPipeline()
{
}

void GameWindowRenderPipeline::setup(Window* window)
{
	WindowRenderPipeline::setup(window);
}

void GameWindowRenderPipeline::render(Rect<float> viewport)
{
	//Set projection matrix
	if (Camera::getMain()) Camera::getMain()->beginFrame({ viewport.size.x, viewport.size.y, 0 });
	else printf("WARNING: No main camera!");

	//Reset screen
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Collect objects to buffer
	std::unordered_map<
		const ShaderProgram*, //Group by shader
		std::unordered_map<
			const Material*, //Then by material
			std::vector<const I3DRenderable*>
		>
	> renderables; //Note: No need for a CallBatcher here, we're guaranteed renderables will be grouped by level, then type since our data source is a CallBatcher
	auto registerRenderable = [&](const I3DRenderable* r) { renderables[r->getShader()][r->getMaterial()].push_back(r); };
	game->visitLevels([&](Level* level) { level->get3DRenderables()->staticCall(registerRenderable); });

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	//Process buffer
	Renderer* renderInterface = window->getRenderer();
	for (const auto& shaderGroup : renderables)
	{
		//Activate shader
		if (shaderGroup.first) shaderGroup.first->activate();
		else ShaderProgram::clear();

		for (const auto& materialGroup : shaderGroup.second)
		{
			//Activate material
			if (materialGroup.first) materialGroup.first->writeSharedUniforms(renderInterface);
			assert(materialGroup.first == nullptr || materialGroup.first->getShader() == shaderGroup.first);

			for (const I3DRenderable* r : materialGroup.second)
			{
				r->loadModelTransform(renderInterface);

				if (materialGroup.first) materialGroup.first->writeInstanceUniforms(renderInterface, r);

				assert(r->getMaterial() == materialGroup.first);
				assert(r->getMaterial() == nullptr || r->getMaterial()->getShader() == shaderGroup.first);
				r->renderImmediate(renderInterface);
			}
		}
	}

	glPopMatrix();

	hud.refreshLayout(viewport);
	hud.tick(); //FIXME logic shouldn't be in render, move elsewhere
	hud.render(renderInterface);
}
