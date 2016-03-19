#include <SDL.h>

#include "texture.h"
#include "render.h"


Colour::Colour(Uint32 colour) : col(colour) {}
Colour::Colour(Uint8 r, Uint8 g, Uint8 b, Uint8 a) : col(0)
{
	colArr[RED_INDEX] = r;
	colArr[GREEN_INDEX] = g;
	colArr[BLUE_INDEX] = b;
	colArr[ALPHA_INDEX] = a;
}

Colour::Colour(const Colour& copy) : col(copy.col) {}

Colour& Colour::operator=(const Colour& assign)
{
	col = assign.col;
	return *this;
}

void Colour::SetR(Uint8 value)
{
	colArr[RED_INDEX] = value;
}

void Colour::SetG(Uint8 value)
{
	colArr[GREEN_INDEX] = value;
}

void Colour::SetB(Uint8 value)
{
	colArr[BLUE_INDEX] = value;
}

void Colour::SetA(Uint8 value)
{
	colArr[ALPHA_INDEX] = value;
}

void Colour::SetColour(Uint32 value)
{
	col = value;
}

Uint8 Colour::GetR() const
{
	return colArr[RED_INDEX];
}

Uint8 Colour::GetG() const
{
	return colArr[GREEN_INDEX];
}

Uint8 Colour::GetB() const
{
	return colArr[BLUE_INDEX];
}

Uint8 Colour::GetA() const
{
	return colArr[ALPHA_INDEX];
}

Uint32 Colour::GetColour() const
{
	return col;
}

Texture::Texture()
	:
	tex(nullptr),
	img(nullptr),
	width(0),
	height(0),
	dirty(true)
{}

Texture::Texture(const char* filename)
	:
	tex(nullptr),
	img(nullptr),
	width(0),
	height(0),
	dirty(true)
{
	if (filename) {
		img = SDL_LoadBMP(filename);
		if (img) {
			width = img->w;
			height = img->h;
		}
	}
}

