#include <wx/wx.h>
#include <wx/statline.h>

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

InputOutputMode::InputOutputMode(ViewFrame * viewFrame)
	: ModePanel(viewFrame, ViewFrame::VFS_OPEN_SAVE | ViewFrame::VFS_CNT_COMPARE)
	, inputCanvas(nullptr)
	, outputCanvas(nullptr)
	, compareCanvas(nullptr)
{
	wxBoxSizer * canvasSizer = new wxBoxSizer(wxHORIZONTAL);
	wxSize sz = GetSize();
	wxString dbg;
	dbg << wxT("Size w: ") << sz.GetWidth() << wxT(", h:") << sz.GetHeight();
	viewFrame->SetStatusText(dbg);
	//inputCanvas = new BitmapCanvas(this);
	inputCanvas = new wxPanel(this);
	canvasSizer->Add(inputCanvas, 1, wxEXPAND | wxALL, 5);
	inputCanvas->SetBackgroundColour(*wxRED);
	inputCanvas->SetForegroundColour(*wxGREEN);
	//canvasSizer->Add(new wxStaticLine, 0);
	//outputCanvas = new BitmapCanvas(this);
	outputCanvas = new wxPanel(this);
	canvasSizer->Add(outputCanvas, 1, wxEXPAND | wxALL, 5);
	outputCanvas->SetBackgroundColour(*wxBLUE);
	outputCanvas->SetForegroundColour(*wxGREEN);
	compareCanvas = new wxPanel(this);
	canvasSizer->Add(compareCanvas, 1, wxEXPAND | wxALL, 5);
	compareCanvas->SetBackgroundColour(*wxGREEN);
	compareCanvas->Show(false);
	mPanelSizer->Add(canvasSizer, 1, wxEXPAND | wxALL, 1);
	SendSizeEvent();
}

InputOutputMode::~InputOutputMode()
{
}

void InputOutputMode::onCommandMenu(wxCommandEvent & ev) {
	switch (ev.GetId()) {
	case (ViewFrame::MID_VF_FILE_OPEN) :
		// TODO:
		break;
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
