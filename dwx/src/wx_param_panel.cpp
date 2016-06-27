#include <wx/panel.h>

#include "util.h"
#include "vector2.h"
#include "wx_param_panel.h"
#include "wx_modes.h"
#include "guimain.h"

CKernelDlg::CKernelDlg(CKernelPanel * parent, const wxString& title, long style)
	: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, style)
{}

const wxString CKernelTableDlg::symmetryName[CKernelTableDlg::ST_COUNT] = {
	wxT("None"),    //!< ST_NO_SYMMETRY
	wxT("Central"),	//!< ST_CENTRAL
	wxT("Radial"),	//!< ST_RADIAL
};

CKernelTableDlg::CKernelTableDlg(CKernelPanel * parent, const wxString& title, int side)
	: CKernelDlg(parent, title, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, currentSymmetry(ST_NO_SYMMETRY)
	, paramPanel(parent)
	, sideCtrl(nullptr)
	, sideButton(nullptr)
	, symmetryRb{ nullptr }
	, symmetryRbId(0)
	, sumText(nullptr)
	, kernelParamsId(0)
	, normalizeButton(nullptr)
	, normalizationValue(nullptr)
	, resetButton(nullptr)
	, kernelSide(0)
{
	Connect(wxEVT_SHOW, wxShowEventHandler(CKernelTableDlg::OnShow), NULL, this);

	sideCtrl = new wxTextCtrl(this, wxID_ANY, wxString() << side);
	const wxWindowID sideButtonId = WinIDProvider::getProvider().getId();
	sideButton = new wxButton(this, sideButtonId, "Change Side");
	Connect(sideButtonId, wxEVT_BUTTON, wxCommandEventHandler(CKernelTableDlg::OnKernelSide), NULL, this);

	sumText = new wxStaticText(this, wxID_ANY, wxT("0"));
	sumText->SetSizeHints(wxSize(20, -1));
	symmetryRbId = WinIDProvider::getProvider().getId(ST_COUNT);
	for (int i = 0; i < ST_COUNT; ++i) {
		const wxWindowID rbId = symmetryRbId + i;
		symmetryRb[i] = new wxRadioButton(this, rbId, symmetryName[i], wxDefaultPosition, wxDefaultSize, (i == 0 ? wxRB_GROUP : 0L));
		Connect(rbId, wxEVT_RADIOBUTTON, wxCommandEventHandler(CKernelTableDlg::OnSymmetryChange), NULL, this);
	}

	const wxWindowID normalizeId = WinIDProvider::getProvider().getId();
	normalizeButton = new wxButton(this, normalizeId, "Normalize");
	Connect(normalizeId, wxEVT_BUTTON, wxCommandEventHandler(CKernelTableDlg::OnNormalize), NULL, this);

	normalizationValue = new wxTextCtrl(this, wxID_ANY, "1.0");
	normalizationValue->SetSizeHints(wxSize(20, -1));

	const wxWindowID resetId = WinIDProvider::getProvider().getId();
	resetButton = new wxButton(this, resetId, "Reset");
	Connect(resetId, wxEVT_BUTTON, wxCommandEventHandler(CKernelTableDlg::OnReset), NULL, this);

	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(CKernelTableDlg::OnClose), NULL, this);
	Connect(wxID_ANY, wxEVT_CHAR_HOOK, wxKeyEventHandler(CKernelTableDlg::OnEscape), NULL, this);
}

ConvolutionKernel CKernelTableDlg::getKernel() const {
	ConvolutionKernel retval(kernelSide);
	float * ckData = retval.getDataPtr();
	for (int i = 0; i < kernelParams.size(); ++i) {
		ckData[i] = wxAtof(kernelParams[i]->GetValue());
	}
	return retval;
}

void CKernelTableDlg::setKernel(const ConvolutionKernel & kernel) {
	const int inKernelSide = kernel.getSide();
	if (inKernelSide != kernelSide) {
		setKernelSide(inKernelSide);
	}
	const int sqSide = inKernelSide * inKernelSide;
	const float * kernelData = kernel.getDataPtr();
	for (int i = 0; i < sqSide; ++i) {
		kernelParams[i]->ChangeValue(wxString() << kernelData[i]);
	}
}

