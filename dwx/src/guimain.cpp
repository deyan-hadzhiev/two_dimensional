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
#include "module_manager.h"

#include "color.h"
#include "bitmap.h"

class ViewApp : public wxApp {
public:
	virtual bool OnInit() override;
};

const wxString ViewFrame::controlNames[] = {
	wxT("&Run\tF5"),
	wxT("&Stop\tShift+F5"),
	wxT("&Compare\tCtrl+C"),
	wxT("&Histogram\tF6"),
};

const wxItemKind ViewFrame::controlKinds[] = {
	wxITEM_NORMAL, //!< Run
	wxITEM_NORMAL, //!< Stop
	wxITEM_CHECK,  //!< Compare
	wxITEM_CHECK,  //!< Histogram
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
	, refreshTimer(this, MID_VF_TIMER)
{
	SetMinClientSize(vfMinSize);

	Connect(wxEVT_IDLE, wxIdleEventHandler(ViewFrame::OnIdle), NULL, this);
	Connect(wxEVT_TIMER, MID_VF_TIMER, wxTimerEventHandler(ViewFrame::OnTimer), NULL, this);
	refreshTimer.Start(200);

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
		modes->Append(modei, MODULE_DESC[modei - MID_VF_MODES_RANGE_START - 1].fullName);
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
	CreateStatusBar(4);
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
	const int modeId = ev.GetId() - MID_VF_MODES_RANGE_START - 1;
	if (modeId > ModuleId::M_VOID && modeId < ModuleId::M_COUNT) {
		const ModuleDescription& md = MODULE_DESC[modeId];
		SetStatusText(wxString(wxT("Using ")) + md.fullName + wxString(wxT(" module")));
		// TODO maybe don't clear if in mutli module mode
		moduleFactory.clear();
		ModuleBase * module = moduleFactory.getModule(md.id);
		if (module) {
			if (md.inputs == 0) {
				mPanel = new GeometricOutput(this, module);
			} else if (md.inputs == 1) {
				mPanel = new InputOutputMode(this, module);
			}
		} else {
			SetStatusText(wxT("[ERROR] Could not create module!"));
		}
	} else {
		SetStatusText(wxT("[ERROR] Unknown on unavailable module!"));
	}
	Update();
	Refresh();
}

void ViewFrame::OnCustomMenuSelect(wxCommandEvent & ev) {
	if (mPanel) {
		mPanel->onCommandMenu(ev);
	}
}

void ViewFrame::OnTimer(wxTimerEvent &) {
	if (mPanel) {
		const wxString& statusText = mPanel->getCbString();
		if (!statusText.empty()) {
			SetStatusText(statusText);
		}
	}
}

void ViewFrame::OnIdle(wxIdleEvent &) {
	if (mPanel) {
		const wxString& statusText = mPanel->getCbString();
		if (!statusText.empty()) {
			SetStatusText(statusText);
		}
	}
}

WinIDProvider globalIdProvider(ViewFrame::MID_VF_LAST_ID);

WinIDProvider::WinIDProvider(wxWindowID initial)
	: id(initial)
{}

WinIDProvider & WinIDProvider::getProvider() {
	return globalIdProvider;
}

wxWindowID WinIDProvider::getId(unsigned count) {
	const wxWindowID retval = id;
	id += count;
	return retval;
}
