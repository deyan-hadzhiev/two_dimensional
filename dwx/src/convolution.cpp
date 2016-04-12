#include "convolution.h"
#include "bitmap.h"

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
	}
}

void ConvolutionKernel::normalize() {
	if (!data || side <= 0)
		return;
	float sum = 0.0f;
	const int kn = side * side;
	for (int i = 0; i < kn; ++i) {
		sum += fabs(data[i]);
	}
	if (fabs(sum - 1.0f) > Eps) {
		const float factor = 1.0f / sum;
		for (int i = 0; i < kn; ++i) {
			data[i] *= factor;
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
	const int k = vec.size();
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
	const int inCount = input.size();
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

template Pixelmap<Color> convolute(const Pixelmap<Color>& in, const ConvolutionKernel & _k);

template<class ColorType>
Pixelmap<ColorType> convolute(const Pixelmap<ColorType>& _in, const ConvolutionKernel & _k) {
	// this may be increased to int64 if necessary, but for now even int16 is an option
	Pixelmap<TColor<int32> > in(_in);
	Pixelmap<TColor<int32> > out(in.getWidth(), in.getHeight());
	ConvolutionKernel k(_k);
	k.normalize();
	const int w = in.getWidth();
	const int h = in.getHeight();
	const int ks = k.getSide();
	const int hs = ks / 2;
	for (int y = 0; y < h; ++y) {
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
	}
	return Pixelmap<ColorType>(out);
}

template std::vector<Extremum> findExtremums<uint32>(const std::vector<uint32>& input);
template std::vector<Extremum> findExtremums<float>(const std::vector<float>& input);

template<class T>
std::vector<Extremum> findExtremums(const std::vector<T>& input) {
	//typedef std::conditional<std::is_floating_point<T>::value, T, std::make_signed<T>::type >::type ST;
	typedef int ST;
	std::vector<Extremum> result;
	const int inCount = input.size();
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

