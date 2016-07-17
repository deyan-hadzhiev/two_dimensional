#include <wx/wx.h>
#include <wx/string.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/wfstream.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::ofstream;

string processCharImage(const wxImage& input, int& packedX0, int& packedX1, int& packedY0, int& packedY1) {
	stringstream output;
	const int w = input.GetWidth();
	const int h = input.GetHeight();
	const int sz = w * h;
	output << "\t{\n";
	output << std::fixed << std::setprecision(4);
	if (input.HasAlpha()) {
		// working on alpha
		const unsigned char * alphaData = input.GetAlpha();
		for (int i = 0; i < sz; ++i) {
			if ((i % w) == 0) {
				output << "\t\t";
			}
			const float a = (alphaData[i] < 255 ? alphaData[i] / 255.0f : 1.0f);
			output << a << 'f' << ',' << (i % w == w - 1 ? '\n' : ' ');
			// update packed bounds
			if (alphaData[i] > 0) {
				packedX0 = std::min(packedX0, i % w);
				packedX1 = std::max(packedX1, i % w);
				packedY0 = std::min(packedY0, i / w);
				packedY1 = std::max(packedY1, i / w);
			}
		}
	} else {
		// TODO (some day)
	}
	output << "\t},\n";
	return output.str();
}

string processImage(const wxImage& input, const string& outFn) {
	// output some headers
	string outFnFormatted = outFn;
	std::transform(outFnFormatted.begin(), outFnFormatted.end(), outFnFormatted.begin(), ::toupper);
	string baseName = outFnFormatted;
	const size_t dotIdx = baseName.find_last_of('.');
	baseName = baseName.substr(0, dotIdx);
	std::replace(outFnFormatted.begin(), outFnFormatted.end(), '.', '_');

	string defineStr = string("__") + outFnFormatted + string("__");
	stringstream output;
	output << string("#ifndef ") << defineStr << "\n";
	output << string("#define ") << defineStr << "\n\n";

	// calculate the width and height
	const int charWidth = input.GetWidth() / 16;
	const int charHeight = input.GetHeight() / 16;
	output << string("static const int ") << baseName << string("_CHAR_WIDTH = ") << std::to_string(charWidth) << string(";\n");
	output << string("static const int ") << baseName << string("_CHAR_HEIGHT = ") << std::to_string(charHeight) << string(";\n");
	output << string("\n");

	// output the start of the constant array
	output << string("static const float ") << baseName << string("_MASKS[256][") << std::to_string(charWidth * charHeight) << string("] = {\n");
	int packedX0 = charWidth;
	int packedX1 = 0;
	int packedY0 = charHeight;
	int packedY1 = 0;
	// now process all the subimages
	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			const wxRect subImgCoords(x * charWidth, y * charHeight, charWidth, charHeight);
			wxImage charImg = input.GetSubImage(subImgCoords);
			output << processCharImage(charImg, packedX0, packedX1, packedY0, packedY1);
		}
	}
	// end the array
	output << string("};\n");
	// output the packed variables
	output << string("\n");
	output << string("static const int ") << baseName << string("_PACKED_OFFSET_X = ") << std::to_string(packedX0) << string(";\n");
	output << string("static const int ") << baseName << string("_PACKED_OFFSET_Y = ") << std::to_string(packedY0) << string(";\n");
	output << string("static const int ") << baseName << string("_PACKED_WIDTH = ") << std::to_string(packedX1 - packedX0 + 1) << string(";\n");
	output << string("static const int ") << baseName << string("_PACKED_HEIGHT = ") << std::to_string(packedY1 - packedY0 + 1) << string(";\n");
	// output some footers
	output << string("\n") << string("#endif // ") << defineStr << "\n";
	return output.str();
}

void printHelp() {
	cout << "Usage of the converter:" << endl;
	cout << "\tpng2header <png_file> <output_header>" << endl;
}

int main(int argc, char * argv[]) {
	if (3 > argc) {
		printHelp();
	} else {
		wxImage::AddHandler(new wxPNGHandler);
		wxImage::AddHandler(new wxJPEGHandler);
		wxImage::AddHandler(new wxBMPHandler);
		const wxString fn(argv[1]);
		wxFileInputStream inputStream(fn);
		if (inputStream.IsOk()) {
			wxImage inputImage(inputStream);
			if (inputImage.IsOk()) {
				string output = processImage(inputImage, argv[2]);
				ofstream outFile;
				outFile.open(argv[2]);
				outFile << output;
				outFile.close();
			} else {
				cout << "ERROR: Invalid input image!" << endl;
			}
		} else {
			cout << "ERROR: Could not open input file: " << argv[1] << endl;
		}
	}
	return 0;
}