void CKernelTableDlg::setKernelSide(int s) {
	if (s > 0 && (s != kernelSide || 0 == kernelSide) && (s & 1) != 0) {
		// clear previous param controls
		for (auto rit = kernelParams.rbegin(); rit != kernelParams.rend(); ++rit) {
			(*rit)->Disconnect(wxEVT_TEXT_ENTER, wxCommandEventHandler(CKernelPanel::OnKernelChange), NULL, this);
			(*rit)->Destroy();
		}
		kernelParams.clear();
		// clear previous sizers in the hierarchy
		wxSizer * prevSizer = GetSizer();
		if (prevSizer) {
			wxSizerItemList szList = prevSizer->GetChildren();
			for (int i = 0; i < szList.size(); ++i) {
				prevSizer->Remove(szList[i]->GetSizer());
			}
		}

		kernelSide = s;
		wxBoxSizer * dlgSizer = new wxBoxSizer(wxVERTICAL);
		wxBoxSizer * topSizer = new wxBoxSizer(wxHORIZONTAL);
		topSizer->Add(sideButton, 0, wxALL, 5);
		topSizer->Add(sideCtrl, 1, wxEXPAND | wxALL, 5);
		dlgSizer->Add(topSizer, 0, wxEXPAND | wxALL);
		wxBoxSizer * symmetrySizer = new wxBoxSizer(wxHORIZONTAL);
		for (int i = 0; i < ST_COUNT; ++i) {
			symmetrySizer->Add(symmetryRb[i], 1, wxEXPAND | wxALL, 5);
		}
		symmetrySizer->Add(sumText, 1, wxEXPAND | wxALL, 5);
		dlgSizer->Add(symmetrySizer, 0, wxEXPAND | wxALL);
		wxBoxSizer * normSizer = new wxBoxSizer(wxHORIZONTAL);
		normSizer->Add(normalizeButton, 0, wxALL, 5);
		normSizer->Add(normalizationValue, 1, wxEXPAND | wxALL, 5);
		normSizer->Add(resetButton, 0, wxALL, 5);
		dlgSizer->Add(normSizer, 0, wxEXPAND | wxSHRINK | wxALL, 5);

		wxGridSizer * gridSizer = new wxGridSizer(kernelSide, kernelSide, 2, 2);
		const int sideSqr = kernelSide * kernelSide;
		// allocate ids
		kernelParamsId = WinIDProvider::getProvider().getId(sideSqr);
		for (int i = 0; i < sideSqr; ++i) {
			const wxWindowID paramId = kernelParamsId + i;
			wxTextCtrl * kernelPar = new wxTextCtrl(this, paramId, wxString() << 0.0, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
			kernelPar->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(CKernelPanel::OnKernelChange), NULL, paramPanel);
			Connect(paramId, wxEVT_TEXT, wxCommandEventHandler(CKernelTableDlg::OnParamChange), NULL, this);
			kernelParams.push_back(kernelPar);
			gridSizer->Add(kernelPar, 1, wxEXPAND | wxSHRINK | wxALL, 2);
		}
		updateSymmetry();
		dlgSizer->Add(gridSizer, 1, wxEXPAND | wxSHRINK | wxALL, 2);
		SetSizerAndFit(dlgSizer);
	}
	Layout();
	SendSizeEvent();
}

void CKernelTableDlg::OnKernelSide(wxCommandEvent & evt) {
	const int kSide = wxAtoi(sideCtrl->GetValue());
	setKernelSide(kSide);
}

void CKernelTableDlg::OnShow(wxShowEvent & evt) {
	setKernelSide(wxAtoi(sideCtrl->GetValue()));
}

void CKernelTableDlg::OnClose(wxCloseEvent & evt) {
	wxCommandEvent dummy;
	paramPanel->OnHideButton(dummy);
}

void CKernelTableDlg::OnEscape(wxKeyEvent & evt) {
	if (evt.GetKeyCode() == WXK_ESCAPE) {
		wxCommandEvent dummy;
		paramPanel->OnHideButton(dummy);
	} else if (evt.GetKeyCode() == WXK_F5) {
		wxCommandEvent dummy;
		paramPanel->OnKernelChange(dummy);
	} else {
		evt.Skip();
	}
}

void CKernelTableDlg::OnParamChange(wxCommandEvent & evt) {
	const int id = evt.GetId();
	if (id >= kernelParamsId && id < kernelParamsId + kernelSide * kernelSide) {
		const int index = id - kernelParamsId;
		if (currentSymmetry == ST_CENTRAL) {
			updateCentral(index);
		} else if (currentSymmetry == ST_RADIAL) {
			updateRadial(index);
		}
		recalculateSum();
	}
}

void CKernelTableDlg::OnSymmetryChange(wxCommandEvent & evt) {
	const wxWindowID evtId = evt.GetId();
	if (evtId >= symmetryRbId && evtId < symmetryRbId + ST_COUNT) {
		currentSymmetry = static_cast<SymmetryType>(evtId - symmetryRbId);
		updateSymmetry();
	}
}

