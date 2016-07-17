#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <functional>
#include <vector>
#include <memory>
#include "color.h"

// point in discrete 2d space
class Point {
public:
	int x;
	int y;
	Point() = default;
	Point(int _x, int _y)
		: x(_x)
		, y(_y)
	{}
};

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

enum ColorChannel {
	CC_RED = 0,
	CC_GREEN,
	CC_BLUE,
	CC_COUNT, //!< the count of available channels
};

// used for operations which require decision on what is to be done beyond the borders of the current pixelmap
enum EdgeFillType {
	EFT_BLANK = 0, //!< coordinates beyond the edge will be initialized with blank color
	EFT_TILE,      //!< coordinates beyond the edge will be initialized with tiled image
	EFT_STRETCH,   //!< coordinates beyond the edge will be the stretched edge
	EFT_MIRROR,    //!< coordinates beyond the edge will start mirroring based on the edge
};

// used for operations which require axis decisions
enum PixelmapAxis {
	PA_NONE = 0,                     //!< none of the axes
	PA_X_AXIS = 1 << 0,              //!< the x axis of the pixelmap
	PA_Y_AXIS = 1 << 1,              //!< the y axis of the pixelmap
	PA_BOTH = PA_X_AXIS | PA_Y_AXIS, //!< both the axes
	PA_COUNT,
};

template<class ColorType = Color>
class Pixelmap {
protected:
	int width, height;
	// TODO make Bitmap data a shared ptr, so bitmaps can be easily shared
	ColorType* data;

	void copy(const Pixelmap& rhs) noexcept;
public:
	Pixelmap() noexcept; //!< Generates an empty bitmap
	Pixelmap(int width, int height, const ColorType * pixelValues = nullptr) noexcept; //!< Generates bitmap with specified dimensions and initializes it with the buffer (if supplied)
	~Pixelmap() noexcept;
	Pixelmap(const Pixelmap& rhs) noexcept;
	Pixelmap& operator = (const Pixelmap& rhs) noexcept;

	template<class OtherColorType>
	Pixelmap(const Pixelmap<OtherColorType>& rhs);

	void freeMem(void) noexcept; //!< Deletes the memory, associated with the bitmap
	int getWidth(void) const noexcept; //!< Gets the width of the image (X-dimension)
	int getHeight(void) const noexcept; //!< Gets the height of the image (Y-dimension)
	int getDimensionProduct() const noexcept; //!< Gets the product of the width and height dimensions
	bool validPixelPosition(int x, int y) const noexcept; //!< Checks if the pixel position is valid (inside the pixelmap)
	bool isOK(void) const noexcept; //!< Returns true if the bitmap is valid
	void generateEmptyImage(int width, int height, bool clear = true) noexcept; //!< Creates an empty image with the given dimensions (and clears it by default)
	void fill(ColorType c, int x = 0, int y = 0, int width = -1, int height = -1);
	ColorType getPixel(int x, int y) const noexcept; //!< Gets the pixel at coordinates (x, y). Returns black if (x, y) is outside of the image
	ColorType getFilteredPixel(float x, float y, EdgeFillType edge = EdgeFillType::EFT_BLANK) const noexcept;
	void remap(std::function<ColorType(ColorType)>) noexcept; // remap R, G, B channels by a function

	void setPixel(int x, int y, const ColorType& col) noexcept; //!< Sets the pixel at coordinates (x, y)

	ColorType* getDataPtr() const noexcept; //!< get the current data ptr - fastest data management for reading

	ColorType * operator[](int row) noexcept;
	const ColorType * operator[](int row) const noexcept;

	// the output channel buffer has to be allocated by the caller (with size of the product of dimensions - width * height)
	template<class ChannelScalar>
	bool getChannel(ChannelScalar * channel, ColorChannel cc) const;

	template<class ChannelScalar>
	bool setChannel(const ChannelScalar * channel, ColorChannel cc);

