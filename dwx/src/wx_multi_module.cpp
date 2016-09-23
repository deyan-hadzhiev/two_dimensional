#include <wx/wx.h>
#include <wx/panel.h>

#include "wx_modes.h"
#include "wx_multi_module.h"

const Size2d ModuleGraphicNode::nodeSize = Size2d(200.0f, 200.0f);
const float ModuleGraphicNode::connectorRadius = 10.0f;
const float ModuleGraphicNode::connectorOffset = 5.0f;

const wxColour MultiModuleCanvas::mmColors[MC_COUNT] = {
	wxColour(0x1e1e1e), // MC_BACKGROUND
	wxColour(0xa4c64d), // MC_NODE_TEXT
	wxColour(0x302d2d), // MC_NODE_BKG
	wxColour(0x373333), // MC_NODE_BORDER
	wxColour(0x0a0a0a), // MC_SELECTED_NODE_BKG
	wxColour(0x686868), // MC_SELECTED_NODE_BORDER
	wxColour(0x3239dd), // MC_CONNECTOR_EMPTY
	wxColour(0xa3d7b8), // MC_CONNECTOR_HOVER
	wxColour(0xc48f40), // MC_CONNECTOR
};

MultiModuleCanvas::MultiModuleCanvas(MultiModuleMode * _parent)
	: ScalablePanel(_parent, -4, 8)
	, parent(_parent)
	, connectorMapIdGen(0)
	, hoveredModuleMapId(InvalidModuleId)
	, hoveredConnectorType(HT_NONE)
	, hoveredModuleConnectorIdx(0)
	, selectedModuleMapId(InvalidModuleId)
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
	if (selectedModuleMapId != InvalidModuleId) {
		drawModuleNode(dc, moduleMap[selectedModuleMapId], selectedModuleMapId);
	}

	// now draw connectors
	for (const auto& it : connectorMap) {
		drawModuleConnector(dc, it.second);
	}
	// and draw the mouse connector
	if (mouseConnector.srcId != InvalidModuleId || mouseConnector.destId != InvalidModuleId) {
		drawModuleConnector(dc, mouseConnector);
	}
}

void MultiModuleCanvas::OnEraseBackground(wxEraseEvent & evt) {
	// disable erase - all will be handled in the paint evt
}

void MultiModuleCanvas::addModuleDescription(ModuleId id, const ModuleDescription & md) {
	// calculate the current center of screen position on the canvas
	const wxPoint screenCenter(panelSize.GetWidth() / 2, panelSize.GetHeight() / 2);
	const Vector2 canvasCenter = convertScreenToCanvas(screenCenter);
	const Vector2 modulePos = canvasCenter - static_cast<Vector2>(ModuleGraphicNode::nodeSize) / 2.0f;
	moduleMap[id] = ModuleGraphicNode(md, Rect(modulePos, ModuleGraphicNode::nodeSize));
	Refresh(eraseBkg);
}

void MultiModuleCanvas::destroyModuleNode(ModuleId id) {
	DASSERT(id >= 0);
	// first destroy all the connectors of the module
	ModuleGraphicNode& mgn = moduleMap[id];
	for (int i = 0; i < mgn.moduleDesc.inputs; ++i) {
		const MGNConnector& mgnc = mgn.inputs[i];
		if (mgnc.connectorId >= 0) {
			destroyConnector(mgnc.connectorId);
		}
	}
	for (int i = 0; i < mgn.moduleDesc.outputs; ++i) {
		const MGNConnector& mgnc = mgn.outputs[i];
		if (mgnc.connectorId >= 0) {
			destroyConnector(mgnc.connectorId);
		}
	}
	// then destroy the module itself from the moduleMap
	moduleMap.erase(id);
	// finally update selection and hovering
	if (hoveredModuleMapId == id) {
		hoveredModuleMapId = InvalidModuleId;
		hoveredConnectorType = HT_NONE;
		hoveredModuleConnectorIdx = 0;
	}
	if (selectedModuleMapId == id) {
		selectedModuleMapId = InvalidModuleId;
		parent->updateSelection(selectedModuleMapId);
	}
	// finally destroy the module form the mode panel
	parent->removeModule(id);
}

