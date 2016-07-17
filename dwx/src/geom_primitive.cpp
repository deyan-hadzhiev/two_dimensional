#include "geom_primitive.h"
#include "quad_tree.h"
#include "drect.h"
#include "module_base.h"

#include <string>

const Color GeometricPrimitive<Color>::axisCol = Color(127, 127, 127);
const uint32 GeometricPrimitive<uint32>::axisCol = ~0 >> 1;
const uint64 GeometricPrimitive<uint64>::axisCol = ~0ULL >> 1;

template<class CType>
Vector2 GeometricPrimitive<CType>::rasterToReal(const Vector2 & r) const noexcept {
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const Vector2 center(bw * 0.5f, bh * 0.5f);
	// apply center and offset and flip y
	const Vector2 realUnscaled(r.x - center.x + offset.x, -(r.y - center.y + offset.y));
	// finally scale the result
	return Vector2(realUnscaled.x / scale.x, realUnscaled.y / scale.y);
}

template<class CType>
Vector2 GeometricPrimitive<CType>::realToRaster(const Vector2 & r) const noexcept {
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const Vector2 center(bw * 0.5f, bh * 0.5f);
	// first scale the coordinates
	const Vector2 realScaled(r.x * scale.x, r.y * scale.y);
	// then apply the offset and center and flip Y
	return Vector2(realScaled.x + center.x - offset.x, -realScaled.y + center.y - offset.y);
}

