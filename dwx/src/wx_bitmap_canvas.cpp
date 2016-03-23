#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>

#include "wx_bitmap_canvas.h"
#include "util.h"

const int BitmapCanvas::minZoom = -8;
const int BitmapCanvas::maxZoom = 16;

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
	, bmpId(0)
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
		const int bmpDim = 129;
		empty.generateEmptyImage(bmpDim, bmpDim);
		const Color border(.5f, .5f, .5f);
		Color * bmpData = empty.getDataPtr();
		for (int i = 0; i < bmpDim; ++i) {
			bmpData[i] = border;
			bmpData[i * bmpDim] = border;
			bmpData[bmpDim * (bmpDim - 1) + i] = border;
			bmpData[bmpDim * i + bmpDim - 1] = border;
			// second row
			bmpData[bmpDim + i] = border;
			bmpData[i * bmpDim + 1] = border;
			bmpData[bmpDim * (bmpDim - 2) + i] = border;
			bmpData[bmpDim * i + bmpDim - 2] = border;
		}
		setBitmap(empty);
	}
	Update();
}

void BitmapCanvas::setBitmap(const Bitmap& bmp, int id) {
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
	setImage(tmpImg, id);
}

void BitmapCanvas::setImage(const wxImage & img, int id) {
	bmp = wxBitmap(img);
	if (id == 0) {
		int rndId = rand();
		while (rndId == 0) {
			rndId = rand();
		}
		bmpId = rndId;
	} else {
		bmpId = id;
	}
	if (id == 0) {
		resetFocus();
	}
	dirtyCanvas = true;
	Refresh();
}

int BitmapCanvas::getBmpId() const {
	return bmpId;
}

