#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>

#include "wx_bitmap_canvas.h"
#include "util.h"

const int BitmapCanvas::minZoom = -8;
const int BitmapCanvas::maxZoom = 16;

BitmapCanvas::BitmapCanvas(wxWindow * parent, wxFrame * topFrame)
	: wxPanel(parent)
	, mouseOverCanvas(false)
	, mouseLeftDrag(false)
	, zoomLvl(0)
	, mousePos(0, 0)
	, bmpRect(0, 0, 0, 0)
	, canvasRect(0, 0, 0, 0)
	, canvasState(CS_DIRTY_FULL)
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

	Update();
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
	canvasState = CS_DIRTY_FULL;
	if (id == 0) {
		// TODO - reset Focus
		//resetFocus();
	}
	Refresh();
}

void BitmapCanvas::updateStatus() const {
	if (mouseOverCanvas) {
		//wxString focusStr;
		//const wxPoint mouseBmp = convertScreenToBmp(mousePos);
		//const wxPoint screenPos = convertBmpToScreen(mouseBmp);
		//focusStr.Printf(wxT("bmp: ( %4d, %4d) screen: ( %4d, %4d)"), mouseBmp.x, mouseBmp.y, screenPos.x, screenPos.y);
		//topFrame->SetStatusText(focusStr);
		wxString posStr;
		const wxPoint mouseBmp = convertScreenToBmp(mousePos);
		posStr.Printf(wxT("Screen x: %4d y: %4d Bmp x: %4d y: %4d"), mousePos.x, mousePos.y, mouseBmp.x, mouseBmp.y);
		topFrame->SetStatusText(posStr, 1);
		wxString rectStr;
		rectStr.Printf(wxT("bmpRect(%d, %d, %d, %d) w: %d, h: %d"),
			bmpRect.x, bmpRect.y, bmpRect.width, bmpRect.height,
			bmp.GetWidth(), bmp.GetHeight()
			);
		topFrame->SetStatusText(rectStr, 2);
		wxString canvStr;
		canvStr.Printf(wxT("zoom: %d canvas(%d, %d, %d, %d) w: %d, h: %d"),
			zoomLvl,
			canvasRect.x, canvasRect.y, canvasRect.width, canvasRect.height,
			canvas.GetWidth(), canvas.GetHeight());
		topFrame->SetStatusText(canvStr, 3);
	}/* else {
		topFrame->SetStatusText(wxT(""), 1);
		topFrame->SetStatusText(wxT(""), 2);
	}*/
}

void BitmapCanvas::recalcBmpRectSize() {
	const wxSize scaledBmpSize = scale(bmp.GetSize());
	// with little hakz for the downscaling to avoid performance issues
	// always have the full bmp stored on downscales
	if (panelSize.GetWidth() >= scaledBmpSize.GetWidth() || zoomLvl < 0) {
		bmpRect.width = bmp.GetWidth();
		bmpClip.x = panelSize.GetWidth() < scaledBmpSize.GetWidth();
	} else {
		bmpRect.width = std::min(unscale(panelSize.GetWidth()) + 2, bmp.GetWidth());
		bmpClip.x = true;
	}
	DASSERT(bmpRect.width <= bmp.GetSize().GetWidth());
	if (panelSize.GetHeight() >= scaledBmpSize.GetHeight() || zoomLvl < 0) {
		bmpRect.height = bmp.GetHeight();
		bmpClip.y = panelSize.GetHeight() < scaledBmpSize.GetHeight();
	} else {
		bmpRect.height = std::min(unscale(panelSize.GetHeight()) + 2, bmp.GetHeight());
		bmpClip.y = true;
	}
	DASSERT(bmpRect.height <= bmp.GetSize().GetHeight());
}

