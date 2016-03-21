#ifndef __WX_BITMAP_CANVAS_H__
#define __WX_BITMAP_CANVAS_H__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dcbuffer.h>

#include "bitmap.h"

class BitmapCanvas : public wxPanel {
public:
	BitmapCanvas(wxWindow * parent, wxFrame * topFrame, const Bitmap * initBmp = nullptr);

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBkg(wxEraseEvent& evt);

	void OnMouseEvt(wxMouseEvent& evt); // !< handles mouse events

	void OnSizeEvt(wxSizeEvent& evt);

	void setBitmap(const Bitmap& bmp);

	void setImage(const wxImage& img);

	void updateStatus() const;
private:
	void resetFocus(); //!< resets the focus to default
	void setFocus(wxPoint f); //!< sets the new focus if it is in bounds
	bool clippedCanvas() const; //!< returns true if the full bmp is currently drawn

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

	wxBitmap bmp;
	wxBitmap canvas;
	bool dirtyCanvas; //!< whether the current canvas is dirty and has to be updated from the bmp based on focus point and zoomLvl

	// topframe for status bar updates
	wxFrame * topFrame;
};

#endif // __WX_BITMAP_CANVAS_H__
