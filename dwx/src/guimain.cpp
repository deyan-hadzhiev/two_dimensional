#include <wx/wx.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/window.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcbuffer.h>

#include "guimain.h"
#include "wx_modes.h"
#include "wx_bitmap_canvas.h"

#include "color.h"
#include "bitmap.h"

class ViewApp : public wxApp {
public:
	virtual bool OnInit() override;
};


const wxString ViewFrame::modeNames[] = {
	wxT("Negative"),
};

const wxString ViewFrame::controlNames[] = {
	wxT("&Run\tF5"),
	wxT("&Compare\tCtrl+C"),
};

const wxItemKind ViewFrame::controlKinds[] = {
	wxITEM_NORMAL, //!< Run
	wxITEM_CHECK,  //!< Compare
};

const wxSize ViewFrame::vfMinSize = wxSize(320, 320);
const wxSize ViewFrame::vfInitSize = wxSize(1024, 768);

wxIMPLEMENT_APP(ViewApp);

bool ViewApp::OnInit() {

	if (!wxApp::OnInit())
		return false;

	// must remain here!!!
	initColor();
	// initialize some image handlers
	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxJPEGHandler);

	ViewFrame * frame = new ViewFrame(wxT("2d graphics"));
	SetTopWindow(frame);
	frame->Show();
	frame->Center();

	return true;
}

ViewFrame::ViewFrame(const wxString& title)
	: wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, vfInitSize)
	, controls{nullptr}
	, mPanel(nullptr)
	, menuBar(nullptr)
	, file(nullptr)
	, fileOpen(nullptr)
	, fileSave(nullptr)
	, statusBar(nullptr)
{
	SetMinClientSize(vfMinSize);
	SetDoubleBuffered(true);

	menuBar = new wxMenuBar;

	file = new wxMenu;
	fileOpen = new wxMenuItem(file, MID_VF_FILE_OPEN, wxT("&Open\tCtrl+O"));
	file->Append(fileOpen);
	Connect(MID_VF_FILE_OPEN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ViewFrame::OnCustomMenuSelect));
	fileSave = new wxMenuItem(file, MID_VF_FILE_SAVE, wxT("&Save\tCtrl+S"));
	file->Append(fileSave);
	Connect(MID_VF_FILE_SAVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ViewFrame::OnCustomMenuSelect));
	file->AppendSeparator();
	file->Append(wxID_EXIT, wxT("&Quit"));
	menuBar->Append(file, wxT("&File"));
	Connect(wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ViewFrame::OnQuit));

	wxMenu * modes = new wxMenu;
	for (unsigned modei = MID_VF_MODES_RANGE_START + 1; modei < MID_VF_MODES_RANGE_END; ++modei) {
		modes->Append(modei, modeNames[modei - MID_VF_MODES_RANGE_START - 1]);
		Connect(modei, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ViewFrame::OnMenuModeSelect));
	}
	menuBar->Append(modes, wxT("&Modes"));
	wxMenu * control = new wxMenu;
	for (int i = 0; i < _countof(controls); ++i) {
		controls[i] = new wxMenuItem(control, i + MID_VF_CNT_RANGE_START + 1, controlNames[i], wxEmptyString, controlKinds[i]);
		control->Append(controls[i]);
		Connect(i + MID_VF_CNT_RANGE_START + 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ViewFrame::OnCustomMenuSelect));
	}
	menuBar->Append(control, wxT("Control"));

	SetMenuBar(menuBar);
	CreateStatusBar(3);
	SetStatusText(wxT("Select a Mode to begin"));

	setCustomStyle(VFS_NOTHING_ENABLED);

	Update();
	Refresh();
}

void ViewFrame::setCustomStyle(unsigned style) {
	fileOpen->Enable((style & VFS_FILE_OPEN) != 0);
	fileSave->Enable((style & VFS_FILE_SAVE) != 0);

	for (unsigned i = 0; (VFS_CNT_RUN << i) < VFS_CNT_LAST && i < _countof(controls); ++i) {
		controls[i]->Enable((style & (VFS_CNT_RUN << i)) != 0);
	}
}

void ViewFrame::OnQuit(wxCommandEvent &) {
	Close();
}

void ViewFrame::OnMenuModeSelect(wxCommandEvent & ev) {
	if (mPanel) {
		mPanel->Destroy();
		mPanel = nullptr;
		setCustomStyle(VFS_NOTHING_ENABLED);
	}
	switch (ev.GetId())
	{
	case(MID_VF_NEGATIVE) :
		SetStatusText(wxT("Using negative mode"));
		mPanel = new NegativePanel(this);
		break;
	default:
		SetStatusText(wxT("[ERROR] Unknown on unavailable model!"));
		break;
	}
	Update();
	Refresh();
}

void ViewFrame::OnCustomMenuSelect(wxCommandEvent & ev) {
	if (mPanel) {
		mPanel->onCommandMenu(ev);
	}
}
