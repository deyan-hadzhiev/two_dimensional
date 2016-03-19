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

#include "color.h"
#include "bitmap.h"

class ViewApp : public wxApp {
public:
	virtual bool OnInit() override;
};

class BitmapCanvas : public wxPanel {
public:
	BitmapCanvas(wxWindow * parent, const Bitmap& initBmp);

	void OnPaint(wxPaintEvent& evt);

	void OnEraseBkg(wxEraseEvent& evt);

	void setBitmap(const Bitmap& bmp);
private:
	wxBitmap bmp;
};

class ViewFrame : public wxFrame {
public:
	ViewFrame(const wxString& title);

	void OnRadioButtonChange(wxCommandEvent&);

	// this should trigger the new image copy
	void OnScrollChange(wxScrollEvent&);

	// this should only change (for now) the value of the text
	void OnSoftScrollChange(wxScrollEvent&);

	void OnScrollTextEnter(wxCommandEvent&);

	// for now it is only init function - it will become an interface later
	void initBitcube();
	enum {
		ID_RB_XY = wxID_HIGHEST + 1,
		ID_RB_YZ,
		ID_RB_ZX,
		ID_SLIDER,
		ID_SLIDER_TEXT,
	};
private:
	void setSlice(int value, unsigned plane);

	wxSlider * slider;
	wxTextCtrl * sliderText;
	BitmapCanvas * bmpCanvas;

	Bitmap slice;
};

wxIMPLEMENT_APP(ViewApp);

bool ViewApp::OnInit() {

	if (!wxApp::OnInit())
		return false;

	// must remain here!!!
	initColor();

	ViewFrame * frame = new ViewFrame(wxT("2d graphics"));
	frame->Show();

	return true;
}

