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
	wxT("Text Segmentation"),
	wxT("Sinosoid curve"),
	wxT("Hough Rho Theta"),
	wxT("Image rotation"),
	wxT("Histograms"),
	wxT("Threshold"),
	wxT("Filter"),
	wxT("Downscale"),
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
	SetDoubleBuffered(true);

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
	switch (ev.GetId())
	{
	case(MID_VF_NEGATIVE) :
		SetStatusText(wxT("Using negative mode"));
		mPanel = new NegativePanel(this);
		break;
	case(MID_VF_TEXT_SEGMENTATION) :
		SetStatusText(wxT("Using text segmentation mode"));
		mPanel = new TextSegmentationPanel(this);
		break;
	case(MID_VF_SINOSOID) :
		SetStatusText(wxT("Using sinosoid geometric kernel"));
		mPanel = new SinosoidPanel(this);
		break;
	case(MID_VF_HOUGH_RO_THETA) :
		SetStatusText(wxT("Using Ro Theta Hough kernel"));
		mPanel = new HoughRoTheta(this);
		break;
	case(MID_VF_ROTATION) :
		SetStatusText(wxT("Using Image rotation"));
		mPanel = new RotationPanel(this);
		break;
	case(MID_VF_HISTOGRAMS) :
		SetStatusText(wxT("Using Histograms"));
		mPanel = new HistogramModePanel(this);
		break;
	case(MID_VF_THRESHOLD) :
		SetStatusText(wxT("Using Threshold"));
		mPanel = new ThresholdModePanel(this);
		break;
	case(MID_VF_FILTER) :
		SetStatusText(wxT("Using Filter"));
		mPanel = new FilterModePanel(this);
		break;
	case(MID_VF_DOWNSCALE) :
		SetStatusText(wxT("Using Downscale"));
		mPanel = new DownScalePanel(this);
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
