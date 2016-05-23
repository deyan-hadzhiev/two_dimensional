#ifndef __WX_MODES_H__
#define __WX_MODES_H__

#include <wx/wx.h>

#include "guimain.h"
#include "wx_bitmap_canvas.h"
#include "wx_param_panel.h"
#include "module_base.h"
#include "modules.h"
#include "geom_primitive.h"

class ViewFrame;

class ModePanel : public wxPanel {
public:
	friend class ParamPanel;
	friend class CKernelPanel;

	ModePanel(ViewFrame * viewFrame, unsigned styles = ViewFrame::VFS_NOTHING_ENABLED);
	virtual ~ModePanel();

	virtual void onCommandMenu(wxCommandEvent& ev) {}

	wxString getCbString() const;
protected:
	ViewFrame * viewFrame;
	wxBoxSizer * mPanelSizer;
	static const int panelBorder; //!< the default border of panels
	ProgressCallback cb;
	ParamPanel * paramPanel;
};

class InputOutputMode : public ModePanel {
public:
	InputOutputMode(ViewFrame * viewFrame, ModuleBase * _module);

	virtual void onCommandMenu(wxCommandEvent& ev) override;
protected:
	ImagePanel * inputPanel;
	ImagePanel * outputPanel;
	wxPanel * compareCanvas;
	ModuleBase * module;

	//wxPanel * inputCanvas;
	//wxPanel * outputCanvas;
	static const wxString ioFileSelector; //!< file selection string - change if a new file type is added
};

class GeometricOutput : public ModePanel {
public:
	GeometricOutput(ViewFrame * vf, ModuleBase * _module);

	virtual void onCommandMenu(wxCommandEvent& ev) override;

protected:
	ModuleBase * module;
	ImagePanel * outputPanel;
};

#endif // __WX_MODES_H__
