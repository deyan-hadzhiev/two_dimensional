#ifndef __GEOM_PRIMITIVE_H__
#define __GEOM_PRIMITIVE_H__

#include "bitmap.h"
#include "vector2.h"

#include <string>
#include <vector>

enum DrawFlags {
	DF_OVER       = 0,      //!< overwrite previous contents (this is the default)
	DF_CLEAR      = 1 << 0, //!< clear the previous image drawn
	DF_ACCUMULATE = 1 << 1, //!< accumulate previous drawn content
	DF_SHOW_AXIS  = 1 << 2, //!< whether to draw an axis too
};

template<class CType>
class DrawPen {
public:
	DrawPen(const CType& _color, double _width = 1.0, double _strength = 0.5)
		: color(_color)
		, width(_width)
		, strength(_strength)
	{}

	inline void setColor(const CType& _color) noexcept {
		color = _color;
	}

	inline CType getColor() const noexcept {
		return color;
	}

	inline void setWidth(const double _width) noexcept {
		width = _width;
	}

	inline double getWidth() const noexcept {
		return width;
	}

	inline void setStrength(const double _strength) noexcept {
		strength = _strength;
	}

	inline double getStrength() const noexcept {
		return strength;
	}

private:
	CType color;
	double width;
	double strength;
};

template<class CType>
class GeometricPrimitive {
protected:
	Pixelmap<CType> bmp;
	static const CType axisCol;
	DrawPen<CType> pen;
public:
	GeometricPrimitive()
		: pen(CType())
	{}
	GeometricPrimitive(int width, int height)
		: bmp(width, height)
		, pen(CType())
	{}

	virtual ~GeometricPrimitive() {}

	const Pixelmap<CType>& getBitmap() const noexcept {
		return bmp;
	}

	Pixelmap<CType>& getBitmap() noexcept {
		return bmp;
	}

	void resize(int width, int height) noexcept {
		bmp.generateEmptyImage(width, height);
	}

	void clear() noexcept {
		bmp.generateEmptyImage(bmp.getWidth(), bmp.getHeight());
	}

	void setPen(const DrawPen<CType>& _pen) noexcept {
		pen = _pen;
	}

	DrawPen<CType> getPen() const noexcept {
		return pen;
	}

	DrawPen<CType>& getPen() noexcept {
		return pen;
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
	virtual void draw(unsigned flags = DF_OVER) = 0;
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

	virtual void draw(unsigned flags = DF_OVER) override final;
};

template<class CType>
class Function : public GeometricPrimitive<CType> {
	std::function<float(float)> fx;
	std::function<float(int)> mapX;
public:
	Function(int width = -1, int height = -1);

	virtual void setFunction(std::function<float(int)>, std::function<float(float)>);

	virtual void draw(unsigned flags = DF_OVER) override final;
};

template<class CType>
class FunctionRaster : public GeometricPrimitive<CType> {
	std::function<double(double, double)> fxy;
public:
	FunctionRaster(int width = -1, int height = -1);

	virtual void setFunction(std::function<double(double, double)>);

	virtual void draw(unsigned flags = DF_OVER) override final;
};

#endif // __GEOM_PRIMITIVE_H__
