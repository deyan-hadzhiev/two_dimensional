#ifndef __WX_MULTI_MODULE_H__
#define __WX_MULTI_MODULE_H__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dcbuffer.h>

#include "wx_bitmap_canvas.h"
#include "module_base.h"
#include "module_manager.h"

struct ModuleGraphicNode {
	ModuleDescription moduleDesc;
	Rect rect;

	ModuleGraphicNode() = default;
	ModuleGraphicNode(const ModuleDescription& _moduleDesc, const Rect& _rect)
		: moduleDesc(_moduleDesc)
		, rect(_rect)
	{}

	ModuleGraphicNode(const ModuleGraphicNode&) = default;
	ModuleGraphicNode& operator=(const ModuleGraphicNode&) = default;
};

class MultiModuleCanvas : public ScalablePanel {
public:
	MultiModuleCanvas(wxWindow * _parent);

	virtual ~MultiModuleCanvas() = default;

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBackground(wxEraseEvent& evt);

	void addModuleDescription(int id, const ModuleDescription& md);

protected:
	bool onMouseMove() override;

	bool onMouseLeftDown() override;

	bool onMouseLeftUp() override;

private:
	enum ModuleColors {
		MC_BACKGROUND = 0,
		MC_NODE_TEXT,
		MC_NODE_BKG,
		MC_NODE_BORDER,
		MC_SELECTED_NODE_BKG,
		MC_SELECTED_NODE_BORDER,
		MC_CONNECTOR,
		MC_COUNT, // must remain last
	};
	static const wxColour mmColors[MC_COUNT];

	// some drawing functions
	void drawModuleNode(wxDC& dc, const ModuleGraphicNode& mgd, int mgdMapId);

	const Size2d defaultModuleSize;
	std::unordered_map<int, ModuleGraphicNode> moduleMap;
	int selectedModuleMapId;
};

#endif // __WX_MULTI_MODULE_H__
