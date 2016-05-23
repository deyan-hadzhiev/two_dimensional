#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/dialog.h>
#include <wx/file.h>
#include <wx/image.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>
#include <wx/valnum.h>
#include <wx/filename.h>

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
	, paramPanel(nullptr)
{
	viewFrame->setCustomStyle(styles);
	mPanelSizer = new wxBoxSizer(wxVERTICAL);
	SetSizerAndFit(mPanelSizer);
	SetSize(viewFrame->GetClientSize());

	paramPanel = new ParamPanel(this);
	mPanelSizer->Add(paramPanel, 0, wxEXPAND | wxALL, panelBorder);
	SendSizeEvent();
}

ModePanel::~ModePanel() {}

wxString ModePanel::getCbString() const {
	wxString statusText;
	const std::string& kernelName = cb.getKernelName();
	const float percentDone = cb.getPercentDone();
	if (!kernelName.empty() && percentDone > 0.05f) {
		statusText.Printf(wxT("%s : %6.2f %%"), kernelName.c_str(), percentDone);
	}
	return statusText;
}

const int ModePanel::panelBorder = 4;

const wxString InputOutputMode::ioFileSelector = wxT("png or jpeg images (*.png;*.jpeg;*.jpg;*.bmp)|*.png;*.jpeg;*.jpg;*.bmp");

InputOutputMode::InputOutputMode(ViewFrame * viewFrame, SimpleKernel * kernel)
	: ModePanel(viewFrame, ViewFrame::VFS_ALL_ENABLED & ~ViewFrame::VFS_CNT_COMPARE) // disable compare for now - it is not done and will not be soon
	, inputPanel(nullptr)
	, outputPanel(nullptr)
	, compareCanvas(nullptr)
	, kernel(kernel)
{
	wxBoxSizer * canvasSizer = new wxBoxSizer(wxHORIZONTAL);
	//inputCanvas = new BitmapCanvas(this, viewFrame);
	inputPanel = new ImagePanel(this, viewFrame);
	//inputCanvas = new wxPanel(this);
	canvasSizer->Add(inputPanel, 1, wxSHRINK | wxEXPAND | wxALL, panelBorder);
	//outputCanvas = new BitmapCanvas(this, viewFrame);
	outputPanel = new ImagePanel(this, viewFrame);
	//outputCanvas = new wxPanel(this);
	canvasSizer->Add(outputPanel, 1, wxSHRINK | wxEXPAND | wxALL, panelBorder);
	BitmapCanvas * inputCanvas = inputPanel->getCanvas();
	BitmapCanvas * outputCanvas = outputPanel->getCanvas();
	// add synchornizers
	inputCanvas->addSynchronizer(outputCanvas);
	outputCanvas->addSynchronizer(inputCanvas);
	// set the background colours
	wxColour bc(75, 75, 75);
	inputCanvas->SetBackgroundColour(bc);
	outputCanvas->SetBackgroundColour(bc);

	kernel->addInputManager(inputPanel);
	kernel->addOutputManager(outputPanel);

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
					inputPanel->setImage(inputImage);
					viewFrame->SetStatusText(wxString(wxT("Loaded input image: ") + fdlg.GetPath()));
					inputPanel->synchronize();
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
	{
		const wxStandardPaths& stdPaths = wxStandardPaths::Get();
		wxFileDialog fdlg(this, wxT("Save output image file"), stdPaths.GetUserDir(wxStandardPaths::Dir_Pictures), wxT(""), ioFileSelector, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fdlg.ShowModal() != wxID_CANCEL) {
			const wxImage outImage = outputPanel->getImage();
			if (outImage.IsOk()) {
				wxFileName outFn(fdlg.GetPath());
				const wxString outExt = outFn.GetExt();
				wxBitmapType outType = wxBitmapType::wxBITMAP_TYPE_BMP;
				if (0 == outExt.CmpNoCase(wxT("png"))) {
					outType = wxBitmapType::wxBITMAP_TYPE_PNG;
				} else if (0 == outExt.CmpNoCase(wxT("jpg")) || 0 == outExt.CmpNoCase(wxT("jpeg"))) {
					outType = wxBitmapType::wxBITMAP_TYPE_JPEG;
				}
				if (outImage.SaveFile(outFn.GetFullPath(), outType)) {
					viewFrame->SetStatusText(wxString(wxT("Saved output image: ")) + outFn.GetFullPath());
				} else {
					viewFrame->SetStatusText(wxString(wxT("Could NOT save output image: ")) + outFn.GetFullPath());
				}
			} else {
				viewFrame->SetStatusText(wxString(wxT("The output image is corrupted!")));
			}
		}
		break;
	}
	case (ViewFrame::MID_VF_CNT_RUN) :
		cb.reset();
		kernel->runKernel(0);
		break;
	case (ViewFrame::MID_VF_CNT_STOP) :
		cb.setAbortFlag();
		break;
	case (ViewFrame::MID_VF_CNT_COMPARE) :
		inputPanel->Show(!inputPanel->IsShown());
		outputPanel->Show(!outputPanel->IsShown());
		compareCanvas->Show(!compareCanvas->IsShown()); // TODO: Fix/change
		SendSizeEvent();
		break;
	case (ViewFrame::MID_VF_CNT_HISTOGRAM) :
		inputPanel->toggleHist();
		outputPanel->toggleHist();
		SendSizeEvent();
		break;
	default:
		break;
	}
}