bool MultiModuleCanvas::onMouseMove() {
	bool panView = true;
	if (mouseLeftDrag) {
		// check if the mouse is dragging a connector
		if (mouseConnector.srcId != InvalidModuleId || mouseConnector.destId != InvalidModuleId) {
			reevaluateMouseHover();
			Refresh(eraseBkg);
			panView = false;
		} else if (selectedModuleMapId != InvalidModuleId) {
			// calculate the difference
			const wxPoint mouseDiff = mousePos - updatedMousePos;
			const Vector2 canvasDiff = unscale(Convert::vector(mouseDiff));
			ModuleGraphicNode& translatedNode = moduleMap[selectedModuleMapId];
			translatedNode.translate(canvasDiff);
			// now update all connector positions of this module
			for (int i = 0; i < translatedNode.moduleDesc.inputs; ++i) {
				MGNConnector& connector = translatedNode.inputs[i];
				if (connector.state == MGNConnector::MGNC_CONNECTED) {
					ModuleConnectorDesc& mcd = connectorMap[connector.connectorId];
					mcd.destPos = connector.r.getCenter();
				}
			}
			for (int i = 0; i < translatedNode.moduleDesc.outputs; ++i) {
				MGNConnector& connector = translatedNode.outputs[i];
				if (connector.state == MGNConnector::MGNC_CONNECTED) {
					ModuleConnectorDesc& mcd = connectorMap[connector.connectorId];
					mcd.srcPos = connector.r.getCenter();
				}
			}
			Refresh(eraseBkg);
			panView = false;
		}
	} else {
		if (reevaluateMouseHover()) {
			Refresh(eraseBkg);
		}
	}

	return panView;
}

bool MultiModuleCanvas::onMouseLeftDown() {
	ModuleId intersectionMapId = InvalidModuleId;
	const Vector2 canvasMousePos(convertScreenToCanvas(mousePos));
	// check if the hovered module is the one under the mouse
	if (hoveredModuleMapId != InvalidModuleId) {
		if (moduleMap[hoveredModuleMapId].rect.inside(canvasMousePos)) {
			intersectionMapId = hoveredModuleMapId;
		}
	}
	// if not, check all the modules still (redundant?)
	if (intersectionMapId == InvalidModuleId) {
		for (const auto& it : moduleMap) {
			if (it.second.rect.inside(canvasMousePos)) {
				intersectionMapId = it.first;
				break;
			}
		}
	}
	// now check if the mouse is over any of the connectors
	bool overConnector = false;
	if (intersectionMapId != InvalidModuleId) {
		ModuleGraphicNode& mgn = moduleMap[intersectionMapId];
		for (int i = 0; i < mgn.moduleDesc.inputs && !overConnector; ++i) {
			MGNConnector& mgnc = mgn.inputs[i];
			if (mgnc.r.inside(canvasMousePos)) {
				// check if it is connected
				if (mgnc.connectorId != -1) {
					// so we need to destroy the connection and preseve only the opposite connector state
					mouseConnector = destroyConnector(mgnc.connectorId);
					// this connector's id and state should be updated by the destroy function
					// so update only the mouseConnector
					mouseConnector.destId = InvalidModuleId;
					mouseConnector.inputIdx = 0;
					// but the other connector state would be EMTPY, while it should be connected
					MGNConnector& srcMgnc = moduleMap[mouseConnector.srcId].outputs[mouseConnector.outputIdx];
					srcMgnc.state = MGNConnector::MGNC_CONNECTED;
					srcMgnc.connectorId = -2;
				} else {
					// first update the mouse connector
					mouseConnector.srcId = InvalidModuleId;
					mouseConnector.destId = intersectionMapId;
					mouseConnector.destPos = mgnc.r.getCenter();
					mouseConnector.inputIdx = i;
					// update the state of the connector
					mgnc.state = MGNConnector::MGNC_CONNECTED;
					// hack the connection state change
					mgnc.connectorId = -2;
				}
				overConnector = true;
			}
		}
		for (int i = 0; i < mgn.moduleDesc.outputs && !overConnector; ++i) {
			MGNConnector& mgnc = mgn.outputs[i];
			if (mgnc.r.inside(canvasMousePos)) {
				if (mgnc.connectorId != -1) {
					// so we need to destroy the connection and preseve only the opposite connector state
					mouseConnector = destroyConnector(mgnc.connectorId);
					// this connector's id and state should be updated by the destroy function
					// so update only the mouseConnector
					mouseConnector.srcId = InvalidModuleId;
					mouseConnector.outputIdx = 0;
					// but the other connector state would be EMTPY, while it should be connected
					MGNConnector& destMgnc = moduleMap[mouseConnector.destId].inputs[mouseConnector.inputIdx];
					destMgnc.state = MGNConnector::MGNC_CONNECTED;
					destMgnc.connectorId = -2;
				} else {
					mouseConnector.srcId = intersectionMapId;
					mouseConnector.srcPos = mgnc.r.getCenter();
					mouseConnector.outputIdx = i;
					mouseConnector.destId = InvalidModuleId;
					mgnc.state = MGNConnector::MGNC_CONNECTED;
					// hack the connection state change
					mgnc.connectorId = -2;
				}
				overConnector = true;
			}
		}
	}
	bool refreshCanvas = intersectionMapId != selectedModuleMapId;
	if (!overConnector) {
		selectedModuleMapId = intersectionMapId;
		parent->updateSelection(selectedModuleMapId);
	}
	return refreshCanvas || overConnector;
}

