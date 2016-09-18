#include "param_handlers.h"
#include "modules.h"

bool AspectRatioHandler::onParamChange(ParamManager * pman, const std::string & paramName, const int & value) {
	if (pman) {
		if (paramName == "width") {
			if (aspectOn) {
				const float height = value / aspectRatio;
				pman->setIntParam(static_cast<int>(height), "height");
			} else {
				int height = 0;
				if (pman->getIntParam(height, "height")) {
					aspectRatio = static_cast<float>(value) / static_cast<float>(height);
					pman->setFloatParam(aspectRatio, "aspectRatio");
				}
			}
		} else if (paramName == "height") {
			if (aspectOn) {
				const float width = value * aspectRatio;
				pman->setIntParam(static_cast<int>(width), "width");
			} else {
				int width = 0;
				if (pman->getIntParam(width, "width")) {
					aspectRatio = static_cast<float>(width) / static_cast<float>(value);
					pman->setFloatParam(aspectRatio, "aspectRatio");
				}
			}
		}
	}
	return false;
}

bool AspectRatioHandler::onParamChange(ParamManager * pman, const std::string & paramName, const bool & value) {
	if (pman && paramName == "keepAspect") {
		aspectOn = value;
	}
	return false;
}

bool AspectRatioHandler::onParamChange(ParamManager * pman, const std::string & paramName, const float & value) {
	if (pman && paramName == "aspectRatio") {
		int width = 0;
		if (pman->getIntParam(width, "width") && value != 0.0f) {
			const int height = static_cast<int>(width / value);
			pman->setIntParam(height, "height");
		}
	}
	return false;
}

void AspectRatioHandler::onImageChange(ParamManager * pman, int width, int height) {
	if (pman) {
		pman->setIntParam(width, "width");
		pman->setIntParam(height, "height");
		aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		pman->setFloatParam(aspectRatio, "aspectRatio");
	}
}

bool RandomNoiseChangeHandler::onParamChange(ParamManager * pman, const std::string & paramName, const unsigned & value) {
	static const std::string distParamName = "distribution";
	bool retval = false;
	if (pman) {
		if (paramName == "randEngine" || paramName == distParamName) {
			retval = true;
			if (paramName == "randEngine") {
				randGen = value;
			} else if (paramName == distParamName) {
				distribution = value;
			}
			pman->enableParam(distParamName, value != RandomNoiseModule::RE_C_RAND);
			if (randGen == RandomNoiseModule::RE_C_RAND) {
				// set the mean and sigma like a uniform distribution
				int width = 0;
				int height = 0;
				if (pman->getIntParam(width, "width") && pman->getIntParam(height, "height")) {
					pman->setFloatParam(0.0f, "mx");
					pman->setFloatParam(static_cast<float>(width), "sx");
					pman->setFloatParam(0.0f, "my");
					pman->setFloatParam(static_cast<float>(height), "sy");
					// enable the two sigma params
					pman->enableParam("sx", true);
					pman->enableParam("sy", true);
				}
			} else {
				int width = 0;
				int height = 0;
				if (pman->getIntParam(width, "width") && pman->getIntParam(height, "height")) {
					float mx = 0.0f;
					float my = 0.0f;
					float sx = 0.0f;
					float sy = 0.0f;
					bool sigmaUsed = true;
					switch (distribution) {
					case(RandomNoiseModule::D_UNIFORM):
						sx = static_cast<float>(width);
						sy = static_cast<float>(height);
						break;
					case(RandomNoiseModule::D_NORMAL):
						mx = static_cast<float>(width) / 2.0f;
						my = static_cast<float>(height) / 2.0f;
						sx = static_cast<float>(width) / 6.0f;
						sy = static_cast<float>(height) / 6.0f;
						break;
					case(RandomNoiseModule::D_CHI_SQUARED):
						mx = static_cast<float>(width) / 2.0f;
						my = static_cast<float>(height) / 2.0f;
						sigmaUsed = false;
						break;
					case(RandomNoiseModule::D_LOG_NORMAL):
						mx = log2f(static_cast<float>(width)) / 2.0f;
						my = log2f(static_cast<float>(height)) / 2.0f;
						sx = sy = 1.0f;
						break;
					case(RandomNoiseModule::D_CAUCHY):
						mx = static_cast<float>(width) / 2.0f;
						my = static_cast<float>(height) / 2.0f;
						sx = log2f(static_cast<float>(width));
						sy = log2f(static_cast<float>(height));
						break;
					case(RandomNoiseModule::D_FISHER_F):
						mx = log2f(static_cast<float>(width));
						my = log2f(static_cast<float>(height));
						sx = sy = 1.0f;
						break;
					case(RandomNoiseModule::D_STUDENT_T):
						mx = log2f(static_cast<float>(width)) / 10.0f;
						my = log2f(static_cast<float>(height)) / 10.0f;
						sigmaUsed = false;
						break;
					default:
						DASSERT(false);
						break;
					}
					pman->setFloatParam(mx, "mx");
					pman->setFloatParam(my, "my");
					pman->enableParam("sx", sigmaUsed);
					pman->enableParam("sy", sigmaUsed);
					if (sigmaUsed) {
						pman->setFloatParam(sx, "sx");
						pman->setFloatParam(sy, "sy");
					}
				}
			}
		}
	}
	return retval;
}
