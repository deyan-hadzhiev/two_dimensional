#ifndef __FFT_BUTTERFLY_H__
#define __FFT_BUTTERFLY_H__

#include <complex>
#include <vector>

#include "util.h"

class ButterflyFFT {
public:
	ButterflyFFT(const int _nfft, bool _inverse);

	// transforms a 1D function to the signal domain
	void transform(const Complex * src, Complex * dest) const;

	// a more generic function using specified strides
	void transform(const Complex * src, Complex * dest, const int inStride) const;
private:
	// fills twiddles as a preparation for the work later
	void prepare();

	// will be specialized for 2, 3, 4 and 5 strides
	template<int n>
	void butterfly(Complex * fOut, const int fStride, const int m) const;

	// will be used for all m that are not divisible by 2,3,4 or 5
	void butterflyGeneric(Complex * fOut, const int fStride, const int m, const int p) const;

	// makes the actual calls to the buttefly functions and recursively calls itself for factored m
	void work(const int stage, Complex * fOut, const Complex * f, const int fStride, const int inStride) const;

	int nfft;
	bool inverse;
	std::vector<Complex> twiddles;
	std::vector<int> stageRadix;
	std::vector<int> stageRemainder;
};

template<int nd = 2>
class MultiDimFFT {
public:
	MultiDimFFT(const std::vector<int>& _dims, bool _inverse);
	~MultiDimFFT();
	MultiDimFFT(const MultiDimFFT&) = delete;
	MultiDimFFT& operator=(const MultiDimFFT&) = delete;

	void transform(const Complex * fIn, Complex * fOut) const;

private:
	int dimProd;
	const bool inverse;
	const std::vector<int> dims;
	const ButterflyFFT * fftConfig[nd];
	Complex * tmpBuf; //!< used for intemediate calculations
};

using FFT2D = MultiDimFFT<2>;

template<int nd>
MultiDimFFT<nd>::MultiDimFFT(const std::vector<int>& _dims, bool _inverse)
	: dimProd(1)
	, inverse(_inverse)
	, dims(_dims)
	, fftConfig{nullptr}
	, tmpBuf(nullptr)
{
	DASSERT(dims.size() >= nd);
	for (int i = 0; i < nd; ++i) {
		const int dim = dims[i];
		fftConfig[i] = new ButterflyFFT(dim, inverse);
		dimProd *= dim;
	}
	tmpBuf = new Complex[dimProd];
}

template<int nd>
MultiDimFFT<nd>::~MultiDimFFT() {
	for (int i = 0; i < nd; ++i) {
		delete fftConfig[i];
		fftConfig[i] = nullptr;
	}
	
	// delete the tmpBuf as well
	delete[] tmpBuf;
	tmpBuf = nullptr;
}

template<int nd>
inline void MultiDimFFT<nd>::transform(const Complex * fIn, Complex * fOut) const {
	const Complex * bufIn = fIn;
	Complex * bufOut = nullptr;
	// arrange it so that bufOut == fOut
	if (nd & 1) {
		bufOut = fOut;
		if (fIn == fOut) {
			memcpy(tmpBuf, fIn, dimProd * sizeof(Complex));
			bufIn = tmpBuf;
		}
	} else {
		bufOut = tmpBuf;
	}

	for (int d = 0; d < nd; ++d) {
		int currDim = dims[d];
		const int stride = dimProd / currDim;

		for (int i = 0; i < stride; ++i) {
			fftConfig[d]->transform(bufIn + i, bufOut + i * currDim, stride);
		}

		// toggle back and forth between the two buffers
		if (bufOut == tmpBuf) {
			bufOut = fOut;
			bufIn = tmpBuf;
		} else {
			bufOut = tmpBuf;
			bufIn = fOut;
		}
	}
}

#endif // __FFT_BUTTERFLY_H__
