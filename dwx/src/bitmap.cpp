#include <memory>
#include <stdio.h>
#include "bitmap.h"
#include "color.h"
#include "constants.h"

Bitmap::Bitmap() noexcept
	: width(-1)
	, height(-1)
	, data(nullptr)
{}

Bitmap::~Bitmap() noexcept {
	freeMem();
}

void Bitmap::freeMem(void) noexcept {
	if (data) delete[] data;
	data = nullptr;
	width = height = -1;
}

void Bitmap::copy(const Bitmap& rhs) noexcept {
	freeMem();
	width = rhs.width;
	height = rhs.height;
	data = new Color[width * height];
	memcpy(data, rhs.data, width * height * sizeof(Color));
}

Bitmap::Bitmap(const Bitmap& rhs) noexcept {
	copy(rhs);
}

Bitmap& Bitmap::operator = (const Bitmap& rhs) noexcept {
	if (this != &rhs) {
		copy(rhs);
	}
	return *this;
}

int Bitmap::getWidth(void) const  noexcept {
	return width;
}
int Bitmap::getHeight(void) const noexcept {
	return height;
}
bool Bitmap::isOK(void) const  noexcept {
	return (data != nullptr);
}

void Bitmap::generateEmptyImage(int w, int h) noexcept {
	freeMem();
	if (w <= 0 || h <= 0)
		return;
	width = w;
	height = h;
	data = new Color[w * h];
	memset(data, 0, sizeof(data[0]) * w * h);
}

Color Bitmap::getPixel(int x, int y) const  noexcept {
	if (!data || x < 0 || x >= width || y < 0 || y >= height)
		return Color(0.0f, 0.0f, 0.0f);
	return data[x + y * width];
}

Color Bitmap::getFilteredPixel(float x, float y) const  noexcept {
	if (!data || !width || !height || x < 0 || x >= width || y < 0 || y >= height)
		return Color(0.0f, 0.0f, 0.0f);
	int tx = (int)floor(x);
	int ty = (int)floor(y);
	int tx_next = (tx + 1) % width;
	int ty_next = (ty + 1) % height;
	float p = x - tx;
	float q = y - ty;
	return
		data[ty      * width + tx] * ((1.0f - p) * (1.0f - q))
		+ data[ty      * width + tx_next] * (p  * (1.0f - q))
		+ data[ty_next * width + tx] * ((1.0f - p) *         q)
		+ data[ty_next * width + tx_next] * (p  *         q);
}

void Bitmap::setPixel(int x, int y, const Color& color) noexcept {
	if (!data || x < 0 || x >= width || y < 0 || y >= height)
		return;
	data[x + y * width] = color;
}

const int BM_MAGIC = 19778;

struct BmpHeader {
	int fs;       // filesize
	int lzero;
	int bfImgOffset;  // basic header size
};
struct BmpInfoHeader {
	int ihdrsize; 	// info header size
	int x, y;      	// image dimensions
	unsigned short channels;// number of planes
	unsigned short bitsperpixel;
	int compression; // 0 = no compression
	int biSizeImage; // used for compression; otherwise 0
	int pixPerMeterX, pixPerMeterY; // dots per meter
	int colors;	 // number of used colors. If all specified by the bitsize are used, then it should be 0
	int colorsImportant; // number of "important" colors (wtf?..)
};

void Bitmap::remapRGB(std::function<float(float)> remapFn) noexcept {
	for (int i = 0; i < width * height; i++) {
		data[i].r = remapFn(data[i].r);
		data[i].g = remapFn(data[i].g);
		data[i].b = remapFn(data[i].b);
	}
}

void Bitmap::decompressGamma_sRGB(void) noexcept {
	remapRGB([](float x) {
		if (x == 0) return 0.0f;
		if (x == 1) return 1.0f;
		if (x <= 0.04045f)
			return x / 12.92f;
		else
			return powf((x + 0.055f) / 1.055f, 2.4f);
	});
}

void Bitmap::decompressGamma(float gamma) noexcept {
	remapRGB([gamma](float x) {
		if (x == 0) return 0.0f;
		if (x == 1) return 1.0f;
		return powf(x, gamma);
	});
}

void Bitmap::differentiate(void) noexcept {
	Bitmap result;
	result.generateEmptyImage(width, height);

	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++) {
			float me = getPixel(x, y).intensity();
			float right = getPixel((x + 1) % width, y).intensity();
			float bottom = getPixel(x, (y + 1) % height).intensity();

			result.setPixel(x, y, Color(me - right, me - bottom, 0.0f));
		}
	(*this) = result;
}

Color * Bitmap::getDataPtr() const noexcept {
	return data;
}

Color * Bitmap::operator[](int row) noexcept {
	return data + row * width;
}

const Color * Bitmap::operator[](int row) const noexcept {
	return data + row * width;
}

