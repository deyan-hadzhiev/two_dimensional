#ifndef __WX_BITMAP_CANVAS_H__
#define __WX_BITMAP_CANVAS_H__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dcbuffer.h>
#include <wx/vector.h>

#include <mutex>
#include <unordered_map>

#include "bitmap.h"
#include "kernel_base.h"
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

class BitmapCanvas;

// a class for rescaling wxImage and wxBitmaps (roughly - no subpixel magics)
// @note: the class keeps cache for downscaled images
class ImageRescaler {
public:
	ImageRescaler() = default;
	~ImageRescaler();
	ImageRescaler(const ImageRescaler&) = delete;
	ImageRescaler& operator=(const ImageRescaler&) = delete;

	void setBitmap(const wxBitmap& _bmp);

	wxBitmap getUpscaledSubBitmap(int scale, const wxRect& subRect, const wxSize& scaledSize) const;

	wxBitmap getDownscaledSubBitmap(int scale, const wxRect& subRect) const;
private:
	void clearCache(); // manual deletion of cached elements

	wxBitmap bmp;
	mutable std::unordered_map<int, wxBitmap*> downscaleCache;
};

// TODO make it a base class and split the input and output - if it makes sense
class BitmapCanvas : public wxPanel {
	friend class ImagePanel;
public:
	BitmapCanvas(wxWindow * parent, wxFrame * topFrame);

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBkg(wxEraseEvent& evt);

	void OnMouseEvt(wxMouseEvent& evt); // !< handles mouse events

	void OnSizeEvt(wxSizeEvent& evt);

	void setImage(const wxImage& img, int id = 0);

	void updateStatus() const;

	void addSynchronizer(BitmapCanvas * s); //!< adds another canvas that will synchronize zooming/panning

	void synchronize(); //!< synchronizes with other canvases to show the same rect

protected:

	wxVector<BitmapCanvas*> synchronizers; //!< a vector of synchronizers that have to be synchronized on changes
private:
	union BmpClip {
		struct {
			bool x : 1;
			bool y : 1;
		};
		unsigned char any : 2;
	} bmpClip;
	// new zooming functions
	void recalcViewSize(); //!< must have a valid bmp loaded
	void resetViewPos(); //!< resets the position of the rect to be centralized
	void recalcViewPos(Vector2 p, const Rect& prevRect); //!< recalculates the rect position preserving the specified relative position (in bmp coords)
	void boundFixView(); //!< Checks the bounds of the view and fixes them

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
	wxPoint getCanvasTopLeftInScreen() const; //!< returns the current screen coordinates of the top left corner of the canvas
	Vector2 convertScreenToBmp(const wxPoint& in) const; //!< convert screen to bmp coordinates (may return out of bounds!)
	wxPoint convertBmpToScreen(const Vector2& in) const; //!< convert bmp to screen coordinates (may return out of bounds!)
	// remaps the current canvas to the new as specified by the zoomLvl
	// NOTE: Call only when necessary!
	void remapCanvas();

	// draws fill on the buffered paint dc based on size and position
	void drawFill(wxBufferedPaintDC& pdc, const wxSize& bmpSize, const wxPoint& bmpCoord);

	bool mouseOverCanvas; //!< whether the mouse is currently over the window
	bool mouseMoveDrag; //!< whether the mouse has initiated a drag inside this wnd
	// zooming
	int zoomLvl; //!< the current zoom level (only update in the remapping function!!)
	int zoomLvlDelta; //!< the delta that has to be applied to the zoom
	static const int maxZoom; //!< maximum zoom level supported
	static const int minZoom; //!< minimum zoom level supported
	wxPoint mousePos; //!< the current pointer position
	wxPoint updatedMousePos; //!< the last position of the mouse that was considered (only update in the remapping function)
	Rect view; //!< The currently seen rect from the bitmap in bmp coordinates

	int bmpId;
	wxBitmap bmp;
	wxBitmap canvas;
	ImageRescaler rescaler; // for cached rescaling of downscaled images
	//bool dirtyCanvas; //!< whether the current canvas is dirty and has to be updated from the bmp based on focus point and zoomLvl
	enum CanvasState {
		CS_CLEAN = 0,
		CS_DIRTY_SIZE = 1 << 0,
		CS_DIRTY_ZOOM = 1 << 1,
		CS_DIRTY_POS = 1 << 2,
		CS_DIRTY_FULL = CS_DIRTY_SIZE | CS_DIRTY_ZOOM | CS_DIRTY_POS,
		CS_DIRTY_SYNCHRONIZE,
	};
	unsigned canvasState;
	wxSize panelSize; //!< the panel size from the last event (NOTE: use this instead of GetSize() because it may be changed between event handling)

	// topframe for status bar updates
	wxFrame * topFrame;
};

class HistogramPanel : public wxPanel {
public:
	HistogramPanel(wxWindow * parent);

	void setImage(const Bitmap& bmp);

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBkg(wxEraseEvent& evt);

private:
	static const Color histFgColor;
	static const Color histBkgColor;
	static const Color histFgIntensity;
	static const Color histBkgIntensity;
	Histogram<HDL_VALUE> hist;
};

class ImagePanel : public wxPanel, public InputManager, public OutputManager {
public:
	ImagePanel(wxWindow * parent, wxFrame * topFrame, const Bitmap * initBmp = nullptr);

	int getBmpId() const;

	void synchronize(); //!< synchronizes with other canvases to show the same rect

	void setImage(const wxImage& img, int id = 0);

	wxImage getImage();

	void toggleHist();

	// from InputManager
	virtual bool getInput(Bitmap& inputBmp, int& id) const override;

	virtual void kernelDone(KernelBase::ProcessResult result) override;

	// from OutputManager
	virtual void setOutput(const Bitmap& outputBmp, int id) override;

	BitmapCanvas * getCanvas() const;

private:
	int bmpId;
	mutable std::mutex bmpMutex;
	Bitmap bmp;
	BitmapCanvas * canvas;
	HistogramPanel * histPanel;
	wxSizer * panelSizer;
};

#endif // __WX_BITMAP_CANVAS_H__