Texture::Texture(const Texture& src, Rect subRect, Render * render)
	:
	tex(nullptr),
	img(nullptr),
	width(subRect.width),
	height(subRect.height),
	dirty(true)
{
	SDL_PixelFormat * pixelFormat = render->GetPixelFormat();
	SDL_Surface * tmp = SDL_CreateRGBSurface(0, subRect.width, subRect.height, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	if (tmp) {
		img = SDL_ConvertSurface(tmp, pixelFormat, 0);
		SDL_FreeSurface(tmp);
		if (img) {
			Uint32 * destPixels = (Uint32*)img->pixels;
			int destPitch = img->pitch;
			Uint32 * srcPixels = (Uint32*)src.img->pixels;
			int srcPitch = src.img->pitch;
			for (int y = 0; y < subRect.height; ++y) {
				Uint32* src = (Uint32*)((Uint8*)srcPixels + (subRect.y + y) * srcPitch) + subRect.x;
				Uint32* dest = (Uint32*)((Uint8*)destPixels + y * destPitch);
				for (int x = 0; x < subRect.width; ++x)
				{
					*(dest + x) = *(src + x);
				}
			}

			tex = SDL_CreateTextureFromSurface(render->renderer, img);
			if (tex) {
				dirty = false;
				SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
			}
		}
	}
}

Texture::Texture(int w, int h, SDL_PixelFormat * pfm)
	:
	tex(nullptr),
	img(nullptr),
	width(w),
	height(h),
	dirty(true)
{
	InitSurface(width, height, pfm);
}

Texture::~Texture() {
	freeMem();
}

void Texture::freeMem() {
	if (tex) {
		SDL_DestroyTexture(tex);
		tex = nullptr;
	}
	if (img) {
		SDL_FreeSurface(img);
		img = nullptr;
	}
}

void Texture::InitSurface(int w, int h, SDL_PixelFormat * pfm)
{
	if (pfm) {
		if (img) {
			SDL_FreeSurface(img);
			img = nullptr;
		}
		width = w;
		height = h;
		SDL_Surface * tmp = SDL_CreateRGBSurface(0, width, height, pfm->BitsPerPixel, pfm->Rmask, pfm->Gmask, pfm->Bmask, pfm->Amask);
		if (tmp) {
			img = SDL_ConvertSurface(tmp, pfm, 0);
			SDL_FreeSurface(tmp);
			dirty = true;
		}
	}
}

bool Texture::LoadTexture(const char* filename)
{
	bool retval = false;
	if (filename) {
		if (tex) {
			SDL_DestroyTexture(tex);
			tex = nullptr;
		}
		if (img) {
			SDL_FreeSurface(img);
			img = nullptr;
		}
		img = SDL_LoadBMP(filename);
		retval = (img != nullptr);
		if (retval) {
			// set dirty
			dirty = true;
			width = img->w;
			height = img->h;
		}
	}
	return retval;
}

bool Texture::SetPixelFormat(SDL_PixelFormat* source, Uint32 flags)
{
	bool retval = false;
	if (source) {
		SDL_Surface* newImg = SDL_ConvertSurface(img, source, flags);
		if (newImg) {
			retval = true;
			SDL_FreeSurface(img);
			img = newImg;
			dirty = true;
		}
	}
	return retval;
}

bool Texture::RenderTexture(Render* render)
{
	bool retval = false;
	if (render && img) {
		if (tex) {
			SDL_DestroyTexture(tex);
			tex = nullptr;
		}
		tex = SDL_CreateTextureFromSurface(render->renderer, img);
		if (tex) {
			retval = true;
			SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
			// clear dirty
			dirty = false;
		}
	}
	return retval;
}

void Texture::InitTransparent(Colour col, Colour mask)
{
	if (img) {
		if (tex) {
			SDL_DestroyTexture(tex);
			tex = nullptr;
		}
		// set dirty
		dirty = true;

		int w = img->w;
		int h = img->h;
		Uint32* pixels = (Uint32*)img->pixels;
		int colourChannels = 4;
		Uint32 maskRaw = mask.GetColour();
		Uint32 colourRaw = col.GetColour();
		Uint32 maskKeep = (colourChannels < 3 ? (~maskRaw & 0xffffff00) : 0xffffff00);
		for (int i = 0; i < w*h; ++i) {
			Uint32 difference = 0;
			difference += ((pixels[i] & (0xff000000 & maskRaw)) ^ (colourRaw & (0xff000000 & maskRaw))) >> 24;
			difference += ((pixels[i] & (0x00ff0000 & maskRaw)) ^ (colourRaw & (0x00ff0000 & maskRaw))) >> 16;
			difference += ((pixels[i] & (0x0000ff00 & maskRaw)) ^ (colourRaw & (0x0000ff00 & maskRaw))) >> 8;
			difference /= colourChannels;
			pixels[i] = (pixels[i] & maskKeep) | difference;
		}
	}
}

SDL_Texture* Texture::GetTexture() const
{
	return tex;
}

void Texture::SetPixel(Colour c, int x, int y)
{
	if (img && x >= 0 && y >= 0 && x < width && y < height) {
		Uint32 * dest = reinterpret_cast<Uint32*>(static_cast<Uint8*>(img->pixels) + (y * img->pitch)) + x;
		*dest = c.GetColour();
		dirty = true;
	}
}

void Texture::SetRect(const Colour arr[], int destX, int destY, int w, int h)
{
	Rect destRect(destX, destY, w, h);
	Rect texRect(0, 0, width, height);
	if (img && arr && destRect <= texRect) {
		for (int y = 0; y < h; ++y) {
			Uint32 * dest = reinterpret_cast<Uint32*>(static_cast<Uint8*>(img->pixels) + (destY + y) * img->pitch) + destX;
			for (int x = 0; x < w; ++x) {
				*dest++ = arr[y * w + x].GetColour();
			}
		}
		dirty = true;
	}
}

void Texture::SetRect(const Colour arr[], const Rect& r) {
	SetRect(arr, r.x, r.y, r.width, r.height);
}

bool Texture::IsLoaded() const
{
	return (img != nullptr);
}

bool Texture::IsRendered() const
{
	return (tex != nullptr);
}

bool Texture::IsDirty() const
{
	return dirty;
}

int Texture::GetWidth() const
{
	return width;
}

int Texture::GetHeight() const
{
	return height;
}
