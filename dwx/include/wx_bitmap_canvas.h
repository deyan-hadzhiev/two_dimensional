#ifndef __WX_BITMAP_CANVAS_H__
#define __WX_BITMAP_CANVAS_H__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dcbuffer.h>

#include "bitmap.h"

class BitmapCanvas : public wxPanel {
public:
	BitmapCanvas(wxWindow * parent, const Bitmap * initBmp = nullptr);

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBkg(wxEraseEvent& evt);

	void setBitmap(const Bitmap& bmp);

	void setImage(const wxImage& img);
private:

	void drawFill(wxBufferedPaintDC& pdc, const wxSize& bmpSize, const wxPoint& bmpCoord);

	bool initialized;
	wxBitmap bmp;
};

#endif // __WX_BITMAP_CANVAS_H__
