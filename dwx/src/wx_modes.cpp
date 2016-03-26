#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/dialog.h>
#include <wx/file.h>
#include <wx/image.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>
#include <wx/valnum.h>

#include <sstream>

#include "guimain.h"
#include "wx_modes.h"
#include "wx_bitmap_canvas.h"
#include "kernels.h"
#include "geom_primitive.h"

ModePanel::ModePanel(ViewFrame * viewFrame, unsigned styles)
	: wxPanel(viewFrame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxNO_BORDER | wxSIZE_AUTO | wxSIZE_FORCE)
	, viewFrame(viewFrame)
	, mPanelSizer(nullptr)
{
	viewFrame->setCustomStyle(styles);
	mPanelSizer = new wxBoxSizer(wxVERTICAL);
	SetSizerAndFit(mPanelSizer);
	SetSize(viewFrame->GetClientSize());
}

ModePanel::~ModePanel() {}

const int ModePanel::panelBorder = 4;

const wxString InputOutputMode::ioFileSelector = wxT("png or jpeg images (*.png;*.jpeg;*.jpg;*.bmp)|*.png;*.jpeg;*.jpg;*.bmp");

InputOutputMode::InputOutputMode(ViewFrame * viewFrame, SimpleKernel * kernel)
	: ModePanel(viewFrame, ViewFrame::VFS_ALL_ENABLED & ~ViewFrame::VFS_CNT_COMPARE) // disable compare for now - it is not done and will not be soon
	, inputCanvas(nullptr)
	, outputCanvas(nullptr)
	, compareCanvas(nullptr)
	, kernel(kernel)
{
	wxBoxSizer * canvasSizer = new wxBoxSizer(wxHORIZONTAL);
	inputCanvas = new BitmapCanvas(this, viewFrame);
	//inputCanvas = new wxPanel(this);
	canvasSizer->Add(inputCanvas, 1, wxSHRINK | wxEXPAND | wxALL, panelBorder);
	outputCanvas = new BitmapCanvas(this, viewFrame);
	//outputCanvas = new wxPanel(this);
	canvasSizer->Add(outputCanvas, 1, wxSHRINK | wxEXPAND | wxALL, panelBorder);
	// add synchornizers
	inputCanvas->addSynchronizer(outputCanvas);
	outputCanvas->addSynchronizer(inputCanvas);
	// set the background colours
	wxColour bc(75, 75, 75);
	inputCanvas->SetBackgroundColour(bc);
	outputCanvas->SetBackgroundColour(bc);

	kernel->addInputManager(inputCanvas);
	kernel->addOutputManager(outputCanvas);

	compareCanvas = new wxPanel(this);
	canvasSizer->Add(compareCanvas, 1, wxEXPAND | wxALL, panelBorder);
	compareCanvas->SetBackgroundColour(*wxGREEN);
	compareCanvas->Show(false);
	mPanelSizer->Add(canvasSizer, 1, wxEXPAND | wxALL);
	SendSizeEvent();
}

InputOutputMode::~InputOutputMode() {
	if (kernel)
		delete kernel;
}

void InputOutputMode::onCommandMenu(wxCommandEvent & ev) {
	switch (ev.GetId()) {
	case (ViewFrame::MID_VF_FILE_OPEN) :
	{
		const wxStandardPaths& stdPaths = wxStandardPaths::Get();
		wxFileDialog fdlg(this, wxT("Open input image file"), stdPaths.GetUserDir(wxStandardPaths::Dir_Pictures), wxT(""), ioFileSelector, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fdlg.ShowModal() != wxID_CANCEL) {
			wxFileInputStream inputStream(fdlg.GetPath());
			if (inputStream.Ok()) {
				wxImage inputImage(inputStream);
				if (inputImage.Ok()) {
					inputCanvas->setImage(inputImage);
					viewFrame->SetStatusText(wxString(wxT("Loaded input image: ") + fdlg.GetPath()));
					// DEBUG
					//const int id = inputCanvas->getBmpId();
					//outputCanvas->setImage(inputImage, id);
					// ENDDEBUG
					// force synchronziation
					inputCanvas->synchronize();
				} else {
					viewFrame->SetStatusText(wxString(wxT("File is NOT a valid image: ")) + fdlg.GetPath());
				}
			} else {
				viewFrame->SetStatusText(wxString(wxT("Could NOT open file: ")) + fdlg.GetPath());
			}
		}
		break;
	}
	case (ViewFrame::MID_VF_FILE_SAVE) :
		// TODO:
		break;
	case (ViewFrame::MID_VF_CNT_RUN) :
		kernel->runKernel(0);
		break;
	case (ViewFrame::MID_VF_CNT_COMPARE) :
		inputCanvas->Show(!inputCanvas->IsShown());
		outputCanvas->Show(!outputCanvas->IsShown());
		compareCanvas->Show(!compareCanvas->IsShown());
		SendSizeEvent();
		break;
	default:
		break;
	}
}

