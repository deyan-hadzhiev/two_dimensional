#include "geom_primitive.h"

void Sinosoid::draw(GeometricPrimitive::DrawMode) {
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const float k = 400;
	const int cx = bw / 2;
	const int cy = bh / 2;
	// draw coord system
	const Color ccol(127, 127, 127);
	Color * bmpData = bmp.getDataPtr();
	for (int x = 0; x < bw; ++x) {
		bmpData[cy * bw + x] = ccol;
	}
	for (int y = 0; y < bh; ++y) {
		bmpData[y * bw + cx] = ccol;
	}
	const Color scol(255, 255, 255);
	auto xToRad = [cx](int x) -> float {
		const int xdeg = (x - cx) % 360;
		return toRadians(float(xdeg) + 0.5f);
	};
	auto fx = [k](float x) -> float {
		return k * cos(x);
	};
	float x0f = xToRad(0);
	float y0f = fx(x0f);
	int y0 = int(floor(y0f - 0.5f) + cy);
	if (y0 >= 0 && y0 < bh) {
		bmpData[y0 * bw] = scol;
	} else {
		y0 = clamp(y0, 0, bh - 1);
	}
	for (int x = 1; x < bw; ++x) {
		const float xf = xToRad(x);
		const float yf = fx(xf);
		int y = int(floorf(yf - 0.5f) + cy);
		if (y < 0 || y >= bh) {
			continue;
		}
		const float dyf = (yf - y0f);
		const int dy = (y - y0);
		if (abs(dy) > 1) {
			int ys = (dy > 0 ? 1 : -1);
			const float dx = (xf - x0f) / (yf - y0f);
			const float dt = fabs(xf - x0f) / 2;
			float xerr = dx;
			const int dyAbs = abs(dy);
			for (int yfill = ys; yfill * ys < dyAbs; yfill += ys) {
				bmpData[(y0 + yfill) * bw + x - (xerr < dt ? 1 : 0)] = scol;
				xerr += dx;
			}
			bmpData[y * bw + x] = scol;
		} else {
			bmpData[y * bw + x] = scol;
		}
		x0f = xf;
		y0f = yf;
		y0 = y;
	}
}
