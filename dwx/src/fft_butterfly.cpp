#include "fft_butterfly.h"

ButterflyFFT::ButterflyFFT(const int _nfft, bool _inverse)
	: nfft(_nfft)
	, inverse(_inverse)
{
	prepare();
}

void ButterflyFFT::transform(const Complex * src, Complex * dest) const {
	work(0, dest, src, 1, 1);
}

void ButterflyFFT::transform(const Complex * src, Complex * dest, const int inStride) const {
	work(0, dest, src, 1, inStride);
}

void ButterflyFFT::prepare() {
	// fill the twiddles first
	twiddles.resize(nfft);
	const double phinc = ((inverse ? 2 : -2) * acos(double(-1.0))) / nfft;
	for (int i = 0; i < nfft; ++i) {
		twiddles[i] = std::exp(Complex(0.0, i * phinc));
	}

	// now prepare the radixes and remainders
	int n = nfft;
	int p = 4;
	do {
		while (n % p) {
			switch (p) {
			case 4: p = 2; break;
			case 2: p = 3; break;
			default: p += 2; break;
			}
			if (p * p > n)
				p = n; // no more factors
		}
		n /= p;
		stageRadix.push_back(p);
		stageRemainder.push_back(n);
	} while (n > 1);
}

template<>
void ButterflyFFT::butterfly<2>(Complex * fOut, const int fStride, const int m) const {
	for (int k = 0; k < m; ++k) {
		const Complex t = fOut[m + k] * twiddles[k * fStride];
		fOut[m + k] = fOut[k] - t;
		fOut[k] += t;
	}
}

template<>
void ButterflyFFT::butterfly<4>(Complex * fOut, const int fStride, const int m) const {
	Complex scratch[6];
	const int sign = (inverse ? -1 : 1);
	for (int k = 0; k < m; ++k) {
		scratch[0] = fOut[k + m] * twiddles[k * fStride];
		scratch[1] = fOut[k + 2 * m] * twiddles[k * fStride * 2];
		scratch[2] = fOut[k + 3 * m] * twiddles[k * fStride * 3];
		scratch[5] = fOut[k] - scratch[1];

		fOut[k] += scratch[1];
		scratch[3] = scratch[0] + scratch[2];
		scratch[4] = scratch[0] - scratch[2];
		scratch[4] = Complex(scratch[4].imag() * sign, -scratch[4].real() * sign);

		fOut[k + 2 * m] = fOut[k] - scratch[3];
		fOut[k] += scratch[3];
		fOut[k + m] = scratch[5] + scratch[4];
		fOut[k + 3 * m] = scratch[5] - scratch[4];
	}
}

template<>
void ButterflyFFT::butterfly<3>(Complex * fOut, const int fStride, const int m) const {
	const int m2 = 2 * m;
	const Complex * tw1 = nullptr;
	const Complex * tw2 = nullptr;
	Complex scratch[4];
	const Complex epi3 = twiddles[fStride * m];
	tw1 = tw2 = &twiddles[0];
	for (int k = m; k > 0; --k) {
		scratch[1] = fOut[m] * (*tw1);
		scratch[2] = fOut[m2] * (*tw2);
		scratch[3] = scratch[1] + scratch[2];
		scratch[0] = scratch[1] - scratch[2];
		tw1 += fStride;
		tw2 += fStride * 2;

		fOut[m] = Complex(fOut->real() - 0.5 * scratch[3].real(), fOut->imag() - 0.5 * scratch[3].imag());
		scratch[0] *= epi3.imag();
		*fOut += scratch[3];
		fOut[m2] = Complex(fOut[m].real() + scratch[0].imag(), fOut[m].imag() - scratch[0].real());
		fOut[m] += Complex(-scratch[0].imag(), scratch[0].real());
		++fOut;
	}
}

