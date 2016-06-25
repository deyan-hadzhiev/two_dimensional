#ifndef __FFT_BUTTERFLY_H__
#define __FFT_BUTTERFLY_H__

#include <complex>
#include <vector>

class ButterflyFFT {
	typedef std::complex<double> Complex;
public:
	ButterflyFFT(const int _nfft, bool _inverse);

	// transforms a 1D function to the signal domain
	void transform(const Complex * src, Complex * dest);
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

#endif // __FFT_BUTTERFLY_H__
