#include "geom_primitive.h"

void Sinosoid::draw(GeometricPrimitive::DrawMode) {
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const int tw = bw / 3;
	const int th = bh / 3;
	const Color f(0, 127, 127);
	bmp.fill(f, tw, th, tw, th); // DEBUG for now
}
