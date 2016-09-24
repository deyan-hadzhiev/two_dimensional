#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/dialog.h>
#include <wx/file.h>
#include <wx/image.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>
#include <wx/valnum.h>
#include <wx/filename.h>

#include <sstream>

#include "guimain.h"
#include "wx_modes.h"
#include "wx_bitmap_canvas.h"
#include "module_manager.h"
#include "geom_primitive.h"

ModePanel::ModePanel(ViewFrame * viewFrame, bool multiParams, unsigned styles)
	: wxPanel(viewFrame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxNO_BORDER | wxSIZE_AUTO | wxSIZE_FORCE)
	, viewFrame(viewFrame)
	, mPanelSizer(nullptr)
	, paramPanel(nullptr)
{
	viewFrame->setCustomStyle(styles);
	mPanelSizer = new wxBoxSizer(multiParams ? wxHORIZONTAL : wxVERTICAL);
	SetSizerAndFit(mPanelSizer);
	SetSize(viewFrame->GetClientSize());

	if (!multiParams) {
		paramPanel = new ParamPanel(this, this);
		mPanelSizer->Add(paramPanel, 0, wxEXPAND | wxALL, panelBorder);
	}
	SendSizeEvent();
}

ModePanel::~ModePanel() {}

wxString ModePanel::getCbString() const {
	wxString statusText;
	const std::string& moduleName = cb.getModuleName();
	const float percentDone = cb.getPercentDone();
	if (!moduleName.empty() && percentDone > 0.05f) {
		statusText.Printf(wxT("%s : %6.2f %%"), moduleName.c_str(), percentDone);
		const int64 duration = cb.getDuration();
		if (0 != duration) {
			wxString durationText;
			durationText.Printf(wxT(" (%d.%03ds)"), static_cast<int>(duration / 1000), static_cast<int>(duration % 1000));
			statusText << durationText;
		}
	}
	return statusText;
}

const int ModePanel::panelBorder = 4;

const wxString InputOutputMode::ioFileSelector = wxT("png or jpeg images (*.png;*.jpeg;*.jpg;*.bmp)|*.png;*.jpeg;*.jpg;*.bmp");

