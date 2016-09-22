#include <wx/wx.h>

#include "wx_scalable.h"
#include "util.h"

ScalablePanel::ScalablePanel(wxWindow * _parent, const int _minZoom, const int _maxZoom, const bool _eraseBkg)
	: wxPanel(_parent)
	, eraseBkg(_eraseBkg)
	, minZoom(_minZoom)
	, maxZoom(_maxZoom)
	, zoomLvl(0)
	, mouseLeftDrag(false)
	, mouseRightDrag(false)
	, mouseOverCanvas(false)
	, mousePos(0, 0)
	, view()
	, canvasState(0)
	, panelSize(0, 0)
{
	// connect mouse events
	Connect(wxEVT_ENTER_WINDOW, wxMouseEventHandler(ScalablePanel::OnMouseEvent), NULL, this);
	Connect(wxEVT_LEAVE_WINDOW, wxMouseEventHandler(ScalablePanel::OnMouseEvent), NULL, this);
	Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(ScalablePanel::OnMouseEvent), NULL, this);
	Connect(wxEVT_MOTION, wxMouseEventHandler(ScalablePanel::OnMouseEvent), NULL, this);
	Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(ScalablePanel::OnMouseEvent), NULL, this);
	Connect(wxEVT_LEFT_UP, wxMouseEventHandler(ScalablePanel::OnMouseEvent), NULL, this);
	Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(ScalablePanel::OnMouseEvent), NULL, this);
	Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(ScalablePanel::OnMouseEvent), NULL, this);
	// connect the sizing event
	Connect(wxEVT_SIZE, wxSizeEventHandler(ScalablePanel::OnSizeEvent), NULL, this);

	Update();
}

void ScalablePanel::OnMouseEvent(wxMouseEvent & evt) {
	const wxEventType evType = evt.GetEventType();
	if (evType == wxEVT_MOTION && mouseOverCanvas) {
		mousePos = wxPoint(evt.GetX(), evt.GetY());
		const bool panView = onMouseMove();
		if (panView && mouseLeftDrag && mousePos != updatedMousePos) {
			canvasState |= CS_DIRTY_POS;
			Refresh(eraseBkg);
		} else {
			updatedMousePos = mousePos;
		}
	} else if (evType == wxEVT_LEFT_DOWN) {
		mousePos = wxPoint(evt.GetX(), evt.GetY());
		SetFocus(); // necessary for Win 7
		mouseLeftDrag = true;
		const bool refresh = onMouseLeftDown();
		if (refresh) {
			Refresh(eraseBkg);
		}
		updatedMousePos = mousePos;
	} else if (evType == wxEVT_LEFT_UP) {
		const bool refresh = onMouseLeftUp();
		if (refresh) {
			Refresh(eraseBkg);
		}
		mouseLeftDrag = false;
	} else if (evType == wxEVT_RIGHT_DOWN) {
		mousePos = wxPoint(evt.GetX(), evt.GetY());
		SetFocus(); // necessary for Win 7
		mouseRightDrag = true;
		const bool refresh = onMouseRightDown();
		if (refresh) {
			Refresh(eraseBkg);
		}
		updatedMousePos = mousePos;
	} else if (evType == wxEVT_RIGHT_UP) {
		const bool refresh = onMouseRightUp();
		if (refresh) {
			Refresh(eraseBkg);
		}
		mouseRightDrag = false;
	} else if (evType == wxEVT_ENTER_WINDOW) {
		if (!mouseOverCanvas) {
			mouseOverCanvas = true;
			Refresh(eraseBkg);
		}
	} else if (evType == wxEVT_LEAVE_WINDOW) {
		if (mouseOverCanvas) {
			mouseOverCanvas = false;
			// if the mouse was dragging - call the mouseUp function
			if (mouseLeftDrag) {
				onMouseLeftUp();
			}
			mouseLeftDrag = false;
			if (mouseRightDrag) {
				onMouseRightUp();
			}
			mouseRightDrag = false;
			Refresh(eraseBkg);
		}
	} else if (evType == wxEVT_MOUSEWHEEL) {
		const int delta = evt.GetWheelDelta();
		const int rot = evt.GetWheelRotation();
		zoomLvlDelta = (rot > 0 ? delta / 120 : -delta / 120);
		const int newZoomLvl = clamp(zoomLvl + zoomLvlDelta, minZoom, maxZoom);
		if (newZoomLvl != zoomLvl) {
			canvasState |= CS_DIRTY_ZOOM;
			Refresh(eraseBkg);
		} else {
			zoomLvlDelta = 0;
		}
	}
}

void ScalablePanel::OnSizeEvent(wxSizeEvent & evt) {
	if (GetAutoLayout()) {
		Layout();
	}
	panelSize = GetSize();
	canvasState |= CS_DIRTY_SIZE;
	Refresh(eraseBkg);
	evt.Skip();
}

void ScalablePanel::recalcViewSize() {
	view.setSize(unscale(Convert::size(panelSize)));
}

void ScalablePanel::recalcViewPos(Vector2 p, const Rect& prevRect) {
	view.x = p.x - ((p.x - prevRect.x) * view.width)  / prevRect.width;
	view.y = p.y - ((p.y - prevRect.y) * view.height) / prevRect.height;
}

Vector2 ScalablePanel::convertScreenToCanvas(const wxPoint & in) const {
	const Vector2 unscaledPos = unscale(Convert::vector(in));
	const Vector2 canvasCoord = view.getTopLeft() + unscaledPos;
	return canvasCoord;
}

wxPoint ScalablePanel::convertCanvasToScreen(const Vector2 & in) const {
	const Vector2 scaledPos = scale(in - view.getTopLeft());
	return wxPoint(roundf(scaledPos.x), roundf(scaledPos.y));
}

void ScalablePanel::remapCanvas() {
	if (canvasState == CS_CLEAN)
		return;
	const Rect prevRect = view;
	const Vector2 canvasMousePos = convertScreenToCanvas(mousePos);
	if ((canvasState & CS_DIRTY_ZOOM) != 0 && zoomLvlDelta != 0) {
		zoomLvl += zoomLvlDelta;
		zoomLvlDelta = 0;
	}
	if ((canvasState & (CS_DIRTY_SIZE | CS_DIRTY_ZOOM)) != 0) {
		recalcViewSize();
	}
	if ((canvasState & CS_DIRTY_ZOOM) != 0) {
		recalcViewPos(canvasMousePos, prevRect);
	} else if ((canvasState & CS_DIRTY_SIZE) != 0) {
		const Vector2 halfDelta = Vector2(
			prevRect.width - view.width,
			prevRect.height - view.height
		) / 2.0f;
		view.setPosition(prevRect.getPosition() + halfDelta);
	} else if ((canvasState & CS_DIRTY_POS) != 0) {
		wxPoint screenDelta = mousePos - updatedMousePos;
		Vector2 canvasDelta = unscale(Convert::vector(screenDelta));
		view.setPosition(view.getPosition() - canvasDelta);
		updatedMousePos = mousePos;
	}
	canvasState = CS_CLEAN;
}