void BitmapCanvas::resetBmpRectPos() {
	bmpRect.x = (bmpClip.x ? (bmp.GetWidth() - bmpRect.width) / 2 : 0);
	DASSERT(bmpRect.x >= 0);
	bmpRect.y = (bmpClip.y ? (bmp.GetHeight() - bmpRect.height) / 2 : 0);
	DASSERT(bmpRect.y >= 0);
}

void BitmapCanvas::recalcBmpRectPos(wxPoint p, const wxRect& prevRect) {
	bmpRect.x = (bmpClip.x ? p.x - ((p.x - prevRect.x) * bmpRect.width ) / (prevRect.width ) : 0);
	bmpRect.y = (bmpClip.y ? p.y - ((p.y - prevRect.y) * bmpRect.height) / (prevRect.height) : 0);
	bmpRect.x = clamp(bmpRect.x, 0, bmp.GetWidth()  - bmpRect.width - 1 );
	bmpRect.y = clamp(bmpRect.y, 0, bmp.GetHeight() - bmpRect.height - 1);
#if 0 // this was the alternative idea with making calculations based on the screen center - doesn't work...
	const wxPoint cp((prevRect.x + prevRect.width) / 2, (prevRect.y + prevRect.height) / 2);
	const wxPoint delta = cp - p;
	const wxPoint cn(p.x - (delta.x * prevRect.width) / bmpRect.width, p.y - (delta.y * prevRect.height) / bmpRect.height);
	const wxPoint n(cn.x - bmpRect.width / 2, cn.y - bmpRect.height / 2);
	bmpRect.x = clamp(n.x, 0, bmp.GetWidth() - bmpRect.width - 1);
	bmpRect.y = clamp(n.y, 0, bmp.GetHeight() - bmpRect.height - 1);
#endif
}

void BitmapCanvas::recalcCanvasRectSize() {
	canvasRect.width = (bmpClip.x ? panelSize.GetWidth() : scale(bmpRect.width));
	canvasRect.height = (bmpClip.y ? panelSize.GetHeight() : scale(bmpRect.height));
}
#if 0
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
#endif // disabled

void BitmapCanvas::resetCanvasRectPos() {
	const wxSize scaledBmpRectSize = scale(bmpRect.GetSize());
	canvasRect.x = (bmpClip.x ? (scaledBmpRectSize.GetWidth() - canvasRect.width) / 2 : 0);
	canvasRect.y = (bmpClip.y ? (scaledBmpRectSize.GetHeight() - canvasRect.height) / 2 : 0);
}

wxPoint BitmapCanvas::getCanvasTopLeftInScreen() const {
	return wxPoint(
		(bmpClip.x ? 0 : (panelSize.GetWidth() - canvasRect.width) / 2),
		(bmpClip.y ? 0 : (panelSize.GetHeight() - canvasRect.height) / 2)
		);
}

wxPoint BitmapCanvas::convertScreenToBmp(const wxPoint in) const {
	const wxPoint unscaled = unscale(in - getCanvasTopLeftInScreen() + canvasRect.GetTopLeft());
	const wxPoint bmpCoord = bmpRect.GetTopLeft() + unscaled;
	return wxPoint(
		clamp(bmpCoord.x, -1, bmp.GetWidth()),
		clamp(bmpCoord.y, -1, bmp.GetHeight())
		);
}

wxPoint BitmapCanvas::convertBmpToScreen(const wxPoint in) const {
	const wxPoint clippedBmpCoord = in - bmpRect.GetTopLeft();
	wxPoint scaled = scale(clippedBmpCoord);
	const wxPoint canvasTopLeft = getCanvasTopLeftInScreen() + canvasRect.GetTopLeft();
	scaled.x += (bmpClip.x ? -canvasRect.x : canvasTopLeft.x);
	scaled.y += (bmpClip.y ? -canvasRect.y : canvasTopLeft.y);
	return wxPoint(
		clamp(scaled.x, -1, panelSize.GetWidth()),
		clamp(scaled.y, -1, panelSize.GetHeight())
		);
}

