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

	// sets the convolution data by interpreting the array as a part of the
	// intersection of a conical function, effectively revolving around the
	// first element of the array
	void setRadialSection(const float * data, int size);

	void normalize(const float targetSum = 1.0f);

	int getSide() const;

	float * getDataPtr();
	const float * getDataPtr() const;

	float * operator[](int i);
	const float * operator[](int i) const;

	// conversion functions from the coordinnated system centered in the
	// middle of the convolution matrix to index of the data array and
	// vice versa
	Point indexToPoint(int i) const;
	// conversion functions from the coordinnated system centered in the
	// middle of the convolution matrix to index of the data array and
	// vice versa
	int pointToIndex(const Point& p) const;
private:

	// applies radial symmetry with PI/2 angle of the value with the
	// given index
	inline void applyRadialSymmetry(int idx);

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
