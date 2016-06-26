#include <wx/panel.h>

#include "util.h"
#include "vector2.h"
#include "wx_param_panel.h"
#include "wx_modes.h"
#include "guimain.h"

CKernelDlg::CKernelDlg(CKernelPanel * parent, const wxString& title, long style)
	: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, style)
{}

const wxString CKernelTableDlg::symmetryName[CKernelTableDlg::ST_COUNT] = {
	wxT("None"),    //!< ST_NO_SYMMETRY
	wxT("Central"),	//!< ST_CENTRAL
	wxT("Radial"),	//!< ST_RADIAL
};

CKernelTableDlg::CKernelTableDlg(CKernelPanel * parent, const wxString& title, int side)
	: CKernelDlg(parent, title, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, currentSymmetry(ST_NO_SYMMETRY)
	, paramPanel(parent)
	, sideCtrl(nullptr)
	, sideButton(nullptr)
	, symmetryRb{ nullptr }
	, symmetryRbId(0)
	, sumText(nullptr)
	, kernelParamsId(0)
	, normalizeButton(nullptr)
	, normalizationValue(nullptr)
	, resetButton(nullptr)
	, kernelSide(0)
{
	Connect(wxEVT_SHOW, wxShowEventHandler(CKernelTableDlg::OnShow), NULL, this);

	sideCtrl = new wxTextCtrl(this, wxID_ANY, wxString() << side);
	const wxWindowID sideButtonId = WinIDProvider::getProvider().getId();
	sideButton = new wxButton(this, sideButtonId, "Change Side");
	Connect(sideButtonId, wxEVT_BUTTON, wxCommandEventHandler(CKernelTableDlg::OnKernelSide), NULL, this);

	sumText = new wxStaticText(this, wxID_ANY, wxT("0"));
	sumText->SetSizeHints(wxSize(20, -1));
	symmetryRbId = WinIDProvider::getProvider().getId(ST_COUNT);
	for (int i = 0; i < ST_COUNT; ++i) {
		const wxWindowID rbId = symmetryRbId + i;
		symmetryRb[i] = new wxRadioButton(this, rbId, symmetryName[i], wxDefaultPosition, wxDefaultSize, (i == 0 ? wxRB_GROUP : 0L));
		Connect(rbId, wxEVT_RADIOBUTTON, wxCommandEventHandler(CKernelTableDlg::OnSymmetryChange), NULL, this);
	}

	const wxWindowID normalizeId = WinIDProvider::getProvider().getId();
	normalizeButton = new wxButton(this, normalizeId, "Normalize");
	Connect(normalizeId, wxEVT_BUTTON, wxCommandEventHandler(CKernelTableDlg::OnNormalize), NULL, this);

	normalizationValue = new wxTextCtrl(this, wxID_ANY, "1.0");
	normalizationValue->SetSizeHints(wxSize(20, -1));

	const wxWindowID resetId = WinIDProvider::getProvider().getId();
	resetButton = new wxButton(this, resetId, "Reset");
	Connect(resetId, wxEVT_BUTTON, wxCommandEventHandler(CKernelTableDlg::OnReset), NULL, this);

	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(CKernelTableDlg::OnClose), NULL, this);
	Connect(wxID_ANY, wxEVT_CHAR_HOOK, wxKeyEventHandler(CKernelTableDlg::OnEscape), NULL, this);
}

ConvolutionKernel CKernelTableDlg::getKernel() const {
	ConvolutionKernel retval(kernelSide);
	float * ckData = retval.getDataPtr();
	for (int i = 0; i < kernelParams.size(); ++i) {
		ckData[i] = wxAtof(kernelParams[i]->GetValue());
	}
	return retval;
}

void CKernelTableDlg::setKernel(const ConvolutionKernel & kernel) {
	const int inKernelSide = kernel.getSide();
	if (inKernelSide != kernelSide) {
		setKernelSide(inKernelSide);
	}
	const int sqSide = inKernelSide * inKernelSide;
	const float * kernelData = kernel.getDataPtr();
	for (int i = 0; i < sqSide; ++i) {
		kernelParams[i]->ChangeValue(wxString() << kernelData[i]);
	}
}

