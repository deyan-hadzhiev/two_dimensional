#ifndef __WX_MODES_H__
#define __WX_MODES_H__

#include <memory>
#include <mutex>
#include <unordered_map>

#include <wx/wx.h>

#include "progress.h"
#include "guimain.h"
#include "wx_bitmap_canvas.h"
#include "wx_multi_module.h"
#include "wx_param_panel.h"
#include "module_base.h"
#include "geom_primitive.h"
#include "module_manager.h"
#include "module_dag.h"

class ViewFrame;

class ModePanel : public wxPanel {
public:
	friend class ParamPanel;

	ModePanel(ViewFrame * viewFrame, bool multiParams, unsigned styles = ViewFrame::VFS_NOTHING_ENABLED);
	virtual ~ModePanel();

	virtual void onCommandMenu(wxCommandEvent& ev) {}

	virtual wxImage getLastInput() const {
		return wxImage();
	}

	virtual void setInput(const wxImage& input) {}

	virtual void addModule(const ModuleDescription& md) {}

	wxString getCbString() const;

	static const int panelBorder; //!< the default border of panels
protected:
	ViewFrame * viewFrame;
	wxBoxSizer * mPanelSizer;
	ProgressCallback cb;
	ParamPanel * paramPanel;
};

class InputOutputMode : public ModePanel {
public:
	InputOutputMode(ViewFrame * viewFrame, ModuleBase * _module);

	virtual void onCommandMenu(wxCommandEvent& ev) override;

	virtual wxImage getLastInput() const override final {
		return lastInput;
	}

	virtual void setInput(const wxImage& input) override final;
protected:
	ImagePanel * inputPanel;
	ImagePanel * outputPanel;
	wxPanel * compareCanvas;
	ModuleBase * module;
	wxImage lastInput;

	static const wxString ioFileSelector; //!< file selection string - change if a new file type is added
};

class GeometricOutput : public ModePanel {
public:
	GeometricOutput(ViewFrame * vf, ModuleBase * _module);

	virtual void onCommandMenu(wxCommandEvent& ev) override;

protected:
	ModuleBase * module;
	ImagePanel * outputPanel;
};

class FileOpenHandler : public InputManager {
public:
	FileOpenHandler(ModuleBase * _moduleHandle)
		: moduleHandle(_moduleHandle)
	{}

	// from InputManager
	bool getInput(Bitmap& bmp, int idx) const override final;

	// own function
	void setInput(const wxImage& img);
private:
	mutable std::mutex bmpMutex;
	Bitmap inputBmp; //!< the input bitmap
	ModuleBase * moduleHandle; // for updates on image loads
};

class FileSaveHandler : public OutputManager {
public:
	// from OutputManager
	void setOutput(const Bitmap& bmp, ModuleId mid) override final;

	// own function
	void getOutput(wxImage& img) const;
private:
	mutable std::mutex bmpMutex;
	Bitmap outputBmp;
};

// holds and operates with  the module, its parameters and graphic properties
class ModuleNodeCollection {
public:
	ModuleDescription moduleDesc;
	ModuleBase * moduleHandle;
	ParamPanel * moduleParamPanel;

	ModuleNodeCollection() = default;
	ModuleNodeCollection(const ModuleDescription& _moduleDesc, ModuleBase * _moduleHandle, ParamPanel * _moduleParamPanel);
};

class MultiModuleMode : public ModePanel {
public:
	MultiModuleMode(ViewFrame * vf, ModuleFactory * mf);
	virtual ~MultiModuleMode();

	// derived from ModePanel
	virtual void onCommandMenu(wxCommandEvent& ev) override;

	virtual void addModule(const ModuleDescription& md) override;

	// own functions
	// used to bring up the param panel for the module
	void updateSelection(ModuleId id);

	// removes a module
	void removeModule(ModuleId id);

	// open image dlg handler
	void OnShowImage(wxCommandEvent& evt);

	void OnHideImage(wxCommandEvent& evt);

	void OnImageDlgClosed(wxCloseEvent& evt);

protected:
	static const ModuleDescription defaultDesc;
	static const wxString imageFileSelector;

	static const int controlsWidth;
	wxBoxSizer * controlsSizer;
	wxPanel * controlsPanel;

	wxButton * showImageButton;
	wxButton * hideImageButton;

	ModuleFactory * moduleFactory;
	MultiModuleCanvas * canvas;
	std::unique_ptr<ModuleDAG> mDag;
	std::unordered_map<ModuleId, ModuleNodeCollection> moduleMap;
	std::unordered_map<ModuleId, ImageDialog*> imgDlgMap;
	std::unordered_map<ModuleId, std::shared_ptr<FileOpenHandler> > inputHandlerMap;
	std::unordered_map<ModuleId, std::shared_ptr<FileSaveHandler> > outputHandlerMap;
	ModuleId selectedModule;

private:
	// used for passing module id with event data
	class ModuleIdHolder : public wxObject {
	public:
		ModuleId mid;
	};
	std::unordered_map<ModuleId, ModuleIdHolder*> moduleIdHolderMap;
};

#endif // __WX_MODES_H__
