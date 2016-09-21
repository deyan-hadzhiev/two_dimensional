#include <wx/wx.h>

#include "wx_multi_module.h"

MultiModuleCanvas::MultiModuleCanvas(wxWindow * _parent)
	: ScalablePanel(_parent, -8, 16)
{
	SetDoubleBuffered(true);

	// connect paint events
	Connect(wxEVT_PAINT, wxPaintEventHandler(MultiModuleCanvas::OnPaint), NULL, this);
	Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(MultiModuleCanvas::OnEraseBackground), NULL, this);
}

void MultiModuleCanvas::OnPaint(wxPaintEvent & evt) {
	remapCanvas();
	wxBufferedPaintDC dc(this);
	// background
	dc.SetBrush(wxBrush(*wxBLUE));
	dc.DrawRectangle(wxPoint(0, 0), panelSize);
	// rectangle
	dc.SetBrush(wxBrush(*wxGREEN));
	dc.SetPen(wxPen(*wxRED));
	const Rect drawnRect = Rect(0, 0, 50, 50);
	const wxPoint topLeft = convertCanvasToScreen(drawnRect.getTopLeft());
	const wxPoint bottomRight = convertCanvasToScreen(drawnRect.getBottomRight());
	const wxRect screenRect(topLeft, bottomRight);
	dc.DrawRectangle(screenRect);
	wxString viewPos = wxString::Format("%6.2f %6.2f %6.2f %6.2f", view.x, view.y, view.width, view.height);
	Vector2 mousePosCanvas = convertScreenToCanvas(mousePos);
	wxString mousePosStr = wxString::Format("s: %d %d v: %6.2f %6.2f", mousePos.x, mousePos.y, mousePosCanvas.x, mousePosCanvas.y);
	dc.DrawText(viewPos, wxPoint(panelSize.GetWidth() - 250, 20));
	dc.DrawText(mousePosStr, wxPoint(panelSize.GetWidth() - 250, 40));
}

void MultiModuleCanvas::OnEraseBackground(wxEraseEvent & evt) {
	// disable erase - all will be handled in the paint evt
}
