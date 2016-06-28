#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>

#include "wx_bitmap_canvas.h"
#include "util.h"
#include "drect.h"

/************************************
*          ImageRescaler            *
*************************************/

ImageRescaler::~ImageRescaler() {
	clearCache();
}

void ImageRescaler::setBitmap(const wxBitmap & _bmp) {
	clearCache();
	bmp = _bmp;
}

wxBitmap ImageRescaler::getUpscaledSubBitmap(int scale, const wxRect & subRect) const {
	const wxSize scaledSize(subRect.GetSize() * scale);
	wxBitmap subBmp = bmp.GetSubBitmap(subRect);
	const int width = subRect.GetWidth();
	const int height = subRect.GetHeight();
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
	return wxBitmap(scaledImg);
}

wxBitmap ImageRescaler::getDownscaledSubBitmap(int scale, const wxRect & subRect) const {
	wxBitmap * fullBmp = nullptr;
	auto it = downscaleCache.find(scale);
	if (it != downscaleCache.end()) {
		fullBmp = it->second;
	} else {
		const int scaleSqr = scale * scale;
		const wxSize scaledSize = bmp.GetSize() / scale;
		const int unscaledWidth = bmp.GetWidth();
		const int width = scaledSize.GetWidth();
		const int height = scaledSize.GetHeight();
		const int bpp = 3;
		int accumulated[bpp] = { 0 };
		const wxImage sourceImg = bmp.ConvertToImage();
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
		wxBitmap * downscaledBmp = new wxBitmap(scaledImg);
		downscaleCache[scale] = downscaledBmp;
		fullBmp = downscaledBmp;
	}
	wxRect scaledSubRect = subRect;
	scaledSubRect.x /= scale;
	scaledSubRect.y /= scale;
	scaledSubRect.width /= scale;
	scaledSubRect.height /= scale;
	return fullBmp->GetSubBitmap(scaledSubRect);
}

void ImageRescaler::clearCache() {
	for (auto it = downscaleCache.begin(); it != downscaleCache.end(); ++it) {
		delete it->second;
	}
	downscaleCache.clear();
}

/************************************
*          BitmapCanvas             *
*************************************/

const int BitmapCanvas::minZoom = -8;
const int BitmapCanvas::maxZoom = 16;

BitmapCanvas::BitmapCanvas(wxWindow * parent, wxFrame * topFrame)
	: wxPanel(parent)
	, mouseOverCanvas(false)
	, mouseMoveDrag(false)
	, zoomLvl(0)
	, zoomLvlDelta(0)
	, mousePos(0, 0)
	, updatedMousePos(0, 0)
	, view(0.0f, 0.0f, 0.0f, 0.0f)
	, canvasState(CS_DIRTY_FULL)
	, topFrame(topFrame)
	, bmpId(0)
{
	SetDoubleBuffered(true);

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
	rescaler.setBitmap(bmp);
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
	Refresh(false);
}

void BitmapCanvas::updateStatus() const {
	if (mouseOverCanvas) {
		wxString posStr;
		const Vector2 mouseBmp = convertScreenToBmp(mousePos);
		posStr.Printf(wxT("Screen x: %4d y: %4d Bmp x: %7.2f y: %7.2f"), mousePos.x, mousePos.y, mouseBmp.x, mouseBmp.y);
		topFrame->SetStatusText(posStr, 1);
		wxString rectStr;
		rectStr.Printf(wxT("width: %5d, hight: %5d zoom: %2d"),
			bmp.GetWidth(), bmp.GetHeight(), zoomLvl
			);
		topFrame->SetStatusText(rectStr, 2);
		// absolutely hacky, but there seems to be no easier way
		const int x = static_cast<int>(mouseBmp.x);
		const int y = static_cast<int>(mouseBmp.y);
		wxBitmap temp = bmp.GetSubBitmap(wxRect(x, y, 1, 1));
		wxImage tempImg = temp.ConvertToImage();
		wxString rgbStr;
		rgbStr.Printf(wxT("R: %3d G: %3d B: %3d"),
			tempImg.GetRed(0, 0), tempImg.GetGreen(0, 0), tempImg.GetBlue(0, 0)
			);
		topFrame->SetStatusText(rgbStr, 3);
	}
}