void CKernelTableDlg::OnNormalize(wxCommandEvent& evt) {
	ConvolutionKernel current = getKernel();
	const float targetValue = wxAtof(normalizationValue->GetValue());
	current.normalize(targetValue);
	setKernel(current);
	recalculateSum();
}

void CKernelTableDlg::OnReset(wxCommandEvent & evt) {
	const int squareSide = kernelSide * kernelSide;
	for (int i = 0; i < squareSide; ++i) {
		kernelParams[i]->ChangeValue(wxString() << 0.0);
	}
}

void CKernelTableDlg::updateCentral(int index) {
	wxPoint op = indexToPoint(index);
	// the update is not necessary for the cental point
	if (op.x != 0 || op.y != 0) {
		const wxString value = kernelParams[index]->GetValue();
		for (int i = 0; i < 3; ++i) {
			// rotate the point by pi/2
			op = wxPoint(-op.y, op.x);
			const int ridx = pointToIndex(op);
			kernelParams[ridx]->ChangeValue(value);
		}
	}
}

void CKernelTableDlg::updateRadial(int index) {
	const wxPoint op = indexToPoint(index);
	// the update is not necessary for the central point
	if (op.x != 0 || op.y != 0) {
		// update the other central points too
		updateCentral(index);
		// now recalculate the whole bottom right part along with the central symmetry
		const int c = kernelSide / 2;
		for (int y = 1; y <= c; ++y) {
			for (int x = 1; x <= c; ++x) {
				const int tidx = pointToIndex(wxPoint(x, y));
				const Vector2 pv(x, y);
				const float len = pv.length();
				const int flen = static_cast<int>(floorf(len));
				const int clen = static_cast<int>(ceilf(len));
				float value = 0.0f;
				if (flen > c) {
					value = 0.0f;
				} else if (clen > c) {
					const int sidx = pointToIndex(wxPoint(0, flen));
					value = wxAtof(kernelParams[sidx]->GetValue()) * (1.0f - (len - flen));
				} else {
					const int fidx = pointToIndex(wxPoint(0, flen));
					const float fvalue = wxAtof(kernelParams[fidx]->GetValue());
					const int cidx = pointToIndex(wxPoint(0, clen));
					const float cvalue = wxAtof(kernelParams[cidx]->GetValue());
					const float t = (len - flen);
					value = cvalue * t + fvalue * (1.0f - t);
				}
				kernelParams[tidx]->ChangeValue(wxString() << value);
				updateCentral(tidx);
			}
		}
	}
}

void CKernelTableDlg::updateSymmetry() {
	if (ST_NO_SYMMETRY == currentSymmetry) {
		for (auto it = kernelParams.begin(); it != kernelParams.end(); ++it) {
			(*it)->Enable();
		}
	} else if (ST_CENTRAL == currentSymmetry) {
		const int sideSqr = kernelSide * kernelSide;
		for (int i = 0; i < sideSqr; ++i) {
			const wxPoint p = indexToPoint(i);
			const bool enable = (p == wxPoint(0, 0) || (p.x >= 0 && p.y < 0)); // don't forget the coord system is negative
			kernelParams[i]->Enable(enable);
		}
	} else if (ST_RADIAL == currentSymmetry) {
		const int sideSqr = kernelSide * kernelSide;
		for (int i = 0; i < sideSqr; ++i) {
			const wxPoint p = indexToPoint(i);
			const bool enable = (p.x == 0 && p.y <= 0);
			kernelParams[i]->Enable(enable);
		}
	}
}

wxPoint CKernelTableDlg::indexToPoint(int index) const {
	const int c = kernelSide / 2;
	return wxPoint((index % kernelSide) - c, (index / kernelSide) - c);
}

int CKernelTableDlg::pointToIndex(const wxPoint & p) const {
	const int c = kernelSide / 2;
	return (p.y + c) * kernelSide + p.x + c;
}

void CKernelTableDlg::recalculateSum() {
	float sum = 0.0f;
	for (auto it = kernelParams.begin(); it != kernelParams.end(); ++it) {
		sum += wxAtof((*it)->GetValue());
	}
	sumText->SetLabel(wxString() << sum);
}

// Curve Kernel dialog

const wxColour CurveCanvas::curveColours[CC_COUNT] = {
	wxColour(0x302d2d), // CC_BACKGROUND
	wxColour(0xc48f40), // CC_AXES
	wxColour(0xa4c64d), // CC_CURVE
	wxColour(0xc261ba), // CC_SAMPLES
	wxColour(0x3239dd), // CC_POLY
	wxColour(0x859dd6), // CC_POLY_HOVER
	wxColour(0xa3d7b8), // CC_POLY_DRAG
};
const int CurveCanvas::pointRadius = 5;

