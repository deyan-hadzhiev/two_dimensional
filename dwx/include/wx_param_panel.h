#ifndef __WX_PARAM_PANEL_H__
#define __WX_PARAM_PANEL_H__

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/choice.h>

#include <unordered_map>

#include "module_base.h"
#include "convolution.h"
#include "wx_modes.h"
#include "vector2.h"

class ParamPanel;
class CKernelPanel;

class CKernelDlg : public wxDialog {
public:
	CKernelDlg(CKernelPanel * parent, const wxString& title, long style);

	virtual ~CKernelDlg() {}

	virtual void getKernel(ConvolutionKernel& out) const = 0;
};

class CKernelTableDlg : public CKernelDlg {
public:
	CKernelTableDlg(CKernelPanel * parent, const wxString& title, int side);

	void getKernel(ConvolutionKernel& out) const override final;

	void setKernel(const ConvolutionKernel& kernel);

	void setKernelSide(int s);

	void OnKernelSide(wxCommandEvent& evt);

	void OnShow(wxShowEvent& evt);

	void OnClose(wxCloseEvent& evt);
	void OnEscape(wxKeyEvent& evt);

	void OnParamChange(wxCommandEvent& evt);

	void OnSymmetryChange(wxCommandEvent& evt);

	void OnNormalize(wxCommandEvent& evt);

	void OnReset(wxCommandEvent& evt);
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
	wxButton * sideButton;
	wxTextCtrl * sideCtrl;
	wxRadioButton * symmetryRb[ST_COUNT];
	wxWindowID symmetryRbId;
	wxStaticText * sumText;
	wxVector<wxTextCtrl*> kernelParams;
	wxWindowID kernelParamsId;
	wxButton * normalizeButton;
	wxTextCtrl * normalizationValue;
	wxButton * resetButton;
	int kernelSide;
	ConvolutionKernel cachedKernel; //!< used only for preview and may not always be updated - don't use it
};

class CKernelCurveDlg;

class CurveCanvas : public wxPanel {
public:
	CurveCanvas(CKernelCurveDlg * parent, int numSamples);

	void setNumSamples(int n);

	// saves all the samples in the supplied vector
	void getSamples(std::vector<float>& output) const;

	void resetSamples();
private:
	enum ColorConstants {
		CC_BACKGROUND = 0,
		CC_AXES,
		CC_CURVE,
		CC_SAMPLES,
		CC_POLY,
		CC_POLY_HOVER,
		CC_POLY_DRAG,
		CC_COUNT,
	};
	static const wxColour curveColours[CC_COUNT];
	static const int pointRadius;

	enum SampleState {
		SS_NORMAL = 0,
		SS_HOVER,
		SS_DRAG,
		SS_COUNT,
	};

	struct DiscreteSample {
		wxPoint pos;
		wxRect bbox;
		unsigned state;
	};

	void OnMouseEvent(wxMouseEvent& evt);

	void updateCanvasSample(int nSample, unsigned state, const wxPoint& dpos);

	void OnPaint(wxPaintEvent& evt);
	void drawSamples(wxBufferedPaintDC& pdc);

	void OnEraseBkg(wxEraseEvent& evt);

	void OnSizeEvent(wxSizeEvent& evt);

	// udpates the canonic samples from the canvas (only specified positions)
	// TODO: Make an array if bulk changes are implemented
	void updateSamples(int sampleN);

	// updates the canvas samples from the canonic samples
	void updateCanvasSamples();

	// some coordinate conversion functions
	Vector2 canvasToReal(const wxPoint& p) const;
	wxPoint realToCanvas(const Vector2& p) const;

	CKernelCurveDlg * parent;
	wxSize panelSize; //!< will always hold the current canvas size
	wxPoint axes;
	int numSamples;
	std::vector<Vector2> samples;
	std::vector<DiscreteSample> canvasSamples; //!< will hold updated samples in canvas space

	// some mouse specific variables
	bool mouseOverCanvas;
	bool mouseDrag; //!< left down
	int hoveredPoint;
	wxPoint mousePos;

	ConvolutionKernel cachedKernel; //!< used only for preview and may not always be updated - don't use it
};

class CKernelCurveDlg : public CKernelDlg {
	friend class CurveCanvas;
public:
	CKernelCurveDlg(CKernelPanel * parent, const wxString& title, int side);

	void getKernel(ConvolutionKernel& out) const override final;

	void OnShow(wxShowEvent& evt);

	void OnClose(wxCloseEvent& evt);

