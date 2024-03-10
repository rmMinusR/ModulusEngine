#pragma once

#include "gui/Widget.hpp"

class PluginManager;
class ShaderProgram;
class Material;

class PluginManagerView : public Widget
{
	int selectionIndex;
	PluginManager* mgr = nullptr;
	Material* material;

public:
	PluginManagerView(HUD* hud, PluginManager* mgr, Material* material);
	~PluginManagerView();

	virtual const Material* getMaterial() const override;
	virtual void renderImmediate(Renderer* target) override;
};