CurveCanvas::CurveCanvas(CKernelCurveDlg * _parent, int _numSamples)
	: wxPanel(_parent)
	, parent(_parent)
	, panelSize(0, 0)
	, axes(10, 0)
	, numSamples(_numSamples)
	, mouseOverCanvas(false)
	, mouseDrag(false)
	, hoveredPoint(-1)
	, mousePos(0, 0)
{
	SetDoubleBuffered(true);
	SetMinClientSize(wxSize(256, 256));

	// connect paint events
	Connect(wxEVT_PAINT, wxPaintEventHandler(CurveCanvas::OnPaint), NULL, this);
	Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(CurveCanvas::OnEraseBkg), NULL, this);

	// connect the mouse events
	Connect(wxEVT_ENTER_WINDOW, wxMouseEventHandler(CurveCanvas::OnMouseEvent), NULL, this);
	Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(CurveCanvas::OnMouseEvent), NULL, this);
	Connect(wxEVT_MOTION, wxMouseEventHandler(CurveCanvas::OnMouseEvent), NULL, this);
	Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CurveCanvas::OnMouseEvent), NULL, this);
	Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CurveCanvas::OnMouseEvent), NULL, this);

	// connect the siziing event
	Connect(wxEVT_SIZE, wxSizeEventHandler(CurveCanvas::OnSizeEvent), NULL, this);

	setNumSamples(numSamples);
	Update();
}

void CurveCanvas::setNumSamples(int n) {
	numSamples = n;
	std::vector<Vector2> newSamples;
	newSamples.resize(n);
	const int commonSize = std::min(n, static_cast<int>(samples.size()));
	for (int i = 0; i < commonSize; ++i) {
		// retain the y coordinate and change with the new x
		newSamples[i].x = float(i);
		newSamples[i].y = samples[i].y;
	}
	// now add new points if not full
	for (int i = commonSize; i < numSamples; ++i) {
		newSamples[i].x = float(i);
		newSamples[i].y = 0.0f;
	}
	samples = newSamples;
	// update the canvas samples
	updateCanvasSamples();
	Refresh(false);
}

void CurveCanvas::OnMouseEvent(wxMouseEvent & evt) {
	const wxEventType evType = evt.GetEventType();
	if (evType == wxEVT_MOTION && mouseOverCanvas) {
		const wxPoint currentMousePos(evt.GetX(), evt.GetY());
		const wxPoint mouseDelta = currentMousePos - mousePos;
		mousePos = currentMousePos;
		if (-1 != hoveredPoint) {
			if (mouseDrag) {
				// directly update the point position
				updateCanvasSample(hoveredPoint, SS_DRAG, mouseDelta);
			} else {
				// check if the point is still under the mouse
				if (!canvasSamples[hoveredPoint].bbox.Contains(mousePos)) {
					updateCanvasSample(hoveredPoint, SS_NORMAL, wxPoint(0, 0));
					hoveredPoint = -1;
				}
			}
		} else {
			// check if the current position of the mouse intersect with any sample
			// note: this may be improved with some Quad tree, but I'm not for performance ATM
			const int sampleCount = canvasSamples.size();
			for (int i = 0; i < sampleCount; ++i) {
				if (canvasSamples[i].bbox.Contains(mousePos)) {
					hoveredPoint = i;
					updateCanvasSample(i, SS_HOVER, wxPoint(0, 0));
					break;
				}
			}
		}
	} else if (evType == wxEVT_ENTER_WINDOW) {
		mouseOverCanvas = true;
	} else if (evType == wxEVT_LEAVE_WINDOW) {
		mouseOverCanvas = false;
		mouseDrag = false;
		if (-1 != hoveredPoint) {
			updateCanvasSample(hoveredPoint, SS_NORMAL, wxPoint(0, 0));
			hoveredPoint = -1;
		}
	} else if (evType == wxEVT_LEFT_DOWN) {
		const wxPoint currentMousePos(evt.GetX(), evt.GetY());
		const wxPoint mouseDelta = currentMousePos - mousePos;
		mousePos = currentMousePos;
		if (-1 != hoveredPoint) {
			updateCanvasSample(hoveredPoint, SS_DRAG, wxPoint(0, 0));
		}
		mouseDrag = true;
	} else if (evType == wxEVT_LEFT_UP) {
		const wxPoint currentMousePos(evt.GetX(), evt.GetY());
		const wxPoint mouseDelta = currentMousePos - mousePos;
		mousePos = currentMousePos;
		if (-1 != hoveredPoint) {
			// fist update the drag
			updateCanvasSample(hoveredPoint, SS_DRAG, mouseDelta);
			// update the cananonic samples
			updateSamples(hoveredPoint);
			// then check if the mouse point is still over the sample
			if (canvasSamples[hoveredPoint].bbox.Contains(mousePos)) {
				updateCanvasSample(hoveredPoint, SS_HOVER, wxPoint(0, 0));
			} else {
				updateCanvasSample(hoveredPoint, SS_NORMAL, wxPoint(0, 0));
				hoveredPoint = -1;
			}
		}
		mouseDrag = false;
	}
}