void BitmapCanvas::updateStatus() const {
	if (true || mouseOverCanvas) {
		wxString focusStr;
		const wxPoint mouseBmp = convertScreenToBmp(mousePos);
		const wxPoint screenPos = convertBmpToScreen(mouseBmp);
		focusStr.Printf(wxT("bmp: ( %4d, %4d) screen: ( %4d, %4d)"), mouseBmp.x, mouseBmp.y, screenPos.x, screenPos.y);
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

void BitmapCanvas::addSynchronizer(BitmapCanvas * s) {
	synchronizers.push_back(s);
}

void BitmapCanvas::synchronize() {
	for (auto i = synchronizers.begin(); i != synchronizers.end(); ++i) {
		BitmapCanvas * s = *i;
		const bool needsUpdate = (zoomLvl != s->zoomLvl || currentFocus != s->currentFocus);
		// this is improtant because it gurads us from bouncing from one synchronization to another recursively
		// and only if the two ids match, unmatching ids mean that the output image was not generated yet
		if (needsUpdate && bmpId == s->bmpId) {
			s->zoomLvl = this->zoomLvl;
			s->setFocus(currentFocus);
			s->dirtyCanvas = true;
			s->Refresh();
		}
	}
}

bool BitmapCanvas::getInput(Bitmap & ibmp, int & id) const {
	bool retval = false;
	if (bmp.Ok()) {
		const wxImage img(bmp.ConvertToImage());
		const int w = img.GetWidth();
		const int h = img.GetHeight();
		ibmp.generateEmptyImage(w, h);
		const unsigned char * imgData = img.GetData();
		for (int dh = 0; dh < h; ++dh) {
			for (int dw = 0; dw < w; ++dw) {
				const unsigned char * rgb = imgData + dh * w * 3 + dw * 3;
				ibmp[dh][dw] = Color(rgb[0], rgb[1], rgb[2]);
			}
		}
		id = bmpId;
		retval = true;
	}
	return retval;
}

void BitmapCanvas::kernelDone(KernelBase::ProcessResult result) {
	if (result == KernelBase::KPR_OK) {
		synchronize();
	}
}

void BitmapCanvas::setOutput(const Bitmap & obmp, int id) {
	setBitmap(obmp, id);
}

void BitmapCanvas::resetFocus() {
	currentFocus = wxPoint(bmp.GetWidth() / 2, bmp.GetHeight() / 2);
}

void BitmapCanvas::setFocus(wxPoint f) {
	const wxSize uphs = unscale(GetSize()) / 2; // unscaledPanelHalfSize
	currentFocus.x = clamp(f.x, uphs.GetWidth(), bmp.GetWidth() - uphs.GetWidth());
	currentFocus.y = clamp(f.y, uphs.GetHeight(), bmp.GetHeight() - uphs.GetHeight());
}

bool BitmapCanvas::clippedCanvas() const {
	const wxSize unscaledBmpSize = unscale(bmp.GetSize());
	const wxSize panelSize = GetSize();
	return unscaledBmpSize.GetWidth() > panelSize.GetWidth() || unscaledBmpSize.GetHeight() < panelSize.GetHeight() || bmpRect.x > 0 || bmpRect.y > 0;
}

wxPoint BitmapCanvas::convertScreenToBmp(const wxPoint in) const {
	const wxPoint unscaled = unscale(in - canvasRect.GetTopLeft());
	const wxPoint bmpCoord = bmpRect.GetTopLeft() + unscaled;
	return wxPoint(
		clamp(bmpCoord.x, -1, bmp.GetWidth()),
		clamp(bmpCoord.y, -1, bmp.GetHeight())
		);
}

wxPoint BitmapCanvas::convertBmpToScreen(const wxPoint in) const {
	const wxPoint clippedBmpCoord = bmpRect.GetTopLeft() + in;
	const wxPoint scaled = scale(clippedBmpCoord) + canvasRect.GetTopLeft();
	const wxSize panelSize = GetSize();
	return wxPoint(
		clamp(scaled.x, -1, panelSize.GetWidth()),
		clamp(scaled.y, -1, panelSize.GetHeight())
		);
}

void BitmapCanvas::recalcBmpRect() {
	const wxSize panelSize = GetSize();
	const wxSize unscaledSize = unscale(panelSize);
	const wxSize rescaledSize = scale(unscaledSize); //!< for remainder offsets
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
			const wxSize scaledSize = this->scale(subBmp.GetSize());
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
			const int scale = -zoomLvl + 1;
			const int scaleSqr = scale * scale;
			wxBitmap subBmp = bmp.GetSubBitmap(bmpRect);
			const wxSize scaledSize = this->scale(subBmp.GetSize());
			const int unscaledWidth = subBmp.GetWidth();
			const int width = scaledSize.GetWidth();
			const int height = scaledSize.GetHeight();
			const int bpp = 3;
			int accumulated[bpp] = { 0 };
			const wxImage sourceImg = subBmp.ConvertToImage();
			wxImage scaledImg(scaledSize);
			const unsigned char * srcData = sourceImg.GetData();
			unsigned char * imgData = scaledImg.GetData();
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					memset(accumulated, 0, bpp * sizeof(accumulated[0]));
					for (int s = 0; s < scaleSqr; ++s) {
						const int row = s / scale;
						const int column = s % scale;
						accumulated[0] += srcData[((y * scale + row) * unscaledWidth) * bpp + (x * scale + column) * bpp + 0];
						accumulated[1] += srcData[((y * scale + row) * unscaledWidth) * bpp + (x * scale + column) * bpp + 1];
						accumulated[2] += srcData[((y * scale + row) * unscaledWidth) * bpp + (x * scale + column) * bpp + 2];
					}
					for (int i = 0; i < bpp; ++i) {
						accumulated[i] /= scaleSqr;
					}
					imgData[(y * width + x) * bpp + 0] = static_cast<unsigned char>(accumulated[0]);
					imgData[(y * width + x) * bpp + 1] = static_cast<unsigned char>(accumulated[1]);
					imgData[(y * width + x) * bpp + 2] = static_cast<unsigned char>(accumulated[2]);
				}
			}
			canvas = wxBitmap(scaledImg);
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
		// call the synchronization routine, so we can update the synchronized canvases as well
		synchronize();
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
