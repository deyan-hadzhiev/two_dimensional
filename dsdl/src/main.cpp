#include <SDL.h>
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <Windows.h>

void gammaCorrection(Colour dest[], const Colour src[], int size, bool inverse = false) {

	auto ToDouble = [](Uint8 c) -> double {
		return (static_cast<double>(c) / 255.0);
	};

	auto ToChar = [](double c) -> Uint8 {
		if (c < 0.0) {
			return 0;
		} else if (c > 1.0) {
			return  255;
		}
		return (static_cast<Uint8>(c * 255.0));
	};

	const double factor = (inverse ? 1.0 / 1.9 : 1.9);
	for (int i = 0; i < size; ++i) {
		double r = ToDouble(src[i].GetR());
		r = pow(r, factor);
		//dest[i].SetR(ToChar(r));

		double g = ToDouble(src[i].GetG());
		g = pow(g, factor);
		//dest[i].SetG(ToChar(g));

		double b = ToDouble(src[i].GetB());
		b = pow(b, factor);
		//dest[i].SetB(ToChar(b));
		dest[i] = Colour(ToChar(r), ToChar(g), ToChar(b));
	}
}

class NopHandler : public EventHandler {
private:

	int x, y, dx, dy, w, h;
	bool texInit;
	Colour * mainArr;
	Texture * tex;
	Texture * texInverse;
	Texture * texInverseGamma;
	Texture * texGamma;
	Texture * texGammaInverse;

	Texture * grayScale[5];

	void initTextures(int dimSize, Render * r) {
		const int arrSize = dimSize * dimSize;
		mainArr = new Colour[arrSize];
		const Uint8 b = 127;
		for (int y = 0; y < dimSize; ++y) {
			const Uint8 g = (y * 256) / dimSize;
			for (int x = 0; x < dimSize; ++x) {
				const Uint8 r = (x * 256) / dimSize;
				mainArr[y * dimSize + x] = Colour(r, g, b);
			}
		}

		auto PrepTexture = [](const Colour arr[], int w, int h, Render * render) -> Texture* {
			Texture * tmp = new Texture(w, h, render->GetPixelFormat());
			tmp->SetRect(arr, 0, 0, w, h);
			tmp->RenderTexture(render);
			return tmp;
		};

		tex = PrepTexture(mainArr, dimSize, dimSize, r);

		// inverse -> gamma
		Colour * inverse = new Colour[arrSize];
		gammaCorrection(inverse, mainArr, arrSize, true);
		texInverse = PrepTexture(inverse, dimSize, dimSize, r);

		Colour * inverseGamma = new Colour[arrSize];
		gammaCorrection(inverseGamma, inverse, arrSize, false);
		texInverseGamma = PrepTexture(inverseGamma, dimSize, dimSize, r);

		delete[] inverse;
		delete[] inverseGamma;

		// gamma -> inverse
		Colour * gamma = new Colour[arrSize];
		gammaCorrection(gamma, mainArr, arrSize, false);
		texGamma = PrepTexture(gamma, dimSize, dimSize, r);

		Colour * gammaInverse = new Colour[arrSize];
		gammaCorrection(gammaInverse, gamma, arrSize, true);
		texGammaInverse = PrepTexture(gammaInverse, dimSize, dimSize, r);

		delete[] gamma;
		delete[] gammaInverse;

		//delete[] mainArr;

		// gray scaled test
		const int grayHeight = 64;
		const int grayArrSize = (dimSize + 2) * (grayHeight + 2);
		Colour * grayArrs[5];
		for (int i = 0; i < 5; ++i) {
			grayArrs[i] = new Colour[grayArrSize];
		}

		for (int i = 0; i < grayArrSize; ++i) {
			grayArrs[0][i] = Colour(127, 127, 127);
		}

		for (int x = 0; x < dimSize; ++x) {
			const Uint8 col = (x * 256) / dimSize;
			for (int y = 0; y < 64; ++y) {
				grayArrs[0][(y + 1) * (dimSize + 2) + x + 1] = Colour(col, col, col);
			}
		}

		gammaCorrection(grayArrs[1], grayArrs[0], grayArrSize, true);
		gammaCorrection(grayArrs[2], grayArrs[1], grayArrSize, false);
		gammaCorrection(grayArrs[3], grayArrs[0], grayArrSize, false);
		gammaCorrection(grayArrs[4], grayArrs[3], grayArrSize, true);

		for (int i = 0; i < 5; ++i) {
			grayScale[i] = PrepTexture(grayArrs[i], dimSize + 2, grayHeight + 2, r);
			delete[] grayArrs[i];
		}
	}
public:
	NopHandler()
		:
		texInit(false),
		tex(nullptr),
		texInverse(nullptr),
		texInverseGamma(nullptr),
		texGamma(nullptr),
		texGammaInverse(nullptr)
	{}
	virtual ~NopHandler() {
		delete tex;
		delete texInverse;
		delete texInverseGamma;
		delete texGamma;
		delete texGammaInverse;

		delete[] mainArr;

		for (int i = 0; i < 5; ++i) {
			delete grayScale[i];
		}
	}

	bool onKeyDown(unsigned) override { return true; }
	bool onMouseDown(MouseButton, int, int) override { return true; }

	bool onIdleDraw(Render * render) override {
		static const int dimSize = 256;
		static const int grayScaleHeight = 64;
		static const int offset = 40;

		if (!texInit) {
			render->BeginDraw();
			initTextures(dimSize, render);
			texInit = true;
			x = y = 0;
			dx = dy = 1;
			w = render->GetWidth();
			h = render->GetHeight();
		}

#if 1
		render->DrawTexture(tex, offset, offset);
		render->DrawTexture(texInverse, offset * 2 + dimSize, offset);
		render->DrawTexture(texInverseGamma, offset * 3 + dimSize * 2, offset);

		render->DrawTexture(tex, offset, offset * 2 + dimSize);
		render->DrawTexture(texGamma, offset * 2 + dimSize, offset * 2 + dimSize);
		render->DrawTexture(texGammaInverse, offset * 3 + dimSize * 2, offset * 2 + dimSize);

		const int grayOffset = (offset + dimSize) * 2;

		render->DrawTexture(grayScale[0], offset, grayOffset + offset);
		render->DrawTexture(grayScale[1], offset * 2 + dimSize, grayOffset + offset);
		render->DrawTexture(grayScale[2], offset * 3 + dimSize * 2, grayOffset + offset);

		render->DrawTexture(grayScale[0], offset, grayOffset + offset * 2 + grayScaleHeight);
		render->DrawTexture(grayScale[3], offset * 2 + dimSize, grayOffset + offset * 2 + grayScaleHeight);
		render->DrawTexture(grayScale[4], offset * 3 + dimSize * 2, grayOffset + offset * 2 + grayScaleHeight);

#endif

		//render->BeginDraw();
#if 0
		render->DrawTexture(tex, x, y);
#endif
		//render->DrawArray(mainArr, x, y, dimSize, dimSize);

		render->EndDraw();

		return true;
	}

	virtual bool onNewFrame() {
		if (!texInit)
			return false;

		if (x + dx < 0 || x + dx + tex->GetWidth() > w) {
			dx *= -1;
		}
		if (y + dy < 0 || y + dy + tex->GetHeight() > h) {
			dy *= -1;
		}
		x += dx;
		y += dy;

		return true;
	}
};

void testRender() {
	Render rend(928, 840, "Test render");
	rend.SetFrameTime(5);
	NopHandler * handler = new NopHandler;
	rend.StartEventLoop(handler);
	delete handler;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	testRender();
	return 0;
}