	void OnEscape(wxKeyEvent& evt);
private:
	void OnSliderChange(wxScrollEvent& evt);

	void OnApplyButton(wxCommandEvent& evt);

	void OnResetButton(wxCommandEvent& evt);

	static const int minSamples;
	static const int maxSamples;

	CKernelPanel * paramPanel;
	wxSlider * sliderCtrl;
	wxButton * applyButton; //!< just sends kernel change event
	wxButton * resetButton; //!< resets the samples
	int currentSamples;

	CurveCanvas * canvas;
	int currentSide;
};

class CKernelPreviewDlg : public wxDialog {
public:
	CKernelPreviewDlg(CKernelPanel * parent);

	void updatePreview(const ConvolutionKernel& kernel);

private:
	void OnPaint(wxPaintEvent& evt);

	void OnErase(wxEraseEvent& evt);

	void OnClose(wxCloseEvent& evt);

	void OnSize(wxSizeEvent& evt);

	CKernelPanel * paramPanel;
	wxPanel * canvas;
	ConvolutionKernel currentKernel;
};

class CKernelPanel : public wxPanel {
public:
	CKernelPanel(ParamPanel * parent, wxWindowID id, const wxString& label, const wxString& defSide);

	ConvolutionKernel GetValue() const;

	void OnKernelDlgType(wxCommandEvent& evt);

	void OnShowButton(wxCommandEvent& evt);
	void OnHideButton(wxCommandEvent& evt);

	void OnShowPreviewButton(wxCommandEvent& evt);
	void OnHidePreviewButton(wxCommandEvent& evt);

	void OnKernelChange(wxCommandEvent& evt);

	bool previewShown() const;
	void updatePreview(const ConvolutionKernel& ck);
private:
	enum DialogType {
		DT_CURVE = 0,
		DT_TABLE,
		DT_COUNT,
	};

	static const wxString dialogTitles[DT_COUNT];

	ParamPanel * paramPanel;

	CKernelDlg * kernelDialogs[DT_COUNT];
	wxWindowID kernelDlgTypeId;
	wxRadioButton * kernelDlgRb[DT_COUNT];

	CKernelDlg * currentDlg;
	wxBoxSizer * mainSizer;
	wxButton * showButton;
	wxButton * hideButton;

	CKernelPreviewDlg * previewDlg;
	wxButton * showPreviewButton;
	wxButton * hidePreviewButton;
};

// TODO: change logic a bit to avoid name clashes

class ParamPanel : public wxPanel, public ParamManager {
	friend class CKernelPanel;
	wxBoxSizer * panelSizer;
	ModePanel * modePanel;

	std::unordered_map<std::string, int> paramMap;
	std::unordered_map<int, ParamDescriptor> paramIdMap;
	std::unordered_map<int, wxTextCtrl*> textCtrlMap;
	std::unordered_map<int, std::pair<wxTextCtrl*, wxTextCtrl*> > pairTextCtrlMap;
	std::unordered_map<int, wxCheckBox*> checkBoxMap;
	std::unordered_map<int, CKernelPanel*> kernelMap;
	std::unordered_map<int, wxChoice*> choiceMap;
	std::unordered_map<const ModuleBase*, wxBoxSizer*> moduleSizers;

	void createTextCtrl(const int id, const ParamDescriptor& pd);

	void createCheckBox(const int id, const ParamDescriptor& pd);

	void createCKernel(const int id, const ParamDescriptor& pd);

	void createChoice(const int id, const ParamDescriptor& pd);

	// will get the sizer associated with the module, or it will create a new one
	wxBoxSizer * getModuleSizer(const ModuleBase*);
public:
	ParamPanel(ModePanel * parent);

	virtual void addParam(const ParamDescriptor& pd) override;

	virtual bool getStringParam(std::string& value, const std::string& paramName) const override;

	virtual bool getIntParam(int& value, const std::string& paramName) const override;

	virtual bool getFloatParam(float& value, const std::string& paramName) const override;

	virtual bool getBoolParam(bool& value, const std::string& paramName) const override;

	virtual bool getCKernelParam(ConvolutionKernel& value, const std::string& paramName) const override;

	virtual bool getEnumParam(unsigned& value, const std::string& paramName) const override;

	virtual bool getVectorParam(Vector2& value, const std::string& paramName) const override;

	virtual void resizeParent();

	void OnParamChange(wxCommandEvent& ev);
};

#endif // __WX_PARAM_PANEL_H__