void BitmapCanvas::remapCanvas() {
	if (canvasState == CS_CLEAN || bmp.GetWidth() == 0 || bmp.GetHeight() == 0)
		return;
	// first obtain all necessary previus state stuff before zoom level changes
	bmpRect.x = clamp(bmpRect.x, 0, bmp.GetWidth() - bmpRect.width - 1);
	bmpRect.y = clamp(bmpRect.y, 0, bmp.GetHeight() - bmpRect.height - 1);
	const wxRect prevBmpRect = bmpRect;
	const wxPoint bmpMousePos = convertScreenToBmp(mousePos);
	// now update zoom if necessary
	if ((canvasState & CS_DIRTY_ZOOM) != 0 && 0 != zoomLvlDelta) {
		zoomLvl += zoomLvlDelta;
		// reset the delta
		zoomLvlDelta = 0;
	}
	if ((canvasState & (CS_DIRTY_SIZE | CS_DIRTY_ZOOM)) != 0) {
		recalcBmpRectSize();
	}
	if (canvasState == CS_DIRTY_FULL) {
		// todo change with preserved point for convenience
		resetBmpRectPos();
	} else {
		if ((canvasState & (CS_DIRTY_SIZE | CS_DIRTY_ZOOM)) != 0) {
			const wxPoint absolutePreserve = ((canvasState & CS_DIRTY_ZOOM) != 0 ?
				bmpMousePos :
				wxPoint(
					(prevBmpRect.x + prevBmpRect.width) / 2,
					(prevBmpRect.y + prevBmpRect.height) / 2
					)
				);
			recalcBmpRectPos(absolutePreserve, prevBmpRect);
		} else {
			// maybe move ?
		}
	}
	recalcCanvasRectSize();
	resetCanvasRectPos();
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
		} else if (scale(bmpRect.GetSize()) != canvas.GetSize()) {
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
	updateStatus();
	canvasState = CS_CLEAN;
}

void BitmapCanvas::OnPaint(wxPaintEvent& evt) {
	wxBufferedPaintDC dc(this);
	remapCanvas();
	const wxPoint drawPos(getCanvasTopLeftInScreen() - canvasRect.GetTopLeft());
	dc.DrawBitmap(canvas, drawPos);

	drawFill(dc, canvasRect.GetSize(), canvasRect.GetTopLeft() + drawPos);
}

void BitmapCanvas::drawFill(wxBufferedPaintDC & pdc, const wxSize & bmpSize, const wxPoint & bmpCoord) {
	const int bx = bmpCoord.x;
	const int by = bmpCoord.y;
	const int bw = bmpSize.GetWidth();
	const int bh = bmpSize.GetHeight();
	const int dw = panelSize.GetWidth();
	const int dh = panelSize.GetHeight();
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
			if (mouseLeftDrag && (bmpClip.any != 0)) {
				//const wxPoint diff = curr - mousePos;
				//setFocus(currentFocus - diff);
				//dirtyCanvas = true;
				//Refresh(); // to force redraw
				canvasState |= CS_DIRTY_POS;
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
		zoomLvlDelta = (rot > 0 ? delta / 120 : -delta / 120);
		const int newZoomLvl = clamp(zoomLvl + zoomLvlDelta, minZoom, maxZoom);
		if (newZoomLvl != zoomLvl) {
			canvasState |= CS_DIRTY_ZOOM;
			Refresh();
		} else {
			zoomLvlDelta = 0;
		}
	}
}

void BitmapCanvas::OnSizeEvt(wxSizeEvent & evt) {
	if (GetAutoLayout()) {
		Layout();
	}
	panelSize = GetSize();
	// this makes the canvas dirty only if the new size requires resizing of the canvas
	canvasState |= CS_DIRTY_SIZE;
	evt.Skip();
}

/************************************
*         HistogramPanel            *
*************************************/

