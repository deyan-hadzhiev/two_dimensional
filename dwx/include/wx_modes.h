#ifndef __WX_MODES_H__
#define __WX_MODES_H__

#include <wx/wx.h>
#include "guimain.h"
#include "wx_bitmap_canvas.h"
#include "kernel_base.h"
#include "kernels.h"
#include "geom_primitive.h"

class ViewFrame;

class ModePanel : public wxPanel {
public:
	ModePanel(ViewFrame * viewFrame, unsigned styles = ViewFrame::VFS_NOTHING_ENABLED);
	virtual ~ModePanel();

	virtual void onCommandMenu(wxCommandEvent& ev) {}

protected:
	ViewFrame * viewFrame;
	wxBoxSizer * mPanelSizer;
	static const int panelBorder; //!< the default border of panels
};

class InputOutputMode : public ModePanel {
public:
	InputOutputMode(ViewFrame * viewFrame, SimpleKernel * kernel);
	virtual ~InputOutputMode();

	virtual void onCommandMenu(wxCommandEvent& ev) override;
protected:
	BitmapCanvas * inputCanvas;
	BitmapCanvas * outputCanvas;
	wxPanel * compareCanvas;
	SimpleKernel * kernel;

	//wxPanel * inputCanvas;
	//wxPanel * outputCanvas;
	static const wxString ioFileSelector; //!< file selection string - change if a new file type is added
};

class GeometricOutput : public ModePanel, public ParamManager {
public:
	GeometricOutput(ViewFrame * vf, GeometricKernel * gkernel);
	virtual ~GeometricOutput();

	virtual void onCommandMenu(wxCommandEvent& ev) override;

	void addParam(const std::string& label, wxTextCtrl * wnd);

	std::string getParam(const std::string& paramName) const override final;

	virtual void setParam(const std::string& paramName, const std::string& paramValue) override final {};
protected:
	GeometricKernel * gkernel;
	std::unordered_map<std::string, wxTextCtrl*> customParams;
	wxSizer * customInputSizer;
	BitmapCanvas * outputCanvas;
	wxTextCtrl * widthCtrl;
	wxTextCtrl * heightCtrl;
	wxCheckBox * additiveCb;
	wxCheckBox * clearCb;
	wxCheckBox * showCoords;
	wxTextCtrl * colorCtrl;
};

class NegativePanel : public InputOutputMode {
public:
	NegativePanel(ViewFrame * viewFrame);
};

class TextSegmentationPanel : public InputOutputMode, public ParamManager {
	wxTextCtrl * threshold;
public:
	TextSegmentationPanel(ViewFrame * vf);

	std::string getParam(const std::string& paramName) const override final;

	virtual void setParam(const std::string& paramName, const std::string& paramValue) override final {};
};

class SinosoidPanel : public GeometricOutput {
public:
	SinosoidPanel(ViewFrame * vf);
};

class HoughRoTheta : public InputOutputMode {
public:
	HoughRoTheta(ViewFrame * vf);
};

#endif // __WX_MODES_H__
