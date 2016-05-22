#ifndef __WX_BITMAP_CANVAS_H__
#define __WX_BITMAP_CANVAS_H__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dcbuffer.h>
#include <wx/vector.h>

#include <mutex>

#include "bitmap.h"
#include "kernel_base.h"

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

	//void addSynchronizer(BitmapCanvas * s); //!< adds another canvas that will synchronize zooming/panning

	//void synchronize(); //!< synchronizes with other canvases to show the same rect

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
	void recalcBmpRectSize(); //!< must have a valid bmp loaded
	void resetBmpRectPos(); //!< resets the position of the rect to be centralized
	void recalcBmpRectPos(wxPoint p, const wxRect& prevRect); //!< recalculates the rect position preserving the specified relative position (in bmp coords)
	void recalcCanvasRectSize(); //!< recalculates the size of the canvas rect
	void resetCanvasRectPos(); //!< resets the position of the canvas to be centralized on the panel

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
	wxPoint convertScreenToBmp(const wxPoint in) const; //!< convert screen to bmp coordinates (may return out of bounds!)
	wxPoint convertBmpToScreen(const wxPoint in) const; //!< convert bmp to screen coordinates (may return out of bounds!)
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
	wxRect bmpRect; //!< the currently seen rect from the bitmap in bmp coordinates
	wxRect canvasRect; //!< the current canvas position and size

	int bmpId;
	wxBitmap bmp;
	wxBitmap canvas;
	//bool dirtyCanvas; //!< whether the current canvas is dirty and has to be updated from the bmp based on focus point and zoomLvl
	enum CanvasState {
		CS_CLEAN = 0,
		CS_DIRTY_SIZE = 1 << 0,
		CS_DIRTY_ZOOM = 1 << 1,
		CS_DIRTY_POS = 1 << 2,
		CS_DIRTY_FULL = CS_DIRTY_SIZE | CS_DIRTY_ZOOM | CS_DIRTY_POS,
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
