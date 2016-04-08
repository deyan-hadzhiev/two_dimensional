#include "util.h"

std::vector<std::string> splitString(const char *str, char c) {
	if (!str) {
		return std::vector<std::string>();
	}
	std::vector<std::string> result;

	do {
		const char *begin = str;
		while (*str != c && *str)
			str++;
		result.push_back(std::string(begin, str));
	} while (0 != *str++);
	return result;
}

template std::vector<uint32> convolute<uint32>(const std::vector<uint32>& input, std::vector<float> vec);
template std::vector<float> convolute<float>(const std::vector<float>& input, std::vector<float> vec);

template<class T>
std::vector<T> convolute(const std::vector<T>& input, std::vector<float> vec) {
	float vecSum = 0.0f;
	const int k = vec.size();
	const float eps = 0.0005f;
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
	const T under = input[0];
	const T over = input[inCount - 1];
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
