#ifndef __WX_MODES_H__
#define __WX_MODES_H__

#include <wx/wx.h>
#include "guimain.h"
#include "wx_bitmap_canvas.h"
#include "kernel_base.h"
#include "kernels.h"

class ViewFrame;

class ModePanel : public wxPanel {
public:
	ModePanel(ViewFrame * viewFrame, unsigned styles = ViewFrame::VFS_NOTHING_ENABLED);
	virtual ~ModePanel();

	virtual void onCommandMenu(wxCommandEvent& ev) {}

protected:
	ViewFrame * viewFrame;
	wxBoxSizer * mPanelSizer;
};

class InputOutputMode : public ModePanel {
public:
	InputOutputMode(ViewFrame * viewFrame, SimpleKernel * kernel);
	virtual ~InputOutputMode();

	virtual void onCommandMenu(wxCommandEvent& ev) override;
protected:
	BitmapCanvas * inputCanvas;
	BitmapCanvas * outputCanvas;
	wxPanel * compareCanvas;
	SimpleKernel * kernel;

	//wxPanel * inputCanvas;
	//wxPanel * outputCanvas;
	static const wxString ioFileSelector; //!< file selection string - change if a new file type is added
	static const int panelBorder; //!< the border of the input/output panels
};

class NegativePanel : public InputOutputMode {
public:
	NegativePanel(ViewFrame * viewFrame);
};

#endif // __WX_MODES_H__
