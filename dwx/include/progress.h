#ifndef __PROGRESS_H__
#define __PROGRESS_H__

#include <string>
#include <atomic>
#include <mutex>

#include "util.h"

class ProgressCallback {
	std::atomic<bool> abortFlag;
	std::atomic<int> fractionsDone;
	std::atomic<int> fractionMax;
	std::atomic<int64> duration;
	mutable std::mutex nameMutex;
	std::string moduleName;
public:
	ProgressCallback()
		: abortFlag(false)
		, fractionsDone(0)
		, fractionMax(1)
		, duration(0)
		, moduleName("None")
	{}

	void setAbortFlag() noexcept {
		abortFlag = true;
	}

	bool getAbortFlag() const noexcept {
		return abortFlag;
	}

	void setPercentDone(int _fractionsDone, int _fractionMax) noexcept {
		fractionsDone = _fractionsDone;
		fractionMax = _fractionMax;
	}

	float getPercentDone() const noexcept {
		return float(fractionsDone * 100) / fractionMax;
	}

	void setDuration(int64 _duration) noexcept {
		duration = _duration;
	}

	int64 getDuration() const noexcept {
		return duration;
	}

	void setModuleName(const std::string& _moduleName) {
		std::unique_lock<std::mutex> a(nameMutex);
		moduleName = _moduleName;
	}

	std::string getModuleName() const {
		std::unique_lock<std::mutex> a(nameMutex);
		return moduleName;
	}

	void reset() {
		abortFlag = false;
		fractionsDone = 0;
		fractionMax = 1;
		duration = 0;
		moduleName = std::string("None");
	}
};


#endif // __PROGRESS_H__
