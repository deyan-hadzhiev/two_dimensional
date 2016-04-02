#ifndef __KERNEL_BASE_H__
#define __KERNEL_BASE_H__

#include <string>
#include <atomic>
#include <mutex>
#include "bitmap.h"

class InputManager;
class OutputManager;
class ParamManager;

class ProgressCallback {
	std::atomic<bool> abortFlag;
	std::atomic<int> fractionsDone;
	std::atomic<int> fractionMax;
	mutable std::mutex nameMutex;
	std::string kernelName;
public:
	ProgressCallback()
		: abortFlag(false)
		, fractionsDone(0)
		, fractionMax(1)
		, kernelName("None")
	{}

	void setAbortFlag() {
		abortFlag = true;
	}

	bool getAbortFlag() const {
		return abortFlag;
	}

	void setPercentDone(int _fractionsDone, int _fractionMax) {
		fractionsDone = _fractionsDone;
		fractionMax = _fractionMax;
	}

	float getPercentDone() const {
		return float(fractionsDone * 100) / fractionMax;
	}

	void setKernelName(const std::string& _kernelName) {
		std::unique_lock<std::mutex> a(nameMutex);
		kernelName = _kernelName;
	}

	std::string getKernelName() const {
		std::unique_lock<std::mutex> a(nameMutex);
		return kernelName;
	}

	void reset() {
		abortFlag = false;
		fractionsDone = 0;
		fractionMax = 1;
		kernelName = std::string("None");
	}
};

class KernelBase {
protected:
	unsigned flags;
	ProgressCallback * cb;
public:
	virtual ~KernelBase() {}
	// adds an input manager (note there may be more than one (probably))
	virtual void addInputManager(InputManager * iman) {}

	// adds an output manager (probably not more than one, but who knows...)
	virtual void addOutputManager(OutputManager * oman) {}

	// adds a parameter manager for getting user parameters for the kernel
	virtual void addParamManager(ParamManager * pman) {}

	virtual void setProgressCallback(ProgressCallback * _cb) {
		cb = _cb;
	}

	// forces an update of the kernel
	virtual void update() {
		runKernel(flags);
	}

	// sets kernel flags
	virtual void setFlags(unsigned _flags) {
		flags = _flags;
	}

	enum ProcessResult {
		KPR_OK = 0,
		KPR_RUNNING,
		KPR_INVALID_INPUT,
		KPR_NO_IMPLEMENTATION,
		KPR_FATAL_ERROR,
	};

	// starts the kernel process (still havent thought of decent flags, but multithreaded is a candidate :)
	virtual ProcessResult runKernel(unsigned flags) = 0;

	// interface for checking the status of processing
	virtual int percentDone() {
		return -1;
	}
};

class InputManager {
public:
	virtual ~InputManager() {}
	// provides a bitmap for processing
	virtual bool getInput(Bitmap& inputBmp, int& id) const = 0;

	// will be called immediately after the kernel is done
	virtual void kernelDone(KernelBase::ProcessResult result) {}
};

class OutputManager {
public:
	virtual ~OutputManager() {}
	// sets the provided bitmap and outputs it in a fine matter
	virtual void setOutput(const Bitmap& outputBmp, int id) = 0;
};

class ParamDescriptor {
public:
	enum class ParamType {
		PT_NONE = 0,
		PT_BOOL,
		PT_INT,
		PT_FLOAT,
		PT_STRING,
	} type;
	KernelBase * kernel;
	std::string name;
	ParamDescriptor(ParamType _type = ParamType::PT_NONE, KernelBase * _kernel = nullptr, const std::string& _name = std::string())
		: type(_type)
		, kernel(_kernel)
		, name(_name)
	{}
};

class ParamManager {
public:
	virtual ~ParamManager() {}
	// param getters - return true if the param is found
	virtual bool getStringParam(std::string& value, const std::string& paramName) const {
		return false;
	}

	virtual bool getIntParam(int& value, const std::string& paramName) const {
		return false;
	}

	virtual bool getFloatParam(float& value, const std::string& paramName) const {
		return false;
	}

	// adds a parameter to internal storage for type info and optionally updates to the kernel
	virtual void addParam(const ParamDescriptor& pd) {} // = 0;

	// sets an output paramter()
	virtual void setParam(const std::string& paramName, const std::string& paramValue) = 0;
};

#endif
