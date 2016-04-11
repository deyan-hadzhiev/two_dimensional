#ifndef __KERNELS_H__
#define __KERNELS_H__

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include "bitmap.h"
#include "kernel_base.h"
#include "geom_primitive.h"

using ParamList = std::unordered_map<std::string, std::string>;

// TODO add a kernel as an input/output manager of another kernel to chain kernels

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

	int bmpId;
	Bitmap bmp;
	InputManager * iman;
	OutputManager * oman;
	ParamManager * pman;
	// setup expected paramters in the constructor and they will be iterated in runKernel(..) function before calling the impelementation
	std::vector<ParamDescriptor> paramList;
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
		for (auto pdesc : paramList) {
			pman->addParam(pdesc);
		}
	}

	// a simple implementation that can be overriden and handles basic input output and calls the kernelImplementation function
	virtual KernelBase::ProcessResult runKernel(unsigned flags) override;

	// this function will be called from the runKernel(..) function and if not overriden will not do anything to the output image
	virtual KernelBase::ProcessResult kernelImplementation(unsigned flags) {
		return KernelBase::KPR_NO_IMPLEMENTATION;
	}
};

class AsyncKernel : public SimpleKernel {
public:
	enum class State {
		AKS_RUNNING = 0,
		AKS_INIT,
		AKS_FINISHED,
		AKS_DIRTY,
		AKS_TERMINATED,
	};
	AsyncKernel();
	virtual ~AsyncKernel();

	State getState() const;

	virtual void update() override;

	virtual KernelBase::ProcessResult runKernel(unsigned flags) override;
protected:
	std::atomic<State> state;
	std::mutex kernelMutex;
	std::condition_variable ev;
	std::thread loopThread;

	static void kernelLoop(AsyncKernel * k);

	bool getAbortState() const;

	//!< override for all AsyncKernels by default
	virtual void setOutput() const override {}
};

class NegativeKernel : public SimpleKernel {
public:
	// starts the kernel process (still havent thought of decent flags, but multithreaded is a candidate :)
	KernelBase::ProcessResult kernelImplementation(unsigned flags) override final;
};

using IntervalList = std::vector<std::pair<int, int> >;
class TextSegmentationKernel : public SimpleKernel {
	static IntervalList extractIntervals(const int * accumValues, const int count, int threshold);
public:
	TextSegmentationKernel() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "threshold"));
	}

	KernelBase::ProcessResult kernelImplementation(unsigned flags) override final;
};

class GeometricKernel : public SimpleKernel {
protected:
	GeometricPrimitive<Color> * primitive;
	int width;
	int height;
	bool dirtySize;
	Color col;
	bool additive;
	bool clear;
	bool axis;
public:
	GeometricKernel(GeometricPrimitive<Color> * p, int _width = 256, int _height = 256)
		: primitive(p)
		, width(_width)
		, height(_height)
		, dirtySize(true)
		, col(255, 255, 255)
		, additive(false)
		, clear(false)
		, axis(false)
	{
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "width", std::to_string(width)));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "height", std::to_string(height)));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_STRING, "color", std::string("ffffff")));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "additive", "false"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "clear", "false"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "axis", "false"));
		const std::vector<std::string>& primitiveParamList = p->getParamList();
		for (const auto& param : primitiveParamList) {
			this->paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_STRING, param));
		}
	}

	virtual ~GeometricKernel() {
		delete primitive;
	}

	virtual void setSize(int width, int height);

	virtual void setColor(Color rgb);

	virtual KernelBase::ProcessResult runKernel(unsigned flags) override;
};

class SinosoidKernel : public GeometricKernel {
public:
	SinosoidKernel();
};

class HoughKernel : public AsyncKernel {
public:
	KernelBase::ProcessResult kernelImplementation(unsigned flags) override final;
};

class RotationKernel : public AsyncKernel {
public:
	RotationKernel() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_FLOAT, "angle"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "tile"));
	}

	KernelBase::ProcessResult kernelImplementation(unsigned flags) override final;
};

class HistogramKernel : public AsyncKernel {
public:
	HistogramKernel() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "width", "512"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "height", "255"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "smooths", "1"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_STRING, "vector", "0.075;0.25;0.35;0.25;0.075"));
	}

	KernelBase::ProcessResult kernelImplementation(unsigned flags) override final;
private:

	void drawIntensityHisto(Bitmap& histBmp, const std::vector<uint32>& data, const uint32 maxIntensity) const;
};

class ThresholdKernel : public AsyncKernel {
public:
	ThresholdKernel() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "lower", "0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "upper", "255"));
	}

	KernelBase::ProcessResult kernelImplementation(unsigned flags) override final;
};

class FilterKernel : public AsyncKernel {
public:
	FilterKernel() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_CKERNEL, "kernel", "3"));
	}

	KernelBase::ProcessResult kernelImplementation(unsigned flags) override final;
};

#endif
