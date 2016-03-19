#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include <SDL.h>
#include "utils.h"

class Render;

static const unsigned RED_INDEX = 3;
static const unsigned GREEN_INDEX = 2;
static const unsigned BLUE_INDEX = 1;
static const unsigned ALPHA_INDEX = 0;

class Colour
{
public:
	Colour(Uint32 colour = 0);
	Colour(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 0xff);
	Colour(const Colour& copy);

	Colour& operator=(const Colour& assign);

	void SetR(Uint8 value);
	void SetG(Uint8 value);
	void SetB(Uint8 value);
	void SetA(Uint8 value);
	void SetColour(Uint32 value);

	Uint8 GetR() const;
	Uint8 GetG() const;
	Uint8 GetB() const;
	Uint8 GetA() const;
	Uint32 GetColour() const;

private:
	union
	{
		Uint8 colArr[4];
		Uint32 col;
	};
};

class Texture
{
public:
	Texture();
	Texture(const char* filename);
	// creates a texture that is part from the provided texture, by using it's surface and renderer
	Texture(const Texture& src, Rect subRect, Render * render);
	Texture(int width, int height, SDL_PixelFormat * pfm);

	~Texture();

	void InitSurface(int width, int height, SDL_PixelFormat * pfm);

	bool LoadTexture(const char* filename);
	bool SetPixelFormat(SDL_PixelFormat* source, Uint32 flags = 0);
	bool RenderTexture(Render* render);
	void InitTransparent(Colour col, Colour mask = 0xffffff00);
	SDL_Texture* GetTexture() const;

	void SetPixel(Colour c, int x, int y);
	void SetRect(const Colour arr[], int destX, int destY, int w, int h);
	void SetRect(const Colour arr[], const Rect& r);

	// returns false if the surface for the image has not been loaded
	bool IsLoaded() const;

	// returns false if the image surface has not been rendered to a texture
	bool IsRendered() const;

	// returns true if the texture is dirty (not rendered since last changes)
	bool IsDirty() const;

	int GetWidth() const;
	int GetHeight() const;

private:
	// disable copy and assign of textures, because no pretty sdl routine seems to exist
	Texture(const Texture& copy);
	Texture& operator=(const Texture& assign);

	void freeMem();

	SDL_Texture* tex;
	SDL_Surface* img;

	int width;
	int height;
	bool dirty; // <! true if the surface image has been changed from the last rendering of the texture or no render is present
};

#endif // __TEXTURE_H__