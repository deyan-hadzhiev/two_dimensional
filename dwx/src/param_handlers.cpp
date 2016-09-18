#include "param_handlers.h"

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