	// mirrors the pixelmap in-place
	bool mirror(PixelmapAxis axis);

	// mirrors the pixelmap in the output pixelmap
	bool mirror(Pixelmap<ColorType>& mirrored, PixelmapAxis axis) const;

	// crops the rectangle with top left (x, y) and w width and h height in place
	bool crop(const int x, const int y, const int w, const int h);

	// crops the rectangle with top left (x, y) and w width and h height in a specified pixelmap
	bool crop(Pixelmap<ColorType>& cropped, const int x, const int y, const int w, const int h) const;

	// expands the bitmap with specified width and height (and remaps the current (0, 0) -> (x, y))
	bool expand(const int w, const int h, const int x, const int y, EdgeFillType fillType = EdgeFillType::EFT_BLANK);

	// expands the bitmap with specified width and height (and remaps the current (0, 0) -> (x, y))
	bool expand(Pixelmap<ColorType>& expanded, const int w, const int h, const int x, const int y, EdgeFillType fillType = EdgeFillType::EFT_BLANK) const;

	// relocates the pixelmap (0, 0) -> (x, y) using a temporary Pixelmap
	bool relocate(const int x, const int y);

	// relocates the pixelmap (0, 0) -> (x, y)
	bool relocate(Pixelmap<ColorType>& relocated, const int x, const int y) const;

	// downscales the bitmap using fractions
	template<class IntermediateColorType>
	bool downscale(Pixelmap<ColorType>& downScaled, const int downWidth, const int downHeight) const;

	// draws a smaller bitmap into a larger one -> return false if the bitmap won't fit
	bool drawBitmap(Pixelmap<ColorType>& subBmp, const int x, const int y) noexcept;

	// draws a line colinear with the supplied axis, with coord offset and from start to end
	bool drawAxisAlignedLine(int start, int end, int coord, PixelmapAxis axis, const ColorType& color);

	// draw a single ASCII character on the bitmap
	bool drawCharacter(char ch, int x, int y, const ColorType& color) noexcept;

	// draw a string of ASCII characters on the bitmap
	bool drawString(const char * str, int x, int y, const ColorType& color) noexcept;

	// returns the text extent considering new lines
	static void getTextExtent(const char * str, int& w, int& h) noexcept;
};

using Bitmap = Pixelmap<>;

enum HistogramDataLayout {
	HDL_CHANNEL = 0, //!< each channel is layed out in continuous memory
	HDL_VALUE,       //!< channels are interleaved and each tuple of numChannels is close (may improve memory cache misses)
};

enum HistogramChannel {
	HC_RED = 0,   //!< The red channel
	HC_GREEN,     //!< The green channel
	HC_BLUE,      //!< The blue channel
	HC_INTENSITY, //!< The intensity channel
	HC_COUNT,
};

union HistogramChunk {
	struct {
		uint32 r;
		uint32 g;
		uint32 b;
		uint32 i;
	};
	uint32 d[HistogramChannel::HC_COUNT];
};

template<HistogramDataLayout hdl = HDL_CHANNEL>
class Histogram {
public:
	Histogram();
	Histogram(const Bitmap& bmp);
	Histogram(const Histogram&) = default;
	Histogram& operator=(const Histogram&) = default;

	HistogramChunk operator[](int i) const;

	void fromBmp(const Bitmap& bmp) noexcept;

	const uint32* getDataPtr() const noexcept;
	uint32 * getDataPtr() noexcept;

	std::vector<uint32> getChannel(HistogramChannel ch) const;
	void setChannel(const std::vector<uint32>& chData, HistogramChannel ch);

	HistogramDataLayout getDataLayout() const noexcept;

	uint32 getMaxColor() const noexcept;
	uint32 getMaxIntensity() const noexcept;

	static const int channelSize = 0xff + 1;
	static const int numChannels = HC_COUNT;

private:
	uint32 data[channelSize * numChannels];
	uint32 maxColor;
	uint32 maxIntensity;
};

#endif // __BITMAP_H__
