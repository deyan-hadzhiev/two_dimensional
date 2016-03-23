#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/dialog.h>
#include <wx/file.h>
#include <wx/image.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>

#include "guimain.h"
#include "wx_modes.h"
#include "wx_bitmap_canvas.h"
#include "kernels.h"

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

const wxString InputOutputMode::ioFileSelector = wxT("png or jpeg images (*.png;*.jpeg;*.jpg;*.bmp)|*.png;*.jpeg;*.jpg;*.bmp");
const int InputOutputMode::panelBorder = 4;

InputOutputMode::InputOutputMode(ViewFrame * viewFrame, SimpleKernel * kernel)
	: ModePanel(viewFrame, ViewFrame::VFS_ALL_ENABLED)
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

NegativePanel::NegativePanel(ViewFrame * viewFrame)
	: InputOutputMode(viewFrame, new NegativeKernel)
{}
