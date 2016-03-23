#ifndef __KERNELS_H__
#define __KERNELS_H__

#include "bitmap.h"
#include "kernel_base.h"

// A simple kernel with a single image, input and output
class SimpleKernel : public KernelBase {
protected:
	bool getInput() {
		if (iman) {
			return iman->getInput(bmp, bmpId);
		}
		return false;
	}

	void setOutput() const {
		if (oman) {
			oman->setOutput(bmp, bmpId);
		}
	}

	int bmpId;
	Bitmap bmp;
	InputManager * iman;
	OutputManager * oman;
public:
	SimpleKernel()
		: bmpId(0)
		, iman(nullptr)
		, oman(nullptr)
	{}

	void addInputManager(InputManager * _iman) override final {
		iman = _iman;
	}

	void addOutputManager(OutputManager * _oman) override final {
		oman = _oman;
	}
};

class NegativeKernel : public SimpleKernel {
public:
	// starts the kernel process (still havent thought of decent flags, but multithreaded is a candidate :)
	KernelBase::ProcessResult runKernel(unsigned flags) override final;
};

#endif
