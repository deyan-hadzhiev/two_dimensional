#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>

#include "wx_bitmap_canvas.h"
#include "util.h"

const int BitmapCanvas::minZoom = -4;
const int BitmapCanvas::maxZoom = 8;

BitmapCanvas::BitmapCanvas(wxWindow * parent, wxFrame * topFrame, const Bitmap * initBmp)
	: wxPanel(parent)
	, mouseOverCanvas(false)
	, mouseLeftDrag(false)
	, zoomLvl(0)
	, currentFocus(0, 0)
	, mousePos(0, 0)
	, bmpRect(0, 0, 0, 0)
	, canvasRect(0, 0, 0, 0)
	, dirtyCanvas(true)
	, topFrame(topFrame)
{
	// connect paint events
	Connect(wxEVT_PAINT, wxPaintEventHandler(BitmapCanvas::OnPaint), NULL, this);
	Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(BitmapCanvas::OnEraseBkg), NULL, this);
	// connect mouse events
	Connect(wxEVT_ENTER_WINDOW, wxMouseEventHandler(BitmapCanvas::OnMouseEvt), NULL, this);
	Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(BitmapCanvas::OnMouseEvt), NULL, this);
	Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(BitmapCanvas::OnMouseEvt), NULL, this);
	Connect(wxEVT_MOTION, wxMouseEventHandler(BitmapCanvas::OnMouseEvt), NULL, this);
	Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(BitmapCanvas::OnMouseEvt), NULL, this);
	Connect(wxEVT_LEFT_UP, wxMouseEventHandler(BitmapCanvas::OnMouseEvt), NULL, this);
	// connect the sizing event
	Connect(wxEVT_SIZE, wxSizeEventHandler(BitmapCanvas::OnSizeEvt), NULL, this);

	if (initBmp) {
		setBitmap(*initBmp);
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
	resetFocus();
	dirtyCanvas = true;
	Refresh();
}

void BitmapCanvas::setImage(const wxImage & img) {
	bmp = wxBitmap(img);
	resetFocus();
	dirtyCanvas = true;
	Refresh();
}

void BitmapCanvas::updateStatus() const {
	if (true || mouseOverCanvas) {
		wxString focusStr;
		const wxSize sz = GetSize();
		
		focusStr.Printf(wxT("focus: x: %4d y: %4d ps: (%d, %d)"), currentFocus.x, currentFocus.y, sz.GetWidth(), sz.GetHeight());
		topFrame->SetStatusText(focusStr);
		wxString posStr;
		posStr.Printf(wxT("x: %4d y: %4d bmp(%d, %d, %d, %d)"), mousePos.x, mousePos.y, bmpRect.x, bmpRect.y, bmpRect.width, bmpRect.height);
		topFrame->SetStatusText(posStr, 1);
		wxString rectStr;
		rectStr.Printf(wxT("zoom: %d canvas(%d, %d, %d, %d)"), zoomLvl, canvasRect.x, canvasRect.y, canvasRect.width, canvasRect.height);
		topFrame->SetStatusText(rectStr, 2);
	} else {
		topFrame->SetStatusText(wxT(""), 1);
		topFrame->SetStatusText(wxT(""), 2);
	}
}

void BitmapCanvas::resetFocus() {
	currentFocus = wxPoint(bmp.GetWidth() / 2, bmp.GetHeight() / 2);
}

void BitmapCanvas::setFocus(wxPoint f) {
	const wxSize uphs = unscaleSize(GetSize()) / 2; // unscaledPanelHalfSize
	currentFocus.x = clamp(f.x, uphs.GetWidth(), bmp.GetWidth() - uphs.GetWidth());
	currentFocus.y = clamp(f.y, uphs.GetHeight(), bmp.GetHeight() - uphs.GetHeight());
}

bool BitmapCanvas::clippedCanvas() const {
	const wxSize unscaledBmpSize = unscaleSize(bmp.GetSize());
	const wxSize panelSize = GetSize();
	return unscaledBmpSize.GetWidth() > panelSize.GetWidth() || unscaledBmpSize.GetHeight() < panelSize.GetHeight() || bmpRect.x > 0 || bmpRect.y > 0;
}

wxSize BitmapCanvas::scaleSize(wxSize input) const {
	if (zoomLvl == 0) {
		return input;
	} else if (zoomLvl > 0) {
		return input * (zoomLvl + 1);
	} else {
		return input / (-zoomLvl + 1);
	}
}

wxSize BitmapCanvas::unscaleSize(wxSize input) const {
	if (zoomLvl == 0) {
		return input;
	} else if (zoomLvl > 0) {
		return input / (zoomLvl + 1);
	} else {
		return input * (-zoomLvl + 1);
	}
}

wxPoint BitmapCanvas::convertScreenToBmp(const wxPoint in) const {

	return wxPoint();
}

wxPoint BitmapCanvas::convertBmpToScreen(const wxPoint in) const {
	return wxPoint();
}

void BitmapCanvas::recalcBmpRect() {
	const wxSize panelSize = GetSize();
	const wxSize unscaledSize = unscaleSize(panelSize);
	const wxSize rescaledSize = scaleSize(unscaledSize); //!< for remainder offsets
	const wxPoint panelCenter(panelSize.GetWidth() / 2, panelSize.GetHeight() / 2);
	bmpRect.x = clamp(currentFocus.x - unscaledSize.GetWidth() / 2, 0, bmp.GetWidth() - unscaledSize.GetWidth());
	bmpRect.y = clamp(currentFocus.y - unscaledSize.GetHeight() / 2, 0, bmp.GetHeight() - unscaledSize.GetHeight());
	bmpRect.width = std::min(bmp.GetWidth() - bmpRect.x, unscaledSize.GetWidth() + (rescaledSize.GetWidth() < panelSize.GetWidth() ? 1 : 0));
	bmpRect.height = std::min(bmp.GetHeight() - bmpRect.y, unscaledSize.GetHeight() + (rescaledSize.GetHeight() < panelSize.GetHeight() ? 1 : 0));
}

