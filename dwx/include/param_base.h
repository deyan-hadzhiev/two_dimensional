#ifndef __PARAM_BASE_H__
#define __PARAM_BASE_H__

#include <string>

#include "color.h"
#include "convolution.h"
#include "vector2.h"

class ModuleBase;
class ParamManager;

/**
* @brief Derivatives handle parameter changes
*/
class ParamChangeHandler {
public:
	virtual ~ParamChangeHandler() {}

	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const int& value) { return false; }
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const bool& value) { return false; }
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const int64& value) { return false; }
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const float& value) { return false; }
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const std::string& value) { return false; }
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const unsigned& value) { return false; }
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const ConvolutionKernel& value) { return false; }
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const Vector2& value) { return false; }
	virtual bool onParamChange(ParamManager * pman, const std::string& paramName, const Color& value) { return false; }

	virtual void onImageChange(ParamManager * pman, int width, int height) {}
};

class ParamDescriptor {
public:
	enum class ParamType {
		PT_NONE = 0,
		PT_BOOL,
		PT_INT,
		PT_INT64,
		PT_FLOAT,
		PT_STRING,
		PT_ENUM,
		PT_CKERNEL,
		PT_VECTOR,
		PT_BIG_STRING,
		PT_COLOR,
	} type;
	ModuleBase * module;
	std::string name;
	std::string defaultValue;
	ParamChangeHandler * changeHandler;
	bool enabled;
	ParamDescriptor(ModuleBase * _module = nullptr,
		ParamType _type = ParamType::PT_NONE,
		const std::string& _name = std::string(),
		const std::string& _defaultValue = std::string("0"),
		ParamChangeHandler * _changeHandler = nullptr,
		bool _enabled = true)
		: module(_module)
		, type(_type)
		, name(_name)
		, defaultValue(_defaultValue)
		, changeHandler(_changeHandler)
		, enabled(_enabled)
	{}
};

class ParamManager {
public:
	virtual ~ParamManager() {}

	virtual void setStringParam(const std::string& value, const std::string& paramName) {}
	virtual bool getStringParam(std::string& value, const std::string& paramName) const {
		return false;
	}

	virtual void setIntParam(const int& value, const std::string& paramName) {}
	virtual bool getIntParam(int& value, const std::string& paramName) const {
		return false;
	}

	virtual void setInt64Param(const int64& value, const std::string& paramName) {}
	virtual bool getInt64Param(int64& value, const std::string& paramName) const {
		return false;
	}

	virtual void setFloatParam(const float& value, const std::string& paramName) {}
	virtual bool getFloatParam(float& value, const std::string& paramName) const {
		return false;
	}

	virtual void setBoolParam(const bool& value, const std::string& paramName) {}
	virtual bool getBoolParam(bool& value, const std::string& paramName) const {
		return false;
	}

	virtual void setCKernelParam(const ConvolutionKernel& value, const std::string& paramName) {}
	virtual bool getCKernelParam(ConvolutionKernel& value, const std::string& paramName) const {
		return false;
	}

	virtual void setEnumParam(const unsigned& value, const std::string& paramName) {}
	virtual bool getEnumParam(unsigned& value, const std::string& paramName) const {
		return false;
	}

	virtual void setVectorParam(const Vector2& value, const std::string& paramName) {}
	virtual bool getVectorParam(Vector2& value, const std::string& paramName) const {
		return false;
	}

	virtual void setColorParam(const Color& value, const std::string& paramName) {}
	virtual bool getColorParam(Color& value, const std::string& paramName) const {
		return false;
	}

	virtual void onImageChange(int width, int height) {}

	// enables/disables a parameter from being set
	virtual void enableParam(const std::string& paramName, bool enable) {}

	// adds a parameter to internal storage for type info and optionally updates to the module
	virtual void addParam(const ParamDescriptor& pd) = 0;
};

#endif // __PARAM_BASE_H__
