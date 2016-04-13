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