void BitmapCanvas::recalcCanvasRect() {
	const wxSize panelSize = GetSize();
	const wxSize canvasSize = canvas.GetSize();
	canvasRect.width = canvasSize.GetWidth();
	canvasRect.height = canvasSize.GetHeight();
	canvasRect.x = (panelSize.GetWidth() > canvasRect.width ? (panelSize.GetWidth() / 2) - (canvasRect.width / 2) : 0);
	canvasRect.y = (panelSize.GetHeight() > canvasRect.height ? (panelSize.GetHeight() / 2) - (canvasRect.height / 2) : 0);
}

void BitmapCanvas::remapCanvas() {
	recalcBmpRect();
	if (bmpRect.width != bmp.GetWidth() || bmpRect.height != bmp.GetHeight() || zoomLvl != 0) {
		if (zoomLvl == 0) {
			canvas = bmp.GetSubBitmap(bmpRect);
		} else if (zoomLvl > 0) {
			const int scale = zoomLvl + 1;
			wxBitmap subBmp = bmp.GetSubBitmap(bmpRect);
			const wxSize scaledSize = scaleSize(subBmp.GetSize());
			const int width = subBmp.GetWidth();
			const int height = subBmp.GetHeight();
			const int bpp = 3;
			const int bppWidth = width * bpp;
			const int rowScaledWidth = bppWidth * scale;
			const wxImage sourceImg = subBmp.ConvertToImage();
			wxImage scaledImg(scaledSize);
			const unsigned char * srcData = sourceImg.GetData();
			unsigned char * imgData = scaledImg.GetData();
			for (int y = 0; y < height; ++y) {
				// set a row
				for (int x = 0; x < width; ++x) {
					for (int s = 0; s < scale; ++s) {
						imgData[y * rowScaledWidth * scale + (x * scale + s) * bpp + 0] = srcData[y * bppWidth + x * bpp + 0];
						imgData[y * rowScaledWidth * scale + (x * scale + s) * bpp + 1] = srcData[y * bppWidth + x * bpp + 1];
						imgData[y * rowScaledWidth * scale + (x * scale + s) * bpp + 2] = srcData[y * bppWidth + x * bpp + 2];
					}
				}
				// copy the row over the next scales
				for (int s = 1; s < scale; ++s) {
					memcpy(imgData + (y * scale + s) * rowScaledWidth, imgData + y * rowScaledWidth * scale, rowScaledWidth);
				}
			}
			canvas = wxBitmap(scaledImg);
		} else {
			// TODO downscale
		}
	} else {
		canvas = bmp;
	}
	recalcCanvasRect();
}

void BitmapCanvas::OnPaint(wxPaintEvent& evt) {
	wxBufferedPaintDC dc(this);
	if (dirtyCanvas) {
		remapCanvas();
		dirtyCanvas = false;
	} else {
		recalcCanvasRect();
	}
	dc.DrawBitmap(canvas, canvasRect.x, canvasRect.y);

	drawFill(dc, canvasRect.GetSize(), canvasRect.GetPosition());
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

void BitmapCanvas::OnMouseEvt(wxMouseEvent & evt) {
	const wxEventType evType = evt.GetEventType();
	if (evType == wxEVT_MOTION) {
		wxPoint curr(evt.GetX(), evt.GetY());
		if (curr != mousePos) {
			if (mouseLeftDrag && clippedCanvas()) {
				const wxPoint diff = curr - mousePos;
				setFocus(currentFocus - diff);
				dirtyCanvas = true;
				Refresh(); // to force redraw
			}
			mousePos = curr;
			updateStatus();
		}
	} else if (evType == wxEVT_LEFT_DOWN) {
		mousePos = wxPoint(evt.GetX(), evt.GetY());
		mouseLeftDrag = true;
	} else if (evType == wxEVT_LEFT_UP) {
		mouseLeftDrag = false;
	} else if (evType == wxEVT_ENTER_WINDOW) {
		if (!mouseOverCanvas) {
			mouseOverCanvas = true;
			updateStatus();
			Refresh();
		}
	} else if (evType == wxEVT_LEAVE_WINDOW) {
		if (mouseOverCanvas) {
			mouseOverCanvas = false;
			mouseLeftDrag = false;
			updateStatus();
			Refresh();
		}
	} else if (evType == wxEVT_MOUSEWHEEL) {
		const int delta = evt.GetWheelDelta();
		const int rot = evt.GetWheelRotation();
		int newZoomLvl = zoomLvl;
		if (rot > 0) {
			newZoomLvl += delta / 120;
		} else {
			newZoomLvl -= delta / 120;
		}
		zoomLvl = clamp(newZoomLvl, minZoom, maxZoom);
		updateStatus();
		dirtyCanvas = true;
		Refresh();
	}
}

void BitmapCanvas::OnSizeEvt(wxSizeEvent & evt) {
	if (GetAutoLayout()) {
		Layout();
	}
	// this makes the canvas dirty only if the new size requires resizing of the canvas
	if (clippedCanvas()) {
		dirtyCanvas = true;
	} else {
		resetFocus();
	}
	updateStatus();
	evt.Skip();
}
