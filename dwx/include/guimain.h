#ifndef __GUIMAIN_H__
#define __GUIMAIN_H__

#include <wx/wx.h>

#include "module_manager.h"

class ModePanel;

class WinIDProvider {
	wxWindowID id;
public:
	WinIDProvider(wxWindowID initial);

	static WinIDProvider& getProvider();

	wxWindowID getId(unsigned count = 1);
};

struct ModuleEnabled {
	ModuleTypeId mti;
	bool single;
	ModuleEnabled(ModuleTypeId _mti = M_VOID, bool _single = false)
		: mti(_mti)
		, single(_single)
	{}
};

class ViewFrame : public wxFrame {
public:
	ViewFrame(const wxString& title);

	void OnQuit(wxCommandEvent&);

	void OnMenuModeSelect(wxCommandEvent&);

	void OnCustomMenuSelect(wxCommandEvent&);

	void OnTimer(wxTimerEvent&);

	void OnIdle(wxIdleEvent&);

	enum ViewFrameStyles {
		VFS_NOTHING_ENABLED = 0,
		VFS_FILE_OPEN       = 1 << 0, // enables open file in the menu
		VFS_FILE_SAVE       = 1 << 1, // enables save file in the menu
		VFS_OPEN_SAVE       = VFS_FILE_OPEN | VFS_FILE_SAVE,
		VFS_CNT_RUN         = 1 << 2, // enables the run control item
		VFS_CNT_STOP        = 1 << 3, // enables the stop control item (only for asynchronous kernels)
		VFS_CNT_COMPARE     = 1 << 4, // enables the compare item
		VFS_CNT_HISTOGRAM   = 1 << 5, // enables the histogram item
		VFS_CNT_LAST,       //!< boundry for style loop
		VFS_ALL_ENABLED     = ((VFS_CNT_LAST - 1) << 1) - 1,
	};
	// sets various styles to the view frame like open/save file options enabled, etc
	void setCustomStyle(unsigned style);
	// gets the current styles of the view frame
	unsigned getCustomStyle() const noexcept;

	void enableModule(ModuleTypeId moduleTypeId, bool enable);

	enum MenuItems {
		MID_VF_FILE_OPEN = wxID_HIGHEST + 1,
		MID_VF_FILE_SAVE,

		MID_VF_CNT_RANGE_START, //!< Add controls in this range
		MID_VF_CNT_RUN,
		MID_VF_CNT_STOP,
		MID_VF_CNT_COMPARE,
		MID_VF_CNT_HISTOGRAM,
		MID_VF_CNT_RANGE_END,

		MID_VF_MULTI_MODULE_SWITCH, //!< switches between mutli and single module
		MID_VF_MODES_RANGE_START,
		// ... this ids will be used for creating modules for single input/output
		MID_VF_MODES_RANGE_END = MID_VF_MODES_RANGE_START + ModuleTypeId::M_COUNT + 1,

		MID_VF_TIMER,

		MID_VF_LAST_ID, //!< The last id used for starting id for custom event handlers (like the ParamPanel)
	};

	void popupModes();

private:

	static const wxString controlNames[MID_VF_CNT_RANGE_END - MID_VF_CNT_RANGE_START - 1]; // names of all the controls
	static const wxItemKind controlKinds[MID_VF_CNT_RANGE_END - MID_VF_CNT_RANGE_START - 1]; // kinds of controls
	wxMenuItem * controls[MID_VF_CNT_RANGE_END - MID_VF_CNT_RANGE_START - 1]; // an array with all the controls

	static const ModuleEnabled enabledModules[];

	// some layout constants
	static const wxSize vfMinSize;
	static const wxSize vfInitSize;

	ModePanel * mPanel;
	wxMenuBar * menuBar;
	wxMenu * file;
	wxMenuItem * fileOpen;
	wxMenuItem * fileSave;
	wxMenu * modes;
	wxTimer refreshTimer;

	ModuleFactory moduleFactory;
	bool multiModuleMode;
	unsigned customStyle;
};

#endif // __GUIMAIN_H__