InputOutputMode::InputOutputMode(ViewFrame * viewFrame, ModuleBase * _module)
	: ModePanel(viewFrame, false, ViewFrame::VFS_ALL_ENABLED & ~ViewFrame::VFS_CNT_COMPARE) // disable compare for now - it is not done and will not be soon
	, inputPanel(nullptr)
	, outputPanel(nullptr)
	, compareCanvas(nullptr)
	, module(_module)
{
	wxBoxSizer * canvasSizer = new wxBoxSizer(wxHORIZONTAL);
	//inputCanvas = new BitmapCanvas(this, viewFrame);
	inputPanel = new ImagePanel(this, viewFrame);
	//inputCanvas = new wxPanel(this);
	canvasSizer->Add(inputPanel, 1, wxSHRINK | wxEXPAND | wxALL, panelBorder);
	//outputCanvas = new BitmapCanvas(this, viewFrame);
	outputPanel = new ImagePanel(this, viewFrame);
	//outputCanvas = new wxPanel(this);
	canvasSizer->Add(outputPanel, 1, wxSHRINK | wxEXPAND | wxALL, panelBorder);
	BitmapCanvas * inputCanvas = inputPanel->getCanvas();
	BitmapCanvas * outputCanvas = outputPanel->getCanvas();
	// add synchornizers
	inputCanvas->addSynchronizer(outputCanvas);
	outputCanvas->addSynchronizer(inputCanvas);
	// set the background colours
	wxColour bc(75, 75, 75);
	inputCanvas->SetBackgroundColour(bc);
	outputCanvas->SetBackgroundColour(bc);

	module->setProgressCallback(&cb);
	module->setInputManager(inputPanel);
	module->setOutputManager(outputPanel);
	module->setParamManager(paramPanel);

	compareCanvas = new wxPanel(this);
	canvasSizer->Add(compareCanvas, 1, wxEXPAND | wxALL, panelBorder);
	compareCanvas->SetBackgroundColour(*wxGREEN);
	compareCanvas->Show(false);
	mPanelSizer->Add(canvasSizer, 1, wxEXPAND | wxALL);
	SendSizeEvent();
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
					inputPanel->setImage(inputImage);
					viewFrame->SetStatusText(wxString(wxT("Loaded input image: ") + fdlg.GetPath()));
					inputPanel->synchronize();
					lastInput = inputImage;
					// finally notify the paramPanel for the changed image
					paramPanel->onImageChange(inputImage.GetWidth(), inputImage.GetHeight());
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
	{
		const wxStandardPaths& stdPaths = wxStandardPaths::Get();
		wxFileDialog fdlg(this, wxT("Save output image file"), stdPaths.GetUserDir(wxStandardPaths::Dir_Pictures), wxT(""), ioFileSelector, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (fdlg.ShowModal() != wxID_CANCEL) {
			const wxImage outImage = outputPanel->getImage();
			if (outImage.IsOk()) {
				wxFileName outFn(fdlg.GetPath());
				const wxString outExt = outFn.GetExt();
				wxBitmapType outType = wxBitmapType::wxBITMAP_TYPE_BMP;
				if (0 == outExt.CmpNoCase(wxT("png"))) {
					outType = wxBitmapType::wxBITMAP_TYPE_PNG;
				} else if (0 == outExt.CmpNoCase(wxT("jpg")) || 0 == outExt.CmpNoCase(wxT("jpeg"))) {
					outType = wxBitmapType::wxBITMAP_TYPE_JPEG;
				}
				if (outImage.SaveFile(outFn.GetFullPath(), outType)) {
					viewFrame->SetStatusText(wxString(wxT("Saved output image: ")) + outFn.GetFullPath());
				} else {
					viewFrame->SetStatusText(wxString(wxT("Could NOT save output image: ")) + outFn.GetFullPath());
				}
			} else {
				viewFrame->SetStatusText(wxString(wxT("The output image is corrupted!")));
			}
		}
		break;
	}
	case (ViewFrame::MID_VF_CNT_RUN) :
		module->runModule();
		break;
	case (ViewFrame::MID_VF_CNT_STOP) :
		cb.setAbortFlag();
		break;
	case (ViewFrame::MID_VF_CNT_COMPARE) :
		inputPanel->Show(!inputPanel->IsShown());
		outputPanel->Show(!outputPanel->IsShown());
		compareCanvas->Show(!compareCanvas->IsShown()); // TODO: Fix/change
		SendSizeEvent();
		break;
	case (ViewFrame::MID_VF_CNT_HISTOGRAM) :
		inputPanel->toggleHist();
		outputPanel->toggleHist();
		SendSizeEvent();
		break;
	default:
		break;
	}
}

void InputOutputMode::setInput(const wxImage & input) {
	if (input.Ok() && inputPanel) {
		inputPanel->setImage(input);
		lastInput = input;
		// notify the paramPanel on the change
		paramPanel->onImageChange(input.GetWidth(), input.GetHeight());
	}
}

GeometricOutput::GeometricOutput(ViewFrame * vf, ModuleBase * _module)
	: ModePanel(vf, false, ViewFrame::VFS_CNT_RUN)
	, module(_module)
	, outputPanel(nullptr)
{
	wxBoxSizer * canvasSizer = new wxBoxSizer(wxHORIZONTAL);
	//outputCanvas = new BitmapCanvas(this, vf);
	outputPanel = new ImagePanel(this, vf);
	canvasSizer->Add(outputPanel, 1, wxSHRINK | wxEXPAND | wxALL, panelBorder);
	mPanelSizer->Add(canvasSizer, 1, wxEXPAND, wxALL);

	module->setProgressCallback(&cb);
	module->setOutputManager(outputPanel);
	module->setParamManager(paramPanel);

	SendSizeEvent();
}

void GeometricOutput::onCommandMenu(wxCommandEvent & ev) {
	switch (ev.GetId())
	{
	case(ViewFrame::MID_VF_CNT_RUN) : {
		module->runModule();
		break;
	}
	default:
		break;
	}
}

