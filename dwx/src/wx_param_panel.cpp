#include <wx/panel.h>

#include "wx_param_panel.h"
#include "wx_modes.h"
#include "guimain.h"


CKernelDlg::CKernelDlg(CKernelPanel * parent, const wxString& title, int side)
	: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, paramPanel(parent)
	, kernelSide(0)
{
	setKernelSide(side);
}

ConvolutionKernel CKernelDlg::getKernel() const {
	ConvolutionKernel retval(kernelSide);
	float * ckData = retval.getDataPtr();
	for (int i = 0; i < kernelParams.size(); ++i) {
		ckData[i] = wxAtof(kernelParams[i]->GetValue());
	}
	return retval;
}

void CKernelDlg::setKernelSide(int s) {
	if (s > 0 && (s != kernelSide || 0 == kernelSide)) {
		for (auto rit = kernelParams.rbegin(); rit != kernelParams.rend(); ++rit) {
			(*rit)->Disconnect(wxEVT_TEXT_ENTER, wxCommandEventHandler(CKernelPanel::OnKernelChange), NULL, this);
			(*rit)->Destroy();
		}
		kernelParams.clear();
		kernelSide = s;
		wxGridSizer * dlgSizer = new wxGridSizer(kernelSide, kernelSide, 2, 2);
		const int sideSqr = kernelSide * kernelSide;
		for (int i = 0; i < sideSqr; ++i) {
			wxTextCtrl * kernelPar = new wxTextCtrl(this, wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
			kernelPar->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(CKernelPanel::OnKernelChange), NULL, paramPanel);
			kernelParams.push_back(kernelPar);
			dlgSizer->Add(kernelPar, 1, wxEXPAND | wxSHRINK | wxALL, 2);
		}
		SetSizerAndFit(dlgSizer);
		SendSizeEvent();
	}
}

CKernelPanel::CKernelPanel(ParamPanel * _parent, wxWindowID id, const wxString & label, const wxString& defSide)
	: wxPanel(_parent, id)
	, paramPanel(_parent)
	, kernelDlg(new CKernelDlg(this, label, wxAtoi(defSide)))
	, mainSizer(nullptr)
	, sideCtrl(nullptr)
	, showButton(nullptr)
	, hideButton(nullptr)
{
	mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(new wxStaticText(this, wxID_ANY, label), 0, wxEXPAND | wxALL);

	sideCtrl = new wxTextCtrl(this, wxID_ANY, defSide, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	sideCtrl->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(CKernelPanel::OnSideChnage), NULL, this);
	mainSizer->Add(sideCtrl, 0, wxEXPAND | wxALL, ModePanel::panelBorder);

	showButton = new wxButton(this, wxID_ANY, wxT("Show"));
	showButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CKernelPanel::OnShowButton), NULL, this);
	mainSizer->Add(showButton, 0, wxEXPAND | wxALL, ModePanel::panelBorder);

	hideButton = new wxButton(this, wxID_ANY, wxT("Hide"));
	hideButton->Hide();
	hideButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CKernelPanel::OnHideButton), NULL, this);
	mainSizer->Add(hideButton, 0, wxEXPAND | wxALL, ModePanel::panelBorder);
	mainSizer->AddStretchSpacer(1);

	SetSizerAndFit(mainSizer);
}

ConvolutionKernel CKernelPanel::GetValue() const {
	return kernelDlg->getKernel();
}

void CKernelPanel::OnSideChnage(wxCommandEvent & evt) {
	kernelDlg->setKernelSide(wxAtoi(sideCtrl->GetValue()));
}

void CKernelPanel::OnShowButton(wxCommandEvent & evt) {
	kernelDlg->Show();
	hideButton->Show();
	showButton->Hide();
	mainSizer->Layout();
	Fit();
}

void CKernelPanel::OnHideButton(wxCommandEvent & evt) {
	kernelDlg->Hide();
	hideButton->Hide();
	showButton->Show();
	mainSizer->Layout();
	Fit();
}

void CKernelPanel::OnKernelChange(wxCommandEvent & evt) {
	wxCommandEvent changeEvt(wxEVT_NULL, this->GetId());
	wxPostEvent(this->GetParent(), changeEvt);
}

int ParamPanel::idOffset = ViewFrame::MID_VF_LAST_ID; //!< set the custom id to the last used id of the view frame

