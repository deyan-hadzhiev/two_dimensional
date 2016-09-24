#ifndef __WX_SCALABLE_H__
#define __WX_SCALABLE_H__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/vector.h>

#include "drect.h"

namespace Convert {
	inline wxPoint vector(const Vector2& v) {
		return wxPoint(
			static_cast<int>(roundf(v.x)),
			static_cast<int>(roundf(v.y))
		);
	}
	inline Vector2 vector(const wxPoint& p) {
		return Vector2(
			static_cast<float>(p.x),
			static_cast<float>(p.y)
		);
	}
	inline wxSize size(const Size2d& sz) {
		return wxSize(
			static_cast<int>(sz.getWidth()),
			static_cast<int>(sz.getHeight())
		);
	}
	inline Size2d size(const wxSize& sz) {
		return Size2d(
			static_cast<float>(sz.GetWidth()),
			static_cast<float>(sz.GetHeight())
		);
	}
	inline wxRect rect(const Rect& r) {
		const float fx = floorf(r.x);
		const float fy = floorf(r.y);
		return wxRect(
			static_cast<int>(fx),
			static_cast<int>(fy),
			static_cast<int>(ceilf(r.width + (r.x - fx))),
			static_cast<int>(ceilf(r.height + (r.y - fy)))
		);
	}
	inline Rect rect(const wxRect& r) {
		return Rect(
			static_cast<float>(r.x),
			static_cast<float>(r.y),
			static_cast<float>(r.width),
			static_cast<float>(r.height)
		);
	}
}

// base class for scalable and movable content
class ScalablePanel : public wxPanel {
public:
	ScalablePanel(wxWindow * _parent, const int _minZoom, const int _maxZoom, const bool _eraseBkg = false);
	virtual ~ScalablePanel() = default;

	virtual void OnMouseEvent(wxMouseEvent& evt);

	virtual void OnSizeEvent(wxSizeEvent& evt);

protected:
	// this function should be overloaded if the mouse moving should not pan the view
	// @returns true if the view should be panned
	virtual bool onMouseMove() { return true; }

	// mouse click events
	// @returns true if the panel should be refreshed
	virtual bool onMouseLeftDown() { return false; }
	virtual bool onMouseDoubleLeftDown() { return false; }
	virtual bool onMouseLeftUp() { return false; }
	virtual bool onMouseRightDown() { return false; }
	virtual bool onMouseRightUp() { return false; }

	template<class Scalable>
	Scalable scale(Scalable input) const {
		if (zoomLvl == 0) {
			return input;
		} else if (zoomLvl > 0) {
			return input * (zoomLvl + 1);
		} else {
			return input / (-zoomLvl + 1);
		}
	}

	template<class Scalable>
	Scalable unscale(Scalable input) const {
		if (zoomLvl == 0) {
			return input;
		} else if (zoomLvl > 0) {
			return input / (zoomLvl + 1);
		} else {
			return input * (-zoomLvl + 1);
		}
	}

	void recalcViewSize();
	void recalcViewPos(Vector2 p, const Rect& prevRect);

	Vector2 convertScreenToCanvas(const wxPoint& in) const;
	wxPoint convertCanvasToScreen(const Vector2& in) const;
	void remapCanvas();

	// variables
	const bool eraseBkg; //!< if refresh calls should erase the background
	const int minZoom;
	const int maxZoom;
	int zoomLvl;
	int zoomLvlDelta; //!< the delta that has to be applied to the zoom
	bool mouseLeftDrag; //!< if the mouse is currently dragging with left button
	bool mouseRightDrag; //!< if the mouse is currently dragging with right button
	bool mouseOverCanvas; //!< if the mouse is over the canvas
	wxPoint mousePos; //!< the current pointer position (implement saving logic in derived classes)
	wxPoint updatedMousePos; //!< the last mouse position taken into account (call remapCanvas() to update)
	Rect view; //!< The currently seen rect from the canvas in canvas coordinates

	enum CanvasState {
		CS_CLEAN = 0,
		CS_DIRTY_SIZE = 1 << 0,
		CS_DIRTY_ZOOM = 1 << 1,
		CS_DIRTY_POS = 1 << 2,
		CS_DIRTY_FULL = CS_DIRTY_SIZE | CS_DIRTY_ZOOM | CS_DIRTY_POS,
	};
	unsigned canvasState;
	wxSize panelSize;
};

#endif // __WX_SCALABLE_H__
