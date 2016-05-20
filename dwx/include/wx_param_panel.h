#ifndef __WX_PARAM_PANEL_H__
#define __WX_PARAM_PANEL_H__

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/choice.h>

#include <unordered_map>

#include "kernel_base.h"
#include "convolution.h"
#include "wx_modes.h"

class ParamPanel;
class CKernelPanel;

class CKernelDlg : public wxDialog {
public:
	CKernelDlg(CKernelPanel * parent, const wxString& title, int side);

	ConvolutionKernel getKernel() const;

	void setKernelSide(int s);

	void OnClose(wxCloseEvent& evt);
	void OnEscape(wxKeyEvent& evt);

	void OnParamChange(wxCommandEvent& evt);

	void OnSymmetryChange(wxCommandEvent& evt);
private:
	enum SymmetryType {
		ST_NO_SYMMETRY = 0,
		ST_CENTRAL,
		ST_RADIAL,
		ST_COUNT,
	} currentSymmetry;

	static const wxString symmetryName[ST_COUNT];

	void updateCentral(int id);

	void updateRadial(int id);

	void updateSymmetry();

	wxPoint indexToPoint(int id) const;
	int pointToIndex(const wxPoint& p) const;

	void recalculateSum();

	CKernelPanel * paramPanel;
	wxStaticText * sumText;
	wxRadioButton * symmetryRb[ST_COUNT];
	wxWindowID symmetryRbId;
	wxVector<wxTextCtrl*> kernelParams;
	wxWindowID kernelParamsId;
	int kernelSide;
};

class CKernelPanel : public wxPanel {
public:
	CKernelPanel(ParamPanel * parent, wxWindowID id, const wxString& label, const wxString& defSide);

	ConvolutionKernel GetValue() const;

	void OnSideChnage(wxCommandEvent& evt);

	void OnShowButton(wxCommandEvent& evt);
	void OnHideButton(wxCommandEvent& evt);

	void OnKernelChange(wxCommandEvent& evt);
private:
	ParamPanel * paramPanel;

	CKernelDlg * kernelDlg;
	wxBoxSizer * mainSizer;
	wxTextCtrl * sideCtrl;
	wxButton * showButton;
	wxButton * hideButton;
};

// TODO: change logic a bit to avoid name clashes

class ParamPanel : public wxPanel, public ParamManager {
	friend class CKernelPanel;
	wxBoxSizer * panelSizer;
	ModePanel * modePanel;

	std::unordered_map<std::string, int> paramMap;
	std::unordered_map<int, ParamDescriptor> paramIdMap;
	std::unordered_map<int, wxTextCtrl*> textCtrlMap;
	std::unordered_map<int, wxCheckBox*> checkBoxMap;
	std::unordered_map<int, CKernelPanel*> kernelMap;
	std::unordered_map<int, wxChoice*> choiceMap;
	std::unordered_map<const KernelBase*, wxBoxSizer*> kernelSizers;

	void createTextCtrl(const int id, const ParamDescriptor& pd);

	void createCheckBox(const int id, const ParamDescriptor& pd);

	void createCKernel(const int id, const ParamDescriptor& pd);

	void createChoice(const int id, const ParamDescriptor& pd);

	// will get the sizer associated with the kernel, or it will create a new one
	wxBoxSizer * getKernelSizer(const KernelBase*);
public:
	ParamPanel(ModePanel * parent);

	virtual void addParam(const ParamDescriptor& pd) override;

	virtual bool getStringParam(std::string& value, const std::string& paramName) const override;

	virtual bool getIntParam(int& value, const std::string& paramName) const override;

	virtual bool getFloatParam(float& value, const std::string& paramName) const override;

	virtual bool getBoolParam(bool& value, const std::string& paramName) const override;

	virtual bool getCKernelParam(ConvolutionKernel& value, const std::string& paramName) const override;

	virtual bool getEnumParam(unsigned& value, const std::string& paramName) const override;

	virtual void resizeParent();

	void OnParamChange(wxCommandEvent& ev);
};

#endif // __WX_PARAM_PANEL_H__
