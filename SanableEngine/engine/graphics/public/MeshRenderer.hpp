#pragma once

#include <EngineCoreReflectionHooks.hpp>
#include "game/Component.hpp"
#include "dllapi.h"

class Mesh;
class Material;
class Renderer;

class MeshRenderer : public Component, public I3DRenderable
{
	SANABLE_REFLECTION_HOOKS
	MeshRenderer();

	Mesh* mesh;
	Material* material;
public:
	ENGINEGRAPHICS_API MeshRenderer(Mesh* mesh, Material* material);
	ENGINEGRAPHICS_API ~MeshRenderer() = default;

	ENGINEGRAPHICS_API Material* getMaterial() const override;
protected:
	ENGINEGRAPHICS_API void renderImmediate(Renderer* renderer) const override;
};