void BitmapCanvas::recalcViewSize() {
	const wxSize scaledBmpSize = scale(bmp.GetSize());
	// with little hakz for the downscaling to avoid performance issues
	// always have the full bmp stored on downscales
	if (panelSize.GetWidth() >= scaledBmpSize.GetWidth()) {
		view.width = static_cast<float>(bmp.GetWidth());
		bmpClip.x = false;
	} else {
		view.width = std::min(unscale(static_cast<float>(panelSize.GetWidth())), static_cast<float>(bmp.GetWidth()));
		bmpClip.x = true;
	}
	DASSERT(view.width <= bmp.GetWidth());
	if (panelSize.GetHeight() >= scaledBmpSize.GetHeight()) {
		view.height = static_cast<float>(bmp.GetHeight());
		bmpClip.y = false;
	} else {
		view.height = std::min(unscale(static_cast<float>(panelSize.GetHeight())), static_cast<float>(bmp.GetHeight()));
		bmpClip.y = true;
	}
	DASSERT(view.height <= bmp.GetHeight());
}

void BitmapCanvas::resetViewPos() {
	view.x = (bmpClip.x ? (static_cast<float>(bmp.GetWidth()) - view.width) / 2 : 0.0f);
	DASSERT(view.x >= 0);
	view.y = (bmpClip.y ? (static_cast<float>(bmp.GetHeight()) - view.height) / 2 : 0.0f);
	DASSERT(view.y >= 0);
}

void BitmapCanvas::recalcViewPos(Vector2 p, const Rect& prevRect) {
	view.x = (bmpClip.x ? p.x - ((p.x - prevRect.x) * view.width ) / (prevRect.width ) : 0);
	view.y = (bmpClip.y ? p.y - ((p.y - prevRect.y) * view.height) / (prevRect.height) : 0);
	view.x = clamp(view.x, 0.0f, static_cast<float>(bmp.GetWidth())  - view.width - 1 );
	view.y = clamp(view.y, 0.0f, static_cast<float>(bmp.GetHeight()) - view.height - 1);
}

void BitmapCanvas::boundFixView() {
	if (view.width >= static_cast<float>(bmp.GetWidth())) {
		view.x = 0.0f;
		view.width = static_cast<float>(bmp.GetWidth());
		bmpClip.x = false;
	} else if (view.x < 0.0f) {
		view.x = 0.0f;
	} else if (view.x + view.width >= static_cast<float>(bmp.GetWidth())) {
		view.width = ceilf(view.width);
		view.x = roundf(static_cast<float>(bmp.GetWidth()) - view.width);
	}
	if (view.height >= static_cast<float>(bmp.GetHeight())) {
		view.y = 0.0f;
		view.height = static_cast<float>(bmp.GetHeight());
		bmpClip.y = false;
	} else if (view.y < 0.0f) {
		view.y = 0.0f;
	} else if (view.y + view.height >= static_cast<float>(bmp.GetHeight())) {
		view.height = ceilf(view.height);
		view.y = roundf(static_cast<float>(bmp.GetHeight()) - view.height);
	}
}

void BitmapCanvas::addSynchronizer(BitmapCanvas * s) {
	synchronizers.push_back(s);
}

void BitmapCanvas::synchronize() {
	for (auto i = synchronizers.begin(); i != synchronizers.end(); ++i) {
		BitmapCanvas * s = *i;
		// this is improtant because it gurads us from bouncing from one synchronization to another recursively
		// and only if the two ids match, unmatching ids mean that the output image was not generated yet
		if (bmpId == s->bmpId) {
			s->zoomLvl = this->zoomLvl;
			s->view = this->view;
			s->canvasState = CS_DIRTY_SYNCHRONIZE;
			s->Refresh(false);
		}
	}
}

wxPoint BitmapCanvas::getCanvasTopLeftInScreen() const {
	return wxPoint(
		(bmpClip.x ? 0 : (panelSize.GetWidth() - static_cast<int>(scale(view.getWidth()))) / 2),
		(bmpClip.y ? 0 : (panelSize.GetHeight() - static_cast<int>(scale(view.getHeight()))) / 2)
		);
}