void CKernelTableDlg::setKernelSide(int s) {
	if (s > 0 && (s != kernelSide || 0 == kernelSide) && (s & 1) != 0) {
		// clear previous param controls
		for (auto rit = kernelParams.rbegin(); rit != kernelParams.rend(); ++rit) {
			(*rit)->Disconnect(wxEVT_TEXT_ENTER, wxCommandEventHandler(CKernelPanel::OnKernelChange), NULL, this);
			(*rit)->Destroy();
		}
		kernelParams.clear();
		// clear previous sizers in the hierarchy
		wxSizer * prevSizer = GetSizer();
		if (prevSizer) {
			wxSizerItemList szList = prevSizer->GetChildren();
			for (int i = 0; i < szList.size(); ++i) {
				prevSizer->Remove(szList[i]->GetSizer());
			}
		}

		kernelSide = s;
		wxBoxSizer * dlgSizer = new wxBoxSizer(wxVERTICAL);
		wxBoxSizer * topSizer = new wxBoxSizer(wxHORIZONTAL);
		topSizer->Add(sideButton, 0, wxALL, 5);
		topSizer->Add(sideCtrl, 1, wxEXPAND | wxALL, 5);
		dlgSizer->Add(topSizer, 0, wxEXPAND | wxALL);
		wxBoxSizer * symmetrySizer = new wxBoxSizer(wxHORIZONTAL);
		for (int i = 0; i < ST_COUNT; ++i) {
			symmetrySizer->Add(symmetryRb[i], 1, wxEXPAND | wxALL, 5);
		}
		symmetrySizer->Add(sumText, 1, wxEXPAND | wxALL, 5);
		dlgSizer->Add(symmetrySizer, 0, wxEXPAND | wxALL);
		wxBoxSizer * normSizer = new wxBoxSizer(wxHORIZONTAL);
		normSizer->Add(normalizeButton, 0, wxALL, 5);
		normSizer->Add(normalizationValue, 1, wxEXPAND | wxALL, 5);
		normSizer->Add(resetButton, 0, wxALL, 5);
		dlgSizer->Add(normSizer, 0, wxEXPAND | wxSHRINK | wxALL, 5);

		wxGridSizer * gridSizer = new wxGridSizer(kernelSide, kernelSide, 2, 2);
		const int sideSqr = kernelSide * kernelSide;
		// allocate ids
		kernelParamsId = WinIDProvider::getProvider().getId(sideSqr);
		for (int i = 0; i < sideSqr; ++i) {
			const wxWindowID paramId = kernelParamsId + i;
			wxTextCtrl * kernelPar = new wxTextCtrl(this, paramId, wxString() << 0.0, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
			kernelPar->Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(CKernelPanel::OnKernelChange), NULL, paramPanel);
			Connect(paramId, wxEVT_TEXT, wxCommandEventHandler(CKernelTableDlg::OnParamChange), NULL, this);
			kernelParams.push_back(kernelPar);
			gridSizer->Add(kernelPar, 1, wxEXPAND | wxSHRINK | wxALL, 2);
		}
		updateSymmetry();
		dlgSizer->Add(gridSizer, 1, wxEXPAND | wxSHRINK | wxALL, 2);
		SetSizerAndFit(dlgSizer);
	}
	Layout();
	SendSizeEvent();
}

void CKernelTableDlg::OnKernelSide(wxCommandEvent & evt) {
	const int kSide = wxAtoi(sideCtrl->GetValue());
	setKernelSide(kSide);
}

void CKernelTableDlg::OnShow(wxShowEvent & evt) {
	setKernelSide(wxAtoi(sideCtrl->GetValue()));
}

void CKernelTableDlg::OnClose(wxCloseEvent & evt) {
	wxCommandEvent dummy;
	paramPanel->OnHideButton(dummy);
}

void CKernelTableDlg::OnEscape(wxKeyEvent & evt) {
	if (evt.GetKeyCode() == WXK_ESCAPE) {
		wxCommandEvent dummy;
		paramPanel->OnHideButton(dummy);
	} else if (evt.GetKeyCode() == WXK_F5) {
		wxCommandEvent dummy;
		paramPanel->OnKernelChange(dummy);
	} else {
		evt.Skip();
	}
}

