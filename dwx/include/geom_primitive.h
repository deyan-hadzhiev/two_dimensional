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

	enum DrawFlags {
		DF_OVER       = 0,      //!< overwrite previous contents (this is the default)
		DF_CLEAR      = 1 << 0, //!< clear the previous image drawn
		DF_ACCUMULATE = 1 << 1, //!< accumulate previous drawn content
		DF_SHOW_AXIS  = 1 << 2, //!< whether to draw an axis too
	};

	// parametric setters and getters
	virtual void setParam(const std::string& p, const std::string& value) {}
	virtual std::string getParam(const std::string& p) const {
		return std::string();
	}
	virtual std::vector<std::string> getParamList() const {
		return std::vector<std::string>();
	}

	// the virtual function for drawing
	virtual void draw(Color col = Color(255, 255, 255), unsigned flags = DF_OVER) = 0;
};

class Sinosoid : public GeometricPrimitive {
	float k;
	float q;
	float offset;
	bool sine;
public:
	Sinosoid();

	virtual void setParams(float _k, float _q, float _offset, bool sine = false);

	virtual void setParam(const std::string& p, const std::string& value) override;

	virtual std::vector<std::string> getParamList() const override;

	virtual void draw(Color col = Color(255, 255, 255), unsigned flags = DF_OVER) override final;
};

#endif // __GEOM_PRIMITIVE_H__