ViewFrame::ViewFrame(const wxString& title)
	: wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(512, 512))
	, slider(nullptr)
	, sliderText(nullptr)
	, bmpCanvas(nullptr)
{
	SetMinClientSize(wxSize(512, 512));
	SetDoubleBuffered(true);

	wxPanel * mainPanel = new wxPanel(this);

	wxBoxSizer * mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->FitInside(mainPanel);
	mainPanel->SetSizer(mainSizer);

	wxBoxSizer * rbSizer = new wxBoxSizer(wxHORIZONTAL);
	wxRadioButton * rbxy = new wxRadioButton(mainPanel, ID_RB_XY, wxT("XY"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	rbSizer->Add(rbxy, 1, wxEXPAND | wxALL, 10);
	wxRadioButton * rbyz = new wxRadioButton(mainPanel, ID_RB_YZ, wxT("YZ"));
	rbSizer->Add(rbyz, 1, wxEXPAND | wxALL, 10);
	wxRadioButton * rbzx = new wxRadioButton(mainPanel, ID_RB_ZX, wxT("ZX"));
	rbSizer->Add(rbzx, 1, wxEXPAND | wxALL, 10);
	mainSizer->Add(rbSizer, 0, wxALL, 5);

	bmpCanvas = new BitmapCanvas(mainPanel, slice);
	mainSizer->Add(bmpCanvas, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer * sliderSizer = new wxBoxSizer(wxHORIZONTAL);
	slider = new wxSlider(mainPanel, ID_SLIDER, 0, 0, 255 - 1);
	sliderSizer->Add(slider, 1, wxEXPAND | wxALL, 5);
	sliderText = new wxTextCtrl(mainPanel, ID_SLIDER_TEXT, wxT("0"), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	sliderSizer->Add(sliderText, 0, wxALL, 5);

	mainSizer->Add(sliderSizer, 0, wxEXPAND | wxALL, 5);

	// add some event handlers
	Connect(ID_RB_XY, wxEVT_RADIOBUTTON, wxCommandEventHandler(ViewFrame::OnRadioButtonChange), NULL, this);
	Connect(ID_RB_YZ, wxEVT_RADIOBUTTON, wxCommandEventHandler(ViewFrame::OnRadioButtonChange), NULL, this);
	Connect(ID_RB_ZX, wxEVT_RADIOBUTTON, wxCommandEventHandler(ViewFrame::OnRadioButtonChange), NULL, this);
	Connect(ID_SLIDER, wxEVT_SCROLL_CHANGED, wxScrollEventHandler(ViewFrame::OnScrollChange), NULL, this);
	Connect(ID_SLIDER, wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler(ViewFrame::OnSoftScrollChange), NULL, this);
	Connect(ID_SLIDER_TEXT, wxEVT_TEXT_ENTER, wxCommandEventHandler(ViewFrame::OnScrollTextEnter), NULL, this);

	initBitcube();
	Update();
	Refresh();
}

void ViewFrame::initBitcube() {

}

void ViewFrame::OnRadioButtonChange(wxCommandEvent& evt) {

}

void ViewFrame::OnScrollChange(wxScrollEvent& evt) {

}

void ViewFrame::OnSoftScrollChange(wxScrollEvent& evt) {

}

void ViewFrame::OnScrollTextEnter(wxCommandEvent& evt) {

}

void ViewFrame::setSlice(int value, unsigned plane) {

}

BitmapCanvas::BitmapCanvas(wxWindow * parent, const Bitmap& initBmp)
	: wxPanel(parent)
{
	Connect(wxEVT_PAINT, wxPaintEventHandler(BitmapCanvas::OnPaint), NULL, this);
	Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(BitmapCanvas::OnEraseBkg), NULL, this);
	setBitmap(initBmp);
	Update();
}

void BitmapCanvas::OnPaint(wxPaintEvent& evt) {
	//wxPaintDC dc(this);
	wxBufferedPaintDC dc(this);
	const wxSize dcSize = GetSize();
	const wxSize bmpSize = bmp.GetSize();
	const int xc = (dcSize.GetWidth() > bmpSize.GetWidth() ? (dcSize.GetWidth() / 2) - (bmpSize.GetWidth() / 2) : 0);
	const int yc = (dcSize.GetHeight() > bmpSize.GetHeight() ? (dcSize.GetHeight() / 2) - (bmpSize.GetHeight() / 2) : 0);
	//wxMemoryDC bmpDC(bmp);
	//dc.Blit(wxPoint(xc, yc), bmpSize, &bmpDC, wxPoint(0, 0));
	dc.DrawBitmap(bmp, xc, yc);
	// draw fill
	const wxColour fillColour(127, 127, 127);
	dc.SetBrush(wxBrush(fillColour));
	dc.SetPen(wxPen(fillColour));
	if (xc > 0) {
		dc.DrawRectangle(0, 0, xc, dcSize.GetHeight());
		dc.DrawRectangle(xc + bmpSize.GetWidth(), 0, xc, dcSize.GetHeight());
	}
	if (yc > 0) {
		dc.DrawRectangle(0, 0, dcSize.GetWidth(), yc);
		dc.DrawRectangle(0, yc + bmpSize.GetHeight(), dcSize.GetWidth(), yc);
	}
}

void BitmapCanvas::setBitmap(const Bitmap& bmp) {
	const int w = bmp.getWidth();
	const int h = bmp.getHeight();
	wxImage tmpImg = wxImage(wxSize(w, h));
	unsigned char * imgData = tmpImg.GetData();
	const Color* colorData = bmp.getDataPtr();
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			const unsigned rgb = colorData[x + y * w].toRGB32();
			imgData[0 + (x + y * w) * 3] = static_cast<unsigned char>((rgb & 0xff0000) >> 16);
			imgData[1 + (x + y * w) * 3] = static_cast<unsigned char>((rgb & 0x00ff00) >> 8);
			imgData[2 + (x + y * w) * 3] = static_cast<unsigned char>(rgb & 0x0000ff);
		}
	}
	this->bmp = wxBitmap(tmpImg);
	Refresh();
}

void BitmapCanvas::OnEraseBkg(wxEraseEvent& evt) {
	// disable erase - all will be handled in the paint evt
}