void CKernelTableDlg::OnParamChange(wxCommandEvent & evt) {
	const int id = evt.GetId();
	if (id >= kernelParamsId && id < kernelParamsId + kernelSide * kernelSide) {
		const int index = id - kernelParamsId;
		if (currentSymmetry == ST_CENTRAL) {
			updateCentral(index);
		} else if (currentSymmetry == ST_RADIAL) {
			updateRadial(index);
		}
		recalculateSum();
	}
}

void CKernelTableDlg::OnSymmetryChange(wxCommandEvent & evt) {
	const wxWindowID evtId = evt.GetId();
	if (evtId >= symmetryRbId && evtId < symmetryRbId + ST_COUNT) {
		currentSymmetry = static_cast<SymmetryType>(evtId - symmetryRbId);
		updateSymmetry();
	}
}

void CKernelTableDlg::OnNormalize(wxCommandEvent& evt) {
	ConvolutionKernel current = getKernel();
	const float targetValue = wxAtof(normalizationValue->GetValue());
	current.normalize(targetValue);
	setKernel(current);
	recalculateSum();
}

void CKernelTableDlg::OnReset(wxCommandEvent & evt) {
	const int squareSide = kernelSide * kernelSide;
	for (int i = 0; i < squareSide; ++i) {
		kernelParams[i]->ChangeValue(wxString() << 0.0);
	}
}

void CKernelTableDlg::updateCentral(int index) {
	wxPoint op = indexToPoint(index);
	// the update is not necessary for the cental point
	if (op.x != 0 || op.y != 0) {
		const wxString value = kernelParams[index]->GetValue();
		for (int i = 0; i < 3; ++i) {
			// rotate the point by pi/2
			op = wxPoint(-op.y, op.x);
			const int ridx = pointToIndex(op);
			kernelParams[ridx]->ChangeValue(value);
		}
	}
}

void CKernelTableDlg::updateRadial(int index) {
	const wxPoint op = indexToPoint(index);
	// the update is not necessary for the central point
	if (op.x != 0 || op.y != 0) {
		// update the other central points too
		updateCentral(index);
		// now recalculate the whole bottom right part along with the central symmetry
		const int c = kernelSide / 2;
		for (int y = 1; y <= c; ++y) {
			for (int x = 1; x <= c; ++x) {
				const int tidx = pointToIndex(wxPoint(x, y));
				const Vector2 pv(x, y);
				const float len = pv.length();
				const int flen = static_cast<int>(floorf(len));
				const int clen = static_cast<int>(ceilf(len));
				float value = 0.0f;
				if (flen > c) {
					value = 0.0f;
				} else if (clen > c) {
					const int sidx = pointToIndex(wxPoint(0, flen));
					value = wxAtof(kernelParams[sidx]->GetValue()) * (1.0f - (len - flen));
				} else {
					const int fidx = pointToIndex(wxPoint(0, flen));
					const float fvalue = wxAtof(kernelParams[fidx]->GetValue());
					const int cidx = pointToIndex(wxPoint(0, clen));
					const float cvalue = wxAtof(kernelParams[cidx]->GetValue());
					const float t = (len - flen);
					value = cvalue * t + fvalue * (1.0f - t);
				}
				kernelParams[tidx]->ChangeValue(wxString() << value);
				updateCentral(tidx);
			}
		}
	}
}

void CKernelTableDlg::updateSymmetry() {
	if (ST_NO_SYMMETRY == currentSymmetry) {
		for (auto it = kernelParams.begin(); it != kernelParams.end(); ++it) {
			(*it)->Enable();
		}
	} else if (ST_CENTRAL == currentSymmetry) {
		const int sideSqr = kernelSide * kernelSide;
		for (int i = 0; i < sideSqr; ++i) {
			const wxPoint p = indexToPoint(i);
			const bool enable = (p == wxPoint(0, 0) || (p.x >= 0 && p.y < 0)); // don't forget the coord system is negative
			kernelParams[i]->Enable(enable);
		}
	} else if (ST_RADIAL == currentSymmetry) {
		const int sideSqr = kernelSide * kernelSide;
		for (int i = 0; i < sideSqr; ++i) {
			const wxPoint p = indexToPoint(i);
			const bool enable = (p.x == 0 && p.y <= 0);
			kernelParams[i]->Enable(enable);
		}
	}
}

wxPoint CKernelTableDlg::indexToPoint(int index) const {
	const int c = kernelSide / 2;
	return wxPoint((index % kernelSide) - c, (index / kernelSide) - c);
}

