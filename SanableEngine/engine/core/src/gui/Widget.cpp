#include "gui/Widget.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "Material.hpp"
#include "Renderer.hpp"

Widget::Widget(HUD* hud) :
    hud(hud)
{
}

Widget::~Widget()
{
}

void Widget::tick()
{
}

const ShaderProgram* Widget::getShader() const
{
    const Material* material = getMaterial();
    return material ? material->getShader() : nullptr;
}

void Widget::loadModelTransform(Renderer* renderer) const
{
    renderer->loadTransform(transform);
}
