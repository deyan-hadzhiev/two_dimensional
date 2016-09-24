#ifndef __WX_MULTI_MODULE_H__
#define __WX_MULTI_MODULE_H__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dcbuffer.h>

#include "module_base.h"
#include "module_manager.h"
#include "module_dag.h"
#include "wx_scalable.h"
#include "wx_bitmap_canvas.h"

struct MGNConnector {
	enum State {
		MGNC_EMPTY = 0,
		MGNC_HOVER,
		MGNC_CONNECTED,
		MGNC_COUNT, // last
	} state;
	Rect r;
	EdgeId connectorId;

	MGNConnector()
		: state(MGNC_EMPTY)
		, r()
		, connectorId(InvalidEdgeId)
	{}
};

class ModuleGraphicNode {
public:
	ModuleDescription moduleDesc;
	Rect rect;
	Rect progRect; // the progress rect
	MGNConnector * inputs;
	MGNConnector * outputs;

	ModuleGraphicNode()
		: moduleDesc()
		, rect()
		, progRect()
		, inputs(nullptr)
		, outputs(nullptr)
	{}
	ModuleGraphicNode(const ModuleDescription& _moduleDesc, const Rect& _rect)
		: moduleDesc(_moduleDesc)
		, rect(_rect)
		, inputs(_moduleDesc.inputs > 0 ? new MGNConnector[_moduleDesc.inputs] : nullptr)
		, outputs(_moduleDesc.outputs > 0 ? new MGNConnector[_moduleDesc.outputs] : nullptr)
	{
		recalcConnectors();
		recalcProgress();
	}

	ModuleGraphicNode(const ModuleGraphicNode& copy)
		: moduleDesc(copy.moduleDesc)
		, rect(copy.rect)
		, progRect(copy.progRect)
		, inputs(copy.moduleDesc.inputs > 0 ? new MGNConnector[copy.moduleDesc.inputs] : nullptr)
		, outputs(copy.moduleDesc.outputs > 0 ? new MGNConnector[copy.moduleDesc.outputs] : nullptr)
	{
		for (int i = 0; i < copy.moduleDesc.inputs; ++i) {
			inputs[i] = copy.inputs[i];
		}
		for (int i = 0; i < copy.moduleDesc.outputs; ++i) {
			outputs[i] = copy.outputs[i];
		}
	}

	ModuleGraphicNode& operator=(const ModuleGraphicNode& assign) {
		if (this != &assign) {
			moduleDesc = assign.moduleDesc;
			rect = assign.rect;
			progRect = assign.progRect;
			delete[] inputs;
			inputs = (moduleDesc.inputs > 0 ? new MGNConnector[moduleDesc.inputs] : nullptr);
			for (int i = 0; i < moduleDesc.inputs; ++i) {
				inputs[i] = assign.inputs[i];
			}
			delete[] outputs;
			outputs = (moduleDesc.outputs > 0 ? new MGNConnector[moduleDesc.outputs] : nullptr);
			for (int i = 0; i < moduleDesc.outputs; ++i) {
				outputs[i] = assign.outputs[i];
			}
		}
		return *this;
	}

	~ModuleGraphicNode() {
		delete[] inputs;
		inputs = nullptr;
		delete[] outputs;
		outputs = nullptr;
	}

	void translate(const Vector2& p) {
		rect.translate(p);
		progRect.translate(p);
		recalcConnectors();
	}

	static const Size2d nodeSize;
	static const float connectorRadius;
	static const float connectorOffset;
private:

	// returns the vertical offset of a connector
	inline float getConnectorOffset(int connIdx, int connCount) const {
		return rect.height * ((1.0f + static_cast<float>(connIdx)) / (1.0f + static_cast<float>(connCount)));
	}

	void recalcConnectors() {
		const float connBboxSize = 2.0f * connectorRadius;
		// first the inputs
		const float ix = rect.x + connectorOffset;
		for (int i = 0; i < moduleDesc.inputs; ++i) {
			const float offsetY = getConnectorOffset(i, moduleDesc.inputs);
			inputs[i].r = Rect(ix, rect.y + offsetY - connectorRadius, connBboxSize, connBboxSize);
		}
		// now the outputs
		const float ox = rect.x + rect.width - (connectorOffset + connBboxSize);
		for (int i = 0; i < moduleDesc.outputs; ++i) {
			const float offsetY = getConnectorOffset(i, moduleDesc.outputs);
			outputs[i].r = Rect(ox, rect.y + offsetY - connectorRadius, connBboxSize, connBboxSize);
		}
	}