void CurveCanvas::updateCanvasSample(int nSample, unsigned state, const wxPoint& dpos) {
	DiscreteSample& csi = canvasSamples[nSample];
	bool refresh = false;
	if (SS_DRAG == state) {
		// update only the y coordinate for now
		const int clampedPos = clamp(csi.pos.y + dpos.y, pointRadius, panelSize.GetHeight() - pointRadius);
		refresh = csi.pos.y != clampedPos;
		csi.bbox.Offset(0, clampedPos - csi.pos.y);
		csi.pos.y = clampedPos;
	}
	refresh = refresh || csi.state != state;
	csi.state = state;
	if (refresh) {
		Refresh(false);
	}
}

void CurveCanvas::OnPaint(wxPaintEvent & evt) {
	wxBufferedPaintDC dc(this);
	panelSize = GetSize();
	const int panelWidth = panelSize.GetWidth();
	const int panelHeight = panelSize.GetHeight();
	// draw the background
	dc.SetBrush(wxBrush(curveColours[CC_BACKGROUND]));
	dc.SetPen(wxPen(curveColours[CC_BACKGROUND]));
	dc.DrawRectangle(wxPoint(0, 0), panelSize);
	// now draw the axes
	dc.SetPen(wxPen(curveColours[CC_AXES]));
	// draw only the y axis, x will be sampled always
	dc.DrawLine(wxPoint(0, axes.y), wxPoint(panelWidth, axes.y));
	// now draw the actual samples
	drawSamples(dc);
}

void CurveCanvas::drawSamples(wxBufferedPaintDC & pdc) {
	const int sampleCount = canvasSamples.size();
	if (sampleCount > 0) {
		const int panelHeight = panelSize.GetHeight();
		// now draw all the sampled parts
		pdc.SetPen(wxPen(curveColours[CC_SAMPLES]));
		for (int i = 0; i < sampleCount; ++i) {
			const int x = canvasSamples[i].pos.x;
			pdc.DrawLine(wxPoint(x, 0), wxPoint(x, panelHeight));
		}
		// first draw the curve so the point will overlap it later
		pdc.SetPen(wxPen(curveColours[CC_CURVE]));
		wxPoint prev = canvasSamples[0].pos;
		for (int i = 1; i < sampleCount; ++i) {
			const wxPoint current = canvasSamples[i].pos;
			pdc.DrawLine(prev, current);
			prev = current;
		}

		const wxPen polyPen[SS_COUNT] = {
			wxPen(curveColours[CC_POLY], 2),
			wxPen(curveColours[CC_POLY_HOVER], 2),
			wxPen(curveColours[CC_POLY_DRAG], 2),
		};
		unsigned lastPenUsed = canvasSamples[0].state;
		pdc.SetPen(polyPen[lastPenUsed]);
		for (int i = 0; i < sampleCount; ++i) {
			const DiscreteSample& csi = canvasSamples[i];
			if (csi.state != lastPenUsed) {
				lastPenUsed = csi.state;
				pdc.SetPen(polyPen[lastPenUsed]);
			}
			pdc.DrawCircle(csi.pos, pointRadius);
		}
	}
}

void CurveCanvas::OnEraseBkg(wxEraseEvent & evt) {
	// nop - just override event
}

void CurveCanvas::OnSizeEvent(wxSizeEvent & evt) {
	panelSize = GetSize();
	axes.y = panelSize.GetHeight() / 2;
	updateCanvasSamples();
	Refresh(false);
	evt.Skip();
}

void CurveCanvas::updateSamples(int sampleN) {
	const Vector2& updatedPos = canvasToReal(canvasSamples[sampleN].pos);
	samples[sampleN].y = updatedPos.y;
}

void CurveCanvas::updateCanvasSamples() {
	const int sampleCount = samples.size();
	if (sampleCount != canvasSamples.size()) {
		canvasSamples.resize(sampleCount);
	}
	const wxPoint bboxOffset = wxPoint(pointRadius, pointRadius);
	for (int i = 0; i < sampleCount; ++i) {
		DiscreteSample& ds = canvasSamples[i];
		ds.pos = realToCanvas(samples[i]);
		ds.bbox.SetTopLeft(ds.pos - bboxOffset);
		ds.bbox.SetBottomRight(ds.pos + bboxOffset);
		ds.state = 0;
	}
}

