#ifndef __WX_PARAM_PANEL_H__
#define __WX_PARAM_PANEL_H__

#include <wx/panel.h>
#include <wx/sizer.h>

#include <unordered_map>

#include "kernel_base.h"
#include "convolution.h"
#include "wx_modes.h"

class ParamPanel;

class CKernelPanel : public wxPanel {
public:
	CKernelPanel(ParamPanel * parent, wxWindowID id, const wxString& label, const wxString& defSide);

	ConvolutionKernel GetValue() const;

	void OnSideChnage(wxCommandEvent& evt);

	void OnShowButton(wxCommandEvent& evt);
	void OnHideButton(wxCommandEvent& evt);

	void OnKernelChange(wxCommandEvent& evt);
private:
	void createKernelControls(const wxString& side);

	ParamPanel * parent;

	wxPanel * detailsPanel;
	wxBoxSizer * mainSizer;
	wxTextCtrl * sideCtrl;
	wxButton * showButton;
	wxButton * hideButton;

	int currentSide;
	wxFlexGridSizer * kernelParamsSizer;
	wxVector<wxTextCtrl*> kernelParams;
};

// TODO: change logic a bit to avoid name clashes

class ParamPanel : public wxPanel, public ParamManager {
	wxBoxSizer * panelSizer;
	ModePanel * modePanel;

	std::unordered_map<std::string, int> paramMap;
	std::unordered_map<int, ParamDescriptor> paramIdMap;
	std::unordered_map<int, wxTextCtrl*> textCtrlMap;
	std::unordered_map<int, wxCheckBox*> checkBoxMap;
	std::unordered_map<int, CKernelPanel*> kernelMap;
	std::unordered_map<KernelBase*, wxBoxSizer*> kernelSizers;

	void createTextCtrl(const int id, const ParamDescriptor& pd);

	void createCheckBox(const int id, const ParamDescriptor& pd);

	void createCKernel(const int id, const ParamDescriptor& pd);

	static int idOffset;
public:
	ParamPanel(ModePanel * parent);

	virtual void addParam(const ParamDescriptor& pd) override;

	virtual bool getStringParam(std::string& value, const std::string& paramName) const override;

	virtual bool getIntParam(int& value, const std::string& paramName) const override;

	virtual bool getFloatParam(float& value, const std::string& paramName) const override;

	virtual bool getBoolParam(bool& value, const std::string& paramName) const override;

	virtual bool getCKernelParam(ConvolutionKernel& value, const std::string& paramName) const override;

	virtual void resizeParent();

	void OnParamChange(wxCommandEvent& ev);
};

#endif // __WX_PARAM_PANEL_H__
