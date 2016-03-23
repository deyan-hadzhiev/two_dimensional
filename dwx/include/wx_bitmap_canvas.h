#ifndef __WX_BITMAP_CANVAS_H__
#define __WX_BITMAP_CANVAS_H__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dcbuffer.h>
#include <wx/vector.h>

#include "bitmap.h"
#include "kernel_base.h"

// TODO make it a base class and split the input and output - if it makes sense
class BitmapCanvas : public wxPanel, public InputManager, public OutputManager {
public:
	BitmapCanvas(wxWindow * parent, wxFrame * topFrame, const Bitmap * initBmp = nullptr);

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBkg(wxEraseEvent& evt);

	void OnMouseEvt(wxMouseEvent& evt); // !< handles mouse events

	void OnSizeEvt(wxSizeEvent& evt);

	void setBitmap(const Bitmap& bmp, int id = 0);

	void setImage(const wxImage& img, int id = 0);

	int getBmpId() const;

	void updateStatus() const;

	void addSynchronizer(BitmapCanvas * s); //!< adds another canvas that will synchronize zooming/panning

	void synchronize(); //!< synchronizes with other canvases to show the same rect

	// from InputManager
	virtual bool getInput(Bitmap& inputBmp, int& id) const override;

	virtual void kernelDone(KernelBase::ProcessResult result) override;

	// from OutputManager
	virtual void setOutput(const Bitmap& outputBmp, int id) override;

protected:

	wxVector<BitmapCanvas*> synchronizers; //!< a vector of synchronizers that have to be synchronized on changes
private:
	void resetFocus(); //!< resets the focus to default
	bool clippedCanvas() const; //!< returns true if the full bmp is currently drawn
	void setFocus(wxPoint f); //!< sets the new focus if it is in bounds

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

	wxPoint convertScreenToBmp(const wxPoint in) const;
	wxPoint convertBmpToScreen(const wxPoint in) const;

	void recalcBmpRect();

	void recalcCanvasRect();

	void remapCanvas();

	void drawFill(wxBufferedPaintDC& pdc, const wxSize& bmpSize, const wxPoint& bmpCoord);

	bool mouseOverCanvas; //!< whether the mouse is currently over the window
	bool mouseLeftDrag; //!< whether the mouse has initiated a drag inside this wnd
	// zooming
	int zoomLvl; //!< the current zoom level
	static const int maxZoom; //!< maximum zoom level supported
	static const int minZoom; //!< minimum zoom level supported
	wxPoint currentFocus; //!< the currently focused point in bmp coordinates
	wxPoint mousePos; //!< the current pointer position
	wxRect bmpRect; //!< the currently seen rect from the bitmap in bmp coordinates
	wxRect canvasRect; //!< the current canvas position and size

	int bmpId;
	wxBitmap bmp;
	wxBitmap canvas;
	bool dirtyCanvas; //!< whether the current canvas is dirty and has to be updated from the bmp based on focus point and zoomLvl

	// topframe for status bar updates
	wxFrame * topFrame;
};

#endif // __WX_BITMAP_CANVAS_H__
