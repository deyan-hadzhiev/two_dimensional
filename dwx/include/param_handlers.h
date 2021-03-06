#ifndef __PARAM_HANDLERS_H__
#define __PARAM_HANDLERS_H__

#include "param_base.h"

class AspectRatioHandler : public ParamChangeHandler {
	bool aspectOn;
	float aspectRatio;
public:
	AspectRatioHandler(bool _aspectOn, float _aspectRatio)
		: aspectOn(_aspectOn)
		, aspectRatio(_aspectRatio)
	{}

	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const int& value) override;
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const bool& value) override;
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const float& value) override;

	virtual void onImageChange(ParamManager * pman, int width, int height) override;
};

class RandomNoiseChangeHandler : public ParamChangeHandler {
	unsigned randGen;
	unsigned distribution;
public:
	RandomNoiseChangeHandler(unsigned _randGen, unsigned _distribution)
		: randGen(_randGen)
		, distribution(_distribution)
	{}

	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const unsigned& value) override;
};

#endif // __PARAM_HANDLERS_H__