	void recalcProgress() {
		const float fractions = 9.0f; // separate the rect to 10 fracions
		// and calculate the progress so it is between the two before the last one
		const float fractionSize = rect.height / fractions;
		progRect.y = rect.y + 7.0f * fractionSize;
		progRect.height = fractionSize;
		// now calculate the offsets from horizontal borders
		progRect.x = rect.x + (connectorOffset + connectorRadius) * 2.0f;
		progRect.width = rect.width - (connectorOffset + connectorRadius) * 4.0f;
	}
};

struct ModuleConnectorDesc {
	ModuleId srcId;
	int inputIdx;
	ModuleId destId;
	int outputIdx;
	Vector2 srcPos;
	Vector2 destPos;

	ModuleConnectorDesc()
		: srcId(-1)
		, destId(-1)
		, inputIdx(0)
		, outputIdx(0)
	{}
};

class MultiModuleMode;

class MultiModuleCanvas : public ScalablePanel {
public:
	MultiModuleCanvas(MultiModuleMode * _parent);

	virtual ~MultiModuleCanvas() = default;

	void OnIdle(wxIdleEvent& evt);

	void OnTimer(wxTimerEvent& evt);

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBackground(wxEraseEvent& evt);

	void addModuleDescription(ModuleId id, const ModuleDescription& md);

	void addModuleProgress(ModuleId id, const std::shared_ptr<ProgressCallback>& cb);

	// return type?
	void destroyModuleNode(ModuleId id);

protected:
	// mouse overrides
	bool onMouseMove() override;
	bool onMouseLeftDown() override;
	bool onMouseDoubleLeftDown() override;
	bool onMouseLeftUp() override;
	bool onMouseRightDown() override;

private:
	enum ModuleColors {
		MC_BACKGROUND = 0,
		MC_NODE_TEXT,
		MC_NODE_BKG,
		MC_NODE_BORDER,
		MC_SELECTED_NODE_BKG,
		MC_SELECTED_NODE_BORDER,
		MC_CONNECTOR_EMPTY,
		MC_CONNECTOR_HOVER,
		MC_CONNECTOR,
		MC_PROGRESS_BKG,
		MC_PROGRESS,
		MC_COUNT, // must remain last
	};
	static const wxColour mmColors[MC_COUNT];

	// some helper functions for module connectivity and hover
	// returns true if a state change requiring a refresh has occured
	bool reevaluateMouseHover();

	void disconnectMouseConnector();

	bool createConnector(const ModuleConnectorDesc& mcd);

	// returns the destroyed connector
	ModuleConnectorDesc destroyConnector(EdgeId id);

	// some drawing functions
	void drawModuleNode(wxDC& dc, const ModuleGraphicNode& mgd, ModuleId mgdMapId);

	void drawModuleConnector(wxDC& dc, const ModuleConnectorDesc& mcd);

	MultiModuleMode * parent;
	std::unordered_map<ModuleId, ModuleGraphicNode> moduleMap;
	std::unordered_map<ModuleId, std::shared_ptr<ProgressCallback> > progressMap;
	std::unordered_map<EdgeId, ModuleConnectorDesc> connectorMap;
	ModuleConnectorDesc mouseConnector;
	ModuleId hoveredModuleMapId;
	enum {
		HT_NONE,
		HT_INPUT,
		HT_OUTPUT,
	} hoveredConnectorType;
	int hoveredModuleConnectorIdx;
	ModuleId selectedModuleMapId;
	bool popupEvent; //!< will be true if the current event is generated by a popup

	// checks if any progress callbacks were made dirty to trigger refresh
	bool checkProgressUpdates() const;

	static const wxWindowID MULTI_MODULE_TIMER_ID;
	wxTimer refreshTimer;
};

#endif // __WX_MULTI_MODULE_H__
