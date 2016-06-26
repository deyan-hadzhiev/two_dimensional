#ifndef __CONVOLUTION_H__
#define __CONVOLUTION_H__

#include <vector>
#include <memory>

#include "bitmap.h"

class ConvolutionKernel {
public:
	ConvolutionKernel();
	ConvolutionKernel(int side);
	ConvolutionKernel(const float * mat, int side);

	~ConvolutionKernel();

	ConvolutionKernel(const ConvolutionKernel&);
	ConvolutionKernel& operator=(const ConvolutionKernel&);

	void init(const float * mat, int side);

	void normalize(const float targetSum = 1.0f);

	int getSide() const;

	float * getDataPtr();
	const float * getDataPtr() const;

	float * operator[](int i);
	const float * operator[](int i) const;
private:
	void freeMem();

	float * data;
	int side;
};

template<class T>
std::vector<T> convolute(const std::vector<T>& input, std::vector<float> vec);

template<class ColorType>
Pixelmap<ColorType> convolute(const Pixelmap<ColorType>& in, const ConvolutionKernel& k, const bool normalize = true, const float normalizationValue = 1.0f);

struct Extremum {
	int start;
	int end;
	int d; //!< will be positive if the extremum is maximum and negative if it is minimum
};

template<class T>
std::vector<Extremum> findExtremums(const std::vector<T>& input);

#endif
