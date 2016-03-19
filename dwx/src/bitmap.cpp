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

bool Bitmap::loadBMP(const char* filename) noexcept {
	freeMem();

	BmpHeader hd;
	BmpInfoHeader hi;
	Color palette[256];
	int toread = 0;
	unsigned char *xx;
	int rowsz;
	unsigned short sign;
	FILE* fp = fopen(filename, "rb");

	if (fp == NULL) {
		printf("loadBMP: Can't open file: `%s'\n", filename);
		return false;
	}

	if (!fread(&sign, 2, 1, fp)) return false;
	if (sign != BM_MAGIC) {
		printf("loadBMP: `%s' is not a BMP file.\n", filename);
		return false;
	}
	if (!fread(&hd, sizeof(hd), 1, fp)) return false;
	if (!fread(&hi, sizeof(hi), 1, fp)) return false;

	/* header correctness checks */
	if (!(hi.bitsperpixel == 8 || hi.bitsperpixel == 24 || hi.bitsperpixel == 32)) {
		printf("loadBMP: Cannot handle file format at %d bpp.\n", hi.bitsperpixel);
		return false;
	}
	if (hi.channels != 1) {
		printf("loadBMP: cannot load multichannel .bmp!\n");
		return false;
	}
	/* ****** header is OK *******/

	// if image is 8 bits per pixel or less (indexed mode), read some pallete data
	if (hi.bitsperpixel <= 8) {
		toread = (1 << hi.bitsperpixel);
		if (hi.colors) toread = hi.colors;
		for (int i = 0; i < toread; i++) {
			unsigned temp;
			if (!fread(&temp, 1, 4, fp)) return false;
			palette[i] = Color(temp);
		}
	}
	toread = hd.bfImgOffset - (54 + toread * 4);
	fseek(fp, toread, SEEK_CUR); // skip the rest of the header
	int k = hi.bitsperpixel / 8;
	rowsz = hi.x * k;
	if (rowsz % 4 != 0)
		rowsz = (rowsz / 4 + 1) * 4; // round the row size to the next exact multiple of 4
	xx = new unsigned char[rowsz];
	generateEmptyImage(hi.x, hi.y);
	if (!isOK()) {
		printf("loadBMP: cannot allocate memory for bitmap! Check file integrity!\n");
		delete[] xx;
		return false;
	}
	for (int j = hi.y - 1; j >= 0; j--) {// bitmaps are saved in inverted y
		if (!fread(xx, 1, rowsz, fp)) {
			printf("loadBMP: short read while opening `%s', file is probably incomplete!\n", filename);
			delete[] xx;
			return 0;
		}
		for (int i = 0; i < hi.x; i++) { // actually read the pixels
			if (hi.bitsperpixel > 8)
				setPixel(i, j, Color(xx[i*k + 2] / 255.0f, xx[i*k + 1] / 255.0f, xx[i*k] / 255.0f));
			else
				setPixel(i, j, palette[xx[i*k]]);
		}
	}
	delete[] xx;

	return true;
}

bool Bitmap::saveBMP(const char* filename) noexcept {
	FILE* fp = fopen(filename, "wb");
	if (!fp) return false;
	BmpHeader hd;
	BmpInfoHeader hi;

	// fill in the header:
	int rowsz = width * 3;
	if (rowsz % 4)
		rowsz += 4 - (rowsz % 4); // each row in of the image should be filled with zeroes to the next multiple-of-four boundary
	char * xx = new char[rowsz];
	hd.fs = rowsz * height + 54; //std image size
	hd.lzero = 0;
	hd.bfImgOffset = 54;
	hi.ihdrsize = 40;
	hi.x = width; hi.y = height;
	hi.channels = 1;
	hi.bitsperpixel = 24; //RGB format
												// set the rest of the header to default values:
	hi.compression = hi.biSizeImage = 0;
	hi.pixPerMeterX = hi.pixPerMeterY = 0;
	hi.colors = hi.colorsImportant = 0;

	fwrite(&BM_MAGIC, 2, 1, fp); // write 'BM'
	fwrite(&hd, sizeof(hd), 1, fp); // write file header
	fwrite(&hi, sizeof(hi), 1, fp); // write image header
	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < width; x++) {
			unsigned t = getPixel(x, y).toRGB32();
			xx[x * 3] = (0xff & t);
			xx[x * 3 + 1] = (0xff00 & t) >> 8;
			xx[x * 3 + 2] = (0xff0000 & t) >> 16;
		}
		fwrite(xx, rowsz, 1, fp);
	}
	fclose(fp);
	delete[] xx;
	return true;
}

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

