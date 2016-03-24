#ifndef __KERNELS_H__
#define __KERNELS_H__

#include <string>
#include <vector>
#include <unordered_map>
#include "bitmap.h"
#include "kernel_base.h"

// A simple kernel with a single image, input and output
class SimpleKernel : public KernelBase {
protected:
	virtual bool getInput() {
		if (iman) {
			return iman->getInput(bmp, bmpId);
		}
		return false;
	}

	virtual void setOutput() const {
		if (oman) {
			oman->setOutput(bmp, bmpId);
		}
	}

	virtual void getParameters() {
		if (pman) {
			for (const auto& p : inputParams) {
				const std::string res = pman->getParam(p);
				if (!res.empty()) {
					paramValues[p] = res;
				}
			}
		}
	}

	int bmpId;
	Bitmap bmp;
	InputManager * iman;
	OutputManager * oman;
	ParamManager * pman;
	// setup expected paramters in the constructor and they will be iterated in runKernel(..) function before calling the impelementation
	std::vector<std::string> inputParams;
	std::unordered_map<std::string, std::string> paramValues;
public:
	SimpleKernel()
		: bmpId(0)
		, iman(nullptr)
		, oman(nullptr)
		, pman(nullptr)
	{}

	void addInputManager(InputManager * _iman) override final {
		iman = _iman;
	}

	void addOutputManager(OutputManager * _oman) override final {
		oman = _oman;
	}

	void addParamManager(ParamManager * _pman) override final {
		pman = _pman;
	}

	// a simple implementation that can be overriden and handles basic input output and calls the kernelImplementation function
	virtual KernelBase::ProcessResult runKernel(unsigned flags) override;

	// this function will be called from the runKernel(..) function and if not overriden will not do anything to the output image
	virtual KernelBase::ProcessResult kernelImplementation(unsigned flags) {
		return KernelBase::KRP_NO_IMPLEMENTATION;
	}
};

class NegativeKernel : public SimpleKernel {
public:
	// starts the kernel process (still havent thought of decent flags, but multithreaded is a candidate :)
	KernelBase::ProcessResult kernelImplementation(unsigned flags) override final;
};

#endif
