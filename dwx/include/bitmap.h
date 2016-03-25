#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <functional>
#include "color.h"

class FloatBitmap {
protected:
	int width, height;
	// TODO make Bitmap data a shared ptr, so bitmaps can be easily shared
	FloatColor* data;

	void remapRGB(std::function<float(float)>) noexcept; // remap R, G, B channels by a function
	void copy(const FloatBitmap& rhs) noexcept;
public:
	FloatBitmap() noexcept; //!< Generates an empty bitmap
	// make virtual if necessary
	~FloatBitmap() noexcept;
	FloatBitmap(const FloatBitmap& rhs) noexcept;
	FloatBitmap& operator = (const FloatBitmap& rhs) noexcept;
	void freeMem(void) noexcept; //!< Deletes the memory, associated with the bitmap
	int getWidth(void) const noexcept; //!< Gets the width of the image (X-dimension)
	int getHeight(void) const noexcept; //!< Gets the height of the image (Y-dimension)
	bool isOK(void) const noexcept; //!< Returns true if the bitmap is valid
	void generateEmptyImage(int width, int height) noexcept; //!< Creates an empty image with the given dimensions
	FloatColor getPixel(int x, int y) const noexcept; //!< Gets the pixel at coordinates (x, y). Returns black if (x, y) is outside of the image
	/// Gets a bilinear-filtered pixel from float coords (x, y). The coordinates wrap when near the edges.
	FloatColor getFilteredPixel(float x, float y) const noexcept;
	void setPixel(int x, int y, const FloatColor& col) noexcept; //!< Sets the pixel at coordinates (x, y).

	/// Saves the image into the EXR format, preserving the dynamic range, using Half for storage. Note that
	/// in contrast with saveBMP(), it does not use gamma-compression on saving.
	//bool saveEXR(const char* filename);
	//virtual bool saveImage(const char* filename); //!< Save the bitmap to an image (the format is detected from extension)
	//enum OutputFormat { /// the two supported writing formats
	//  outputFormat_BMP,
	//  outputFormat_EXR,
	//};

	void decompressGamma_sRGB(void) noexcept; //!< assuming the pixel data is in sRGB, decompress to linear RGB values
	void decompressGamma(float gamma) noexcept; //!< as above, but assume a specific gamma value

	void differentiate(void) noexcept; //!< differentiate image (red = dx, green = dy, blue = 0)

	FloatColor* getDataPtr() const noexcept; //!< get the current data ptr - fastest data management for reading

	FloatColor * operator[](int row) noexcept;
	const FloatColor * operator[](int row) const noexcept;
};

class Bitmap {
protected:
	int width, height;
	// TODO make Bitmap data a shared ptr, so bitmaps can be easily shared
	Color* data;

	void copy(const Bitmap& rhs) noexcept;
public:
	Bitmap() noexcept; //!< Generates an empty bitmap
										 // make virtual if necessary
	~Bitmap() noexcept;
	Bitmap(const Bitmap& rhs) noexcept;
	Bitmap& operator = (const Bitmap& rhs) noexcept;
	void freeMem(void) noexcept; //!< Deletes the memory, associated with the bitmap
	int getWidth(void) const noexcept; //!< Gets the width of the image (X-dimension)
	int getHeight(void) const noexcept; //!< Gets the height of the image (Y-dimension)
	bool isOK(void) const noexcept; //!< Returns true if the bitmap is valid
	void generateEmptyImage(int width, int height) noexcept; //!< Creates an empty image with the given dimensions
	Color getPixel(int x, int y) const noexcept; //!< Gets the pixel at coordinates (x, y). Returns black if (x, y) is outside of the image
	void remap(std::function<unsigned char(unsigned char)>) noexcept; // remap R, G, B channels by a function

	void setPixel(int x, int y, const Color& col) noexcept; //!< Sets the pixel at coordinates (x, y)

	Color* getDataPtr() const noexcept; //!< get the current data ptr - fastest data management for reading

	Color * operator[](int row) noexcept;
	const Color * operator[](int row) const noexcept;

};

#endif // __BITMAP_H__