bool FileOpenHandler::getInput(Bitmap & bmp, int idx) const {
	bool res = false;
	if (inputBmp.isOK()) {
		std::lock_guard<std::mutex> lk(bmpMutex);
		bmp = inputBmp;
		res = true;
	}
	return res;
}

void FileOpenHandler::setInput(const wxImage & img) {
	bool updated = false;
	if (img.IsOk()) {
		std::lock_guard<std::mutex> lk(bmpMutex);
		const int w = img.GetWidth();
		const int h = img.GetHeight();
		inputBmp.generateEmptyImage(w, h, false);
		Color * bmpData = inputBmp.getDataPtr();
		memcpy(bmpData, img.GetData(), w * h * sizeof(Color));
		updated = true;
	}

	if (updated && moduleHandle != nullptr) {
		moduleHandle->update();
	}
}

void FileSaveHandler::setOutput(const Bitmap & bmp, ModuleId mid) {
	if (bmp.isOK()) {
		{
			std::lock_guard<std::mutex> lk(bmpMutex);
			outputBmp = bmp;
		}
		if (externalOutput != nullptr) {
			externalOutput->setOutput(bmp, mid);
		}
	}
}

void FileSaveHandler::getOutput(wxImage & img) const {
	if (outputBmp.isOK()) {
		std::lock_guard<std::mutex> lk(bmpMutex);
		img = wxImage(
			wxSize(outputBmp.getWidth(), outputBmp.getHeight()),
			reinterpret_cast<unsigned char *>(outputBmp.getDataPtr()),
			true);
	}
}

void FileSaveHandler::setExternalOutput(OutputManager * oman) {
	externalOutput = oman;
}

ModuleNodeCollection::ModuleNodeCollection(const ModuleDescription & _moduleDesc, ModuleBase * _moduleHandle, ParamPanel * _moduleParamPanel)
	: moduleDesc(_moduleDesc)
	, moduleHandle(_moduleHandle)
	, moduleParamPanel(_moduleParamPanel)
{}

const ModuleDescription MultiModuleMode::defaultDesc = ModuleDescription();
const wxString MultiModuleMode::imageFileSelector = wxT("png or jpeg images (*.png;*.jpeg;*.jpg;*.bmp)|*.png;*.jpeg;*.jpg;*.bmp");
const int MultiModuleMode::controlsWidth = 200;

MultiModuleMode::MultiModuleMode(ViewFrame * vf, ModuleFactory * mf)
	: ModePanel(vf, true, ViewFrame::VFS_ALL_ENABLED & ~ViewFrame::VFS_CNT_COMPARE & ~ViewFrame::VFS_OPEN_SAVE)
	, controlsSizer(new wxBoxSizer(wxVERTICAL))
	, controlsPanel(new wxPanel(this))
	, showImageButton(nullptr)
	, hideImageButton(nullptr)
	, moduleFactory(mf)
	, canvas(new MultiModuleCanvas(this))
	, mDag(new ModuleDAG)
	, selectedModule(InvalidModuleId)
{
	moduleFactory->clear();
	// some sizers
	controlsSizer->SetMinSize(wxSize(controlsWidth, wxDefaultCoord));
	// add an intermediate panel to use its max size
	controlsPanel->SetMaxSize(wxSize(controlsWidth, wxDefaultCoord));
	controlsPanel->SetSizerAndFit(controlsSizer);
	controlsSizer->FitInside(controlsPanel);
	// and now make a sizer for the buttons
	wxBoxSizer * buttonsSizer = new wxBoxSizer(wxVERTICAL);
	// add a button to the controls
	showImageButton = new wxButton(controlsPanel, wxID_ANY, wxT("Show image"));
	showImageButton->Enable(false);
	showImageButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MultiModuleMode::OnShowImage), NULL, this);
	controlsSizer->Add(showImageButton, 0, wxEXPAND | wxALL, ModePanel::panelBorder);

	hideImageButton = new wxButton(controlsPanel, wxID_ANY, wxT("Hide image"));
	hideImageButton->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MultiModuleMode::OnHideImage), NULL, this);
	controlsSizer->Add(hideImageButton, 0, wxEXPAND | wxALL, ModePanel::panelBorder);
	hideImageButton->Show(false);

	controlsSizer->Add(new wxStaticLine(controlsPanel), 0, wxEXPAND | wxALL, ModePanel::panelBorder);
	controlsSizer->AddStretchSpacer();
	// finally add to the whole panel sizer
	mPanelSizer->Add(controlsPanel, 0, wxEXPAND | wxALL);
	mPanelSizer->Add(canvas, 1, wxEXPAND | wxALL);
	SendSizeEvent();
}