bool MultiModuleCanvas::onMouseLeftUp() {
	// if the mouse was dragging a connector reset it
	if (mouseConnector.srcId != InvalidModuleId || mouseConnector.destId != InvalidModuleId) {
		// now check if the mouse connector is over a connector or not
		if (hoveredModuleMapId != InvalidModuleId) {
			if (hoveredConnectorType == HT_NONE
				|| (mouseConnector.srcId == InvalidModuleId && hoveredConnectorType != HT_OUTPUT)
				|| (mouseConnector.destId == InvalidModuleId && hoveredConnectorType != HT_INPUT)
				) {
				disconnectMouseConnector();
			} else {
				if (mouseConnector.srcId == InvalidModuleId) {
					mouseConnector.srcId = hoveredModuleMapId;
					mouseConnector.outputIdx = hoveredModuleConnectorIdx;
					// the pos is a bit trickier
					mouseConnector.srcPos = moduleMap[hoveredModuleMapId].outputs[hoveredModuleConnectorIdx].r.getCenter();
				} else if (mouseConnector.destId == InvalidModuleId) {
					mouseConnector.destId = hoveredModuleMapId;
					mouseConnector.inputIdx = hoveredModuleConnectorIdx;
					// the pos is a bit trickier
					mouseConnector.destPos = moduleMap[hoveredModuleMapId].inputs[hoveredModuleConnectorIdx].r.getCenter();
				}
				// still check if the two ids don't match - this is plainly invalid
				if (mouseConnector.srcId == mouseConnector.destId) {
					disconnectMouseConnector();
				} else {
					// now create the connector (check validity?)
					createConnector(mouseConnector);
					// reset the mouse connector
					mouseConnector = ModuleConnectorDesc();
				}
			}
		} else {
			disconnectMouseConnector();
		}
	}
	return true;
}

bool MultiModuleCanvas::onMouseRightDown() {
	bool refresh = false;
	if (hoveredModuleMapId != InvalidModuleId) {
		// check if a connector is hovered over
		if (hoveredConnectorType != HT_NONE) {
			// check if the connector is connected
			const MGNConnector& mgnc = (hoveredConnectorType == HT_INPUT ?
				moduleMap[hoveredModuleMapId].inputs[hoveredModuleConnectorIdx] :
				moduleMap[hoveredModuleMapId].outputs[hoveredModuleConnectorIdx]
			);
			if (mgnc.connectorId != -1 && mgnc.connectorId != -2) {
				destroyConnector(mgnc.connectorId);
			} else {
				destroyModuleNode(hoveredModuleMapId);
			}
		} else {
			destroyModuleNode(hoveredModuleMapId);
		}
		refresh = true;
	} else if (selectedModuleMapId != InvalidModuleId) {
		selectedModuleMapId = InvalidModuleId;
		parent->updateSelection(selectedModuleMapId);
		refresh = true;
	}
	return refresh;
}

