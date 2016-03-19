#include <wx/dcbuffer.h>

#include "wx_bitmap_canvas.h"

BitmapCanvas::BitmapCanvas(wxWindow * parent, const Bitmap * initBmp)
	: wxPanel(parent)
	, initialized(false)
{
	Connect(wxEVT_PAINT, wxPaintEventHandler(BitmapCanvas::OnPaint), NULL, this);
	Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(BitmapCanvas::OnEraseBkg), NULL, this);
	if (initBmp) {
		setBitmap(*initBmp);
		initialized = true;
	} else {
		Bitmap empty;
		empty.generateEmptyImage(64, 64);
		setBitmap(empty);
	}
	Update();
}

void BitmapCanvas::OnPaint(wxPaintEvent& evt) {
	if (!initialized)
		return;
	//wxPaintDC dc(this);
	wxBufferedPaintDC dc(this);
	const wxSize dcSize = GetSize();
	const wxSize bmpSize = bmp.GetSize();
	const int xc = (dcSize.GetWidth() > bmpSize.GetWidth() ? (dcSize.GetWidth() / 2) - (bmpSize.GetWidth() / 2) : 0);
	const int yc = (dcSize.GetHeight() > bmpSize.GetHeight() ? (dcSize.GetHeight() / 2) - (bmpSize.GetHeight() / 2) : 0);
	//wxMemoryDC bmpDC(bmp);
	//dc.Blit(wxPoint(xc, yc), bmpSize, &bmpDC, wxPoint(0, 0));
	dc.DrawBitmap(bmp, xc, yc);
	// draw fill
	const wxColour fillColour(127, 127, 127);
	dc.SetBrush(wxBrush(fillColour));
	dc.SetPen(wxPen(fillColour));
	if (xc > 0) {
		dc.DrawRectangle(0, 0, xc, dcSize.GetHeight());
		dc.DrawRectangle(xc + bmpSize.GetWidth(), 0, xc, dcSize.GetHeight());
	}
	if (yc > 0) {
		dc.DrawRectangle(0, 0, dcSize.GetWidth(), yc);
		dc.DrawRectangle(0, yc + bmpSize.GetHeight(), dcSize.GetWidth(), yc);
	}
}

void BitmapCanvas::setBitmap(const Bitmap& bmp) {
	const int w = bmp.getWidth();
	const int h = bmp.getHeight();
	wxImage tmpImg = wxImage(wxSize(w, h));
	unsigned char * imgData = tmpImg.GetData();
	const Color* colorData = bmp.getDataPtr();
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			const unsigned rgb = colorData[x + y * w].toRGB32();
			imgData[0 + (x + y * w) * 3] = static_cast<unsigned char>((rgb & 0xff0000) >> 16);
			imgData[1 + (x + y * w) * 3] = static_cast<unsigned char>((rgb & 0x00ff00) >> 8);
			imgData[2 + (x + y * w) * 3] = static_cast<unsigned char>(rgb & 0x0000ff);
		}
	}
	this->bmp = wxBitmap(tmpImg);
	Refresh();
}

void BitmapCanvas::OnEraseBkg(wxEraseEvent& evt) {
	// disable erase - all will be handled in the paint evt
}