Vector2 CurveCanvas::canvasToReal(const wxPoint & p) const {
	const int panelWidth = panelSize.GetWidth();
	const int panelHalfHeight = panelSize.GetHeight() / 2;
	const float sampleSize = (numSamples > 1 ? (panelWidth - axes.x * 2) / float(numSamples - 1) : 1.0f);
	const float x = static_cast<float>((p.x - axes.x) / sampleSize);
	const float y = -(static_cast<float>(p.y) / panelHalfHeight - 1.0f);
	return Vector2(x, y);
}

wxPoint CurveCanvas::realToCanvas(const Vector2 & p) const {
	const int panelWidth = panelSize.GetWidth();
	const int panelHalfHeight = panelSize.GetHeight() / 2;
	const float sampleSize = (numSamples > 1 ? (panelWidth - axes.x * 2) / float(numSamples - 1) : 1.0f);
	const int x = static_cast<int>(p.x * sampleSize + axes.x);
	const int y = static_cast<int>((-p.y + 1.0f) * panelHalfHeight);
	return wxPoint(x, y);
}

const int CKernelCurveDlg::minSamples = 2;
const int CKernelCurveDlg::maxSamples = 64;

CKernelCurveDlg::CKernelCurveDlg(CKernelPanel * parent, const wxString & title, int side)
	: CKernelDlg(parent, title, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, paramPanel(parent)
	, sliderCtrl(nullptr)
	, currentSamples(clamp(side / 2 + 1, minSamples, maxSamples))
	, canvas(new CurveCanvas(this, currentSamples))
	, currentSide(side)
{
	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(CKernelCurveDlg::OnClose), NULL, this);
	Connect(wxID_ANY, wxEVT_CHAR_HOOK, wxKeyEventHandler(CKernelCurveDlg::OnEscape), NULL, this);

	wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * topSizer = new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Samples")), 0, wxALL | wxCENTER, 5);
	const wxWindowID sliderId = WinIDProvider::getProvider().getId();
	sliderCtrl = new wxSlider(
		this,
		sliderId,
		currentSamples,
		minSamples,
		maxSamples,
		wxDefaultPosition,
		wxDefaultSize,
		wxSL_HORIZONTAL | wxSL_MIN_MAX_LABELS | wxSL_AUTOTICKS);
	Connect(sliderId, wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler(CKernelCurveDlg::OnSliderChange), NULL, this);
	topSizer->Add(sliderCtrl, 1, wxEXPAND | wxALL, 5);

	mainSizer->Add(topSizer, 0, wxEXPAND | wxALL);
	mainSizer->Add(canvas, 1, wxEXPAND | wxALL, 5);

	SetInitialSize(wxSize(400, 400));

	mainSizer->FitInside(this);
	SetSizerAndFit(mainSizer);
}

ConvolutionKernel CKernelCurveDlg::getKernel() const {
	return ConvolutionKernel();
}

void CKernelCurveDlg::OnShow(wxShowEvent & evt) {
	Layout();
	SendSizeEvent();
}

void CKernelCurveDlg::OnClose(wxCloseEvent & evt) {
	wxCommandEvent dummy;
	paramPanel->OnHideButton(dummy);
}

void CKernelCurveDlg::OnEscape(wxKeyEvent & evt) {
	if (evt.GetKeyCode() == WXK_ESCAPE) {
		wxCommandEvent dummy;
		paramPanel->OnHideButton(dummy);
	} else if (evt.GetKeyCode() == WXK_F5) {
		wxCommandEvent dummy;
		paramPanel->OnKernelChange(dummy);
	} else {
		evt.Skip();
	}
}

void CKernelCurveDlg::OnSliderChange(wxScrollEvent & evt) {
	const int sliderValue = sliderCtrl->GetValue();
	canvas->setNumSamples(sliderValue);
}

// CKernelPanel

const wxString CKernelPanel::dialogTitles[CKernelPanel::DT_COUNT] = {
	wxT("Curve"),
	wxT("Table"),
};

