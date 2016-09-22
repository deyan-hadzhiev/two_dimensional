#ifndef __MODULE_BASE_H__
#define __MODULE_BASE_H__

#include <string>

#include "param_base.h"
#include "color.h"
#include "bitmap.h"
#include "convolution.h"
#include "vector2.h"

class InputManager;
class OutputManager;
class ParamManager;
class ProgressCallback;

class ModuleBase {
protected:
	unsigned flags;
	ProgressCallback * cb;
public:
	ModuleBase()
		: flags(0U)
		, cb(nullptr)
	{}

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
		KPR_ABORTED,
	};

	// starts the module process (still havent thought of decent flags, but multithreaded is a candidate :)
	virtual ProcessResult runModule(unsigned flags) = 0;

	// interface for checking the status of processing
	virtual int percentDone() {
		return -1;
	}
};

class NullModule : public ModuleBase {
	virtual ModuleBase::ProcessResult runModule(unsigned) override {
		return ModuleBase::KPR_NO_IMPLEMENTATION;
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

#endif // __MODULE_BASE_H__
