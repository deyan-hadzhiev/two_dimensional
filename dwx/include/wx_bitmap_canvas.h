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

	void setBitmap(const Bitmap& bmp);

	void setImage(const wxImage& img);

	void updateStatus() const;
private:

	void drawFill(wxBufferedPaintDC& pdc, const wxSize& bmpSize, const wxPoint& bmpCoord);

	bool mouseOverCanvas; //!< whether the mouse is currently over the window
	// zooming
	int zoomLvl; //!< the current zoom level
	static const int maxZoom; //!< maximum zoom level supported
	static const int minZoom; //!< minimum zoom level supported
	wxPoint currentFocus; //!< the currently focused point
	wxPoint mousePos; //!< the current pointer position
	wxRect bmpRect; //!< the currently seen rect from the bitmap
	wxRect viewRect; //!< the currently seen rect on the canvas

	wxBitmap bmp;
	wxBitmap canvas;
	bool dirtyCanvas; //!< whether the current canvas is dirty and has to be updated from the bmp based on focus point and zoomLvl

	// topframe for status bar updates
	wxFrame * topFrame;
};

#endif // __WX_BITMAP_CANVAS_H__