GeometricOutput::GeometricOutput(ViewFrame * vf, GeometricKernel * gk)
	: ModePanel(vf, ViewFrame::VFS_CNT_RUN)
	, gkernel(gk)
	, outputCanvas(nullptr)
	, customInputSizer(nullptr)
	, widthCtrl(nullptr)
	, heightCtrl(nullptr)
	, additiveCb(nullptr)
	, clearCb(nullptr)
	, showCoords(nullptr)
	, colorCtrl(nullptr)
{
	wxBoxSizer * inputSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * defInputSizer = new wxBoxSizer(wxHORIZONTAL);
	// TODO add checkbox for clear output
	wxBoxSizer * wSizer = new wxBoxSizer(wxVERTICAL);
	wSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Width: ")), 1, wxEXPAND | wxALL);
	defInputSizer->Add(wSizer, 1, wxEXPAND | wxALL, panelBorder);
	widthCtrl = new wxTextCtrl(this, wxID_ANY, wxString() << GetSize().GetWidth());
	defInputSizer->Add(widthCtrl, 1, wxEXPAND | wxALL, panelBorder);
	wxBoxSizer * hSizer = new wxBoxSizer(wxVERTICAL);
	hSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Height: ")), 1, wxEXPAND | wxALL);
	defInputSizer->Add(hSizer, 1, wxEXPAND | wxALL, panelBorder);
	heightCtrl = new wxTextCtrl(this, wxID_ANY, wxString() << GetSize().GetHeight());
	defInputSizer->Add(heightCtrl, 1, wxEXPAND | wxALL, panelBorder);

	additiveCb = new wxCheckBox(this, wxID_ANY, wxT("Additive"));
	defInputSizer->Add(additiveCb, 1, wxEXPAND | wxALL, panelBorder);
	clearCb = new wxCheckBox(this, wxID_ANY, wxT("Clear"));
	defInputSizer->Add(clearCb, 1, wxEXPAND | wxALL, panelBorder);
	showCoords = new wxCheckBox(this, wxID_ANY, wxT("Axis"));
	defInputSizer->Add(showCoords, 1, wxEXPAND | wxALL, panelBorder);

	wxBoxSizer * chSizer = new wxBoxSizer(wxVERTICAL);
	chSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Color:")));
	defInputSizer->Add(chSizer, 1, wxEXPAND | wxALL, panelBorder);
	colorCtrl = new wxTextCtrl(this, wxID_ANY, "ffffff");
	defInputSizer->Add(colorCtrl, 1, wxEXPAND | wxALL);

	customInputSizer = new wxBoxSizer(wxHORIZONTAL);
	inputSizer->Add(defInputSizer, 1, wxALL, panelBorder);
	inputSizer->Add(customInputSizer, 1, wxALL, panelBorder);

	mPanelSizer->Add(inputSizer, 0, wxALL, panelBorder);

	wxBoxSizer * canvasSizer = new wxBoxSizer(wxHORIZONTAL);
	outputCanvas = new BitmapCanvas(this, vf);
	canvasSizer->Add(outputCanvas, 1, wxSHRINK | wxEXPAND | wxALL, panelBorder);
	mPanelSizer->Add(canvasSizer, 1, wxEXPAND, wxALL);

	gkernel->addOutputManager(outputCanvas);
	gkernel->addParamManager(this);

	SendSizeEvent();
}

