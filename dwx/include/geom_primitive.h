#ifndef __GEOM_PRIMITIVE_H__
#define __GEOM_PRIMITIVE_H__

#include "bitmap.h"
#include <string>
#include <vector>

enum DrawFlags {
	DF_OVER       = 0,      //!< overwrite previous contents (this is the default)
	DF_CLEAR      = 1 << 0, //!< clear the previous image drawn
	DF_ACCUMULATE = 1 << 1, //!< accumulate previous drawn content
	DF_SHOW_AXIS  = 1 << 2, //!< whether to draw an axis too
};

template<class CType>
class GeometricPrimitive {
protected:
	Pixelmap<CType> bmp;
	static const CType axisCol;
public:
	GeometricPrimitive() {}
	GeometricPrimitive(int width, int height)
		: bmp(width, height)
	{}

	virtual ~GeometricPrimitive() {}

	const Pixelmap<CType>& getBitmap() const {
		return bmp;
	}

	Pixelmap<CType>& getBitmap() {
		return bmp;
	}

	void resize(int width, int height) {
		bmp.generateEmptyImage(width, height);
	}

	void clear() {
		bmp.generateEmptyImage(bmp.getWidth(), bmp.getHeight());
	}

	// parametric setters and getters
	virtual void setParam(const std::string& p, const std::string& value) {}
	virtual std::string getParam(const std::string& p) const {
		return std::string();
	}
	virtual std::vector<std::string> getParamList() const {
		return std::vector<std::string>();
	}

	// the virtual function for drawing
	virtual void draw(CType col = CType(), unsigned flags = DF_OVER) = 0;
};

template<class CType>
class Sinosoid : public GeometricPrimitive<CType> {
	float k;
	float q;
	float offset;
	bool sine;
public:
	Sinosoid();

	virtual void setParams(float _k, float _q, float _offset, bool sine = false);

	virtual void setParam(const std::string& p, const std::string& value) override;

	virtual std::vector<std::string> getParamList() const override;

	virtual void draw(CType pen = CType(), unsigned flags = DF_OVER) override final;
};

template<class CType>
class Function : public GeometricPrimitive<CType> {
	std::function<float(float)> fx;
	std::function<float(int)> mapX;
public:
	Function(int width = -1, int height = -1);

	virtual void setFunction(std::function<float(int)>, std::function<float(float)>);

	virtual void draw(CType pen = CType(), unsigned flags = DF_OVER) override final;
};

#endif // __GEOM_PRIMITIVE_H__
