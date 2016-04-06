#include <wx/panel.h>

#include "wx_param_panel.h"
#include "guimain.h"

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
	default:
		break;
	}
	SetSizerAndFit(panelSizer, false);
	SendSizeEvent();
	SendSizeEventToParent();
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

void ParamPanel::OnParamChange(wxCommandEvent & ev) {
	const int evtId = ev.GetId();
	auto pdIt = paramIdMap.find(evtId);
	if (pdIt != paramIdMap.end()) {
		ParamDescriptor& pd = pdIt->second;
		pd.kernel->update();
	}
}

