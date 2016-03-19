#include <SDL.h>
#include "render.h"
#include "texture.h"
#include "error.h"
#include "utils.h"

Render::Render(int width, int height, const char * winName)
	:
	window(nullptr),
	renderer(nullptr),
	mainTexture(nullptr),
	frameWidth(width),
	frameHeight(height),
	quit(false),
	lastFrameTime(0),
	frameTime(0),
	currentTime(0)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) ErrorExit();

	atexit(SDL_Quit);

	window = SDL_CreateWindow(
		winName,
		100,
		100,
		width,
		height,
		SDL_WINDOW_SHOWN
	);

	if(window == NULL) ErrorExit();

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE /*| SDL_RENDERER_PRESENTVSYNC*/);

	if(renderer == NULL) ErrorExit();

	mainTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

	if(mainTexture == NULL) ErrorExit();

	// init pixel format
	pixelFormat = new SDL_PixelFormat;
	pixelFormat->format = SDL_PIXELFORMAT_RGBA8888;
	pixelFormat->Rshift = 24;
	pixelFormat->Rmask = 0xff000000;
	pixelFormat->Rloss = 0;
	pixelFormat->Gshift = 16;
	pixelFormat->Gmask = 0x00ff0000;
	pixelFormat->Gloss = 0;
	pixelFormat->Bshift = 8;
	pixelFormat->Bmask = 0x0000ff00;
	pixelFormat->Bloss = 0;
	pixelFormat->Ashift = 0;
	pixelFormat->Amask = 0x000000ff;
	pixelFormat->Aloss = 0;
	pixelFormat->palette = NULL;
	pixelFormat->padding[0] = '\0';
	pixelFormat->padding[1] = '\0';
	pixelFormat->BitsPerPixel = 32;
	pixelFormat->BytesPerPixel = 4;
}

Render::~Render()
{
	FreeResources();
}

int Render::GetWidth() const
{
	return frameWidth;
}

int Render::GetHeight() const
{
	return frameHeight;
}

SDL_PixelFormat* Render::GetPixelFormat() const
{
	return pixelFormat;
}

void Render::BeginDraw()
{
	SDL_RenderClear(renderer);
}

void Render::DrawRectangle(Colour col, int destX, int destY, int width, int height)
{
	void * pixels;
	int pitch;
	Uint32 destColour = col.GetColour();
	SDL_LockTexture(mainTexture, NULL, &pixels, &pitch);
	for(int y = 0; y < height; ++y) {
		Uint32* dest = (Uint32*)((Uint8*)pixels + (destY + y) * pitch) + destX;
		for(int x = 0; x < width; ++x) {
			*dest++ = destColour;
		}
	}
	SDL_UnlockTexture(mainTexture);
	
	// update the renderer
	SDL_Rect updateRect;
	updateRect.x = destX;
	updateRect.y = destY;
	updateRect.w = width;
	updateRect.h = height;
	SDL_RenderCopy(renderer, mainTexture, &updateRect, &updateRect);
}

void Render::DrawRectangle(Colour col, Rect r)
{
	DrawRectangle(col, r.x, r.y, r.width, r.height);
}

void Render::DrawArray(const Colour arr[], int destX, int destY, int width, int height)
{
	void * pixels = nullptr;
	int pitch = 0;
	SDL_Rect updateRect;
	updateRect.x = destX;
	updateRect.y = destY;
	updateRect.w = width;
	updateRect.h = height;
	SDL_LockTexture(mainTexture, &updateRect, &pixels, &pitch);
	for(int y = 0; y < height; ++y) {
		Uint32* dest = (Uint32*)((Uint8*)pixels + y * pitch);
		for(int x = 0; x < width; ++x) {
			*dest++ = arr[y * width + x].GetColour();
		}
	}
	SDL_UnlockTexture(mainTexture);
	SDL_RenderCopy(renderer, mainTexture, &updateRect, &updateRect);
}

void Render::DrawArray(const Colour arr[], Rect r)
{
	DrawArray(arr, r.x, r.y, r.width, r.height);
}

void Render::DrawTexture(const Texture* tex, int x, int y)
{
	if(tex) {
		SDL_Rect dest;
		dest.x = x;
		dest.y = y;
		dest.w = tex->GetWidth();
		dest.h = tex->GetHeight();
		//SDL_QueryTexture(tex->GetTexture(), NULL, NULL, &dest.w, &dest.h);
		SDL_RenderCopy(renderer, tex->GetTexture(), NULL, &dest);
	}
}

void Render::EndDraw()
{
	SDL_RenderPresent(renderer);
}

float Render::GetFPS() const {
	const float delta = static_cast<float>(currentTime - lastTime);
	return 1000.0f / delta;
}

void Render::SetFrameTime(Uint32 ft) {
	frameTime = ft;
}

void Render::FreeResources()
{
	if(mainTexture) {
		SDL_DestroyTexture(mainTexture);
		mainTexture = nullptr;
	}
	if(renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
	if(window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}
	if(pixelFormat) {
		delete pixelFormat;
		pixelFormat = nullptr;
	}
}

void Render::ErrorExit()
{
	FreeResources();
	Error( SDL_GetError()).Post().Exit(1);
}

void Render::StartEventLoop(EventHandler * handler)
{
	bool quitLoop = false;
	SDL_Event evnt;
	bool refresh = true;
	while (!quitLoop) {
		// get the current time
		lastTime = currentTime;
		currentTime = SDL_GetTicks();

		while (SDL_PollEvent( &evnt)) {
			switch (evnt.type)
			{
			case SDL_QUIT:
				quitLoop = true;
				break;
			case SDL_KEYDOWN:
				if(evnt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					quitLoop = true;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				{
					EventHandler::MouseButton button = static_cast<EventHandler::MouseButton>(evnt.button.button);
					handler->onMouseDown(button, evnt.button.x, evnt.button.y);
					refresh = true;
					break;
				}
			}
		}

		if (frameTime != 0 && (currentTime - lastFrameTime > frameTime)) {
			lastFrameTime = currentTime;
			refresh |= handler->onNewFrame();
		}

		if (refresh) {
			refresh = handler->onIdleDraw(this);
		}
		quit |= this->quit;
	}
}