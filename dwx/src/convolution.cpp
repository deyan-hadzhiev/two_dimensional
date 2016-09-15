#include "convolution.h"
#include "bitmap.h"
#include "vector2.h"
#include "module_base.h"
#include "progress.h"

ConvolutionKernel::ConvolutionKernel()
	: data(nullptr)
	, side(0)
{}

ConvolutionKernel::ConvolutionKernel(int _side)
	: data(new float[_side * _side])
	, side(_side)
{}

ConvolutionKernel::ConvolutionKernel(const float * mat, int _side)
	: data(nullptr)
	, side(_side)
{
	init(mat, _side);
}

ConvolutionKernel::~ConvolutionKernel() {
	freeMem();
}

ConvolutionKernel::ConvolutionKernel(const ConvolutionKernel & rhs)
	: data(nullptr)
	, side(rhs.side)
{
	init(rhs.data, rhs.side);
}

ConvolutionKernel & ConvolutionKernel::operator=(const ConvolutionKernel & rhs) {
	if (this != &rhs) {
		init(rhs.data, rhs.side);
	}
	return *this;
}

void ConvolutionKernel::init(const float * mat, int _side) {
	if (_side <= 0)
		return;
	if (side != _side) {
		freeMem();
	}
	if (!data) {
		data = new float[_side * _side];
	}
	side = _side;
	if (mat) {
		memcpy(data, mat, side * side * sizeof(float));
	} else {
		memset(data, 0, side * side * sizeof(float));
	}
}

void ConvolutionKernel::setRadialSection(const float * srcData, int size) {
	if (!srcData || size <= 0) {
		return;
	}
	const int matSide = size * 2 - 1;
	init(nullptr, matSide);
	// this equals size - 1, but this is more readable
	const int halfSide = side / 2;
	// first copy the data on the positive y axis and rotate it
	for (int i = 0; i <= halfSide; ++i) {
		const int idx = pointToIndex(Point(0, i));
		data[idx] = srcData[i];
		// after the initial value is set - rotate the value
		applyRadialSymmetry(idx);
	}
	// now recalculate all the values in the 1 quadrant in the matrix coord system
	for (int y = 1; y <= halfSide; ++y) {
		for (int x = 1; x <= halfSide; ++x) {
			const int posIdx = pointToIndex(Point(x, y));
			const Vector2 posVec(static_cast<float>(x), static_cast<float>(y));
			const float length = posVec.length();
			// determining the actual value based on the position of the sampled point
			// and where it would it be mapped on one of the cetral axes
			const int floorLength = static_cast<int>(floorf(length));
			const int ceilLength = static_cast<int>(ceilf(length));
			float value = 0.0f;
			if (floorLength > halfSide) {
				// the point maps outside the circle - zero value
				value = 0.0f;
			} else if (ceilLength > halfSide) {
				// the point is right on the edge - so get interpolated value with the
				// fraction that is part of the circle
				const int centralIdx = pointToIndex(Point(0, floorLength));
				// note that floorLength - length is in the range of (-1.0, 0.0]
				value = data[centralIdx] * (1.0f + floorLength - length);
			} else {
				// otherwise interpolate over the two values that are closest on the central axis
				const int floorIdx = pointToIndex(Point(0, floorLength));
				const float floorValue = data[floorIdx];
				const int ceilIdx = pointToIndex(Point(0, ceilLength));
				const float ceilValue = data[ceilIdx];
				// calculate the interpolation
				const float t = (length - floorLength);
				value = ceilValue * t + floorValue * (1.0f - t);
			}
			data[posIdx] = value;
			// update the radially symmetrical spaces
			applyRadialSymmetry(posIdx);
		}
	}
}

