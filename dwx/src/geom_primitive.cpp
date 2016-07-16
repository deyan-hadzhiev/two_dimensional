#include "geom_primitive.h"

const Color GeometricPrimitive<Color>::axisCol = Color(127, 127, 127);
const uint32 GeometricPrimitive<uint32>::axisCol = ~0 >> 1;
const uint64 GeometricPrimitive<uint64>::axisCol = ~0ULL >> 1;

template class Sinosoid<Color>;
template class Sinosoid<uint32>;
template class Sinosoid<uint64>;

template<class CType>
Sinosoid<CType>::Sinosoid()
	: k(1.0f)
	, q(1.0f)
	, offset(0.0f)
	, sine(false)
{}

template<class CType>
void Sinosoid<CType>::setParams(float _k, float _q, float _offset, bool _sine) {
	k = _k;
	q = _q;
	offset = _offset;
	sine = _sine;
}

template<class CType>
void Sinosoid<CType>::setParam(const std::string & p, const std::string & value) {
	if (p == "k") {
		k = static_cast<float>(atof(value.c_str()));
	} else if (p == "q") {
		q = static_cast<float>(atof(value.c_str()));
	} else if (p == "offset") {
		offset = static_cast<float>(atof(value.c_str()));
	} else if (p == "function") {
		sine = (value == "sine" || value == "sin");
	}
}

template<class CType>
std::vector<std::string> Sinosoid<CType>::getParamList() const
{
	std::vector<std::string> retval{
		std::string("k"),
		std::string("q"),
		std::string("offset"),
		std::string("function"),
	};
	return retval;
}

template<class CType>
void Sinosoid<CType>::draw(unsigned flags) {
	if ((flags & DF_CLEAR) != 0) {
		clear();
	}
	const bool additive = ((flags & DF_ACCUMULATE) != 0);
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const int cx = bw / 2;
	const int cy = bh / 2;
	CType * bmpData = bmp.getDataPtr();
	if ((flags & DF_SHOW_AXIS) != 0) {
		// draw coord system
		for (int x = 0; x < bw; ++x) {
			bmpData[cy * bw + x] = axisCol;
		}
		for (int y = 0; y < bh; ++y) {
			bmpData[y * bw + cx] = axisCol;
		}
	}
	const CType scol = pen.getColor();
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

template class Function<uint64>;

template<class CType>
Function<CType>::Function(int width = -1, int height = -1)
	: GeometricPrimitive<CType>(width, height)
{}

template<class CType>
void Function<CType>::setFunction(std::function<float(int)> _mapX, std::function<float(float)> _fx) {
	mapX = _mapX;
	fx = _fx;
}

template<class CType>
void Function<CType>::draw(unsigned flags) {
	if ((flags & DF_CLEAR) != 0) {
		clear();
	}
	const bool additive = ((flags & DF_ACCUMULATE) != 0);
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const int cx = bw / 2;
	const int cy = bh / 2;
	CType * bmpData = bmp.getDataPtr();
	if ((flags & DF_SHOW_AXIS) != 0) {
		// draw coord system
		for (int x = 0; x < bw; ++x) {
			bmpData[cy * bw + x] = axisCol;
		}
		for (int y = 0; y < bh; ++y) {
			bmpData[y * bw + cx] = axisCol;
		}
	}
	const CType scol = pen.getColor();
	float x0f = mapX(-cx);
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
		const float xf = mapX(x - cx);
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

template class FunctionRaster<Color>;

template<class CType>
FunctionRaster<CType>::FunctionRaster(int width, int height)
	: GeometricPrimitive<CType>(width, height)
{}

template<class CType>
void FunctionRaster<CType>::setFunction(std::function<double(double, double)> _fxy) {
	fxy = _fxy;
}

template<class CType>
void FunctionRaster<CType>::draw(unsigned flags) {
	if ((flags & DF_CLEAR) != 0) {
		clear();
	}
	const bool additive = ((flags & DF_ACCUMULATE) != 0);
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const int cx = bw / 2;
	const int cy = bh / 2;
	CType * rasterData = bmp.getDataPtr();
	if ((flags & DF_SHOW_AXIS) != 0) {
		// draw coord system
		for (int x = 0; x < bw; ++x) {
			rasterData[cy * bw + x] = axisCol;
		}
		for (int y = 0; y < bh; ++y) {
			rasterData[y * bw + cx] = axisCol;
		}
	}
	const double halfPenWidth = pen.getWidth() / 2.0;
	const double fullColorRange = pen.getStrength() * halfPenWidth;
	const double halfToneRange = halfPenWidth - fullColorRange;
	const CType penColor = pen.getColor();
	for (int y = 0; y < bh; ++y) {
		for (int x = 0; x < bw; ++x) {
			const double sampleX = x - cx + 0.5;
			const double sampleY = -y + cy - 0.5;
			const double error = abs(fxy(sampleX, sampleY));
			if (error <= halfPenWidth) {
				const CType currentColor = rasterData[y * bw + x];
				if (error <= fullColorRange) {
					rasterData[y * bw + x] = (additive ? currentColor + penColor : penColor);
				} else {
					double amount = 1.0 - (error - fullColorRange) / halfToneRange;
					if (additive) {
						rasterData[y * bw + x] += static_cast<CType>(penColor * amount);
					} else {
						rasterData[y * bw + x] = static_cast<CType>(penColor * amount + currentColor * (1.0 - amount));
					}
				}
			}
		}
	}
}
