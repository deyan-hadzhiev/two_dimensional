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
		} else if (i - hk < 0) {
			for (int v = 0; v < k; ++v) {
				const int si = i - hk + v;
				conv += (si < 0 ? under : input[si]) * vec[v];
			}
		} else {
			for (int v = 0; v < k; ++v) {
				const int ei = i - hk + v;
				conv += (ei >= inCount ? over : input[ei]) * vec[v];
			}
		}
		result[i] = static_cast<T>(conv);
	}
	return result;
}
