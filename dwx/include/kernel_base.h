#ifndef __KERNEL_BASE_H__
#define __KERNEL_BASE_H__

#include <string>
#include "bitmap.h"

class InputManager;
class OutputManager;
class ParamManager;

class KernelBase {
public:
	virtual ~KernelBase() {}
	// adds an input manager (note there may be more than one (probably))
	virtual void addInputManager(InputManager * iman) {}

	// adds an output manager (probably not more than one, but who knows...)
	virtual void addOutputManager(OutputManager * oman) {}

	// adds a parameter manager for getting user parameters for the kernel
	virtual void addParamManager(ParamManager * pman) {}

	enum ProcessResult {
		KPR_OK = 0,
		KPR_INVALID_INPUT,
		KRP_NO_IMPLEMENTATION,
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

class ParamManager {
public:
	virtual ~ParamManager() {}
	// gets a paramter value - may return emtpy string if there is no such parameter
	virtual std::string getParam(const std::string& paramName) const {}

	// sets an output paramter()
	virtual void setParam(const std::string& paramName, const std::string& paramValue) {}
};

#endif
