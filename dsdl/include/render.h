#ifndef __RENDER_H__
#define __RENDER_H__

#include <SDL.h>
#include "utils.h"
#include "texture.h"

class Render;

class EventHandler
{
public:
	enum MouseButton
	{
		NO_TYPE,
		LEFT,
		MIDDLE,
		RIGHT,
	};

	virtual ~EventHandler() {}

	virtual bool onKeyDown(unsigned key) = 0;
	virtual bool onMouseDown(MouseButton button, int x, int y) = 0;
	virtual bool onIdleDraw(Render *) = 0;
	virtual bool onNewFrame() { return false; };
};

class Render
{
public:
	friend class Texture;

	Render(int width, int height, const char * winName = "Default Render Name");
	~Render();

	//some accessors
	int GetWidth() const;
	int GetHeight() const;
	SDL_PixelFormat* GetPixelFormat() const;

	void BeginDraw();
	void DrawRectangle(Colour col, int x, int y, int width, int height);
	void DrawRectangle(Colour col, Rect r);
	void DrawArray(const Colour arr[], int x, int y, int width, int height);
	void DrawArray(const Colour arr[], Rect r);
	void DrawTexture(const Texture* tex, int x, int y);
	void EndDraw();

	// time and frame related
	void SetFrameTime(Uint32 ft);
	float GetFPS() const;

	void StartEventLoop(EventHandler * eventHandler);
	void Quit();
private:
	// disable copy and assignment of redner objects
	Render(const Render& copy);
	Render& operator=(const Render& assign);

	void FreeResources();
	void ErrorExit();

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* mainTexture;
	SDL_PixelFormat* pixelFormat;

	int frameWidth;
	int frameHeight;
	bool quit : 1;
	Uint32 lastFrameTime;
	Uint32 frameTime;
	// for fps measurements
	Uint32 lastTime;
	Uint32 currentTime;
};

#endif // __RENDER_H__