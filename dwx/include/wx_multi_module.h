#ifndef __WX_MULTI_MODULE_H__
#define __WX_MULTI_MODULE_H__

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/dcbuffer.h>

#include "wx_bitmap_canvas.h"
#include "module_base.h"

class MultiModuleCanvas : public ScalablePanel {
public:
	MultiModuleCanvas(wxWindow * _parent);

	virtual ~MultiModuleCanvas() = default;

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBackground(wxEraseEvent& evt);
};

#endif // __WX_MULTI_MODULE_H__