MultiModuleMode::~MultiModuleMode() {
	// destroy everything that is not automatically destroyed by wx
	for (auto it : moduleIdHolderMap) {
		delete it.second;
		it.second = nullptr;
	}
}

void MultiModuleMode::onCommandMenu(wxCommandEvent & ev) {
	switch (ev.GetId()) {
	case (ViewFrame::MID_VF_FILE_OPEN): {
		if (selectedModule != InvalidModuleId) {
			auto it = inputHandlerMap.find(selectedModule);
			if (it != inputHandlerMap.end() && it->second) {
				const wxStandardPaths& stdPaths = wxStandardPaths::Get();
				wxFileDialog fdlg(this, wxT("Open input image file"), stdPaths.GetUserDir(wxStandardPaths::Dir_Pictures), wxT(""), imageFileSelector, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
				if (fdlg.ShowModal() != wxID_CANCEL) {
					wxFileInputStream inputStream(fdlg.GetPath());
					if (inputStream.Ok()) {
						wxImage inputImage(inputStream);
						if (inputImage.Ok()) {
							it->second->setInput(inputImage);
						}
					}
				}
			}
		}
		break;
	}
	case (ViewFrame::MID_VF_FILE_SAVE): {
		if (selectedModule != InvalidModuleId) {
			auto it = outputHandlerMap.find(selectedModule);
			if (it != outputHandlerMap.end() && it->second) {
				const wxStandardPaths& stdPaths = wxStandardPaths::Get();
				wxFileDialog fdlg(this, wxT("Save output image file"), stdPaths.GetUserDir(wxStandardPaths::Dir_Pictures), wxT(""), imageFileSelector, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
				if (fdlg.ShowModal() != wxID_CANCEL) {
					wxImage outputImage;
					it->second->getOutput(outputImage);
					if (outputImage.IsOk()) {
						wxFileName outFn(fdlg.GetPath());
						const wxString outExt = outFn.GetExt();
						wxBitmapType outType = wxBitmapType::wxBITMAP_TYPE_BMP;
						if (0 == outExt.CmpNoCase(wxT("png"))) {
							outType = wxBitmapType::wxBITMAP_TYPE_PNG;
						} else if (0 == outExt.CmpNoCase(wxT("jpg")) || 0 == outExt.CmpNoCase(wxT("jpeg"))) {
							outType = wxBitmapType::wxBITMAP_TYPE_JPEG;
						}
						if (outputImage.SaveFile(outFn.GetFullPath(), outType)) {
							viewFrame->SetStatusText(wxString(wxT("Saved output image: ")) + outFn.GetFullPath());
						} else {
							viewFrame->SetStatusText(wxString(wxT("Could NOT save output image: ")) + outFn.GetFullPath());
						}
					} else {
						viewFrame->SetStatusText(wxString(wxT("The output image is corrupted!")));
					}
				}
			}
		}
		break;
	}
	case (ViewFrame::MID_VF_CNT_RUN): {
		std::vector<ModuleId> invalidateList;
		if (selectedModule != InvalidModuleId) {
			invalidateList.push_back(selectedModule);
		} else {
			// push all modules with 0 inputs to invalidate the whole configuration
			for (const auto& moduleIt : moduleMap) {
				const ModuleNodeCollection& mnc = moduleIt.second;
				if (mnc.moduleDesc.inputs == 0) {
					invalidateList.push_back(moduleIt.first);
				}
			}
		}
		// call the invalidation
		mDag->invalidateModules(invalidateList);
		break;
	}
	default:
		break;
	}
}

void MultiModuleMode::addModule(const ModuleDescription & md) {
	// first allocate everything neccessary
	ModuleBase * moduleHandle = moduleFactory->getModule(md.id);
	const ModuleId mid = moduleHandle->getModuleId();
	ParamPanel * moduleParamPanel = new ParamPanel(controlsPanel, this, false);
	// hide the panel and add it to the sizer
	moduleParamPanel->Show(false);
	controlsSizer->Add(moduleParamPanel, 0, wxEXPAND | wxALL, ModePanel::panelBorder);
	// and add the param panel to the module
	moduleHandle->setParamManager(moduleParamPanel);
	// if the module is output or input it has special managers
	if (md.id == M_INPUT) {
		std::shared_ptr<FileOpenHandler> openHandler(new FileOpenHandler(moduleHandle));
		moduleHandle->setInputManager(openHandler.get());
		inputHandlerMap[mid] = openHandler;
	} else if (md.id == M_OUTPUT) {
		std::shared_ptr<FileSaveHandler> saveHandler(new FileSaveHandler());
		moduleHandle->setOutputManager(saveHandler.get());
		outputHandlerMap[mid] = saveHandler;
	}
	// add it to the module map
	moduleMap[mid] = ModuleNodeCollection(md, moduleHandle, moduleParamPanel);
	ModuleDescription& mmd = moduleMap[mid].moduleDesc;
	const std::string mmdMapIdStr = std::to_string(mid);
	mmd.fullName += std::string(" ") + mmdMapIdStr;

	// add the module to the dag, before adding it to the canvas
	mDag->addModule(moduleHandle, mmd);

	// add the module to the canvas to be shown
	canvas->addModuleDescription(mid, mmd);
}

void MultiModuleMode::updateSelection(ModuleId id) {
	// change selection?
	if (id != selectedModule) {
		bool refresh = false;
		// check if the module is input/output and set proper flags to the view frame
		const unsigned currentFrameStyle = viewFrame->getCustomStyle();
		unsigned modifiedStyle = currentFrameStyle;
		// get currently selected module
		if (selectedModule != -1) {
			// check if the module still exists
			auto mncIt = moduleMap.find(selectedModule);
			if (mncIt != moduleMap.end()) {
				ModuleNodeCollection& mnc = mncIt->second;
				// hide the param panel
				mnc.moduleParamPanel->Show(false);
				// update the styles
				modifiedStyle = currentFrameStyle & ~ViewFrame::VFS_OPEN_SAVE;
				refresh = true;
			}
		}
		if (id != -1) {
			ModuleNodeCollection& mnc = moduleMap[id];
			mnc.moduleParamPanel->Show(true);
			if (mnc.moduleDesc.id == M_INPUT) {
				modifiedStyle = (currentFrameStyle | ViewFrame::VFS_FILE_OPEN) & ~ViewFrame::VFS_FILE_SAVE;
			} else if (mnc.moduleDesc.id == M_OUTPUT) {
				modifiedStyle = (currentFrameStyle | ViewFrame::VFS_FILE_SAVE) & ~ViewFrame::VFS_FILE_OPEN;
			} else {
				modifiedStyle = currentFrameStyle & ~ViewFrame::VFS_OPEN_SAVE;
			}
			// check if this module has shown its image dialog
			showImageButton->Enable(true);
			hideImageButton->Enable(true);
			auto& dlgIt = imgDlgMap.find(id);
			if (dlgIt != imgDlgMap.end()) {
				// check if shown and base some other changes on that
				const bool shown = dlgIt->second->IsShown();
				showImageButton->Show(!shown);
				hideImageButton->Show(shown);
			} else {
				showImageButton->Show();
				hideImageButton->Hide();
			}
			refresh = true;
		} else {
			// disable both the show and hide image dialog buttons
			hideImageButton->Hide();
			hideImageButton->Enable(false);
			showImageButton->Show();
			showImageButton->Enable(false);
			refresh = true;
		}
		selectedModule = id;
		if (currentFrameStyle != modifiedStyle) {
			viewFrame->setCustomStyle(modifiedStyle);
		}
		if (refresh) {
			SendSizeEvent();
		}
	}
}

void MultiModuleMode::removeModule(ModuleId id) {
	// if the removed module is selected - update the selection
	if (selectedModule == id) {
		updateSelection(InvalidModuleId);
	}
	// first remove the module from the dag
	mDag->removeModule(id);
	// remove any image dialogs of the module
	auto& dlgIt = imgDlgMap.find(id);
	if (dlgIt != imgDlgMap.end()) {
		dlgIt->second->Destroy();
		imgDlgMap.erase(id);
		// there should also be a module id holder associated with the moduleId
		delete moduleIdHolderMap[id];
		moduleIdHolderMap.erase(id);
	}
	auto& it = moduleMap.find(id);
	if (it != moduleMap.end()) {
		ModuleNodeCollection& mnc = it->second;
		// TODO - call the abort on the progress handler
		// if the module has special input/output handlers - remove them
		if (mnc.moduleDesc.id == M_INPUT) {
			inputHandlerMap.erase(id);
		} else if (mnc.moduleDesc.id == M_OUTPUT) {
			outputHandlerMap.erase(id);
		}
		// first remove the module to prevent further running
		moduleFactory->destroyModule(mnc.moduleHandle);
		mnc.moduleHandle = nullptr;
		// first remove its paramPanel
		mnc.moduleParamPanel->Destroy();
		mnc.moduleParamPanel = nullptr;
		// after all remove the module from the map
		moduleMap.erase(id);
	}
}

EdgeId MultiModuleMode::addConnection(ModuleId srcId, ModuleId destId, int destSrcIdx) {
	return mDag->addConnector(srcId, destId, destSrcIdx);
}

void MultiModuleMode::removeConnection(EdgeId eid) {
	mDag->removeConnector(eid);
}

bool MultiModuleMode::validConnection(ModuleId srcId, ModuleId destId, int destSrcIdx) {
	// TODO
	return true;
}

void MultiModuleMode::OnShowImage(wxCommandEvent & evt) {
	if (selectedModule != InvalidModuleId) {
		showImageDialog(selectedModule, true);
	}
}

void MultiModuleMode::OnHideImage(wxCommandEvent & evt) {
	if (selectedModule != InvalidModuleId) {
		showImageDialog(selectedModule, false);
	}
}

void MultiModuleMode::OnImageDlgClosed(wxCloseEvent & evt) {
	ModuleIdHolder * idHandle = dynamic_cast<ModuleIdHolder*>(evt.GetEventUserData());
	if (idHandle) {
		const ModuleId mid = idHandle->mid;
		showImageDialog(mid, false);
	}
}

void MultiModuleMode::toggleImageShow(ModuleId mid) {
	if (mid != InvalidModuleId) {
		// check if the image dialog is shown
		auto imgIt = imgDlgMap.find(mid);
		bool shown = false;
		if (imgIt != imgDlgMap.end() && imgIt->second->IsShown()) {
			shown = true;
		}
		showImageDialog(mid, !shown);
	}
}

void MultiModuleMode::showImageDialog(ModuleId mid, bool show) {
	if (mid != InvalidModuleId) {
		auto it = imgDlgMap.find(mid);
		ImageDialog * dlg = nullptr;
		if (it != imgDlgMap.end()) {
			dlg = it->second;
		} else if (show) {
			ModuleNodeCollection& mnc = moduleMap[mid];
			const std::string moduleName = mnc.moduleDesc.fullName;
			dlg = new ImageDialog(this, viewFrame, moduleName);
			imgDlgMap[mid] = dlg;
			ModuleIdHolder * idHandle = new ModuleIdHolder;
			idHandle->mid = mid;
			dlg->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(MultiModuleMode::OnImageDlgClosed), idHandle, this);
			OutputManager * exOman = dlg->getImagePanel();
			// add it as an output handler based on the type of the module (output modules are special - duh)
			if (mnc.moduleDesc.id == M_OUTPUT) {
				// add it to the output handlers
				outputHandlerMap[mid]->setExternalOutput(exOman);
			} else {
				mDag->setExternalOutput(mid, exOman);
			}
		}
		// if this function was called with no show - still the dialog may not be shown
		if (dlg) {
			dlg->Show(show);
			if (mid == selectedModule) {
				showImageButton->Show(!show);
				hideImageButton->Show(show);
				Layout();
				Update();
			}
		}
	}
}