GeometricOutput::~GeometricOutput() {
	delete gkernel;
}

void GeometricOutput::onCommandMenu(wxCommandEvent & ev) {
	switch (ev.GetId())
	{
	case(ViewFrame::MID_VF_CNT_RUN) : {
		const int width = wxAtoi(widthCtrl->GetValue());
		const int height = wxAtoi(heightCtrl->GetValue());
		const std::string colStr = colorCtrl->GetValue();
		std::stringstream ss;
		ss << std::hex << colStr;
		unsigned col = 0;
		ss >> col;
		gkernel->setColor(Color(col));
		unsigned flags = GeometricPrimitive::DF_OVER;
		flags |= (additiveCb->GetValue() ? GeometricPrimitive::DF_ACCUMULATE : 0);
		flags |= (clearCb->GetValue() ? GeometricPrimitive::DF_CLEAR : 0);
		flags |= (showCoords->GetValue() ? GeometricPrimitive::DF_SHOW_AXIS : 0);
		gkernel->setSize(width, height);
		gkernel->runKernel(flags);
		break;
	}
	default:
		break;
	}
}

void GeometricOutput::addParam(const std::string & label, wxTextCtrl * wnd) {
	wxBoxSizer * pSizer = new wxBoxSizer(wxVERTICAL);
	pSizer->Add(new wxStaticText(this, wxID_ANY, wxString(label)), 1, wxEXPAND | wxALL);
	customInputSizer->Add(pSizer, 1, wxEXPAND | wxALL, panelBorder);
	customParams[label] = wnd;
	customInputSizer->Add(wnd, 1, wxEXPAND | wxALL, panelBorder);
	SendSizeEvent();
}

std::string GeometricOutput::getParam(const std::string & paramName) const {
	std::string retval;
	const auto& pv = customParams.find(paramName);
	if (pv != customParams.end()) {
		retval = std::string(pv->second->GetValue());
	}
	return retval;
}

NegativePanel::NegativePanel(ViewFrame * viewFrame)
	: InputOutputMode(viewFrame, new NegativeKernel)
{}

TextSegmentationPanel::TextSegmentationPanel(ViewFrame * vf)
	: InputOutputMode(vf, new TextSegmentationKernel)
{
	wxBoxSizer * inputSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer * statSizer = new wxBoxSizer(wxVERTICAL);
	statSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Threshold: ")), 1, wxEXPAND | wxALL);
	inputSizer->Add(statSizer, 1, wxEXPAND | wxALL, panelBorder);
	threshold = new wxTextCtrl(this, wxID_ANY, "25");
	//wxIntegerValidator<unsigned char> validator(); // disabling for now because we would need proper overloads and a lot of code
	//threshold->SetValidator(validator);
	inputSizer->Add(threshold, 1, wxBOTTOM, panelBorder);
	mPanelSizer->Prepend(inputSizer, 0, wxALL, panelBorder);
	// add this class as a param handler for the kernel
	kernel->addParamManager(this);

	SendSizeEvent();
}

std::string TextSegmentationPanel::getParam(const std::string & paramName) const {
	if (paramName == "threshold") {
		return std::string(threshold->GetValue());
	}
	return std::string();
}

SinosoidPanel::SinosoidPanel(ViewFrame * vf)
	: GeometricOutput(vf, new SinosoidKernel)
{
	gkernel->addParamManager(this);
	// TODO make the list of paramters callable from the kernel, so the gui may initialize what it need for the user input
	addParam("k", new wxTextCtrl(this, wxID_ANY, wxT("50")));
	addParam("q", new wxTextCtrl(this, wxID_ANY, wxT("1")));
	addParam("offset", new wxTextCtrl(this, wxID_ANY, wxT("0.0")));
	addParam("function", new wxTextCtrl(this, wxID_ANY, wxT("cos")));
}

HoughRoTheta::HoughRoTheta(ViewFrame * vf)
	: InputOutputMode(vf, new HoughKernel)
{}
