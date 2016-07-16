#ifndef __MODULES_H__
#define __MODULES_H__

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>

#include "bitmap.h"
#include "module_base.h"
#include "geom_primitive.h"

using ParamList = std::unordered_map<std::string, std::string>;

// TODO add a module as an input/output manager of another module to chain modules

// A simple module with a single image, input and output
class SimpleModule : public ModuleBase {
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
	// setup expected paramters in the constructor and they will be iterated in runModule(..) function before calling the impelementation
	std::vector<ParamDescriptor> paramList;
public:
	SimpleModule()
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

	// a simple implementation that can be overriden and handles basic input output and calls the moduleImplementation function
	virtual ModuleBase::ProcessResult runModule(unsigned flags) override;

	// this function will be called from the runModule(..) function and if not overriden will not do anything to the output image
	virtual ModuleBase::ProcessResult moduleImplementation(unsigned flags) {
		return ModuleBase::KPR_NO_IMPLEMENTATION;
	}
};

class AsyncModule : public SimpleModule {
public:
	enum class State {
		AKS_RUNNING = 0,
		AKS_INIT,
		AKS_FINISHED,
		AKS_DIRTY,
		AKS_TERMINATED,
	};
	AsyncModule();
	virtual ~AsyncModule();

	State getState() const;

	virtual void update() override;

	virtual ModuleBase::ProcessResult runModule(unsigned flags) override;
protected:
	std::atomic<State> state;
	std::mutex moduleMutex;
	std::condition_variable ev;
	std::thread loopThread;

	static void moduleLoop(AsyncModule * k);

	bool getAbortState() const;

	//!< override for all AsyncModules by default
	virtual void setOutput() const override {}
};

class IdentityModule : public SimpleModule {
public:
	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class NegativeModule : public SimpleModule {
public:
	// starts the module process (still havent thought of decent flags, but multithreaded is a candidate :)
	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

using IntervalList = std::vector<std::pair<int, int> >;
class TextSegmentationModule : public SimpleModule {
	static IntervalList extractIntervals(const int * accumValues, const int count, int threshold);
public:
	TextSegmentationModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "threshold"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class GeometricModule : public AsyncModule {
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
	GeometricModule(GeometricPrimitive<Color> * p, int _width = 256, int _height = 256)
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

	virtual ~GeometricModule() {
		delete primitive;
	}

	virtual void setSize(int width, int height);

	virtual void setColor(Color rgb);

	virtual ModuleBase::ProcessResult moduleImplementation(unsigned flags) override;
};

class SinosoidModule : public GeometricModule {
public:
	SinosoidModule();
};

class FunctionRasterModule : public AsyncModule {
public:
	FunctionRasterModule()
		: raster(new FunctionRaster<Color>)
		, width(1024)
		, height(1024)
	{
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_STRING, "function", "x + y"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_FLOAT, "penWidth", "1.0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_FLOAT, "penStrength", "0.0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "width", std::to_string(width)));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "height", std::to_string(height)));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_STRING, "color", std::string("ffffff")));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "additive", "false"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "clear", "false"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "axis", "false"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
private:
	std::unique_ptr<FunctionRaster<Color> > raster;
	int width;
	int height;
};

class HoughModule : public AsyncModule {
public:
	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class RotationModule : public AsyncModule {
public:
	RotationModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_FLOAT, "angle"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_ENUM, "edge", "blank;tile;stretch;mirror"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class HistogramModule : public AsyncModule {
public:
	HistogramModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "width", "512"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "height", "255"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "smooths", "1"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_STRING, "vector", "0.075;0.25;0.35;0.25;0.075"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
private:

	void drawIntensityHisto(Bitmap& histBmp, const std::vector<uint32>& data, const uint32 maxIntensity) const;
};

class ThresholdModule : public AsyncModule {
public:
	ThresholdModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "lower", "0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "upper", "255"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class FilterModule : public AsyncModule {
public:
	FilterModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_CKERNEL, "kernel", "3"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "normalize", "true"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_FLOAT, "normalValue", "1.0"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class DownScaleModule : public AsyncModule {
public:
	DownScaleModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "downscaleWidth", "128"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "downscaleHeight", "128"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_ENUM, "medium", "uint16;uint8;float;double"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class RelocateModule : public AsyncModule {
public:
	RelocateModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "relocateX", "0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "relocateY", "0"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class CropModule : public AsyncModule {
public:
	CropModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "cropX", "0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "cropY", "0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "cropWidth", "0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "cropHeight", "0"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class MirrorModule : public AsyncModule {
public:
	MirrorModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "mirrorX", "true"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "mirrorY", "true"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class ExpandModule : public	AsyncModule {
public:
	ExpandModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_ENUM, "edgeFill", "blank;tile;stretch;mirror"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "expandWidth", "0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "expandHeight", "0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "expandX", "0"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_INT, "expandY", "0"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class ChannelModule : public AsyncModule {
public:
	ChannelModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_ENUM, "channel", "red;green;blue"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class FFTDomainModule : public AsyncModule {
public:
	FFTDomainModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "logScale", "true"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "centralized", "true"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "squareDimension", "false"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "powerOf2", "false"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class FFTCompressionModule : public AsyncModule {
public:
	FFTCompressionModule() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_FLOAT, "compressPercent", "50.0"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

class FFTFilter : public AsyncModule {
public:
	FFTFilter() {
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_CKERNEL, "kernelFFT", "5"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_BOOL, "normalizeKernel", "true"));
		paramList.push_back(ParamDescriptor(this, ParamDescriptor::ParamType::PT_FLOAT, "normalizationValue", "1.0"));
	}

	ModuleBase::ProcessResult moduleImplementation(unsigned flags) override final;
};

#endif // __MODULES_H__
