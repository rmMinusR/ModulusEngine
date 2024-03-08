#pragma once

#include <filesystem>

#include <SDL_render.h>
#include <GL/glew.h>

#include "dllapi.h"

class Renderer;

class Texture
{
	friend class Renderer;

	GLuint id;

	int width;
	int height;
	int nChannels;

public:
	ENGINEGRAPHICS_API Texture(const std::filesystem::path&, SDL_GLContext ctx); //FIXME should use own renderer instead?
	ENGINEGRAPHICS_API ~Texture();

	ENGINEGRAPHICS_API Texture(Texture&& mov);
	ENGINEGRAPHICS_API Texture& operator=(Texture&& mov);
	Texture(const Texture& cpy) = delete;
	Texture& operator=(const Texture& cpy) = delete;

	inline int getNativeWidth () const { return width; }
	inline int getNativeHeight() const { return height; }
};