template<class CType>
void GeometricPrimitive<CType>::drawAxes() {
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	// get the (0, 0) from real to raster coordinates
	const Vector2 center = realToRaster(Vector2(0.0f, 0.0f));
	// all draws should be done with the bmp.setPixel(..) function
	// since it has boundary checks
	const int cx = static_cast<int>(center.x);
	const int cy = static_cast<int>(center.y);
	// check if the center is in the raster
	bool centerInRaster = (cx >= 0 && cx < bw && cy >= 0 && cy < bh);
	const int subdivs = 3; //!< the number of subdivisions shown
	const int subdivLineLen = 9; //!< the length of the subdivision line
	// some constants for the text drawing
	const int textOffsetX = 3;
	const int textOffsetY = 2;
	char workStr[128] = "";
	const char format[] = "%.2f";
	// first draw some stuff on the horizontal axis (if it is in the raster)
	if (cy >= 0 && cy < bh) {
		bmp.drawAxisAlignedLine(0, bw - 1, cy, PA_X_AXIS, axisCol);
		const int step = bw / (subdivs + 1);
		std::vector<int> points;
		if (centerInRaster) {
			for (int i = 0; i < subdivs; ++i) {
				const int pointOffset = (i + 1) * step;
				if (bmp.validPixelPosition(cx + pointOffset, cy)) {
					points.push_back(cx + pointOffset);
				}
				if (bmp.validPixelPosition(cx - pointOffset, cy)) {
					points.push_back(cx - pointOffset);
				}
			}
		} else {
			// else just put the points that are on step size from the edge
			for (int i = 0; i < subdivs; ++i) {
				const int pointOffset = (i + 1) * step;
				points.push_back(pointOffset);
			}
		}
		const int halfLen = subdivLineLen / 2;
		// now draw a separator line for each subdivision
		for (int x : points) {
			bmp.drawAxisAlignedLine(cy - halfLen, cy + halfLen, x, PA_Y_AXIS, axisCol);
			// and now draw some text about the actual value
			const Vector2 rasterPos(x, cy);
			const Vector2 realPos = rasterToReal(rasterPos);
			snprintf(workStr, _countof(workStr), format, realPos.x);
			bmp.drawString(workStr, x + textOffsetX, cy + textOffsetY, axisCol);
		}
		// and finally draw some text and lines on the two ends (only if the center is not in the raster)
		bmp.drawAxisAlignedLine(cy - halfLen, cy + halfLen, bw - 1, PA_Y_AXIS, axisCol);
		const Vector2 rightReal = rasterToReal(Vector2(bw - 1, cy));
		snprintf(workStr, _countof(workStr), format, rightReal.x);
		// for the other end the text extent must be calculated first
		int tw = 0, th = 0;
		bmp.getTextExtent(workStr, tw, th);
		bmp.drawString(workStr, bw - tw - textOffsetX - 1, cy + textOffsetY, axisCol);
		if (!centerInRaster) {
			bmp.drawAxisAlignedLine(cy - halfLen, cy + halfLen, 0, PA_Y_AXIS, axisCol);
			const Vector2 leftReal = rasterToReal(Vector2(0, cy));
			snprintf(workStr, _countof(workStr), format, leftReal.x);
			bmp.drawString(workStr, textOffsetX, cy + textOffsetY, axisCol);
		}
	}
	// second draw some stuff on the vertical axis (if it is in the raster)
	if (cx >= 0 && cx < bw){
		bmp.drawAxisAlignedLine(0, bh - 1, cx, PA_Y_AXIS, axisCol);
		const int step = bh / (subdivs + 1);
		std::vector<int> points;
		if (centerInRaster) {
			for (int i = 0; i < subdivs; ++i) {
				const int pointOffset = (i + 1) * step;
				if (bmp.validPixelPosition(cx, cy + pointOffset)) {
					points.push_back(cy + pointOffset);
				}
				if (bmp.validPixelPosition(cx, cy - pointOffset)) {
					points.push_back(cy - pointOffset);
				}
			}
		} else {
			// else just put the points that are on step size from the edge
			for (int i = 0; i < subdivs; ++i) {
				const int pointOffset = (i + 1) * step;
				points.push_back(pointOffset);
			}
		}
		const int halfLen = subdivLineLen / 2;
		// now draw a separator line for each subdivision
		for (int y : points) {
			bmp.drawAxisAlignedLine(cx - halfLen, cx + halfLen, y, PA_X_AXIS, axisCol);
			// and now draw some text about the actual value
			const Vector2 rasterPos(cx, y);
			const Vector2 realPos = rasterToReal(rasterPos);
			snprintf(workStr, _countof(workStr), format, realPos.y);
			bmp.drawString(workStr, cx + textOffsetX, y + textOffsetY, axisCol);
		}
		// and finally draw some text and lines on the two ends (only if the center is not in the raster
		bmp.drawAxisAlignedLine(cx - halfLen, cx + halfLen, bh - 1, PA_X_AXIS, axisCol);
		const Vector2 rightReal = rasterToReal(Vector2(cx, bh - 1));
		snprintf(workStr, _countof(workStr), format, rightReal.y);
		// for the other end the text extent must be calculated first
		int tw = 0, th = 0;
		bmp.getTextExtent(workStr, tw, th);
		bmp.drawString(workStr, cx + textOffsetX, bh - th - textOffsetY - 1, axisCol);
		if (!centerInRaster) {
			bmp.drawAxisAlignedLine(cx - halfLen, cx + halfLen, 0, PA_X_AXIS, axisCol);
			const Vector2 leftReal = rasterToReal(Vector2(cx, 0));
			snprintf(workStr, _countof(workStr), format, leftReal.y);
			bmp.drawString(workStr, cx + textOffsetX, textOffsetY, axisCol);
		}
	}
	// finally draw the 0 indication in the center if it is in the raster
	if (centerInRaster) {
		bmp.drawString("0.0", cx + textOffsetX, cy + textOffsetY, axisCol);
	}
}

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
		drawAxes();
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
		drawAxes();
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
		drawAxes();
	}
	const double halfPenWidth = pen.getWidth() / 2.0 + 1.0;
	const double fullColorRange = clamp(pen.getStrength(), 0.0, 1.0) * halfPenWidth;
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
					double amount = clamp(1.0 - (error - fullColorRange) / halfToneRange, 0.0, 1.0);
					if (additive) {
						rasterData[y * bw + x] += static_cast<CType>(penColor * amount);
					} else {
						rasterData[y * bw + x] = static_cast<CType>(penColor * amount + currentColor * (1.0 - amount));
					}
				}
			}
		}
		if (cb) {
			if (cb->getAbortFlag()) {
				break;
			} else {
				cb->setPercentDone(y + 1, bh);
			}
		}
	}
}