void ConvolutionKernel::normalize(const float targetSum) {
	if (!data || side <= 0)
		return;
	float negSum = 0.0f;
	float posSum = 0.0f;
	const int kn = side * side;
	for (int i = 0; i < kn; ++i) {
		const float fi = data[i];
		if (fi > 0.0f) {
			posSum += fi;
		} else {
			negSum += fi;
		}
	}
	const float sum = posSum + negSum;
	const float absSum = posSum - negSum;
	if (fabs(sum - targetSum) > Eps) {
		if (fabs(targetSum) < Eps) {
			// this is to assert for NaNs and also there is no way other than making all zero
			if (posSum > Eps && negSum < -Eps) {
				// normalize the positive and negative separately to 1 and -1 respectively
				const float posFactor = 1.0f / posSum;
				const float negFactor = -1.0f / negSum;
				for (int i = 0; i < kn; ++i) {
					if (data[i] > 0.0f) {
						data[i] *= posFactor;
					} else {
						data[i] *= negFactor;
					}
				}
			}
		} else {
			if (sum * targetSum > Eps) {
				// if the current sum and target sums have equal signs - just scale the parameters
				const float factor = targetSum / sum;
				for (int i = 0; i < kn; ++i) {
					data[i] *= factor;
				}
			} else if (posSum > Eps && negSum < -Eps) {
				// this is only possible when both the positive and negative sums are not 0
				// scale both to 1 and -1
				const float posFactor = 1.0f / posSum;
				const float negFactor = -1.0f / negSum;
				for (int i = 0; i < kn; ++i) {
					if (data[i] > Eps) {
						data[i] *= posFactor;
					} else if (data[i] < -Eps) {
						data[i] *= negFactor;
					}
				}
				// and afterfwards scale only the ones that are required
				if (targetSum > Eps) {
					// increase the positive sum
					const float scaleFactor = targetSum + 1.0f;
					for (int i = 0; i < kn; ++i) {
						if (data[i] > Eps) {
							data[i] *= scaleFactor;
						}
					}
				} else if (targetSum < Eps) {
					const float scaleFactor = -targetSum - 1.0f;
					for (int i = 0; i < kn; ++i) {
						if (data[i] < -Eps) {
							data[i] *= scaleFactor;
						}
					}
				}
			}
		}
	}
}

int ConvolutionKernel::getSide() const {
	return side;
}

float * ConvolutionKernel::getDataPtr() {
	return data;
}

const float * ConvolutionKernel::getDataPtr() const {
	return data;
}

float * ConvolutionKernel::operator[](int i) {
	return data + i * side;
}

const float * ConvolutionKernel::operator[](int i) const {
	return data + i * side;
}

Point ConvolutionKernel::indexToPoint(int i) const {
	const int c = side / 2;
	return Point((i % side) - c, (i / side) - c);
}

int ConvolutionKernel::pointToIndex(const Point & p) const {
	const int c = side / 2;
	return (p.y + c) * side + p.x + c;
}

void ConvolutionKernel::applyRadialSymmetry(int idx) {
	const float value = data[idx];
	Point p = indexToPoint(idx);
	// (0, 0) does not need rotation
	if (0 != p.x || 0 != p.y) {
		for (int i = 0; i < 3; ++i) {
			// rotate the point by Pi / 2 around (0, 0)
			p = Point(-p.y, p.x);
			const int destIdx = pointToIndex(p);
			data[destIdx] = value;
		}
	}
}

void ConvolutionKernel::freeMem() {
	if (data)
		delete[] data;
	data = nullptr;
	side = 0;
}

template std::vector<uint32> convolute(const std::vector<uint32>& input, std::vector<float> vec);
template std::vector<float> convolute(const std::vector<float>& input, std::vector<float> vec);

