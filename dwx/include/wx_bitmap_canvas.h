#ifndef __WX_BITMAP_CANVAS_H__
#define __WX_BITMAP_CANVAS_H__

#include <wx/wx.h>
#include <wx/panel.h>

#include "bitmap.h"

class BitmapCanvas : public wxPanel {
public:
	BitmapCanvas(wxWindow * parent, const Bitmap * initBmp = nullptr);

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBkg(wxEraseEvent& evt);

	void setBitmap(const Bitmap& bmp);
private:
	bool initialized;
	wxBitmap bmp;
};

#endif // __WX_BITMAP_CANVAS_H__