CKernelPanel::CKernelPanel(ParamPanel * _parent, wxWindowID id, const wxString & label, const wxString& defSide)
	: wxPanel(_parent, id)
	, paramPanel(_parent)
	, kernelDialogs{nullptr}
	, kernelDlgTypeId(0)
	, kernelDlgRb{nullptr}
	, currentDlg(nullptr)
	, mainSizer(nullptr)
	, showButton(nullptr)
	, hideButton(nullptr)
{
	// allocate the kernel dialogs
	kernelDialogs[DT_CURVE] = new CKernelCurveDlg(this, label, wxAtoi(defSide));
	kernelDialogs[DT_TABLE] = new CKernelTableDlg(this, label, wxAtoi(defSide));
	currentDlg = kernelDialogs[0];

	mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(new wxStaticText(this, wxID_ANY, label), 0, wxEXPAND | wxALL, 5);

	kernelDlgTypeId = WinIDProvider::getProvider().getId(DT_COUNT);
	for (int i = 0; i < DT_COUNT; ++i) {
		const wxWindowID typeId = kernelDlgTypeId + i;
		kernelDlgRb[i] = new wxRadioButton(this, typeId, dialogTitles[i], wxDefaultPosition, wxDefaultSize, (i == 0 ? wxRB_GROUP : 0L));
		Connect(typeId, wxEVT_RADIOBUTTON, wxCommandEventHandler(CKernelPanel::OnKernelDlgType), NULL, this);
		mainSizer->Add(kernelDlgRb[i], 1, wxEXPAND | wxALL, 5);
	}

	showButton = new wxButton(this, wxID_ANY, wxT("Show"));
	showButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CKernelPanel::OnShowButton), NULL, this);
	mainSizer->Add(showButton, 0, wxEXPAND | wxALL, ModePanel::panelBorder);

	hideButton = new wxButton(this, wxID_ANY, wxT("Hide"));
	hideButton->Hide();
	hideButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CKernelPanel::OnHideButton), NULL, this);
	mainSizer->Add(hideButton, 0, wxEXPAND | wxALL, ModePanel::panelBorder);
	mainSizer->AddStretchSpacer(1);

	SetSizerAndFit(mainSizer);
}

ConvolutionKernel CKernelPanel::GetValue() const {
	return currentDlg->getKernel();
}

void CKernelPanel::OnKernelDlgType(wxCommandEvent & evt) {
	const wxWindowID evtId = evt.GetId();
	if (evtId >= kernelDlgTypeId && evtId < kernelDlgTypeId + DT_COUNT) {
		const bool shown = currentDlg->IsShown();
		if (shown) {
			currentDlg->Hide();
		}
		currentDlg = kernelDialogs[evtId - kernelDlgTypeId];
		if (shown) {
			currentDlg->Show();
		}
	}
}

void CKernelPanel::OnShowButton(wxCommandEvent & evt) {
	currentDlg->Show();
	hideButton->Show();
	showButton->Hide();
	mainSizer->Layout();
	Fit();
}

void CKernelPanel::OnHideButton(wxCommandEvent & evt) {
	currentDlg->Hide();
	hideButton->Hide();
	showButton->Show();
	mainSizer->Layout();
	Fit();
}

void CKernelPanel::OnKernelChange(wxCommandEvent & evt) {
	wxCommandEvent changeEvt(wxEVT_NULL, this->GetId());
	wxPostEvent(this->GetParent(), changeEvt);
}