bool MultiModuleCanvas::reevaluateMouseHover() {
	const Vector2 mousePosCanvas(convertScreenToCanvas(mousePos));
	if (hoveredModuleMapId != InvalidModuleId) {
		ModuleGraphicNode& mgn = moduleMap[hoveredModuleMapId];
		// check if still hovered
		if (!mgn.rect.inside(mousePosCanvas)) {
			// not hovered anymore
			// reset the connector state
			for (int i = 0; i < mgn.moduleDesc.inputs; ++i) {
				mgn.inputs[i].state = (mgn.inputs[i].connectorId != -1 ? MGNConnector::MGNC_CONNECTED : MGNConnector::MGNC_EMPTY);
			}
			for (int i = 0; i < mgn.moduleDesc.outputs; ++i) {
				mgn.outputs[i].state = (mgn.outputs[i].connectorId != -1 ? MGNConnector::MGNC_CONNECTED : MGNConnector::MGNC_EMPTY);
			}
			// reset the hoveredID
			hoveredModuleMapId = InvalidModuleId;
			Refresh(eraseBkg);
		}
	} else {
		// check if the mouse is hovering over any node currently
		for (const auto& it : moduleMap) {
			if (it.second.rect.inside(mousePosCanvas)) {
				hoveredModuleMapId = it.first;
				break;
			}
		}
	}
	bool stateChange = false;
	if (hoveredModuleMapId != InvalidModuleId) {
		ModuleGraphicNode& mgn = moduleMap[hoveredModuleMapId];
		bool overConnector = false;
		// reevaluate the connectors
		for (int i = 0; i < mgn.moduleDesc.inputs; ++i) {
			MGNConnector& conn = mgn.inputs[i];
			const MGNConnector::State currState = conn.state;
			if (conn.r.inside(mousePosCanvas)) {
				conn.state = MGNConnector::MGNC_HOVER;
				hoveredConnectorType = HT_INPUT;
				hoveredModuleConnectorIdx = i;
				overConnector = true;
			} else {
				conn.state = (conn.connectorId != -1 ? MGNConnector::MGNC_CONNECTED : MGNConnector::MGNC_EMPTY);
			}
			stateChange |= (currState != conn.state);
		}
		for (int i = 0; i < mgn.moduleDesc.outputs; ++i) {
			MGNConnector& conn = mgn.outputs[i];
			const MGNConnector::State currState = conn.state;
			if (conn.r.inside(mousePosCanvas)) {
				conn.state = MGNConnector::MGNC_HOVER;
				hoveredConnectorType = HT_OUTPUT;
				hoveredModuleConnectorIdx = i;
				overConnector = true;
			} else {
				conn.state = (conn.connectorId != -1 ? MGNConnector::MGNC_CONNECTED : MGNConnector::MGNC_EMPTY);
			}
			stateChange |= (currState != conn.state);
		}
		if (!overConnector) {
			hoveredConnectorType = HT_NONE;
		}
	} else {
		hoveredConnectorType = HT_NONE;
	}
	return stateChange;
}

void MultiModuleCanvas::disconnectMouseConnector() {
	// disconnect the connector
	if (mouseConnector.srcId != InvalidModuleId) {
		MGNConnector& mgnc = moduleMap[mouseConnector.srcId].outputs[mouseConnector.outputIdx];
		mgnc.connectorId = -1;
		mgnc.state = MGNConnector::MGNC_EMPTY;
		mouseConnector.srcId = InvalidModuleId;
	}
	if (mouseConnector.destId != InvalidModuleId) {
		MGNConnector& mgnc = moduleMap[mouseConnector.destId].inputs[mouseConnector.inputIdx];
		mgnc.connectorId = -1;
		mgnc.state = MGNConnector::MGNC_EMPTY;
		mouseConnector.destId = InvalidModuleId;
	}
}

bool MultiModuleCanvas::createConnector(const ModuleConnectorDesc & mcd) {
	bool validConnection = true;
	// TODO : check if the connection is valid - no loops, etc
	if (validConnection) {
		connectorMap[connectorMapIdGen] = mcd;
		// now update the appropriate module map connectors
		MGNConnector& outputConnector = moduleMap[mcd.srcId].outputs[mcd.outputIdx];
		if (outputConnector.connectorId != -1 && outputConnector.connectorId != -2) {
			destroyConnector(outputConnector.connectorId);
		}
		outputConnector.state = MGNConnector::MGNC_CONNECTED;
		outputConnector.connectorId = connectorMapIdGen;

		MGNConnector& inputConnector = moduleMap[mcd.destId].inputs[mcd.inputIdx];
		if (inputConnector.connectorId != -1 && inputConnector.connectorId != -2) {
			destroyConnector(inputConnector.connectorId);
		}
		inputConnector.state = MGNConnector::MGNC_CONNECTED;
		inputConnector.connectorId = connectorMapIdGen;

		// increase the id generator
		connectorMapIdGen++;
	}
	return validConnection;
}