template<class T>
std::vector<T> convolute(const std::vector<T>& input, std::vector<float> vec) {
	float vecSum = 0.0f;
	const int k = static_cast<int>(vec.size());
	const float eps = Eps;
	for (int i = 0; i < k; ++i) {
		vecSum += vec[i];
	}
	if (fabs(vecSum - 1.0f) > eps) {
		const float factor = 1.0f / vecSum;
		for (int i = 0; i < k; ++i) {
			vec[i] = vec[i] * factor;
		}
	}
	// now to the actual convolution
	const int inCount = static_cast<int>(input.size());
	std::vector<T> result(inCount);
	const int hk = k / 2;
	const int hke = k & 1;
	//const T under = input[0];
	//const T over = input[inCount - 1];
	for (int i = 0; i < inCount; ++i) {
		double conv = 0;
		if (i - hk >= 0 && i + hk + hke < inCount) {
			for (int v = 0; v < k; ++v) {
				conv += input[i - hk + v] * vec[v];
			}
		} else {
			for (int v = 0; v < k; ++v) {
				const int si = i - hk + v;
				conv += (si < 0 || si >= inCount ? 0 : input[si] * vec[v]);
			}
		}
		result[i] = static_cast<T>(conv);
	}
	return result;
}

template Pixelmap<Color> convolute(const Pixelmap<Color>& in, const ConvolutionKernel & _k, const bool normalize, const float normalizationValue, ProgressCallback * cb);

template<class ColorType>
Pixelmap<ColorType> convolute(const Pixelmap<ColorType>& _in, const ConvolutionKernel & _k, const bool normalize, const float normalizationValue, ProgressCallback * cb) {
	// this may be increased to int64 if necessary, but for now even int16 is an option
	Pixelmap<TColor<int32> > in(_in);
	Pixelmap<TColor<int32> > out(in.getWidth(), in.getHeight());
	ConvolutionKernel k(_k);
	if (normalize) {
		k.normalize(normalizationValue);
	}
	const int w = in.getWidth();
	const int h = in.getHeight();
	const int ks = k.getSide();
	const int hs = ks / 2;
	for (int y = 0; y < h && (!cb || !cb->getAbortFlag()); ++y) {
		for (int x = 0; x < w; ++x) {
			TColor<int32> res;
			if (y - hs >= 0 && x - hs >= 0 && y + hs < h && x + hs < w) {
				for (int yk = 0; yk < ks; ++yk) {
					for (int xk = 0; xk < ks; ++xk) {
						res += (in[y - hs + yk][x - hs + xk] * k[yk][xk]);
					}
				}
			} else {
				for (int yk = 0; yk < ks; ++yk) {
					for (int xk = 0; xk < ks; ++xk) {
						const int cyk = clamp(y - hs + yk, 0, h - 1);
						const int cxk = clamp(x - hs + xk, 0, w - 1);
						res += (in[cyk][cxk] * k[yk][xk]);
					}
				}
			}
			out[y][x] = res;
		}
		if (cb)
			cb->setPercentDone(y, h);
	}
	if (cb)
		cb->setPercentDone(1, 1);

	return Pixelmap<ColorType>(out);
}

template std::vector<Extremum> findExtremums<uint32>(const std::vector<uint32>& input);
template std::vector<Extremum> findExtremums<float>(const std::vector<float>& input);

template<class T>
std::vector<Extremum> findExtremums(const std::vector<T>& input) {
	//typedef std::conditional<std::is_floating_point<T>::value, T, std::make_signed<T>::type >::type ST;
	typedef int ST;
	std::vector<Extremum> result;
	const int inCount = static_cast<int>(input.size());
	int lastExtStart = 0;
	ST lastDx = 0;
	for (int x = 0; x < inCount - 1; ++x) {
		const ST dx = static_cast<ST>(input[x + 1]) - static_cast<ST>(input[x]);
		const ST mx = lastDx * dx;
		if (mx < 0) {
			// changed derivatives
			result.push_back({ lastExtStart + 1, x + 1, dx > 0 ? -1 : 1 });
			lastDx = dx;
			lastExtStart = x;
		} else if (mx > 0) {
			lastDx = dx;
			lastExtStart = x;
		} else if (lastDx == 0) {
			// zero do nothing on updates
			lastDx = dx;
		}
	}
	result.push_back({ lastExtStart, inCount - 1, lastDx > 0 ? 1 : -1 });
	return result;
}

