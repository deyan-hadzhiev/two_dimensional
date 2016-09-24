#ifndef __PROGRESS_H__
#define __PROGRESS_H__

#include <string>
#include <mutex>

#include "util.h"

class ProgressCallback {
	mutable bool dirty;
	bool abortFlag;
	int64 fractionsDone;
	int64 fractionMax;
	int64 duration;
	mutable std::mutex nameMutex;
	std::string moduleName;
public:
	ProgressCallback()
		: dirty(true)
		, abortFlag(false)
		, fractionsDone(0)
		, fractionMax(1)
		, duration(0)
		, moduleName("None")
	{}

	inline void setAbortFlag() noexcept {
		abortFlag = true;
	}

	inline bool getAbortFlag() const noexcept {
		return abortFlag;
	}

	inline void setPercentDone(int64 _fractionsDone, int64 _fractionMax) noexcept {
		fractionsDone = _fractionsDone;
		fractionMax = _fractionMax;
		dirty = true;
	}

	inline float getPercentDone() const noexcept {
		dirty = false;
		return static_cast<float>((fractionsDone * 10000) / fractionMax) / 100.0f;
	}

	inline float getFractionDone() const noexcept {
		dirty = false;
		return static_cast<float>(fractionsDone) / static_cast<float>(fractionMax);
	}

	inline bool getDirty() const noexcept {
		return dirty;
	}

	inline void setDuration(int64 _duration) noexcept {
		dirty = true;
		duration = _duration;
	}

	inline int64 getDuration() const noexcept {
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
		dirty = true;
		abortFlag = false;
		fractionsDone = 0;
		fractionMax = 1;
		duration = 0;
		moduleName = std::string("None");
	}
};

#endif // __PROGRESS_H__
