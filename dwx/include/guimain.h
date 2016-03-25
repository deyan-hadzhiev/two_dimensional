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
		VFS_CNT_RUN         = 1 << 2, // enables the run control item
		VFS_CNT_COMPARE     = 1 << 3, // enables the compare item
		VFS_CNT_LAST,       //!< boundry for style loop
		VFS_ALL_ENABLED     = ((VFS_CNT_LAST - 1) << 1) - 1,
	};
	// sets various styles to the view frame like open/save file options enabled, etc
	void setCustomStyle(unsigned style);

	enum MenuItems {
		MID_VF_FILE_OPEN = wxID_HIGHEST + 1,
		MID_VF_FILE_SAVE,

		MID_VF_CNT_RANGE_START, //!< Add controls in this range
		MID_VF_CNT_RUN,
		MID_VF_CNT_COMPARE,
		MID_VF_CNT_RANGE_END,

		MID_VF_MODES_RANGE_START,
		MID_VF_NEGATIVE,
		MID_VF_TEXT_SEGMENTATION,
		MID_VF_MODES_RANGE_END,
	};
private:

	static const wxString controlNames[MID_VF_CNT_RANGE_END - MID_VF_CNT_RANGE_START - 1]; // names of all the controls
	static const wxItemKind controlKinds[MID_VF_CNT_RANGE_END - MID_VF_CNT_RANGE_START - 1]; // kinds of controls
	wxMenuItem * controls[MID_VF_CNT_RANGE_END - MID_VF_CNT_RANGE_START - 1]; // an array with all the controls

	static const wxString modeNames[MID_VF_MODES_RANGE_END - MID_VF_MODES_RANGE_START - 1]; // name of the modes supported

	// some layout constants
	static const wxSize vfMinSize;
	static const wxSize vfInitSize;

	ModePanel * mPanel;
	wxMenuBar * menuBar;
	wxMenu * file;
	wxMenuItem * fileOpen;
	wxMenuItem * fileSave;
	wxStatusBar * statusBar;
};

#endif // __GUIMAIN_H__