ModuleConnectorDesc MultiModuleCanvas::destroyConnector(int id) {
	DASSERT(id >= 0);
	ModuleConnectorDesc destroyed = connectorMap[id];
	connectorMap.erase(id);
	// now detach the connected modules
	MGNConnector& outputConnector = moduleMap[destroyed.srcId].outputs[destroyed.outputIdx];
	outputConnector.state = MGNConnector::MGNC_EMPTY;
	outputConnector.connectorId = -1;
	MGNConnector& inputConnector = moduleMap[destroyed.destId].inputs[destroyed.inputIdx];
	inputConnector.state = MGNConnector::MGNC_EMPTY;
	inputConnector.connectorId = -1;
	return destroyed;
}

void MultiModuleCanvas::drawModuleNode(wxDC & dc, const ModuleGraphicNode & mgd, ModuleId mgdMapId) {
	if (!mgd.rect.intersects(view)) {
		// the module is not in the view - do not draw as all draw calls will go in vain
		return;
	}
	static const wxBrush nodeBkg(mmColors[MC_NODE_BKG]);
	static const wxPen nodeBorder(mmColors[MC_NODE_BORDER], 2);
	static const wxBrush selectedNodeBkg(mmColors[MC_SELECTED_NODE_BKG]);
	static const wxPen selectedNodeBorder(mmColors[MC_SELECTED_NODE_BORDER], 2);
	static const wxBrush connectorBrush[MGNConnector::MGNC_COUNT] = {
		wxBrush(mmColors[MC_CONNECTOR_EMPTY]),
		wxBrush(mmColors[MC_CONNECTOR_HOVER]),
		wxBrush(mmColors[MC_CONNECTOR]),
	};

	// first draw the rectangle
	const wxRect nodeRect(
		wxPoint(convertCanvasToScreen(mgd.rect.getTopLeft())),
		wxPoint(convertCanvasToScreen(mgd.rect.getBottomRight()))
	);
	dc.SetBrush(selectedModuleMapId == mgdMapId ? selectedNodeBkg : nodeBkg);
	dc.SetPen(selectedModuleMapId == mgdMapId ? selectedNodeBorder : nodeBorder);
	dc.DrawRectangle(nodeRect);

	// now draw the connectors
	const wxCoord scaledRadius = roundf(scale(ModuleGraphicNode::connectorRadius));
	for (int i = 0; i < mgd.moduleDesc.inputs; ++i) {
		dc.SetBrush(connectorBrush[mgd.inputs[i].state]);
		const wxPoint connCenter = convertCanvasToScreen(mgd.inputs[i].r.getCenter());
		dc.DrawCircle(connCenter, scaledRadius);
	}
	for (int i = 0; i < mgd.moduleDesc.outputs; ++i) {
		dc.SetBrush(connectorBrush[mgd.outputs[i].state]);
		const wxPoint connCenter = convertCanvasToScreen(mgd.outputs[i].r.getCenter());
		dc.DrawCircle(connCenter, scaledRadius);
	}

	// draw the node name
	dc.SetTextForeground(mmColors[MC_NODE_TEXT]);
	// TODO - some logic for the scaling
	const wxSize textExtent = dc.GetTextExtent(mgd.moduleDesc.fullName);
	const int textX = (nodeRect.GetWidth() - textExtent.GetWidth()) / 2;
	dc.DrawText(mgd.moduleDesc.fullName, nodeRect.x + textX, nodeRect.y + 10);
}

void MultiModuleCanvas::drawModuleConnector(wxDC & dc, const ModuleConnectorDesc & mcd) {
	static const wxPen connectorPen(mmColors[MC_CONNECTOR], 2);
	bool drawConnector = false;
	// check if the connector should be drawn at all
	if (mcd.srcId != mcd.destId && (mcd.srcId == InvalidModuleId || mcd.destId == InvalidModuleId)) {
		// this has to be mouse dragging
		drawConnector = true;
	}
	if (!drawConnector) {
		if (mcd.srcId != InvalidModuleId && moduleMap[mcd.srcId].rect.intersects(view)) {
			drawConnector |= true;
		}
		if (mcd.destId != InvalidModuleId && moduleMap[mcd.destId].rect.intersects(view)) {
			drawConnector |= true;
		}
	}
	if (drawConnector) {
		const wxPoint srcPos = (mcd.srcId != InvalidModuleId ? convertCanvasToScreen(mcd.srcPos) : mousePos);
		const wxPoint destPos = (mcd.destId != InvalidModuleId ? convertCanvasToScreen(mcd.destPos) : mousePos);
		dc.SetPen(connectorPen);
		dc.DrawLine(srcPos, destPos);
	}
}