Vector2 BitmapCanvas::convertScreenToBmp(const wxPoint& in) const {
	const Vector2 unscaled = unscale(Convert::vector(in - getCanvasTopLeftInScreen()));
	const Vector2 bmpCoord = view.getTopLeft() + unscaled;
	return Vector2(
		clamp(bmpCoord.x, -1.0f, static_cast<float>(bmp.GetWidth())),
		clamp(bmpCoord.y, -1.0f, static_cast<float>(bmp.GetHeight()))
		);
}

wxPoint BitmapCanvas::convertBmpToScreen(const Vector2& in) const {
	const Vector2 clippedBmpCoord = in - view.getTopLeft();
	Vector2 scaled = scale(clippedBmpCoord);
	const wxPoint canvasTopLeft = getCanvasTopLeftInScreen();// + canvasRect.GetTopLeft();
	scaled.x += (bmpClip.x ? 0.0f : canvasTopLeft.x);
	scaled.y += (bmpClip.y ? 0.0f : canvasTopLeft.y);
	return wxPoint(
		clamp(static_cast<int>(roundf(scaled.x)), -1, panelSize.GetWidth()),
		clamp(static_cast<int>(roundf(scaled.y)), -1, panelSize.GetHeight())
		);
}

void BitmapCanvas::remapCanvas() {
	if (canvasState == CS_CLEAN || bmp.GetWidth() == 0 || bmp.GetHeight() == 0)
		return;
	// first obtain all necessary previus state stuff before zoom level changes
	view.x = clamp(view.x, 0.0f, static_cast<float>(bmp.GetWidth())  - view.width - 1);
	view.y = clamp(view.y, 0.0f, static_cast<float>(bmp.GetHeight()) - view.height - 1);
	if ((canvasState & CS_DIRTY_SYNCHRONIZE) == CS_DIRTY_SYNCHRONIZE) {
		const wxSize scaledBmpSize = scale(bmp.GetSize());
		bmpClip.x = panelSize.GetWidth() < scaledBmpSize.GetWidth();
		bmpClip.y = panelSize.GetHeight() < scaledBmpSize.GetHeight();
		zoomLvlDelta = 0;
	} else {
		const Rect prevBmpRect = view;
		const Vector2 bmpMousePos = convertScreenToBmp(mousePos);
		// now update zoom if necessary
		if ((canvasState & CS_DIRTY_ZOOM) != 0 && 0 != zoomLvlDelta) {
			zoomLvl += zoomLvlDelta;
			// reset the delta
			zoomLvlDelta = 0;
		}
		if ((canvasState & (CS_DIRTY_SIZE | CS_DIRTY_ZOOM)) != 0) {
			recalcViewSize();
		}
		if (canvasState == CS_DIRTY_FULL) {
			resetViewPos();
		} else {
			if ((canvasState & CS_DIRTY_ZOOM) != 0) {
				recalcViewPos(bmpMousePos, prevBmpRect);
			} else if ((canvasState & CS_DIRTY_SIZE) != 0) {
				const Vector2 halfDelta = Vector2(
					prevBmpRect.width - view.width,
					prevBmpRect.height - view.height
				) / 2.0f;
				view.setPosition(prevBmpRect.getPosition() + halfDelta);
			} else if ((canvasState & CS_DIRTY_POS) != 0) {
				wxPoint screenDelta = mousePos - updatedMousePos;
				Vector2 bmpDelta = unscale(Convert::vector(screenDelta));
				view.setPosition(view.getPosition() - bmpDelta);
				updatedMousePos = mousePos;
			}
		}
		synchronize();
	}
	const Size2d bmpSize = Convert::size(bmp.GetSize());
	boundFixView(); // fix any bound errors
	if (view.getSize() != bmpSize || zoomLvl != 0) {
		if (zoomLvl == 0) {
			canvas = bmp.GetSubBitmap(Convert::rect(view));
		} else if (zoomLvl > 0) {
			const int scale = zoomLvl + 1;
			const wxRect subRect = Convert::rect(view);
			canvas = rescaler.getUpscaledSubBitmap(scale, subRect);
		} else {
			const int scale = -zoomLvl + 1;
			const wxRect subRect = Convert::rect(view);
			canvas = rescaler.getDownscaledSubBitmap(scale, subRect);
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
	const Vector2 offset = Vector2(floorf(view.x) - view.x, floorf(view.y) - view.y);
	const wxPoint scaledOffset = Convert::vector(scale(offset));
	const wxPoint canvasPos = getCanvasTopLeftInScreen();
	const wxPoint drawPos = scaledOffset + canvasPos;
	dc.DrawBitmap(canvas, drawPos);

	drawFill(dc, canvas.GetSize(), drawPos);
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
			if (mouseMoveDrag && (bmpClip.any != 0) && curr != updatedMousePos) {
				canvasState |= CS_DIRTY_POS;
				Refresh(false);
			}
			mousePos = curr;
			updateStatus();
		}
	} else if (evType == wxEVT_LEFT_DOWN) {
		mousePos = wxPoint(evt.GetX(), evt.GetY());
		mouseMoveDrag = true;
		updatedMousePos = mousePos;
	} else if (evType == wxEVT_LEFT_UP) {
		mouseMoveDrag = false;
	} else if (evType == wxEVT_ENTER_WINDOW) {
		if (!mouseOverCanvas) {
			SetFocus();
			mouseOverCanvas = true;
			updateStatus();
			Refresh(false);
		}
	} else if (evType == wxEVT_LEAVE_WINDOW) {
		if (mouseOverCanvas) {
			mouseOverCanvas = false;
			mouseMoveDrag = false;
			updateStatus();
			Refresh(false);
		}
	} else if (evType == wxEVT_MOUSEWHEEL) {
		const int delta = evt.GetWheelDelta();
		const int rot = evt.GetWheelRotation();
		zoomLvlDelta = (rot > 0 ? delta / 120 : -delta / 120);
		const int newZoomLvl = clamp(zoomLvl + zoomLvlDelta, minZoom, maxZoom);
		if (newZoomLvl != zoomLvl) {
			canvasState |= CS_DIRTY_ZOOM;
			Refresh(false);
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
	Refresh(false);
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
	Connect(wxEVT_SIZE, wxSizeEventHandler(HistogramPanel::OnSizeEvent), NULL, this);
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
			chData[(y * chw + x) * 3 + 0] = (chh - y <= static_cast<int>(chunk.r) * chh / maxColor ? histFgColor.r : histBkgColor.r);
			chData[(y * chw + x) * 3 + 1] = (chh - y <= static_cast<int>(chunk.g) * chh / maxColor ? histFgColor.g : histBkgColor.g);
			chData[(y * chw + x) * 3 + 2] = (chh - y <= static_cast<int>(chunk.b) * chh / maxColor ? histFgColor.b : histBkgColor.b);
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
			const bool t = (ihh - y <= static_cast<int>(hist[xh].i) * ihh / maxIntensity);
			ihData[(y * ihw + x) * 3 + 0] = ( t ? histFgIntensity.r : histBkgIntensity.r);
			ihData[(y * ihw + x) * 3 + 1] = ( t ? histFgIntensity.g : histBkgIntensity.g);
			ihData[(y * ihw + x) * 3 + 2] = ( t ? histFgIntensity.b : histBkgIntensity.b);
		}
	}
	pdc.DrawBitmap(wxBitmap(ihist), wxPoint(0, cHistSize.GetHeight()));
}

void HistogramPanel::OnEraseBkg(wxEraseEvent & evt) {}

void HistogramPanel::OnSizeEvent(wxSizeEvent & evt) {
	if (GetAutoLayout()) {
		Layout();
	}
	Refresh(false);
}

ImagePanel::ImagePanel(wxWindow * parent, wxFrame * topFrame, const Bitmap * initBmp)
	: wxPanel(parent)
	, canvas(new BitmapCanvas(this, topFrame))
	, histPanel(new HistogramPanel(this))
	, panelSizer(new wxBoxSizer(wxVERTICAL))
{
	panelSizer->Add(canvas, 1, wxEXPAND | wxSHRINK);
	histPanel->SetInitialSize(wxSize(-1, 256));
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
	canvas->synchronize();
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

wxImage ImagePanel::getImage() {
	return canvas->bmp.ConvertToImage();
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

void ImagePanel::moduleDone(ModuleBase::ProcessResult result) {
	if (result == ModuleBase::KPR_OK) {
		canvas->synchronize();
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