template class FineFunctionRaster<Color>;

template<class CType>
FineFunctionRaster<CType>::FineFunctionRaster(int width, int height)
	: GeometricPrimitive<CType>(width, height)
{}

template<class CType>
void FineFunctionRaster<CType>::setFunction(std::function<double(double, double)> _fxy) {
	fxy = _fxy;
}

template<class CType>
void FineFunctionRaster<CType>::draw(unsigned flags) {
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
		drawAxes();
	}

	const float topQtDimension = static_cast<float>(std::max(bw, bh));
	// get the proper number of levels in order to have small number of points in the tree
	const int qtLevels = QuadTree<Vector2>::getLevels(5.0f, static_cast<float>(topQtDimension)) - 1;
	// quad tree containing all intersections
	QuadTree<Vector2> plotQt(qtLevels, topQtDimension / 2);

	const float epsIntersection = 1.0e-6f;
	// find all intersections
	for (int y = 0; y < bh - 1; ++y) {
		for (int x = 0; x < bw - 1; ++x) {
			const double baseX = x - cx + 0.5;
			const double baseY = y - cy + 0.5;
			const double evalBase = fxy(baseX, baseY);
			// if the base evaluation is close to 0 it is a direct intersection
			if (std::abs(evalBase) < epsIntersection) {
				const Vector2 intersection(static_cast<float>(baseX), static_cast<float>(baseY));
				plotQt.addElement(intersection, intersection);
			} else {
				// otherwise check for horizontal and vertical intersection
				const double nextX = baseX + 1.0;
				const double evalHoriz = fxy(nextX, baseY);
				// there is an intersection if the signs of the two evaluations differ
				const bool ix = (evalBase * evalHoriz) < 0.0;
				// if there is intersection - perform binary search to find the exact point
				if (ix) {
					double x0 = baseX;
					double x1 = nextX;
					double evalX0 = evalBase;
					double evalX1 = evalHoriz;
					double dx = (x0 + x1) * 0.5;
					double evalDx = fxy(dx, baseY);
					while (x1 - x0 > epsIntersection) {
						if (std::abs(evalDx) < epsIntersection) {
							// if there is a direct intersection - just break
							break;
						} else if (evalDx * evalX0 > 0.0) {
							// then if the new evaluation has the same sign as the lower bound - increase the lower bound
							x0 = dx;
							evalX0 = evalDx;
						} else {
							// otherwise the upper bound must be lowered
							x1 = dx;
							evalX1 = evalDx;
						}
						dx = (x0 + x1) * 0.5;
						evalDx = fxy(dx, baseY);
					}
					// now dx contains the midPoint of two close evaluations - use it directly
					const Vector2 intersection(static_cast<float>(dx), static_cast<float>(baseY));
					plotQt.addElement(intersection, intersection);
				}
				const double nextY = baseY + 1.0;
				const double evalVert = fxy(baseX, nextY);
				// there is an intersection if the signs of the two evaluations differ
				const bool iy = (evalBase * evalVert) < 0.0;
				if (iy) {
					double y0 = baseY;
					double y1 = nextY;
					double evalY0 = evalBase;
					double evalY1 = evalVert;
					double dy = (y0 + y1) * 0.5;
					double evalDy = fxy(baseX, dy);
					while (y1 - y0 > epsIntersection) {
						if (std::abs(evalDy) < epsIntersection) {
							// if there is a direct intersection - just break
							break;
						} else if (evalDy * evalY0 > 0.0) {
							// then if the new evaluation has the same sign as the lower bound - increase the lower bound
							y0 = dy;
							evalY0 = evalDy;
						} else {
							// otherwise the upper bound must be lowered
							y1 = dy;
							evalY1 = evalDy;
						}
						dy = (y0 + y1) * 0.5;
						evalDy = fxy(baseX, dy);
					}
					// now dy contains the midPoint of two close evaluations - use it directly
					const Vector2 intersection(static_cast<float>(baseX), static_cast<float>(dy));
					plotQt.addElement(intersection, intersection);
				}
			}
		}
		if (cb) {
			if (cb->getAbortFlag()) {
				break;
			} else {
				cb->setPercentDone(y, bh * 2);
			}
		}
	}

	if (cb && cb->getAbortFlag()) {
		return;
	}

	const CType penColor = pen.getColor();
	if (treeOutput) {
		std::vector<const QuadTree<Vector2>*> intersections;
		plotQt.getBottomTrees(intersections);
		for (const auto& iqt : intersections) {
			for (const auto& qte : *(iqt->getTreeElements())) {
				const int dx = static_cast<int>(std::floor(qte.position.x)) + cx;
				const int dy = static_cast<int>(std::floor(-qte.position.y)) + cy;
				if (dx >= 0 && dy >= 0 && dx < bw && dy < bh) {
					rasterData[dy * bw + dx] = (additive ? rasterData[dy * bw + dx] + penColor : penColor);
				}
			}
			if (cb && cb->getAbortFlag()) {
				break;
			}
		}
		if (cb && !cb->getAbortFlag()) {
			cb->setPercentDone(1, 1);
		}
	} else {
		// now extract the neighbouring intersection point for each pixel in the raster based on the pen width
		const float halfPenWidth = static_cast<float>(pen.getWidth() / 2.0) + 1.0f; // add one to properly color edging pixels
		const float fullColorRange = static_cast<float>(clamp(pen.getStrength(), 0.0, 1.0) * halfPenWidth);
		const float halfToneRange = halfPenWidth - fullColorRange;
		std::vector<QuadTree<Vector2>::QuadTreeElement> intersections;
		const Vector2 penHalfSize(halfPenWidth + 1.0f, halfPenWidth + 1.0f); // add one to have correct results for small pens
		for (int y = 0; y < bh; ++y) {
			for (int x = 0; x < bw; ++x) {
				const Vector2 samplePos(x - cx + 0.5f, y - cy + 0.5f);
				const Rect pixelBBox(samplePos - penHalfSize, samplePos + penHalfSize);
				intersections.clear();
				plotQt.getElements(intersections, pixelBBox);
				// now find the point closest to the current pixel
				bool found = false; // may remain false if the elements are empty
				float closestSquareDist = penHalfSize.lengthSqr(); // use square distance, it is enough for finding the closest
				for (const auto& qte : intersections) {
					const Vector2 vecToIntersect = qte.position - samplePos;
					const float ptSquareDist = vecToIntersect.lengthSqr();
					if (ptSquareDist <= closestSquareDist) {
						found = true;
						closestSquareDist = ptSquareDist;
					}
				}
				// now if a point was found calculate the actual distance and the coloring of the pixel
				if (found) {
					const int ay = bh - y - 1; // recalculate y, because the coordinate system is mirrored
					CType& rdata = rasterData[ay * bw + x];
					const float dist = sqrtf(closestSquareDist);
					if (dist <= fullColorRange) {
						rdata = (additive ? rdata + penColor : penColor);
					} else {
						double amount = clamp(1.0 - (dist - fullColorRange) / halfToneRange, 0.0, 1.0);
						if (additive) {
							rdata += static_cast<CType>(penColor * amount);
						} else {
							rdata = static_cast<CType>(penColor * amount + rdata * (1.0 - amount));
						}
					}
				}
			}
			if (cb) {
				if (cb->getAbortFlag()) {
					break;
				} else {
					cb->setPercentDone(y + bh, bh * 2);
				}
			}
		}
		if (cb && !cb->getAbortFlag()) {
			cb->setPercentDone(1, 1);
		}
	}
}
