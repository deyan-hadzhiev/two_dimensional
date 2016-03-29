#include "geom_primitive.h"

Sinosoid::Sinosoid()
	: k(1.0f)
	, q(1.0f)
	, offset(0.0f)
	, sine(false)
{}

void Sinosoid::setParams(float _k, float _q, float _offset, bool _sine) {
	k = _k;
	q = _q;
	offset = _offset;
	sine = _sine;
}

void Sinosoid::setParam(const std::string & p, const std::string & value) {
	if (p == "k") {
		k = atof(value.c_str());
	} else if (p == "q") {
		q = atof(value.c_str());
	} else if (p == "offset") {
		offset = atof(value.c_str());
	} else if (p == "function") {
		sine = (value == "sine");
	}
}

std::vector<std::string> Sinosoid::getParamList() const
{
	std::vector<std::string> retval{
		std::string("k"),
		std::string("q"),
		std::string("offset"),
		std::string("function"),
	};
	return retval;
}

void Sinosoid::draw(Color col, unsigned flags) {
	if ((flags & DF_CLEAR) != 0) {
		clear();
	}
	const bool additive = ((flags & DF_ACCUMULATE) != 0);
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const int cx = bw / 2;
	const int cy = bh / 2;
	Color * bmpData = bmp.getDataPtr();
	if ((flags & DF_SHOW_AXIS) != 0) {
		// draw coord system
		const Color ccol(127, 127, 127);
		for (int x = 0; x < bw; ++x) {
			bmpData[cy * bw + x] = ccol;
		}
		for (int y = 0; y < bh; ++y) {
			bmpData[y * bw + cx] = ccol;
		}
	}
	const Color scol = col;
	auto xToRad = [cx](int x) -> float {
		const int xdeg = (x - cx);
		return toRadians(float(xdeg) + 0.5f);
	};
	std::function<float(float)> fx;
	if (sine) {
		fx = [this](float x) -> float { return k * sin(q * (x + offset)); };
	} else {
		fx = [this](float x) -> float { return k * cos(q * (x + offset)); };
	}
	float x0f = xToRad(0);
	float y0f = fx(x0f);
	int y0 = int(floor(y0f - 0.5f) + cy);
	if (y0 >= 0 && y0 < bh) {
		if (additive)
			bmpData[y0 * bw] += scol;
		else
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
				if (additive)
					bmpData[(y0 + yfill) * bw + x - (xerr < dt ? 1 : 0)] += scol;
				else
					bmpData[(y0 + yfill) * bw + x - (xerr < dt ? 1 : 0)] = scol;
				xerr += dx;
			}
			if (additive)
				bmpData[y * bw + x] += scol;
			else
				bmpData[y * bw + x] = scol;
		} else {
			if (additive)
				bmpData[y * bw + x] += scol;
			else
				bmpData[y * bw + x] = scol;
		}
		x0f = xf;
		y0f = yf;
		y0 = y;
	}
}
