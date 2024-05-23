#include "Texture.hpp"

#include <cassert>

#include <SDL_render.h>
#include <SDL_image.h>

#include "stb_image.h"

#include "Renderer.hpp"
#include "application/Window.hpp"

GTexture* GTexture::fromFile(const std::filesystem::path& path, Renderer* ctx)
{
	//Load onto CPU
	int width, height, nChannels;
	void* data = stbi_load(path.u8string().c_str(), &width, &height, &nChannels, 0);
	assert(data);
	//cpu = IMG_Load(path.u8string().c_str());
	//assert(cpu);
	
	GTexture* out = new GTexture(ctx, width, height, nChannels, data);

	//Cleanup
	stbi_image_free(data);

	return out;
}

GTexture::GTexture(Renderer* ctx, int width, int height, int nChannels, void* data) :
	id(0),
	width(width),
	height(height),
	nChannels(nChannels)
{
	Window::setActiveDrawTarget(ctx->getOwner());

	glGenTextures(1, &id);
	assert(id);

	glBindTexture(GL_TEXTURE_2D, id);

	//Set filtering/wrapping modes
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//Send data to GPU
	//TODO better channels description detection
	int glChannelsDesc;
	switch (nChannels)
	{
		case 1: glChannelsDesc = GL_RED; break;
		case 2: glChannelsDesc = GL_RG; break;
		case 3: glChannelsDesc = GL_RGB; break;
		case 4: glChannelsDesc = GL_RGBA; break;
		default: assert(false); break;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, glChannelsDesc, width, height, 0, glChannelsDesc, GL_UNSIGNED_BYTE, data);

	ctx->errorCheck();
}

GTexture::~GTexture()
{
	glDeleteTextures(1, &id);
}

GTexture::GTexture(GTexture&& mov)
{
	*this = std::move(mov); //Defer
}

GTexture& GTexture::operator=(GTexture&& mov)
{
	if (this->id) glDeleteTextures(1, &id);

	this->id = mov.id;
	mov.id = 0;

	this->width = mov.width;
	this->height = mov.height;
	this->nChannels = mov.nChannels;

	return *this;
}

int GTexture::getWidth() const
{
	return width;
}

int GTexture::getHeight() const
{
	return height;
}

Vector2<int> GTexture::getSize() const
{
	return Vector2<int>(width, height);
}

int GTexture::getNChannels() const
{
	return nChannels;
}

GTexture::operator bool() const
{
	return id != 0;
}
