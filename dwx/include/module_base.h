#ifndef __MODULE_BASE_H__
#define __MODULE_BASE_H__

#include <string>

#include "param_base.h"
#include "color.h"
#include "bitmap.h"

class InputManager;
class OutputManager;
class ParamManager;
class ProgressCallback;

using ModuleId = int; //!< unique module id set on allocation
enum {
	InvalidModuleId = -1,
};

class ModuleBase {
private:
	static ModuleId getNewModuleId() {
		static ModuleId id = 0;
		return id++;
	}
protected:
	const ModuleId moduleId;
	ProgressCallback * cb;
public:
	ModuleBase()
		: moduleId(getNewModuleId())
		, cb(nullptr)
	{}

	// returns the module id
	ModuleId getModuleId() const noexcept {
		return moduleId;
	}

	virtual ~ModuleBase() {}
	// adds an input manager (multiple inputs are handled by the manager itself)
	virtual void setInputManager(InputManager * iman) {}
	virtual InputManager * getInputManager() const { return nullptr; }

	// adds an output manager (always only one input -> mulitplying the result is the manager job)
	virtual void setOutputManager(OutputManager * oman) {}
	virtual OutputManager * getOutputManager() const { return nullptr; }

	// adds a parameter manager for getting user parameters for the module
	virtual void setParamManager(ParamManager * pman) {}
	// add a getter?

	virtual void setProgressCallback(ProgressCallback * _cb) {
		cb = _cb;
	}
	virtual ProgressCallback * getProgressCallback() const {
		return cb;
	}

	virtual std::string getName() const {
		return std::string("None");
	}

	// forces an update of the module
	virtual void update() {
		runModule();
	}

	enum ProcessResult {
		KPR_OK = 0,
		KPR_RUNNING,
		KPR_INVALID_INPUT,
		KPR_NO_IMPLEMENTATION,
		KPR_FATAL_ERROR,
		KPR_ABORTED,
	};

	// starts the module process
	virtual ProcessResult runModule() = 0;
};

class NullModule : public ModuleBase {
	virtual ModuleBase::ProcessResult runModule() override {
		return ModuleBase::KPR_NO_IMPLEMENTATION;
	}
};

class InputManager {
public:
	virtual ~InputManager() {}
	// provides a bitmap for processing - with the specified index - used by module with more than one input
	virtual bool getInput(Bitmap& inputBmp, int idx) const = 0;

	// will be called immediately after the module is done
	virtual void moduleDone(ModuleBase::ProcessResult result) {}
};

class OutputManager {
public:
	virtual ~OutputManager() {}
	// sets the provided bitmap and outputs it in a fine matter
	virtual void setOutput(const Bitmap& outputBmp, ModuleId mid) = 0;
};

#endif // __MODULE_BASE_H__
