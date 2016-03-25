#ifndef __GEOM_PRIMITIVE_H__
#define __GEOM_PRIMITIVE_H__

#include "bitmap.h"
#include <string>
#include <vector>

class GeometricPrimitive {
protected:
	Bitmap bmp;
public:
	GeometricPrimitive() {}
	GeometricPrimitive(int width, int height)
		: bmp(width, height)
	{}

	virtual ~GeometricPrimitive() {}

	const Bitmap& getBitmap() const {
		return bmp;
	}

	Bitmap& getBitmap() {
		return bmp;
	}

	void resize(int width, int height) {
		bmp.generateEmptyImage(width, height);
	}

	void clear() {
		bmp.generateEmptyImage(bmp.getWidth(), bmp.getHeight());
	}

	enum DrawMode {
		DM_CLEAR = 0,  //!< clear the previous image drawn
		DM_OVER,       //!< overwrite previous contents
		DM_ACCUMULATE, //!< accumulate previous drawn content
	};

	// parametric setters and getters
	virtual void setParam(const std::string& p, const std::string& value) {}
	virtual const std::string& getParam(const std::string& p) const {
		return std::string();
	}
	virtual const std::vector<std::string>& getParamList() const {
		return std::vector<std::string>();
	}

	// the virtual function for drawing
	virtual void draw(DrawMode mode = DM_CLEAR) = 0;
};

class Sinosoid : public GeometricPrimitive {
public:
	virtual void draw(GeometricPrimitive::DrawMode = DM_CLEAR) override final;
};

#endif // __GEOM_PRIMITIVE_H__