int CKernelTableDlg::pointToIndex(const wxPoint & p) const {
	const int c = kernelSide / 2;
	return (p.y + c) * kernelSide + p.x + c;
}

void CKernelTableDlg::recalculateSum() {
	float sum = 0.0f;
	for (auto it = kernelParams.begin(); it != kernelParams.end(); ++it) {
		sum += wxAtof((*it)->GetValue());
	}
	sumText->SetLabel(wxString() << sum);
}

CKernelPanel::CKernelPanel(ParamPanel * _parent, wxWindowID id, const wxString & label, const wxString& defSide)
	: wxPanel(_parent, id)
	, paramPanel(_parent)
	, kernelDlg(new CKernelTableDlg(this, label, wxAtoi(defSide)))
	, mainSizer(nullptr)
	, showButton(nullptr)
	, hideButton(nullptr)
{
	mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(new wxStaticText(this, wxID_ANY, label), 0, wxEXPAND | wxALL);

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

void ParamPanel::createTextCtrl(const int id, const ParamDescriptor& pd) {
	wxBoxSizer * sizer = getModuleSizer(pd.module);
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
	wxBoxSizer * sizer = getModuleSizer(pd.module);
	wxCheckBox * cb = new wxCheckBox(this, id, pd.name);
	cb->SetValue(pd.defaultValue != "false" && pd.defaultValue != "0");
	sizer->Add(cb, 1, wxEXPAND);
	checkBoxMap[id] = cb;
	// also connect the event
	Connect(id, wxEVT_CHECKBOX, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

void ParamPanel::createCKernel(const int id, const ParamDescriptor & pd) {
	wxBoxSizer * sizer = getModuleSizer(pd.module);
	CKernelPanel * ckp = new CKernelPanel(this, id, pd.name, pd.defaultValue);
	sizer->Add(ckp, 1, wxEXPAND);
	kernelMap[id] = ckp;
	Connect(id, wxEVT_NULL, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

void ParamPanel::createChoice(const int id, const ParamDescriptor & pd) {
	wxBoxSizer * sizer = getModuleSizer(pd.module);
	wxBoxSizer * labelSizer = new wxBoxSizer(wxVERTICAL);
	labelSizer->Add(new wxStaticText(this, wxID_ANY, pd.name), 1, wxEXPAND | wxCENTER);
	sizer->Add(labelSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, ModePanel::panelBorder);
	wxChoice * ch = new wxChoice(this, id);
	const std::vector<std::string> choices = splitString(pd.defaultValue.c_str(), ';');
	for (unsigned i = 0; i < choices.size(); ++i) {
		ch->Insert(wxString(choices[i]), i);
	}
	if (choices.size() > 0) {
		ch->SetSelection(0);
	}
	sizer->Add(ch, 0, wxEXPAND | wxLEFT | wxRIGHT, ModePanel::panelBorder);
	choiceMap[id] = ch;
	Connect(id, wxEVT_CHOICE, wxCommandEventHandler(ParamPanel::OnParamChange), NULL, this);
}

wxBoxSizer * ParamPanel::getModuleSizer(const ModuleBase * module) {
	auto moduleSizerIt = moduleSizers.find(module);
	wxBoxSizer * sizer = nullptr;
	if (moduleSizerIt == moduleSizers.end()) {
		sizer = new wxBoxSizer(wxHORIZONTAL);
		moduleSizers[module] = sizer;
		panelSizer->Add(sizer, 1, wxEXPAND);
	} else {
		sizer = moduleSizerIt->second;
	}
	return sizer;
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
	const int id = static_cast<int>(WinIDProvider::getProvider().getId());
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
	case(ParamDescriptor::ParamType::PT_ENUM) :
		createChoice(id, pd);
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

bool ParamPanel::getEnumParam(unsigned & value, const std::string & paramName) const {
	const auto paramIt = paramMap.find(paramName);
	if (paramIt != paramMap.end()) {
		const auto ctrlIt = choiceMap.find(paramIt->second);
		if (ctrlIt != choiceMap.end()) {
			const int selection = ctrlIt->second->GetSelection();
			if (selection == wxNOT_FOUND) {
				return false;
			} else {
				value = static_cast<int>(selection);
				return true;
			}
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
		pd.module->update();
	}
}
