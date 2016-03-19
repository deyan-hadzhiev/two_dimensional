#ifndef __GUIMAIN_H__
#define __GUIMAIN_H__

#include <wx/wx.h>

class ModePanel;

class ViewFrame : public wxFrame {
public:
	ViewFrame(const wxString& title);

	void OnQuit(wxCommandEvent&);

	void OnMenuModeSelect(wxCommandEvent&);

	void OnCustomMenuSelect(wxCommandEvent&);

	enum ViewFrameStyles {
		VFS_NOTHING_ENABLED = 0,
		VFS_FILE_OPEN       = 1 << 0, // enables open file in the menu
		VFS_FILE_SAVE       = 1 << 1, // enables save file in the menu
		VFS_OPEN_SAVE       = VFS_FILE_OPEN | VFS_FILE_SAVE,
		VFS_CNT_COMPARE     = 1 << 2, // enables the comapre item
	};
	// sets various styles to the view frame like open/save file options enabled, etc
	void setCustomStyle(unsigned style);

	void addCustomMenu(wxMenu * menu);

	enum MenuItems {
		MID_VF_FILE_OPEN = wxID_HIGHEST + 1,
		MID_VF_FILE_SAVE,
		MID_VF_CNT_COMPARE,

		MID_VF_MODES_START,
		MID_VF_NEGATIVE,
		MID_VF_MODES_END,
	};
private:

	static const wxString modeNames[MID_VF_MODES_END - MID_VF_MODES_START - 1];

	ModePanel * mPanel;
	wxMenuBar * menuBar;
	wxMenu * file;
	wxMenuItem * fileOpen;
	wxMenuItem * fileSave;
	wxMenuItem * controlCmp;
	wxStatusBar * statusBar;
};

#endif // __GUIMAIN_H__
