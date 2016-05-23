#ifndef __MODULE_BASE_H__
#define __MODULE_BASE_H__

#include <string>
#include <atomic>
#include <mutex>

#include "bitmap.h"
#include "convolution.h"

class InputManager;
class OutputManager;
class ParamManager;

class ProgressCallback {
	std::atomic<bool> abortFlag;
	std::atomic<int> fractionsDone;
	std::atomic<int> fractionMax;
	mutable std::mutex nameMutex;
	std::string moduleName;
public:
	ProgressCallback()
		: abortFlag(false)
		, fractionsDone(0)
		, fractionMax(1)
		, moduleName("None")
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
		moduleName = std::string("None");
	}
};

class ModuleBase {
protected:
	unsigned flags;
	ProgressCallback * cb;
public:
	virtual ~ModuleBase() {}
	// adds an input manager (note there may be more than one (probably))
	virtual void addInputManager(InputManager * iman) {}

	// adds an output manager (probably not more than one, but who knows...)
	virtual void addOutputManager(OutputManager * oman) {}

	// adds a parameter manager for getting user parameters for the module
	virtual void addParamManager(ParamManager * pman) {}

	virtual void setProgressCallback(ProgressCallback * _cb) {
		cb = _cb;
	}

	virtual std::string getName() const {
		return std::string("None");
	}

	// forces an update of the module
	virtual void update() {
		runModule(flags);
	}

	// sets module flags
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

	// starts the module process (still havent thought of decent flags, but multithreaded is a candidate :)
	virtual ProcessResult runModule(unsigned flags) = 0;

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

	// will be called immediately after the module is done
	virtual void moduleDone(ModuleBase::ProcessResult result) {}
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
		PT_ENUM,
		PT_CKERNEL,
	} type;
	ModuleBase * module;
	std::string name;
	std::string defaultValue;
	ParamDescriptor(ModuleBase * _module = nullptr,
	                ParamType _type = ParamType::PT_NONE,
	                const std::string& _name = std::string(),
	                const std::string& _defaultValue = std::string("0"))
		: module(_module)
		, type(_type)
		, name(_name)
		, defaultValue(_defaultValue)
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

	virtual bool getBoolParam(bool& value, const std::string& paramName) const {
		return false;
	}

	virtual bool getCKernelParam(ConvolutionKernel& value, const std::string& paramName) const {
		return false;
	}

	virtual bool getEnumParam(unsigned& value, const std::string& paramName) const {
		return false;
	}

	// adds a parameter to internal storage for type info and optionally updates to the module
	virtual void addParam(const ParamDescriptor& pd) = 0;
};

#endif // __MODULE_BASE_H__