GeometricOutput::GeometricOutput(ViewFrame * vf, GeometricKernel * gk)
	: ModePanel(vf, ViewFrame::VFS_CNT_RUN)
	, gkernel(gk)
	, outputPanel(nullptr)
{
	wxBoxSizer * canvasSizer = new wxBoxSizer(wxHORIZONTAL);
	//outputCanvas = new BitmapCanvas(this, vf);
	outputPanel = new ImagePanel(this, vf);
	canvasSizer->Add(outputPanel, 1, wxSHRINK | wxEXPAND | wxALL, panelBorder);
	mPanelSizer->Add(canvasSizer, 1, wxEXPAND, wxALL);

	gkernel->addOutputManager(outputPanel);
	//gkernel->addParamManager(paramPanel);

	SendSizeEvent();
}

GeometricOutput::~GeometricOutput() {
	delete gkernel;
}

void GeometricOutput::onCommandMenu(wxCommandEvent & ev) {
	switch (ev.GetId())
	{
	case(ViewFrame::MID_VF_CNT_RUN) : {
		gkernel->runKernel(0);
		break;
	}
	default:
		break;
	}
}

NegativePanel::NegativePanel(ViewFrame * viewFrame)
	: InputOutputMode(viewFrame, new NegativeKernel)
{
	kernel->setProgressCallback(&cb);
}

TextSegmentationPanel::TextSegmentationPanel(ViewFrame * vf)
	: InputOutputMode(vf, new TextSegmentationKernel)
{
	kernel->setProgressCallback(&cb);
	kernel->addParamManager(paramPanel);

	SendSizeEvent();
}

SinosoidPanel::SinosoidPanel(ViewFrame * vf)
	: GeometricOutput(vf, new SinosoidKernel)
{
	gkernel->setProgressCallback(&cb);
	gkernel->addParamManager(paramPanel);
}

HoughRoTheta::HoughRoTheta(ViewFrame * vf)
	: InputOutputMode(vf, new HoughKernel)
{
	kernel->setProgressCallback(&cb);
}

RotationPanel::RotationPanel(ViewFrame * vf)
	: InputOutputMode(vf, new RotationKernel)
{
	kernel->setProgressCallback(&cb);
	kernel->addParamManager(paramPanel);

	SendSizeEvent();
}

HistogramModePanel::HistogramModePanel(ViewFrame * vf)
	: InputOutputMode(vf, new HistogramKernel)
{
	kernel->setProgressCallback(&cb);
	kernel->addParamManager(paramPanel);
	SendSizeEvent();
}

ThresholdModePanel::ThresholdModePanel(ViewFrame * vf)
	: InputOutputMode(vf, new ThresholdKernel)
{
	kernel->setProgressCallback(&cb);
	kernel->addParamManager(paramPanel);
	SendSizeEvent();
}

FilterModePanel::FilterModePanel(ViewFrame * vf)
	: InputOutputMode(vf, new FilterKernel)
{
	kernel->setProgressCallback(&cb);
	kernel->addParamManager(paramPanel);
	SendSizeEvent();
}

DownScalePanel::DownScalePanel(ViewFrame * vf)
	: InputOutputMode(vf, new DownScaleKernel)
{
	kernel->setProgressCallback(&cb);
	kernel->addParamManager(paramPanel);
	SendSizeEvent();
}