void ParamPanel::createTextCtrl(const int id, const ParamDescriptor& pd) {
	auto kernelSizerIt = kernelSizers.find(pd.kernel);
	wxBoxSizer * sizer = nullptr;
	if (kernelSizerIt == kernelSizers.end()) {
		sizer = new wxBoxSizer(wxHORIZONTAL);
		kernelSizers[pd.kernel] = sizer;
		panelSizer->Add(sizer, 1, wxEXPAND);
	} else {
		sizer = kernelSizerIt->second;
	}
	wxBoxSizer * vert = new wxBoxSizer(wxVERTICAL);
	vert->Add(new wxStaticText(this, wxID_ANY, pd.name), 1, wxEXPAND | wxCENTER);
	sizer->Add(vert, 0, wxEXPAND | wxLEFT | wxRIGHT, ModePanel::panelBorder);
	wxTextCtrl * ctrl = new wxTextCtrl(this, id, wxT("0"), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	ctrl->SetValue(pd.defaultValue);
	sizer->Add(ctrl, 0, wxEXPAND | wxLEFT | wxRIGHT, ModePanel::panelBorder);
	textCtrlMap[id] = ctrl;
	// also connect the event
	Connect(id, wxEVT_TEXT_ENTER, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

void ParamPanel::createCheckBox(const int id, const ParamDescriptor& pd) {
	auto kernelSizerIt = kernelSizers.find(pd.kernel);
	wxBoxSizer * sizer = nullptr;
	if (kernelSizerIt == kernelSizers.end()) {
		sizer = new wxBoxSizer(wxHORIZONTAL);
		kernelSizers[pd.kernel] = sizer;
		panelSizer->Add(sizer, 1, wxEXPAND);
	} else {
		sizer = kernelSizerIt->second;
	}
	wxCheckBox * cb = new wxCheckBox(this, id, pd.name);
	cb->SetValue(pd.defaultValue != "false" || pd.defaultValue != "0");
	sizer->Add(cb, 1, wxEXPAND);
	checkBoxMap[id] = cb;
	// also connect the event
	Connect(id, wxEVT_CHECKBOX, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

void ParamPanel::createCKernel(const int id, const ParamDescriptor & pd) {
	auto kernelSizerIt = kernelSizers.find(pd.kernel);
	wxBoxSizer * sizer = nullptr;
	if (kernelSizerIt == kernelSizers.end()) {
		sizer = new wxBoxSizer(wxHORIZONTAL);
		kernelSizers[pd.kernel] = sizer;
		panelSizer->Add(sizer, 1, wxEXPAND);
	} else {
		sizer = kernelSizerIt->second;
	}
	CKernelPanel * ckp = new CKernelPanel(this, id, pd.name, pd.defaultValue);
	sizer->Add(ckp, 1, wxEXPAND);
	kernelMap[id] = ckp;
	Connect(id, wxEVT_NULL, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

ParamPanel::ParamPanel(ModePanel * parent)
	: wxPanel(parent)
	, modePanel(parent)
	, panelSizer(nullptr)
{
	panelSizer = new wxBoxSizer(wxVERTICAL);
	SetSizerAndFit(panelSizer);
}

void ParamPanel::addParam(const ParamDescriptor & pd) {
	const int id = ++idOffset;
	paramMap[pd.name] = id;
	paramIdMap[id] = pd;
	switch (pd.type)
	{
	case(ParamDescriptor::ParamType::PT_BOOL) :
		createCheckBox(id, pd);
		break;
	case(ParamDescriptor::ParamType::PT_INT) :
	case(ParamDescriptor::ParamType::PT_FLOAT) :
	case(ParamDescriptor::ParamType::PT_STRING) :
		createTextCtrl(id, pd);
		break;
	case(ParamDescriptor::ParamType::PT_CKERNEL) :
		createCKernel(id, pd);
		break;
	default:
		break;
	}
	SetSizerAndFit(panelSizer, false);
	SendSizeEvent();
	resizeParent();
}

bool ParamPanel::getStringParam(std::string & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = textCtrlMap.find(paramIt->second);
		if (ctrlIt != textCtrlMap.end()) {
			value = std::string(ctrlIt->second->GetValue());
			return true;
		}
	}
	return false;
}

bool ParamPanel::getIntParam(int & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = textCtrlMap.find(paramIt->second);
		if (ctrlIt != textCtrlMap.end()) {
			value = atoi(ctrlIt->second->GetValue().c_str());
			return true;
		}
	}
	return false;
}

bool ParamPanel::getFloatParam(float & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = textCtrlMap.find(paramIt->second);
		if (ctrlIt != textCtrlMap.end()) {
			value = atof(ctrlIt->second->GetValue().c_str());
			return true;
		}
	}
	return false;
}

bool ParamPanel::getBoolParam(bool & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = checkBoxMap.find(paramIt->second);
		if (ctrlIt != checkBoxMap.end()) {
			value = ctrlIt->second->GetValue();
			return true;
		}
	}
	return false;
}

bool ParamPanel::getCKernelParam(ConvolutionKernel & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = kernelMap.find(paramIt->second);
		if (ctrlIt != kernelMap.end()) {
			value = ctrlIt->second->GetValue();
			return true;
		}
	}
	return false;
}

void ParamPanel::resizeParent() {
	SendSizeEventToParent();
	SendSizeEvent();
}

void ParamPanel::OnParamChange(wxCommandEvent & ev) {
	const int evtId = ev.GetId();
	auto pdIt = paramIdMap.find(evtId);
	if (pdIt != paramIdMap.end()) {
		ParamDescriptor& pd = pdIt->second;
		pd.kernel->update();
	}
}
