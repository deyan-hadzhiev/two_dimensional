/***************************************************************************
*   Copyright (C) 2016 by Deyan Hadzhiev                                  *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <memory>
#include <stdio.h>
#include <algorithm>
#include "bitmap.h"
#include "color.h"
#include "constants.h"
#include "ascii_table.h"

FloatBitmap::FloatBitmap() noexcept
	: width(-1)
	, height(-1)
	, data(nullptr)
{}

FloatBitmap::~FloatBitmap() noexcept {
	freeMem();
}

void FloatBitmap::freeMem(void) noexcept {
	if (data) delete[] data;
	data = nullptr;
	width = height = -1;
}

void FloatBitmap::copy(const FloatBitmap& rhs) noexcept {
	freeMem();
	width = rhs.width;
	height = rhs.height;
	data = new FloatColor[width * height];
	memcpy(data, rhs.data, width * height * sizeof(FloatColor));
}

FloatBitmap::FloatBitmap(const FloatBitmap& rhs) noexcept
	: width(-1)
	, height(-1)
	, data(nullptr)
{
	copy(rhs);
}

FloatBitmap& FloatBitmap::operator = (const FloatBitmap& rhs) noexcept {
	if (this != &rhs) {
		copy(rhs);
	}
	return *this;
}

int FloatBitmap::getWidth(void) const  noexcept {
	return width;
}
int FloatBitmap::getHeight(void) const noexcept {
	return height;
}
bool FloatBitmap::isOK(void) const  noexcept {
	return (data != nullptr);
}

void FloatBitmap::generateEmptyImage(int w, int h) noexcept {
	freeMem();
	if (w <= 0 || h <= 0)
		return;
	width = w;
	height = h;
	data = new FloatColor[w * h];
	memset(data, 0, sizeof(data[0]) * w * h);
}

FloatColor FloatBitmap::getPixel(int x, int y) const  noexcept {
	if (!data || x < 0 || x >= width || y < 0 || y >= height)
		return FloatColor(0.0f, 0.0f, 0.0f);
	return data[x + y * width];
}

FloatColor FloatBitmap::getFilteredPixel(float x, float y) const noexcept {
	if (!data || !width || !height || x < 0 || x >= width || y < 0 || y >= height)
		return FloatColor(0.0f, 0.0f, 0.0f);
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

void FloatBitmap::setPixel(int x, int y, const FloatColor& color) noexcept {
	if (!data || x < 0 || x >= width || y < 0 || y >= height)
		return;
	data[x + y * width] = color;
}

void FloatBitmap::remapRGB(std::function<float(float)> remapFn) noexcept {
	for (int i = 0; i < width * height; i++) {
		data[i].r = remapFn(data[i].r);
		data[i].g = remapFn(data[i].g);
		data[i].b = remapFn(data[i].b);
	}
}

void FloatBitmap::decompressGamma_sRGB(void) noexcept {
	remapRGB([](float x) {
		if (x == 0) return 0.0f;
		if (x == 1) return 1.0f;
		if (x <= 0.04045f)
			return x / 12.92f;
		else
			return powf((x + 0.055f) / 1.055f, 2.4f);
	});
}

void FloatBitmap::decompressGamma(float gamma) noexcept {
	remapRGB([gamma](float x) {
		if (x == 0) return 0.0f;
		if (x == 1) return 1.0f;
		return powf(x, gamma);
	});
}

void FloatBitmap::differentiate(void) noexcept {
	FloatBitmap result;
	result.generateEmptyImage(width, height);

	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++) {
			float me = getPixel(x, y).intensity();
			float right = getPixel((x + 1) % width, y).intensity();
			float bottom = getPixel(x, (y + 1) % height).intensity();

			result.setPixel(x, y, FloatColor(me - right, me - bottom, 0.0f));
		}
	(*this) = result;
}

FloatColor * FloatBitmap::getDataPtr() const noexcept {
	return data;
}

FloatColor * FloatBitmap::operator[](int row) noexcept {
	return data + row * width;
}

const FloatColor * FloatBitmap::operator[](int row) const noexcept {
	return data + row * width;
}

// CoeffCache - used for downscaling

struct CoeffCacheElem {
	int src, dest;
	float * coefficients;
	CoeffCacheElem * next;
};

class CoeffCache {
	CoeffCacheElem * root;
public:
	CoeffCache() {
		root = nullptr;
	}
	~CoeffCache() {
		CoeffCacheElem * next = nullptr;
		for (CoeffCacheElem * e = root; e != nullptr; e = next) {
			next = e->next;
			delete[] e->coefficients;
			delete e;
		}
	}

	// will return the scaling coefficients with destLen to normalize the resized array properly
	const float * getCoefficients(const int srcLen, const int destLen) {
		CoeffCacheElem * e = root;
		while (e) {
			if (e->src == srcLen && e->dest == destLen)
				return e->coefficients;
			e = e->next;
		}
		CoeffCacheElem * newElement = new CoeffCacheElem;
		newElement->src = srcLen;
		newElement->dest = destLen;
		newElement->coefficients = new float[destLen];
		memset(newElement->coefficients, 0, sizeof(float) * destLen);
		float ratio = (destLen - 1) / ((float)srcLen - 1);
		for (int i = 0; i < srcLen; i++) {
			float x = i * ratio;
			int xx = int(x);
			float mul1 = 1.0f - (x - xx);
			newElement->coefficients[xx] += mul1;
			if (xx + 1 < destLen)
				newElement->coefficients[xx + 1] += 1.0f - mul1;
		}
		for (int i = 0; i < destLen; i++)
			newElement->coefficients[i] = 1.0f / newElement->coefficients[i];

		newElement->next = root;
		root = newElement;
		return newElement->coefficients;
	}
};

static CoeffCache coeffsCache;

// Pixelmap<T>

template class TColor<uint16>;
template class TColor<int32>;
template class TColor<Complex>;

template Color::Color(const TColor<uint16>&);

// instantiate scalar pixel maps
template class Pixelmap<uint32>;
template class Pixelmap<uint64>;
template class Pixelmap<Complex>;

// instantiate color type pixel maps
template class Pixelmap<Color>;
template class Pixelmap<TColor<uint16> >;
template class Pixelmap<TColor<int32> >;
template class Pixelmap<TColor<Complex> >;
template class Pixelmap<TColor<double> >;

// conversion Color <-> TColor<int32>
template Pixelmap<TColor<int32> >::Pixelmap(const Pixelmap<Color>&);
template Pixelmap<Color>::Pixelmap(const Pixelmap<TColor<int32> >&);

// conversion Color <-> TColor<Complex>
template Pixelmap<TColor<Complex> >::Pixelmap(const Pixelmap<Color>&);
template Pixelmap<Color>::Pixelmap(const Pixelmap<TColor<Complex> >&);

// conversion Color <-> TColor<double>
template Pixelmap<TColor<double> >::Pixelmap(const Pixelmap<Color>&);
template Pixelmap<Color>::Pixelmap(const Pixelmap<TColor<double> >&);

template bool Pixelmap<Color>::getChannel<uint8>(uint8 *, ColorChannel) const;
template bool Pixelmap<TColor<Complex> >::getChannel<Complex>(Complex *, ColorChannel) const;

template bool Pixelmap<Color>::setChannel<uint8>(const uint8 *, ColorChannel);
template bool Pixelmap<TColor<Complex> >::setChannel<Complex>(const Complex *, ColorChannel);

template bool Pixelmap<Color>::downscale<TColor<uint16> >(Pixelmap<Color>&, const int, const int) const;
template bool Pixelmap<Color>::downscale<Color>(Pixelmap<Color>&, const int, const int) const;
template bool Pixelmap<Color>::downscale<TColor<float> >(Pixelmap<Color>&, const int, const int) const;
template bool Pixelmap<Color>::downscale<TColor<double> >(Pixelmap<Color>&, const int, const int) const;

template bool Pixelmap<Color>::upscale<TColor<double> >(Pixelmap<Color>&, const int, const int, UpscaleFiltering filterType) const;

template<class ColorType>
Pixelmap<ColorType>::Pixelmap() noexcept
	: width(-1)
	, height(-1)
	, data(nullptr)
{}

template<class ColorType>
Pixelmap<ColorType>::Pixelmap(int width, int height, const ColorType * pixelValues) noexcept
	: width(width)
	, height(height)
	, data(nullptr)
{
	// clear the output image only if there is no input buffer specified
	generateEmptyImage(width, height, nullptr != pixelValues);
	if (pixelValues) {
		memcpy(data, pixelValues, width * height * sizeof(ColorType));
	}
}

template<class ColorType>
Pixelmap<ColorType>::~Pixelmap() noexcept {
	freeMem();
}

template<class ColorType>
void Pixelmap<ColorType>::freeMem(void) noexcept {
	if (data) delete[] data;
	data = nullptr;
	width = height = -1;
}

template<class ColorType>
inline bool Pixelmap<ColorType>::remapCoord(float& c, int bound, EdgeFillType edge) noexcept {
	if (c < 0.0f || static_cast<int>(c) >= bound) {
		if (EFT_BLANK == edge) {
			return false;
		} else if (EFT_TILE == edge) {
			c = c - static_cast<float>((static_cast<int>(c) / bound - (c < 0.0f ? 1 : 0)) * bound);
			if (c >= bound) {
				c = bound - 0.05f;
			}
		} else if (EFT_STRETCH == edge) {
			c = (c < 0.0f ? 0.0f : (c >= static_cast<float>(bound) ? bound - 1 : c));
		} else if (EFT_MIRROR == edge) {
			// make absolute
			c = fabs(c);
			const int dBound = bound * 2;
			// tile to the bound * domain
			if (c >= dBound) {
				c = c - static_cast<float>((static_cast<int>(c) / dBound) * dBound);
			}
			// calculate mirrored position
			if (c >= bound) {
				c = (dBound - c);
			}
		} else {
			return false;
		}
	}
	return true;
}

template<class ColorType>
void Pixelmap<ColorType>::copy(const Pixelmap<ColorType>& rhs) noexcept {
	// free memory only if necessary
	if (width * height != rhs.width * rhs.height) {
		freeMem();
		data = new ColorType[rhs.width * rhs.height];
	}
	width = rhs.width;
	height = rhs.height;
	memcpy(data, rhs.data, width * height * sizeof(ColorType));
}

template<class ColorType>
Pixelmap<ColorType>::Pixelmap(const Pixelmap& rhs) noexcept
	: width(-1)
	, height(-1)
	, data(nullptr)
{
	copy(rhs);
}

template<class ColorType>
Pixelmap<ColorType>& Pixelmap<ColorType>::operator = (const Pixelmap<ColorType>& rhs) noexcept {
	if (this != &rhs) {
		copy(rhs);
	}
	return *this;
}

template<class ColorType>
template<class OtherColorType>
Pixelmap<ColorType>::Pixelmap(const Pixelmap<OtherColorType>& rhs)
	: width(rhs.getWidth())
	, height(rhs.getHeight())
	, data(nullptr)
{
	generateEmptyImage(width, height);
	const int n = width * height;
	const OtherColorType * rhsData = rhs.getDataPtr();
	for (int i = 0; i < n; ++i) {
		data[i] = static_cast<ColorType>(rhsData[i]);
	}
}

template<class ColorType>
int Pixelmap<ColorType>::getWidth(void) const noexcept {
	return width;
}

template<class ColorType>
int Pixelmap<ColorType>::getHeight(void) const noexcept {
	return height;
}

template<class ColorType>
int Pixelmap<ColorType>::getDimensionProduct() const noexcept {
	return width * height;
}

template<class ColorType>
bool Pixelmap<ColorType>::validPixelPosition(int x, int y) const noexcept {
	return (x >= 0 && x < width && y >= 0 && y < height);
}

template<class ColorType>
bool Pixelmap<ColorType>::isOK(void) const  noexcept {
	return (data != nullptr);
}

template<class ColorType>
void Pixelmap<ColorType>::generateEmptyImage(int w, int h, bool clear) noexcept {
	if (w <= 0 || h <= 0)
		return;
	// free memory only if necessary
	if ((width * height != w * h) || nullptr == data) {
		freeMem();
		data = new ColorType[w * h];
	}
	width = w;
	height = h;
	if (clear) {
		memset(data, 0, sizeof(ColorType) * w * h);
	}
}

template<class ColorType>
void Pixelmap<ColorType>::fill(ColorType c, int x, int y, int _width, int _height) {
	// check if the filled area is completely outside this bitmap
	if (!this->isOK() || (_width != -1 && x + _width < 0) || (_height != -1 && y + _height < 0) || x >= width || y >= height)
		return;
	// TODO - maybe move this sub rect calculations to separate function?
	const int dx = (x < 0 ? 0 : x);
	const int dy = (y < 0 ? 0 : y);
	const int dw = (_width == -1 ?
		(x < 0 ? width : width - x) :
		(x < 0 ? std::min(x + _width, width) : std::min(width - x, _width)));
	const int dh = (_height == -1 ?
		(y < 0 ? height : height - y) :
		(y < 0 ? std::min(y + _height, height) : std::min(height - y, _height)));

	ColorType * firstRow = (data + dy * width + dx);
	// fill the first row
	for (int xx = 0; xx < dw; ++xx) {
		firstRow[xx] = c;
	}
	const int rowSize = dw * sizeof(ColorType);
	// start the copy from the next row
	for (int yy = 1; yy < dh; ++yy) {
		ColorType * destRow = data + (dy + yy) * width + dx;
		memcpy(destRow, firstRow, rowSize);
	}
}

template<class ColorType>
ColorType Pixelmap<ColorType>::getPixel(int x, int y) const  noexcept {
	if (!data || x < 0 || x >= width || y < 0 || y >= height)
		return ColorType();
	return data[x + y * width];
}

template<class ColorType>
template<class IntermediateColorType>
ColorType Pixelmap<ColorType>::getBilinearFilteredPixel(float x, float y, EdgeFillType edge) const noexcept {
	if (!data || !width || !height)
		return ColorType();
	if (!remapCoord(x, width, edge) || !remapCoord(y, height, edge)) {
		return ColorType();
	}
	float xn = x + 1.0f;
	float yn = y + 1.0f;
	if (!remapCoord(xn, width, edge)) {
		xn = x;
	}
	if (!remapCoord(yn, height, edge)) {
		yn = y;
	}
	const int tx = static_cast<int>(floor(x));
	const int ty = static_cast<int>(floor(y));
	const int txNext = static_cast<int>(floor(xn));
	const int tyNext = static_cast<int>(floor(yn));
	const double p = x - tx;
	const double q = y - ty;
	return static_cast<ColorType>(
		  static_cast<IntermediateColorType>(data[ty     * width + tx]     * ((1.0 - p) * (1.0 - q)))
		+ static_cast<IntermediateColorType>(data[ty     * width + txNext] * (p         * (1.0 - q)))
		+ static_cast<IntermediateColorType>(data[tyNext * width + tx]     * ((1.0 - p) *         q))
		+ static_cast<IntermediateColorType>(data[tyNext * width + txNext] * (p         *         q)));
}

template<class ColorType>
template<class IntermediateColorType>
ColorType Pixelmap<ColorType>::getBicubicFilteredPixel(float x, float y, EdgeFillType edge) const noexcept {
	if (!isOK() || 0 == width || 0 == height)
		return ColorType();
	if (!remapCoord(x, width, edge) || !remapCoord(y, height, edge)) {
		return ColorType();
	}
	float xReal[3] = { x - 1.0f, x + 1.0f, x + 2.0f };
	float yReal[3] = { y - 1.0f, y + 1.0f, y + 2.0f };
	for (int i = 0; i < 3; ++i) {
		if (!remapCoord(xReal[i], width, edge)) {
			xReal[i] = x;
		}
		if (!remapCoord(yReal[i], height, edge)) {
			yReal[i] = y;
		}
	}
	const int xCoords[4] = {
		static_cast<int>(floor(xReal[0])),
		static_cast<int>(floor(x)),
		static_cast<int>(floor(xReal[1])),
		static_cast<int>(floor(xReal[2])),
	};
	const int yCoords[4] = {
		static_cast<int>(floor(yReal[0])),
		static_cast<int>(floor(y)),
		static_cast<int>(floor(yReal[1])),
		static_cast<int>(floor(yReal[2])),
	};
	const double p = x - xCoords[1];
	const double q = y - yCoords[1];
	IntermediateColorType horizontal[4];
	for (int i = 0; i < 4; ++i) {
		horizontal[i] = cubicInterpolate<IntermediateColorType>(
			static_cast<IntermediateColorType>(data[yCoords[i] * width + xCoords[0]]),
			static_cast<IntermediateColorType>(data[yCoords[i] * width + xCoords[1]]),
			static_cast<IntermediateColorType>(data[yCoords[i] * width + xCoords[2]]),
			static_cast<IntermediateColorType>(data[yCoords[i] * width + xCoords[3]]),
			p
		);
	}
	return static_cast<ColorType>(cubicInterpolate<IntermediateColorType>(
		horizontal[0],
		horizontal[1],
		horizontal[2],
		horizontal[3],
		q
	));
}

template<class ColorType>
void Pixelmap<ColorType>::setPixel(int x, int y, const ColorType& color) noexcept {
	if (!data || x < 0 || x >= width || y < 0 || y >= height)
		return;
	data[x + y * width] = color;
}

template<class ColorType>
void Pixelmap<ColorType>::remap(std::function<ColorType(ColorType)> remapFn) noexcept {
	for (int i = 0; i < width * height; i++) {
		data[i] = remapFn(data[i]);
	}
}

template<class ColorType>
ColorType * Pixelmap<ColorType>::getDataPtr() const noexcept {
	return data;
}

template<class ColorType>
ColorType * Pixelmap<ColorType>::operator[](int row) noexcept {
	return data + row * width;
}

template<class ColorType>
const ColorType * Pixelmap<ColorType>::operator[](int row) const noexcept {
	return data + row * width;
}

template<class ColorType>
bool Pixelmap<ColorType>::mirror(PixelmapAxis axis) {
	if (!this->isOK()) {
		return false;
	} else if (PA_NONE == axis) {
		return true;
	}
	if ((axis & PA_Y_AXIS) != 0) {
		std::unique_ptr<ColorType[]> tmp(new ColorType[width]);
		const int halfHeight = height / 2;
		const int rowSize = width * sizeof(ColorType);
		for (int y = 0, yy = height - 1; y < halfHeight; ++y, --yy) {
			memcpy(tmp.get(), data + y * width, rowSize);
			memcpy(data + y * width, data + yy * width, rowSize);
			memcpy(data + yy * width, tmp.get(), rowSize);
		}
	}
	if ((axis & PA_X_AXIS) != 0) {
		const int halfWidth = width / 2;
		for (int y = 0; y < height; ++y) {
			ColorType * rowData = data + y * width;
			for (int x = 0, xx = width - 1; x < halfWidth; ++x, --xx) {
				std::swap(rowData[x], rowData[xx]);
			}
		}
	}
	return true;
}

template<class ColorType>
bool Pixelmap<ColorType>::mirror(Pixelmap<ColorType>& mirrored, PixelmapAxis axis) const {
	if (!this->isOK() || this == &mirrored) {
		return false;
	}
	mirrored = *this;
	return mirrored.mirror(axis);
}

template<class ColorType>
bool Pixelmap<ColorType>::crop(const int x, const int y, const int w, const int h) {
	if (!this->isOK() || x < 0 || y < 0 || x + w > width || y + h > height) {
		return false;
	}
	// create the data for the cropped image
	ColorType * croppedData = new ColorType[w * h];
	// now copy over the data
	for (int cy = 0; cy < h; ++cy) {
		// copy whole row at a time
		ColorType * dest = croppedData + cy * w;
		const ColorType * src = data + (y + cy) * width + x;
		memcpy(dest, src, w * sizeof(ColorType));
	}
	// use freeMem() since it may change in the future
	freeMem();
	// now just set the data with the new cropped data
	data = croppedData;
	width = w;
	height = h;
	return true;
}

template<class ColorType>
bool Pixelmap<ColorType>::crop(Pixelmap<ColorType>& cropped, const int x, const int y, const int w, const int h) const {
	if (!this->isOK() || this == &cropped || x < 0 || y < 0 || x + w > width || y + h > height) {
		return false;
	}
	// create the data for the cropped image
	cropped.generateEmptyImage(w, h);
	ColorType * croppedData = cropped.getDataPtr();
	// now copy over the data
	for (int cy = 0; cy < h; ++cy) {
		// copy whole row at a time
		ColorType * dest = croppedData + cy * w;
		const ColorType * src = data + (y + cy) * width + x;
		memcpy(dest, src, w * sizeof(ColorType));
	}
	return true;
}

template<class ColorType>
bool Pixelmap<ColorType>::expand(const int w, const int h, const int x, const int y, EdgeFillType fillType) {
	if (!this->isOK() || w < 0 || h < 0) {
		return false;
	} else if (0 == w && 0 == h) {
		return true;
	}
	Pixelmap<ColorType> current(*this);
	const int oldWidth = width;
	const int oldHeight = height;
	generateEmptyImage(width + w, height + h); // this will also change width and height
	// draw the current pixelmap according to the new x and y coords
	drawBitmap(current, x, y);
	if (EFT_STRETCH == fillType) {
		// fill the space left of the old image
		const int dx = (x < 0 ? 0 : x);
		const int dy = (y < 0 ? 0 : y);
		const int sx = (x < 0 ? -x : 0);
		const int sy = (y < 0 ? -y : 0);
		// volatiles are necessary because of some msvc optimizations bug -.-
		volatile const int dw = (x < 0 ? std::min(oldWidth + x, width) : std::min(width - x, oldWidth));
		volatile const int dh = (y < 0 ? std::min(oldHeight + y, height) : std::min(height - y, oldHeight));
		// buffers that will be used later for the vertical stretch
		std::unique_ptr<ColorType[]> topRow(new ColorType[width]);
		std::unique_ptr<ColorType[]> bottomRow(new ColorType[width]);
		const ColorType * currentData = current.getDataPtr();
		// apparently these may go out of range
		if (sx < oldWidth && dw > 0) {
			// copy the first and last row to the row buffers
			memcpy(topRow.get() + dx, currentData + sx, dw * sizeof(ColorType));
			memcpy(bottomRow.get() + dx, currentData + (oldHeight - 1) * oldWidth + sx, dw * sizeof(ColorType));
		}
		// fill the left part of the image
		if (x > 0) {
			const int ddw = std::min(x, width);
			// fill the top and bottom row
			const ColorType topFillValue = current.getPixel(0, 0);
			const ColorType bottomFillValue = current.getPixel(0, oldHeight - 1);
			for (int xx = 0; xx < ddw; ++xx) {
				topRow[xx] = topFillValue;
				bottomRow[xx] = bottomFillValue;
			}
			// now fill the actual data
			for (int yy = 0; yy < dh; ++yy) {
				const ColorType fillValue = current.getPixel(0, sy + yy);
				// fill the whole data array
				ColorType * dest = data + (dy + yy) * width;
				for (int xx = 0; xx < ddw; ++xx) {
					dest[xx] = fillValue;
				}
			}
		}
		// fill the right part
		if (x + oldWidth < width) {
			const int ddx = (x + oldWidth > 0 ? x + oldWidth : 0);
			const int ddw = (x + oldWidth > 0 ? width - x - oldWidth : width);
			// fill the top and bottom row
			const ColorType topFillValue = current.getPixel(oldWidth - 1, 0);
			const ColorType bottomFillValue = current.getPixel(oldWidth -1, oldHeight - 1);
			for (int xx = 0; xx < ddw; ++xx) {
				topRow[xx + ddx] = topFillValue;
				bottomRow[xx + ddx] = bottomFillValue;
			}
			// now fill the actual data
			for (int yy = 0; yy < dh; ++yy) {
				const ColorType fillValue = current.getPixel(oldWidth - 1, sy + yy);
				// fill the whole data array
				ColorType * dest = data + (dy + yy) * width + ddx;
				for (int xx = 0; xx < ddw; ++xx) {
					dest[xx] = fillValue;
				}
			}
		}
		// fill the top
		if (y > 0) {
			const int ddh = std::min(y, height);
			for (int yy = 0; yy < ddh; ++yy) {
				ColorType * dest = data + yy * width;
				memcpy(dest, topRow.get(), width * sizeof(ColorType));
			}
		}
		// fill the bottom
		if (y + oldHeight < height) {
			const int ddy = (y + oldHeight > 0 ? y + oldHeight : 0);
			const int ddh = (y + oldHeight > 0 ? height - y - oldHeight : height);
			for (int yy = 0; yy < ddh; ++yy) {
				ColorType * dest = data + (ddy + yy) * width;
				memcpy(dest, bottomRow.get(), width * sizeof(ColorType));
			}
		}
	} else if (EFT_TILE == fillType || EFT_MIRROR == fillType) {
		const int firstX = (x == 0 ? 0 : (x % oldWidth) - (x < 0 ? 0 : oldWidth));
		const int firstY = (y == 0 ? 0 : (y % oldHeight) - (y < 0 ? 0 : oldHeight));

		const int baseWidthCount = width / oldWidth;
		const int allignWidthCount = (x % oldWidth != 0 ? 1 : 0);
		const int endAllignWidthCount = ((firstX + (baseWidthCount + allignWidthCount) * oldWidth) < width ? 1 : 0);
		const int fillWidthCount = baseWidthCount + allignWidthCount + endAllignWidthCount;
		const int baseHeightCount = height / oldHeight;
		const int allignHeightCount = (y % oldHeight != 0 ? 1 : 0);
		const int endAllignHeightCount = ((firstY + (baseHeightCount + allignHeightCount) * oldHeight) < height ? 1 : 0);
		const int fillHeightCount = baseHeightCount + allignHeightCount + endAllignHeightCount;
		// will use such an array to loop it for drawing
		using PixelmapDraw = std::pair<Point, Pixelmap<ColorType>* >;
		const int fullCount = fillWidthCount * fillHeightCount;
		std::unique_ptr<PixelmapDraw[]> drawArray(new PixelmapDraw[fullCount]);
		for (int yy = 0; yy < fillHeightCount; ++yy) {
			for (int xx = 0; xx < fillWidthCount; ++xx) {
				drawArray[yy * fillWidthCount + xx].first = Point(firstX + xx * oldWidth, firstY + yy * oldHeight);
			}
		}
		// we already have the id image so we need one less than the count
		// note: these will be allocated only if the fill type is mirror
		// but have to be declared here to use the RAII scope of the unique_ptr<>
		std::unique_ptr<Pixelmap<ColorType> > mirrors[PA_COUNT - 1];
		// this is for convenience during assignment
		if (EFT_TILE == fillType) {
			for (int i = 0; i < fullCount; ++i) {
				drawArray[i].second = &current;
			}
		} else {
			for (int i = 0; i < PA_COUNT - 1; ++i) {
				mirrors[i].reset(new Pixelmap<ColorType>(current));
				mirrors[i]->mirror(static_cast<PixelmapAxis>(i + 1));
			}
			Pixelmap<ColorType> * mirrorPointers[PA_COUNT] = {
				&current,
				mirrors[0].get(),
				mirrors[1].get(),
				mirrors[2].get()
			};
			const bool firstXOdd = (((x - firstX) / oldWidth) % 2) != 0;
			const bool firstYOdd = (((y - firstY) / oldHeight) % 2) != 0;
			const unsigned firstAxis = (firstYOdd ? PA_Y_AXIS : PA_NONE) | (firstXOdd ? PA_X_AXIS : PA_NONE);
			for (int yy = 0; yy < fillHeightCount; ++yy) {
				for (int xx = 0; xx < fillWidthCount; ++xx) {
					const unsigned axis =
						((xx & 1 ? ~firstAxis : firstAxis) & PA_X_AXIS) |
						((yy & 1 ? ~firstAxis : firstAxis) & PA_Y_AXIS);
					drawArray[yy * fillWidthCount + xx].second = mirrorPointers[axis];
				}
			}
		}
		for (int i = 0; i < fullCount; ++i) {
			const PixelmapDraw& drawn = drawArray[i];
			drawBitmap(*(drawn.second), drawn.first.x, drawn.first.y);
		}
	}
	return true;
}

template<class ColorType>
bool Pixelmap<ColorType>::expand(Pixelmap<ColorType>& expanded, const int w, const int h, const int x, const int y, EdgeFillType fillType) const {
	if (!this->isOK() || this == &expanded || w < 0 || h < 0) {
		return false;
	}
	expanded = *this;
	return expanded.expand(w, h, x, y, fillType);
}

template<class ColorType>
bool Pixelmap<ColorType>::relocate(const int nx, const int ny) {
	if (!this->isOK() || nx < 0 || nx >= width || ny < 0 || ny >= height) {
		return false;
	}
	Pixelmap<ColorType> tmp(*this);
	return tmp.relocate(*this, nx, ny);
}

template<class ColorType>
bool Pixelmap<ColorType>::relocate(Pixelmap<ColorType>& relocated, const int nx, const int ny) const {
	if (!this->isOK() || this == &relocated || nx < 0 || nx >= width || ny < 0 || ny >= height) {
		return false;
	}
	relocated.generateEmptyImage(width, height);
	ColorType * relocatedData = relocated.getDataPtr();

	const int rwidth = width - nx;
	const int bheight = height - ny;
	if (nx != 0) {
		for (int y = 0; y < height; ++y) {
			const int yDest = (y + ny) % height;
			const ColorType * src = data + y * width;
			ColorType * lDest = relocatedData + yDest * width;
			ColorType * dest = lDest + nx;
			memcpy(dest, src, rwidth * sizeof(ColorType));
			// now the left part
			const ColorType * lsrc = src + rwidth;
			memcpy(lDest, lsrc, nx * sizeof(ColorType));
		}
	} else {
		// first copy the first ny rows
		const int size = ny * width;
		const int rsize = bheight * width;
		ColorType * dest = relocatedData + rsize; // bheight * width
		memcpy(dest, data, size * sizeof(ColorType));
		// then copy the remiaining
		const ColorType * src = data + size; // ny * width
		memcpy(relocatedData, src, rsize * sizeof(ColorType));
	}

	return true;
}

template<class ColorType>
template<class ScalarType>
bool Pixelmap<ColorType>::getChannel(ScalarType * channel, ColorChannel cc) const {
	if (!this->isOK() || !channel) {
		return false;
	}

	const int dim = getDimensionProduct();
	for (int i = 0; i < dim; ++i) {
		channel[i] = data[i][cc];
	}
	return true;
}

template<class ColorType>
template<class ChannelScalar>
bool Pixelmap<ColorType>::setChannel(const ChannelScalar * channel, ColorChannel cc) {
	if (!this->isOK() || !channel) {
		return false;
	}

	const int dim = getDimensionProduct();
	for (int i = 0; i < dim; ++i) {
		data[i][cc] = channel[i];
	}
	return true;
}

template<class ColorType>
template<class IntermediateColorType>
bool Pixelmap<ColorType>::downscale(Pixelmap<ColorType>& downScaled, const int downWidth, const int downHeight) const {
	if (!this->isOK() || this == &downScaled || downWidth <= 0 || downWidth > width || downHeight <= 0 || downHeight > height) {
		return false;
	} else if (downWidth == width && downHeight == height) {
		downScaled = *this;
		return true;
	}

	auto arrayResize = [](const IntermediateColorType * src, const int srcLen, IntermediateColorType * dest, const int destLen, const float * coefficients) {
		for (int i = 0; i < destLen; ++i)
			dest[i].makeZero();
		const float ratio = (destLen - 1) / (float(srcLen - 1));
		for (int i = 0; i < srcLen; ++i) {
			const float x = i * ratio;
			const int xx = int(x);
			const float mult = 1.0f - (x - xx);
			dest[xx] += mult * src[i];
			if (xx + 1 < destLen) {
				const float multRest = 1.0f - mult;
				dest[xx + 1] += multRest * src[i];
			}
		}
		for (int i = 0; i < destLen; ++i) {
			dest[i] *= coefficients[i];
		}
	};

	Pixelmap<IntermediateColorType> srcPmp(*this);
	Pixelmap<IntermediateColorType> destPmp(downWidth, downHeight);
	IntermediateColorType * srcData = srcPmp.getDataPtr();
	IntermediateColorType * destData = destPmp.getDataPtr();

	if (width != downWidth) {
		std::unique_ptr<IntermediateColorType[]> row(new IntermediateColorType[width]);
		const float * widthCoefficients = coeffsCache.getCoefficients(width, downWidth);
		for (int y = 0; y < height; ++y) {
			arrayResize(srcData + (y * width), width, row.get(), downWidth, widthCoefficients);
			memcpy(srcData + (y * width), row.get(), downWidth * sizeof(IntermediateColorType));
		}
	}

	if (height != downHeight) {
		std::unique_ptr<IntermediateColorType[]> column(new IntermediateColorType[height]);
		std::unique_ptr<IntermediateColorType[]> newColumn(new IntermediateColorType[downHeight]);
		const float * heightCoefficients = coeffsCache.getCoefficients(height, downHeight);
		for (int x = 0; x < downWidth; ++x) {
			for (int y = 0; y < height; ++y) {
				column[y] = srcData[x + y * width]; // this has to use the previous width
			}
			arrayResize(column.get(), height, newColumn.get(), downHeight, heightCoefficients);
			for (int y = 0; y < downHeight; ++y) {
				destData[x + y * downWidth] = newColumn[y];
			}
		}
	} else {
		for (int y = 0; y < downHeight; ++y) {
			memcpy(destData + (y * downWidth), srcData + (y * width), downWidth * sizeof(IntermediateColorType));
		}
	}
	downScaled = Pixelmap<ColorType>(destPmp);
	return true;
}

template<class ColorType>
template<class IntermediateColorType>
bool Pixelmap<ColorType>::upscale(Pixelmap<ColorType>& upScaled, const int upWidth, const int upHeight, UpscaleFiltering filterType) const {
	if (!isOK() || upWidth < width || upHeight < height) {
		return false;
	} else if (upWidth == width && upHeight == height) {
		upScaled = *this;
		return true;
	}
	upScaled.generateEmptyImage(upWidth, upHeight, false);
	const double ratioWidth = width / static_cast<double>(upWidth);
	const double ratioHeight = height / static_cast<double>(upHeight);
	ColorType * upData = upScaled.getDataPtr();
	if (UF_NEAREST_NEIGHBOUR == filterType) {
		for (int y = 0; y < upHeight; ++y) {
			for (int x = 0; x < upWidth; ++x) {
				const int downY = clamp(nearestInt(y * ratioHeight), 0, height - 1);
				const int downX = clamp(nearestInt(x * ratioWidth), 0, width - 1);
				upData[y * upWidth + x] = data[downY * width + downX];
			}
		}
	} else if (UF_BILINEAR == filterType) {
		for (int y = 0; y < upHeight; ++y) {
			for (int x = 0; x < upWidth; ++x) {
				upData[y * upWidth + x] = getBilinearFilteredPixel<IntermediateColorType>(static_cast<float>(x * ratioWidth), static_cast<float>(y * ratioHeight), EFT_STRETCH);
			}
		}
	} else if (UF_BICUBIC == filterType) {
		for (int y = 0; y < upHeight; ++y) {
			for (int x = 0; x < upWidth; ++x) {
				upData[y * upWidth + x] = getBicubicFilteredPixel<IntermediateColorType>(static_cast<float>(x * ratioWidth), static_cast<float>(y * ratioHeight), EFT_STRETCH);
			}
		}
	}

	return true;
}

template<class ColorType>
bool Pixelmap<ColorType>::drawBitmap(Pixelmap<ColorType> & subBmp, const int x, const int y) noexcept {
	if (!subBmp.isOK() || !this->isOK() || this == &subBmp)
		return false;
	const int sw = subBmp.getWidth();
	const int sh = subBmp.getHeight();
	// check if the drawn subbitmap is completely outside this
	if (x + sw < 0 || y + sh < 0 || x >= width || y >= height) {
		return false;
	}
	if (x < 0 || y < 0 || x + sw > width || y + sh > height) {
		// calculate the source and destination coordinates
		const int dx = (x < 0 ? 0 : x);
		const int dy = (y < 0 ? 0 : y);
		const int sx = (x < 0 ? -x : 0);
		const int sy = (y < 0 ? -y : 0);
		// calculate the actual width and height that will be copied
		const int dw = (x < 0 ? std::min(x + sw, width) : std::min(width - x, sw));
		const int dh = (y < 0 ? std::min(y + sh, height) : std::min(height - y, sh));
		// check if the submap is even within bounds
		if (dw <= 0 || dh <= 0 || sx >= sw || sy >= sh) {
			return false;
		}
		const ColorType * subData = subBmp.getDataPtr();
		for (int yy = 0; yy < dh; ++yy) {
			const ColorType * src = subData + (sy + yy) * sw + sx;
			ColorType * dest = data + (dy + yy) * width + dx;
			memcpy(dest, src, dw * sizeof(ColorType));
		}
	} else {
		const ColorType * subData = subBmp.getDataPtr();
		for (int sy = 0; sy < sh; ++sy) {
			ColorType * dest = data + (y + sy) * width + x;
			// copy the whole row with the destinations width as size
			memcpy(dest, subData + sy * sw, sw * sizeof(ColorType));
		}
	}
	return true;
}

template<class ColorType>
bool Pixelmap<ColorType>::drawAxisAlignedLine(int start, int end, int coord, PixelmapAxis axis, const ColorType & color) {
	if (!isOK() ||
		coord < 0 ||
		(start < 0 && end < 0) ||
		axis == PixelmapAxis::PA_NONE ||
		axis == PixelmapAxis::PA_BOTH ||
		(axis == PixelmapAxis::PA_X_AXIS && (coord >= height || (start >= width  && end >= width ))) ||
		(axis == PixelmapAxis::PA_Y_AXIS && (coord >= width  || (start >= height && end >= height))))
	{
		return false;
	}
	int x0, x1, y0, y1, stepX, stepY;
	if (PixelmapAxis::PA_X_AXIS == axis) {
		stepX = 1;
		stepY = 0;
		x0 = clamp(std::min(start, end), 0, width - 1);
		x1 = clamp(std::max(start, end), 0, width - 1);
		y0 = y1 = coord;
	} else if (PixelmapAxis::PA_Y_AXIS == axis) {
		stepX = 0;
		stepY = 1;
		x0 = x1 = coord;
		y0 = clamp(std::min(start, end), 0, height - 1);
		y1 = clamp(std::max(start, end), 0, height - 1);
	} else {
		return false;
	}
	for (int x = x0, y = y0; x != x1 || y != y1; x += stepX, y += stepY) {
		data[y * width + x] = color;
	}
	return true;
}

template<class ColorType>
bool Pixelmap<ColorType>::drawCharacter(char ch, int x, int y, const ColorType& color) noexcept {
	if (!isOK() || x + ASCII_TABLE_PACKED_WIDTH < 0 || y + ASCII_TABLE_PACKED_HEIGHT < 0 || x >= width || y >= height) {
		return false;
	}

	const int sw = ASCII_TABLE_PACKED_WIDTH;
	const int sh = ASCII_TABLE_PACKED_HEIGHT;
	const int dx = (x < 0 ? 0 : x);
	const int dy = (y < 0 ? 0 : y);
	const int sx = (x < 0 ? -x : 0) + ASCII_TABLE_PACKED_OFFSET_X;
	const int sy = (y < 0 ? -y : 0) + ASCII_TABLE_PACKED_OFFSET_Y;
	const int dw = (x < 0 ? std::min(x + sw, width) : std::min(width - x, sw));
	const int dh = (y < 0 ? std::min(y + sh, height) : std::min(height - y, sh));
	// check for getting out of bouds
	if (dw <= 0 || dh <= 0 || sx >= sw || sy >= sh) {
		return false;
	}
	const float * asciiData = ASCII_TABLE_MASKS[ch];
	for (int yy = 0; yy < dh; ++yy) {
		ColorType * destRow = data + (dy + yy) * width + dx;
		// use the actual width, because the data is not packed
		const float * maskRow = asciiData + (sy + yy) * ASCII_TABLE_CHAR_WIDTH + sx;
		for (int xx = 0; xx < dw; ++xx) {
			ColorType& dataPixel = destRow[xx];
			const float maskValue = maskRow[xx];
			dataPixel = static_cast<ColorType>(dataPixel * (1.0 - maskValue)) + static_cast<ColorType>(color * maskValue);
		}
	}
	return true;
}

template<class ColorType>
bool Pixelmap<ColorType>::drawString(const char * str, int x, int y, const ColorType& color, int strLen) noexcept {
	if (!isOK() || !str || x >= width || y >= height) {
		return false;
	}
	// get the text extent
	int textWidth = 0;
	int textHeight = 0;
	getTextExtent(str, textWidth, textHeight, strLen);
	if (textWidth == 0 || textHeight == 0 || x + textWidth <= 0 || y + textHeight <= 0) {
		return false;
	}

	const int actualStrLen = static_cast<int>(strlen(str));
	if (strLen < 0) {
		strLen = actualStrLen;
	} else {
		strLen = std::min(strLen, actualStrLen);
	}

	// output the characters one by one
	for (int i = 0, dx = x, dy = y; i < strLen; ++i) {
		if (str[i] == '\n') {
			dx = x; // reset dx
			dy += ASCII_TABLE_PACKED_HEIGHT + 1; // and accumulate dy
		} else {
			drawCharacter(str[i], dx, dy, color);
			dx += ASCII_TABLE_PACKED_WIDTH + 1;
		}
	}
	return true;
}

template<class ColorType>
void Pixelmap<ColorType>::getTextExtent(const char * str, int & w, int & h, int strLen) noexcept {
	if (!str || str[0] == '\0') {
		w = 0;
		h = 0;
		return;
	}
	const int actualStrLen = static_cast<int>(strlen(str));
	if (strLen < 0) {
		strLen = actualStrLen;
	} else {
		strLen = std::min(strLen, actualStrLen);
	}
	// check for new line characters
	int lines = 1;
	int maxWidth = 0;
	int lastNewLine = 0;
	for (int i = 0; i < strLen; ++i) {
		if (str[i] == '\n') {
			lines++;
			lastNewLine = i;
		}
		maxWidth = std::max(i - lastNewLine + 1, maxWidth);
	}
	w = maxWidth * (ASCII_TABLE_PACKED_WIDTH + 1);
	h = lines * (ASCII_TABLE_PACKED_HEIGHT + 1);
}

template class Histogram<HDL_CHANNEL>;
template class Histogram<HDL_VALUE>;

template<HistogramDataLayout hdl>
Histogram<hdl>::Histogram()
	: data{ 0U }
	, maxColor(1)
	, maxIntensity(1)
{}

template<HistogramDataLayout hdl>
Histogram<hdl>::Histogram(const Bitmap & bmp)
	: data{ 0U }
	, maxColor(1)
	, maxIntensity(1)
{
	fromBmp(bmp);
}

template<>
void Histogram<HDL_CHANNEL>::fromBmp(const Bitmap & bmp) noexcept {
	memset(data, 0, sizeof(data));
	maxColor = 1;
	maxIntensity = 1;
	const Color * bmpData = bmp.getDataPtr();
	const int bmpSize = bmp.getWidth() * bmp.getHeight();
	for (int i = 0; i < bmpSize; ++i) {
		const Color& ci = bmpData[i];
		maxColor = std::max(data[channelSize * int(HistogramChannel::HC_RED)   + ci.r]++, maxColor);
		maxColor = std::max(data[channelSize * int(HistogramChannel::HC_GREEN) + ci.g]++, maxColor);
		maxColor = std::max(data[channelSize * int(HistogramChannel::HC_BLUE)  + ci.b]++, maxColor);
		const uint8 cii = ci.intensity();
		maxIntensity = std::max(data[channelSize * int(HistogramChannel::HC_INTENSITY) + cii]++, maxIntensity);
	}
}

template<>
HistogramChunk Histogram<HDL_CHANNEL>::operator[](int i) const {
	HistogramChunk retval;
	retval.r = data[channelSize * int(HistogramChannel::HC_RED)       + i];
	retval.g = data[channelSize * int(HistogramChannel::HC_GREEN)     + i];
	retval.b = data[channelSize * int(HistogramChannel::HC_BLUE)      + i];
	retval.i = data[channelSize * int(HistogramChannel::HC_INTENSITY) + i];
	return retval;
}

template<>
HistogramChunk Histogram<HDL_VALUE>::operator[](int i) const {
	HistogramChunk retval;
	memcpy(&retval, data + i * numChannels, sizeof(retval));
	return retval;
}

template<>
void Histogram<HDL_VALUE>::fromBmp(const Bitmap & bmp) noexcept {
	memset(data, 0, sizeof(data));
	maxColor = 1;
	maxIntensity = 1;
	const Color * bmpData = bmp.getDataPtr();
	const int bmpSize = bmp.getWidth() * bmp.getHeight();
	for (int i = 0; i < bmpSize; ++i) {
		const Color& ci = bmpData[i];
		maxColor = std::max(data[ci.r * numChannels + int(HistogramChannel::HC_RED)  ]++, maxColor);
		maxColor = std::max(data[ci.g * numChannels + int(HistogramChannel::HC_GREEN)]++, maxColor);
		maxColor = std::max(data[ci.b * numChannels + int(HistogramChannel::HC_BLUE) ]++, maxColor);
		const uint8 cii = ci.intensity();
		maxIntensity = std::max(data[cii * numChannels + int(HistogramChannel::HC_INTENSITY)]++, maxIntensity);
	}
}

template<HistogramDataLayout hdl>
const uint32 * Histogram<hdl>::getDataPtr() const noexcept {
	return data;
}

template<HistogramDataLayout hdl>
uint32 * Histogram<hdl>::getDataPtr() noexcept {
	return data;
}

template<>
std::vector<uint32> Histogram<HDL_CHANNEL>::getChannel(HistogramChannel ch) const {
	std::vector<uint32> retval(channelSize);
	retval.assign(data + int(ch) * channelSize, data + (int(ch) + 1) * channelSize);
	return retval;
}

template<>
std::vector<uint32> Histogram<HDL_VALUE>::getChannel(HistogramChannel ch) const {
	std::vector<uint32> retval(channelSize);
	for (int i = 0; i < channelSize; ++i) {
		retval[i] = data[i * numChannels + int(ch)];
	}
	return retval;
}

template<>
void Histogram<HDL_CHANNEL>::setChannel(const std::vector<uint32>& chData, HistogramChannel ch) {
	memcpy(data + int(ch) * channelSize, chData.data(), std::min(channelSize, int(chData.size())) * sizeof(uint32));
}

template<>
void Histogram<HDL_VALUE>::setChannel(const std::vector<uint32>& chData, HistogramChannel ch) {
	const int count = std::min(channelSize, int(chData.size()));
	for (int i = 0; i < count; ++i) {
		data[i * numChannels + int(ch)] = chData[i];
	}
}

template<HistogramDataLayout hdl>
HistogramDataLayout Histogram<hdl>::getDataLayout() const noexcept {
	return hdl;
}

template<HistogramDataLayout hdl>
uint32 Histogram<hdl>::getMaxColor() const noexcept {
	return maxColor;
}

template<HistogramDataLayout hdl>
uint32 Histogram<hdl>::getMaxIntensity() const noexcept {
	return maxIntensity;
}
