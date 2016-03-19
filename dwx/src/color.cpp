#include "color.h"

unsigned char SRGB_COMPRESS_CACHE[4097];

void initColor(void) noexcept {
	// precache the results of convertTo8bit_sRGB, in order to avoid the costly pow()
	// in it and use a lookup table instead, see Color::convertTo8bit_sRGB_cached().
	for (int i = 0; i <= 4096; i++)
		SRGB_COMPRESS_CACHE[i] = (unsigned char)convertTo8bit_sRGB(i / 4096.0f);
}

unsigned convertTo8bit_sRGB_cached(float x) noexcept {
	if (x <= 0) return 0;
	if (x >= 1) return 255;
	return SRGB_COMPRESS_CACHE[int(x * 4096.0f)];
}

