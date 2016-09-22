#include <wx/wx.h>

#include "wx_multi_module.h"

const wxColour MultiModuleCanvas::mmColors[MC_COUNT] = {
	wxColour(0x1e1e1e), // MC_BACKGROUND
	wxColour(0xa4c64d), // MC_NODE_TEXT
	wxColour(0x302d2d), // MC_NODE_BKG
	wxColour(0x373333), // MC_NODE_BORDER
	wxColour(0x0a0a0a), // MC_SELECTED_NODE_BKG
	wxColour(0x686868), // MC_SELECTED_NODE_BORDER
	wxColour(0xc48f40), // MC_CONNECTOR
};

MultiModuleCanvas::MultiModuleCanvas(wxWindow * _parent)
	: ScalablePanel(_parent, -6, 8)
	, defaultModuleSize(200.0f, 200.0f)
	, selectedModuleMapId(-1)
{
	SetDoubleBuffered(true);

	// connect paint events
	Connect(wxEVT_PAINT, wxPaintEventHandler(MultiModuleCanvas::OnPaint), NULL, this);
	Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(MultiModuleCanvas::OnEraseBackground), NULL, this);
}

void MultiModuleCanvas::OnPaint(wxPaintEvent & evt) {
	remapCanvas();
	wxBufferedPaintDC dc(this);
	static const wxBrush bkgBrush(mmColors[MC_BACKGROUND]);
	static const wxPen bkgPen(mmColors[MC_BACKGROUND]);
	// background
	dc.SetBrush(bkgBrush);
	dc.SetPen(bkgPen);
	dc.DrawRectangle(wxPoint(0, 0), panelSize);

	// draw all the nodes
	for (const auto& it : moduleMap) {
		// skip the currently selected module to draw it last (on top)
		if (it.first != selectedModuleMapId) {
			drawModuleNode(dc, it.second, it.first);
		}
	}
	// now draw the selected module on top of all
	if (selectedModuleMapId != -1) {
		drawModuleNode(dc, moduleMap[selectedModuleMapId], selectedModuleMapId);
	}
}

void MultiModuleCanvas::OnEraseBackground(wxEraseEvent & evt) {
	// disable erase - all will be handled in the paint evt
}

void MultiModuleCanvas::addModuleDescription(int id, const ModuleDescription & md) {
	// calculate the current center of screen position on the canvas
	const wxPoint screenCenter(panelSize.GetWidth() / 2, panelSize.GetHeight() / 2);
	const Vector2 canvasCenter = convertScreenToCanvas(screenCenter);
	const Vector2 modulePos = canvasCenter - static_cast<Vector2>(defaultModuleSize) / 2.0f;
	moduleMap[id] = ModuleGraphicNode(md, Rect(modulePos, defaultModuleSize));
	Refresh(false);
}

bool MultiModuleCanvas::onMouseMove() {
	bool panView = true;
	if (selectedModuleMapId != -1 && mouseDrag) {
		// calculate the difference
		const wxPoint mouseDiff = updatedMousePos - mousePos;
		const Vector2 canvasDiff = unscale(Convert::vector(mouseDiff));
		moduleMap[selectedModuleMapId].rect.translate(-canvasDiff);
		Refresh(false);
		panView = false;
	}
	return panView;
}

bool MultiModuleCanvas::onMouseLeftDown() {
	int intersectionMapId = -1;
	const Vector2 canvasMousePos(convertScreenToCanvas(mousePos));
	for (const auto& it : moduleMap) {
		if (it.second.rect.inside(canvasMousePos)) {
			intersectionMapId = it.first;
		}
	}
	const bool refreshCanvas = intersectionMapId != selectedModuleMapId;
	selectedModuleMapId = intersectionMapId;
	return refreshCanvas;
}

bool MultiModuleCanvas::onMouseLeftUp() {
	return true;
}

void MultiModuleCanvas::drawModuleNode(wxDC & dc, const ModuleGraphicNode & mgd, int mgdMapId) {
	if (!mgd.rect.intersects(view)) {
		// the module is not in the view - do not draw as all draw calls will go in vain
		return;
	}
	static const wxBrush nodeBkg(mmColors[MC_NODE_BKG]);
	static const wxPen nodeBorder(mmColors[MC_NODE_BORDER], 2);
	static const wxBrush selectedNodeBkg(mmColors[MC_SELECTED_NODE_BKG]);
	static const wxPen selectedNodeBorder(mmColors[MC_SELECTED_NODE_BORDER], 2);

	// first draw the rectangle
	const wxRect nodeRect(
		wxPoint(convertCanvasToScreen(mgd.rect.getTopLeft())),
		wxPoint(convertCanvasToScreen(mgd.rect.getBottomRight()))
	);
	dc.SetBrush(selectedModuleMapId == mgdMapId ? selectedNodeBkg : nodeBkg);
	dc.SetPen(selectedModuleMapId == mgdMapId ? selectedNodeBorder : nodeBorder);
	dc.DrawRectangle(nodeRect);

	// draw the node name
	dc.SetTextForeground(mmColors[MC_NODE_TEXT]);
	// TODO - some logic for the scaling
	const wxSize textExtent = dc.GetTextExtent(mgd.moduleDesc.fullName);
	const int textX = (nodeRect.GetWidth() - textExtent.GetWidth()) / 2;
	dc.DrawText(mgd.moduleDesc.fullName, nodeRect.x + textX, nodeRect.y + 10);
}