void ParamPanel::createTextCtrl(const int id, const ParamDescriptor& pd) {
	wxBoxSizer * sizer = getModuleSizer(pd.module);
	wxBoxSizer * vert = new wxBoxSizer(wxVERTICAL);
	vert->Add(new wxStaticText(this, wxID_ANY, pd.name), 1, wxEXPAND | wxCENTER);
	sizer->Add(vert, 0, wxEXPAND | wxLEFT | wxRIGHT, ModePanel::panelBorder);
	wxTextCtrl * ctrl = new wxTextCtrl(this, id, wxT("0"), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	ctrl->SetValue(pd.defaultValue);
	sizer->Add(ctrl, 0, wxEXPAND | wxLEFT | wxRIGHT, ModePanel::panelBorder);
	textCtrlMap[id] = ctrl;
	// also connect the event
	Connect(id, wxEVT_TEXT_ENTER, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

void ParamPanel::createCheckBox(const int id, const ParamDescriptor& pd) {
	wxBoxSizer * sizer = getModuleSizer(pd.module);
	wxCheckBox * cb = new wxCheckBox(this, id, pd.name);
	cb->SetValue(pd.defaultValue != "false" && pd.defaultValue != "0");
	sizer->Add(cb, 1, wxEXPAND);
	checkBoxMap[id] = cb;
	// also connect the event
	Connect(id, wxEVT_CHECKBOX, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

void ParamPanel::createCKernel(const int id, const ParamDescriptor & pd) {
	wxBoxSizer * sizer = getModuleSizer(pd.module);
	CKernelPanel * ckp = new CKernelPanel(this, id, pd.name, pd.defaultValue);
	sizer->Add(ckp, 1, wxEXPAND);
	kernelMap[id] = ckp;
	Connect(id, wxEVT_NULL, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

void ParamPanel::createChoice(const int id, const ParamDescriptor & pd) {
	wxBoxSizer * sizer = getModuleSizer(pd.module);
	wxBoxSizer * labelSizer = new wxBoxSizer(wxVERTICAL);
	labelSizer->Add(new wxStaticText(this, wxID_ANY, pd.name), 1, wxEXPAND | wxCENTER);
	sizer->Add(labelSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, ModePanel::panelBorder);
	wxChoice * ch = new wxChoice(this, id);
	const std::vector<std::string> choices = splitString(pd.defaultValue.c_str(), ';');
	for (unsigned i = 0; i < choices.size(); ++i) {
		ch->Insert(wxString(choices[i]), i);
	}
	if (choices.size() > 0) {
		ch->SetSelection(0);
	}
	sizer->Add(ch, 0, wxEXPAND | wxLEFT | wxRIGHT, ModePanel::panelBorder);
	choiceMap[id] = ch;
	Connect(id, wxEVT_CHOICE, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

wxBoxSizer * ParamPanel::getModuleSizer(const ModuleBase * module) {
	auto moduleSizerIt = moduleSizers.find(module);
	wxBoxSizer * sizer = nullptr;
	if (moduleSizerIt == moduleSizers.end()) {
		sizer = new wxBoxSizer(wxHORIZONTAL);
		moduleSizers[module] = sizer;
		panelSizer->Add(sizer, 1, wxEXPAND);
	} else {
		sizer = moduleSizerIt->second;
	}
	return sizer;
}

ParamPanel::ParamPanel(ModePanel * parent)
	: wxPanel(parent)
	, modePanel(parent)
	, panelSizer(nullptr)
{
	panelSizer = new wxBoxSizer(wxVERTICAL);
	SetSizerAndFit(panelSizer);
}

void ParamPanel::addParam(const ParamDescriptor & pd) {
	const int id = static_cast<int>(WinIDProvider::getProvider().getId());
	paramMap[pd.name] = id;
	paramIdMap[id] = pd;
	switch (pd.type)
	{
	case(ParamDescriptor::ParamType::PT_BOOL) :
		createCheckBox(id, pd);
		break;
	case(ParamDescriptor::ParamType::PT_INT) :
	case(ParamDescriptor::ParamType::PT_FLOAT) :
	case(ParamDescriptor::ParamType::PT_STRING) :
		createTextCtrl(id, pd);
		break;
	case(ParamDescriptor::ParamType::PT_CKERNEL) :
		createCKernel(id, pd);
		break;
	case(ParamDescriptor::ParamType::PT_ENUM) :
		createChoice(id, pd);
		break;
	default:
		break;
	}
	SetSizerAndFit(panelSizer, false);
	SendSizeEvent();
	resizeParent();
}

bool ParamPanel::getStringParam(std::string & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = textCtrlMap.find(paramIt->second);
		if (ctrlIt != textCtrlMap.end()) {
			value = std::string(ctrlIt->second->GetValue());
			return true;
		}
	}
	return false;
}

bool ParamPanel::getIntParam(int & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = textCtrlMap.find(paramIt->second);
		if (ctrlIt != textCtrlMap.end()) {
			value = atoi(ctrlIt->second->GetValue().c_str());
			return true;
		}
	}
	return false;
}

bool ParamPanel::getFloatParam(float & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = textCtrlMap.find(paramIt->second);
		if (ctrlIt != textCtrlMap.end()) {
			value = atof(ctrlIt->second->GetValue().c_str());
			return true;
		}
	}
	return false;
}

bool ParamPanel::getBoolParam(bool & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = checkBoxMap.find(paramIt->second);
		if (ctrlIt != checkBoxMap.end()) {
			value = ctrlIt->second->GetValue();
			return true;
		}
	}
	return false;
}

bool ParamPanel::getCKernelParam(ConvolutionKernel & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = kernelMap.find(paramIt->second);
		if (ctrlIt != kernelMap.end()) {
			value = ctrlIt->second->GetValue();
			return true;
		}
	}
	return false;
}

bool ParamPanel::getEnumParam(unsigned & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = choiceMap.find(paramIt->second);
		if (ctrlIt != choiceMap.end()) {
			const int selection = ctrlIt->second->GetSelection();
			if (selection == wxNOT_FOUND) {
				return false;
			} else {
				value = static_cast<int>(selection);
				return true;
			}
		}
	}
	return false;
}

void ParamPanel::resizeParent() {
	SendSizeEventToParent();
	SendSizeEvent();
}

void ParamPanel::OnParamChange(wxCommandEvent & ev) {
	const int evtId = ev.GetId();
	auto pdIt = paramIdMap.find(evtId);
	if (pdIt != paramIdMap.end()) {
		ParamDescriptor& pd = pdIt->second;
		pd.module->update();
	}
}
