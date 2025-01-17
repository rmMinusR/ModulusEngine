#pragma once

#include <ReflectionSpec.hpp>
#include "WidgetTransform.hpp"

class Renderer;
class HUD;
class Material;
class ShaderProgram;

class STIX_ENABLE_IMAGE_CAPTURE Widget
{
protected:
	HUD* hud;
	friend class HUD;

public:
	WidgetTransform transform;

	ENGINEGUI_API Widget(HUD* hud);
	ENGINEGUI_API virtual ~Widget();

	ENGINEGUI_API virtual void refreshLayout(); //TODO refactor to separate interface? Will likely do when implementing caching for WidgetTransforms
	ENGINEGUI_API virtual void tick();

	//These should return true if they consume the click event
	//TODO refactor to separate interface?
	ENGINEGUI_API virtual bool onMouseDown(Vector2f pos);
	ENGINEGUI_API virtual bool onMouseUp(Vector2f pos);

	ENGINEGUI_API const ShaderProgram* getShader() const;
	virtual const Material* getMaterial() const = 0;
	ENGINEGUI_API virtual void loadModelTransform(Renderer* renderer) const; //Makes no assumptions about current model state or model mode, just writes
	virtual void renderImmediate(Renderer* renderer) = 0;
};
