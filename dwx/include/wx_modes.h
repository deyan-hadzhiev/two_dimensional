#ifndef __WX_MODES_H__
#define __WX_MODES_H__

#include <wx/wx.h>

#include "guimain.h"
#include "wx_bitmap_canvas.h"
#include "wx_param_panel.h"
#include "kernel_base.h"
#include "kernels.h"
#include "geom_primitive.h"

class ViewFrame;

class ModePanel : public wxPanel {
public:
	friend class ParamPanel;

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
	InputOutputMode(ViewFrame * viewFrame, SimpleKernel * kernel);
	virtual ~InputOutputMode();

	virtual void onCommandMenu(wxCommandEvent& ev) override;
protected:
	ImagePanel * inputPanel;
	ImagePanel * outputPanel;
	wxPanel * compareCanvas;
	SimpleKernel * kernel;

	//wxPanel * inputCanvas;
	//wxPanel * outputCanvas;
	static const wxString ioFileSelector; //!< file selection string - change if a new file type is added
};

class GeometricOutput : public ModePanel {
public:
	GeometricOutput(ViewFrame * vf, GeometricKernel * gkernel);
	virtual ~GeometricOutput();

	virtual void onCommandMenu(wxCommandEvent& ev) override;

protected:
	GeometricKernel * gkernel;
	ImagePanel * outputPanel;
};

class NegativePanel : public InputOutputMode {
public:
	NegativePanel(ViewFrame * viewFrame);
};

class TextSegmentationPanel : public InputOutputMode {
public:
	TextSegmentationPanel(ViewFrame * vf);
};

class SinosoidPanel : public GeometricOutput {
public:
	SinosoidPanel(ViewFrame * vf);
};

class HoughRoTheta : public InputOutputMode {
public:
	HoughRoTheta(ViewFrame * vf);
};

class RotationPanel : public InputOutputMode {
public:
	RotationPanel(ViewFrame * vf);
};

class HistogramModePanel : public InputOutputMode {
public:
	HistogramModePanel(ViewFrame * vf);
};

class ThresholdModePanel : public InputOutputMode {
public:
	ThresholdModePanel(ViewFrame * vf);
};

class FilterModePanel : public InputOutputMode {
public:
	FilterModePanel(ViewFrame * vf);
};

#endif // __WX_MODES_H__
