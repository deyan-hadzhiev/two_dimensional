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

const wxString InputOutputMode::ioFileSelector = wxT("png or jpeg images (*.png;*.jpeg;*.jpg)|*.png;*.jpeg;*.jpg");
const int InputOutputMode::panelBorder = 4;

InputOutputMode::InputOutputMode(ViewFrame * viewFrame)
	: ModePanel(viewFrame, ViewFrame::VFS_OPEN_SAVE | ViewFrame::VFS_CNT_COMPARE)
	, inputCanvas(nullptr)
	, outputCanvas(nullptr)
	, compareCanvas(nullptr)
{
	wxBoxSizer * canvasSizer = new wxBoxSizer(wxHORIZONTAL);
	inputCanvas = new BitmapCanvas(this);
	//inputCanvas = new wxPanel(this);
	canvasSizer->Add(inputCanvas, 1, wxEXPAND | wxALL, panelBorder);
	inputCanvas->SetBackgroundColour(*wxRED);
	inputCanvas->SetForegroundColour(*wxGREEN);
	//canvasSizer->Add(new wxStaticLine, 0);
	outputCanvas = new BitmapCanvas(this);
	//outputCanvas = new wxPanel(this);
	canvasSizer->Add(outputCanvas, 1, wxEXPAND | wxALL, panelBorder);
	outputCanvas->SetBackgroundColour(*wxBLUE);
	outputCanvas->SetForegroundColour(*wxGREEN);
	compareCanvas = new wxPanel(this);
	canvasSizer->Add(compareCanvas, 1, wxEXPAND | wxALL, panelBorder);
	compareCanvas->SetBackgroundColour(*wxGREEN);
	compareCanvas->Show(false);
	mPanelSizer->Add(canvasSizer, 1, wxEXPAND | wxALL);
	SendSizeEvent();
}

InputOutputMode::~InputOutputMode()
{
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
	: InputOutputMode(viewFrame)
{}