template<>
void ButterflyFFT::butterfly<5>(Complex * fOut, const int fStride, const int m) const {
	const Complex tw1 = twiddles[fStride * m];
	const Complex tw2 = twiddles[fStride * m * 2];
	Complex scratch[13];
	Complex * fOutM[5] = { nullptr };
	for (int i = 0; i < _countof(fOutM); ++i) {
		fOutM[i] = fOut + i * m;
	}
	for (int k = 0; k < m; ++k) {
		scratch[0] = *fOutM[0];

		scratch[1] = *fOutM[1] * twiddles[k * fStride];
		scratch[2] = *fOutM[2] * twiddles[k * fStride * 2];
		scratch[3] = *fOutM[3] * twiddles[k * fStride * 3];
		scratch[4] = *fOutM[4] * twiddles[k * fStride * 4];

		scratch[7] = scratch[1] + scratch[4];
		scratch[10] = scratch[1] - scratch[4];
		scratch[8] = scratch[2] + scratch[3];
		scratch[9] = scratch[2] - scratch[3];

		*fOutM[0] += scratch[7] + scratch[8];

		scratch[5] = scratch[0] + Complex(
			scratch[7].real() * tw1.real() + scratch[8].real() * tw2.real(),
			scratch[7].imag() * tw1.real() + scratch[8].imag() * tw2.real()
		);

		scratch[6] = Complex(
			scratch[10].imag() * tw1.imag() + scratch[9].imag() * tw2.imag(),
			-scratch[10].real() * tw1.imag() - scratch[9].real() * tw2.imag()
		);

		*fOutM[1] = scratch[5] - scratch[6];
		*fOutM[4] = scratch[5] + scratch[6];

		scratch[11] = scratch[0] + Complex(
			scratch[7].real() * tw2.real() + scratch[8].real() * tw1.real(),
			scratch[7].imag() * tw2.real() + scratch[8].imag() * tw1.real()
		);

		scratch[12] = Complex(
			-scratch[10].imag() * tw2.imag() + scratch[9].imag() * tw1.imag(),
			scratch[10].real() * tw2.imag() - scratch[9].real() * tw1.imag()
		);

		*fOutM[2] = scratch[11] + scratch[12];
		*fOutM[3] = scratch[11] - scratch[12];

		for (int i = 0; i < _countof(fOutM); ++i) {
			++fOutM[i];
		}
	}
}

void ButterflyFFT::butterflyGeneric(Complex * fOut, const int fStride, const int m, const int p) const {
	// perform the buttefly for one stage of a mixed radix FFT
	std::vector<Complex> scratch;
	scratch.resize(p);
	for (int u = 0; u < m; ++u) {
		for (int q = 0, k = u; q < p; ++q, k += m) {
			scratch[q] = fOut[k];
		}
		for (int q = 0, k = u; q < p; ++q, k += m) {
			fOut[k] = scratch[0];
			int twidx = 0;
			for (int q1 = 1; q1 < p; ++q1) {
				twidx += fStride * k;
				if (twidx >= nfft) {
					twidx -= nfft;
				}
				fOut[k] += scratch[q1] * twiddles[twidx];
			}
		}
	}
}

void ButterflyFFT::work(const int stage, Complex * fOut, const Complex * f, const int fStride, const int inStride) const {
	const int p = stageRadix[stage];
	const int m = stageRemainder[stage];
	Complex * fOutBegin = fOut;
	const Complex * fOutEnd = fOut + p * m;
	if (1 == m) {
		do {
			*fOut = *f;
			f += fStride * inStride;
		} while (++fOut != fOutEnd);
	} else {
		do {
			// recursive call:
			// DFT of size m*p performed by doing
			// p instances of smaller DFTs of size m,
			// each one takes a decimated version of the input
			work(stage + 1, fOut, f, fStride * p, inStride);
			f += fStride * inStride;
		} while ((fOut += m) != fOutEnd);
	}

	fOut = fOutBegin;

	switch (p) {
	case 2: butterfly<2>(fOut, fStride, m); break;
	case 3: butterfly<3>(fOut, fStride, m); break;
	case 4: butterfly<4>(fOut, fStride, m); break;
	case 5: butterfly<5>(fOut, fStride, m); break;
	default: butterflyGeneric(fOut, fStride, m, p); break;
	}
}
