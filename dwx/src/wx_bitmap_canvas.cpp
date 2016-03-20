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
		const int bmpDim = 64;
		empty.generateEmptyImage(bmpDim, bmpDim);
		const Color border(.5f, .5f, .5f);
		Color * bmpData = empty.getDataPtr();
		for (int i = 0; i < bmpDim; ++i) {
			bmpData[i] = border;
			bmpData[i * bmpDim] = border;
			bmpData[bmpDim * (bmpDim - 1) + i] = border;
			bmpData[bmpDim * i + bmpDim - 1] = border;
		}
		setBitmap(empty);
	}
	Update();
}

void BitmapCanvas::OnPaint(wxPaintEvent& evt) {
	//wxPaintDC dc(this);
	wxBufferedPaintDC dc(this);
	const wxSize dcSize = GetSize();
	const wxSize bmpSize = bmp.GetSize();
	const int xc = (dcSize.GetWidth() > bmpSize.GetWidth() ? (dcSize.GetWidth() / 2) - (bmpSize.GetWidth() / 2) : 0);
	const int yc = (dcSize.GetHeight() > bmpSize.GetHeight() ? (dcSize.GetHeight() / 2) - (bmpSize.GetHeight() / 2) : 0);
	//wxMemoryDC bmpDC(bmp);
	//dc.Blit(wxPoint(xc, yc), bmpSize, &bmpDC, wxPoint(0, 0));
	dc.DrawBitmap(bmp, xc, yc);

	drawFill(dc, bmpSize, wxPoint(xc, yc));
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

void BitmapCanvas::setImage(const wxImage & img) {
	bmp = wxBitmap(img);
	Refresh();
}

void BitmapCanvas::drawFill(wxBufferedPaintDC & pdc, const wxSize & bmpSize, const wxPoint & bmpCoord) {
	const int bx = bmpCoord.x;
	const int by = bmpCoord.y;
	const int bw = bmpSize.GetWidth();
	const int bh = bmpSize.GetHeight();
	const wxSize& dcSize = pdc.GetSize();
	const int dw = dcSize.GetWidth();
	const int dh = dcSize.GetHeight();
	const int ew = (dw - bw); // the empty space width
	const int eh = (dh - bh); // the empty space height
	if (ew > 0 || eh > 0) {
		const wxColour fillColour = GetBackgroundColour();
		pdc.SetBrush(wxBrush(fillColour));
		pdc.SetPen(wxPen(fillColour));
		const int ewl = (ew >> 1) + (ew & 1); // the leftside leftover width
		const int ehb = (eh >> 1) + (eh & 1); // the bottom leftover height
		if (ew > 0) {
			pdc.DrawRectangle(0, by, bx, bh);
			pdc.DrawRectangle(bx + bw, by, ewl, bh);
		}
		if (eh > 0) {
			pdc.DrawRectangle(bx, 0, bw, by);
			pdc.DrawRectangle(bx, by + bh, bw, ehb);
		}
		// if both the empty height and width are non-zero - draw four additional rects
		if (ew > 0 && eh > 0) {
			pdc.DrawRectangle(0, 0, bx, by);
			pdc.DrawRectangle(bx + bw, 0, ewl, by);
			pdc.DrawRectangle(0, by + bh, bx, ehb);
			pdc.DrawRectangle(bx + bw, by + bh, ewl, ehb);
		}
	}
}

void BitmapCanvas::OnEraseBkg(wxEraseEvent& evt) {
	// disable erase - all will be handled in the paint evt
}