const Color HistogramPanel::histFgColor = Color(0x70, 0x70, 0x70);
const Color HistogramPanel::histBkgColor = Color(16, 16, 16);
const Color HistogramPanel::histFgIntensity = Color(0x70, 0x70, 0x70);
const Color HistogramPanel::histBkgIntensity = Color(16, 16, 16);

HistogramPanel::HistogramPanel(wxWindow * parent)
	: wxPanel(parent)
{
	// connect paint events
	Connect(wxEVT_PAINT, wxPaintEventHandler(HistogramPanel::OnPaint), NULL, this);
	Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(HistogramPanel::OnEraseBkg), NULL, this);
}

void HistogramPanel::setImage(const Bitmap & bmp) {
	hist.fromBmp(bmp);
	if (IsShown()) {
		Refresh();
	}
}

void HistogramPanel::OnPaint(wxPaintEvent & evt) {
	wxBufferedPaintDC pdc(this);
	const wxSize histSize = GetSize();
	const wxSize cHistSize = wxSize(histSize.GetWidth(), histSize.GetHeight() / 2);

	wxImage chist(cHistSize);
	unsigned char * chData = chist.GetData();
	const int chh = cHistSize.GetHeight();
	const int chw = cHistSize.GetWidth();
	const float widthRatioRecip = hist.channelSize / float(chw);
	const int maxColor = hist.getMaxColor();
	for (int y = 0; y < chh; ++y) {
		for (int x = 0; x < chw; ++x) {
			const int xh = int(x * widthRatioRecip);
			const HistogramChunk& chunk = hist[xh];
			chData[(y * chw + x) * 3 + 0] = (chh - y <= chunk.r * chh / maxColor ? histFgColor.r : histBkgColor.r);
			chData[(y * chw + x) * 3 + 1] = (chh - y <= chunk.g * chh / maxColor ? histFgColor.g : histBkgColor.g);
			chData[(y * chw + x) * 3 + 2] = (chh - y <= chunk.b * chh / maxColor ? histFgColor.b : histBkgColor.b);
		}
	}
	pdc.DrawBitmap(wxBitmap(chist), wxPoint(0, 0));

	const wxSize iHistSize = wxSize( histSize.GetWidth(), histSize.GetHeight() / 2 + (histSize.GetHeight() & 1));
	wxImage ihist(iHistSize);
	unsigned char * ihData = ihist.GetData();
	const int ihh = iHistSize.GetHeight();
	const int ihw = iHistSize.GetWidth();
	const float widthRatioRecipInt = hist.channelSize / float(ihw);
	const int maxIntensity = hist.getMaxIntensity();
	for (int y = 0; y < ihh; ++y) {
		for (int x = 0; x < ihw; ++x) {
			const int xh = int(x * widthRatioRecipInt);
			const bool t = (ihh - y <= hist[xh].i * ihh / maxIntensity);
			ihData[(y * ihw + x) * 3 + 0] = ( t ? histFgIntensity.r : histBkgIntensity.r);
			ihData[(y * ihw + x) * 3 + 1] = ( t ? histFgIntensity.g : histBkgIntensity.g);
			ihData[(y * ihw + x) * 3 + 2] = ( t ? histFgIntensity.b : histBkgIntensity.b);
		}
	}
	pdc.DrawBitmap(wxBitmap(ihist), wxPoint(0, cHistSize.GetHeight()));
}

void HistogramPanel::OnEraseBkg(wxEraseEvent & evt) {}

