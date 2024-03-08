#include "Texture.hpp"

#include <cassert>

#include <SDL_render.h>
#include <SDL_image.h>

#include "stb_image.h"

#include "Renderer.hpp"
#include "application/Window.hpp"

Texture* Texture::fromFile(const std::filesystem::path& path, Renderer* ctx)
{
	//Load onto CPU
	int width, height, nChannels;
	void* data = stbi_load(path.u8string().c_str(), &width, &height, &nChannels, 0);
	assert(data);
	//cpu = IMG_Load(path.u8string().c_str());
	//assert(cpu);
	
	Texture* out = new Texture(ctx, width, height, nChannels, data);

	//Cleanup
	stbi_image_free(data);

	return out;
}

Texture::Texture(Renderer* ctx, int width, int height, int nChannels, void* data) :
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
		case 1: glChannelsDesc = GL_R8; break;
		case 2: glChannelsDesc = GL_RG; break;
		case 3: glChannelsDesc = GL_RGB; break;
		case 4: glChannelsDesc = GL_RGBA; break;
		default: assert(false); break;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

Texture::~Texture()
{
	glDeleteTextures(1, &id);
}

Texture::Texture(Texture&& mov)
{
	*this = std::move(mov); //Defer
}

Texture& Texture::operator=(Texture&& mov)
{
	this->id = mov.id;
	mov.id = 0;

	this->width = mov.width;
	this->height = mov.height;
	this->nChannels = mov.nChannels;

	return *this;
}