ImagePanel::ImagePanel(wxWindow * parent, wxFrame * topFrame, const Bitmap * initBmp)
	: wxPanel(parent)
	, canvas(new BitmapCanvas(this, topFrame))
	, histPanel(new HistogramPanel(this))
	, panelSizer(new wxBoxSizer(wxVERTICAL))
{
	panelSizer->Add(canvas, 1, wxEXPAND | wxSHRINK);
	histPanel->SetBestFittingSize(wxSize(-1, 256));
	panelSizer->Add(histPanel, 0, wxEXPAND | wxSHRINK);
	histPanel->Hide();
	SetSizerAndFit(panelSizer);
	if (nullptr != initBmp) {
		std::lock_guard<std::mutex> lk(bmpMutex);
		bmp = *initBmp;
	} else {
		const int side = 8;
		const int bmpSide = side * side * 16;
		const int bmpFullSize = bmpSide * bmpSide;
		const Color dark(0, 0, 0);
		const Color bright(128, 128, 128);
		const Color offset(32, 32, 32);
		const int colRange = 255 - 128 - 32;
		std::lock_guard<std::mutex> lk(bmpMutex);
		bmp.generateEmptyImage(bmpSide, bmpSide);
		const int centerSide = bmpSide / (side * 2);
		Color * bmpData = bmp.getDataPtr();
		for (int y = 0; y < bmpSide; ++y) {
			for (int x = 0; x < bmpSide; ++x) {
				Color& c = bmpData[y * bmpSide + x];
				const int xd = x / side;
				const int yd = y / side;
				if (((xd + 1) == centerSide || xd == centerSide) &&
						((yd + 1) == centerSide || yd == centerSide)) {
					c.components[(xd + yd + 0) % 3] = 255;
					c.components[(xd + yd + 1) % 3] = 0;
					c.components[(xd + yd + 2) % 3] = 0;
				} else {
					c = ((xd + yd) & 1 ? bright : dark);
					const int xdd = xd / side;
					const int ydd = yd / side;
					if ((xdd + ydd) & 1) {
						c += offset;
					}
					c.r += (x * colRange) / bmpSide;
					c.b += (y * colRange) / bmpSide;
					c.g += colRange / 2;
				}
			}
		}
	}
	setOutput(bmp, 0);
	SendSizeEvent();
}

int ImagePanel::getBmpId() const {
	return bmpId;
}

void ImagePanel::synchronize() {
	//canvas->synchronize();
}

void ImagePanel::setImage(const wxImage & img, int id) {
	if (id == 0) {
		int rndId = rand();
		while (rndId == 0) {
			rndId = rand();
		}
		bmpId = rndId;
	} else {
		bmpId = id;
	}
	canvas->setImage(img, bmpId);
	if (id == 0) {
		// TODO
		//canvas->resetFocus();
	}
	{
		std::lock_guard<std::mutex> lk(bmpMutex);
		const int w = img.GetWidth();
		const int h = img.GetHeight();
		bmp.generateEmptyImage(w, h);
		if (bmp.isOK()) {
			Color * bmpData = bmp.getDataPtr();
			memcpy(bmpData, img.GetData(), w * h * sizeof(Color));
			histPanel->setImage(bmp);
		}
	}
	Refresh();
}

void ImagePanel::toggleHist() {
	histPanel->Show(!histPanel->IsShown());
}

bool ImagePanel::getInput(Bitmap & ibmp, int & id) const {
	bool retval = false;
	{
		std::lock_guard<std::mutex> lk(bmpMutex);
		if (bmp.isOK()) {
			ibmp = bmp;
			id = bmpId;
			retval = true;
		}
	}
	return retval;
}

void ImagePanel::kernelDone(KernelBase::ProcessResult result) {
	if (result == KernelBase::KPR_OK) {
		//canvas->synchronize();
	}
}

void ImagePanel::setOutput(const Bitmap & obmp, int id) {
	//canvas->setBitmap(obmp, id);
	if (obmp.isOK()) {
		std::lock_guard<std::mutex> lk(bmpMutex);
		bmp = obmp;
		histPanel->setImage(bmp);
		Color * bmpDataPtr = bmp.getDataPtr();
		wxImage canvasImg(wxSize(obmp.getWidth(), obmp.getHeight()), reinterpret_cast<unsigned char *>(bmpDataPtr), true);
		canvas->setImage(canvasImg, id);
	}
}

BitmapCanvas * ImagePanel::getCanvas() const {
	return canvas;
}
