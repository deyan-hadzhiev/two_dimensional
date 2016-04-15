#include <memory>
#include <vector>
#include <utility>
#include <algorithm>

#include "kernels.h"
#include "vector2.h"
#include "matrix2.h"
#include "util.h"
#include "convolution.h"

KernelBase::ProcessResult SimpleKernel::runKernel(unsigned flags) {
	const bool hasInput = getInput();
	KernelBase::ProcessResult retval;
	if (hasInput && bmp.isOK()) {
		retval = kernelImplementation(flags);
		if (KernelBase::KPR_OK == retval) {
			setOutput();
			if (iman)
				iman->kernelDone(retval);
		}
	} else {
		retval = KernelBase::KPR_INVALID_INPUT;
	}
	return retval;
}

void AsyncKernel::kernelLoop(AsyncKernel * k) {
	while(k->state != State::AKS_TERMINATED) {
		std::unique_lock<std::mutex> lk(k->kernelMutex);
		if (k->state == State::AKS_FINISHED || k->state == State::AKS_INIT) {
			k->ev.wait(lk);
			State dirty = State::AKS_DIRTY;
			k->state.compare_exchange_weak(dirty, State::AKS_RUNNING);
		}
		if (k->state != State::AKS_TERMINATED) {
			k->kernelImplementation(0);
			// be carefull not to change the state!!!
			State fin = State::AKS_RUNNING; // expected state
			k->state.compare_exchange_weak(fin, State::AKS_FINISHED);
		}
	}
}

bool AsyncKernel::getAbortState() const {
	return
		state == State::AKS_TERMINATED ||
		state == State::AKS_DIRTY ||
		(nullptr != cb && cb->getAbortFlag());
}

AsyncKernel::AsyncKernel()
	: state(State::AKS_INIT)
	, loopThread(kernelLoop, this)
{}

AsyncKernel::~AsyncKernel() {
	state = State::AKS_TERMINATED;
	ev.notify_one();
	loopThread.join();
}

AsyncKernel::State AsyncKernel::getState() const {
	return state;
}

void AsyncKernel::update() {
	state = State::AKS_DIRTY;
	ev.notify_one();
}

KernelBase::ProcessResult AsyncKernel::runKernel(unsigned flags) {
	update();
	return KPR_RUNNING;
}

KernelBase::ProcessResult NegativeKernel::kernelImplementation(unsigned flags) {
	auto negativePix = [](Color a) -> Color {
		return Color(255, 255, 255) - a;
	};
	bmp.remap(negativePix);
	return KernelBase::KPR_OK;
}

IntervalList TextSegmentationKernel::extractIntervals(const int * accumValues, const int count, int threshold) {
	int firstOverThreshold = -1;
	IntervalList list;
	for (int i = 0; i < count; ++i) {
		if (accumValues[i] > threshold) {
			if (firstOverThreshold == -1) {
				firstOverThreshold = i;
			}
		} else if (firstOverThreshold != -1) {
			list.push_back(std::make_pair(firstOverThreshold, i));
			firstOverThreshold = -1;
		}
	}
	// if the last line was not closed, close it manually
	if (firstOverThreshold != -1) {
		list.push_back(std::make_pair(firstOverThreshold, count));
		firstOverThreshold = -1;
	}
	return list;
}

KernelBase::ProcessResult TextSegmentationKernel::kernelImplementation(unsigned flags) {
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	int threshold = 25;
	if (pman) {
		pman->getIntParam(threshold, "threshold");
	}
	// first negate the image so black would add less to the accumulators
	auto negativePix = [](Color a) -> Color {
		return Color(255, 255, 255) - a;
	};
	// NOTE this should be condition on the mean value
	Bitmap negative = bmp;
	negative.remap(negativePix);
	Color * bmpData = negative.getDataPtr();
	const int bmpDim = bh * bw;
	std::unique_ptr<int[]> accRows(new int[bh]);
	// now accumulate all rows of the input image;
	for (int row = 0; row < bh; ++row) {
		int& accum = accRows[row];
		accum = 0;
		const Color * rowData = bmpData + row * bw;
		for (int px = 0; px < bw; ++px) {
			accum += rowData[px].intensity();
		}
		// to get mean value - divide the row accumulator to the number of pixel on the row
		accum /= bw;
	}

	// now iterate over all the accumulated rows and make intervals of values over the threshold
	IntervalList textLines = extractIntervals(accRows.get(), bh, threshold);

	// now we have to make the whole operation again but for each interval detected before
	std::unique_ptr<int[]> accColumns(new int[bw]);
	std::unique_ptr<IntervalList[]> textColumns(new IntervalList[textLines.size()]);
	// iterate for each found interval over the threshold
	for (int lineInterval = 0; lineInterval < textLines.size(); ++lineInterval) {
		const std::pair<int, int>& rowInterval = textLines[lineInterval];
		// memset to clear from previous usage
		memset(accColumns.get(), 0, bw * sizeof(int));
		// now accumulate the values from each column of the text lines in the interval
		for (int h = rowInterval.first; h < rowInterval.second; ++h) {
			const Color * rowData = bmpData + h * bw;
			for (int w = 0; w < bw; ++w) {
				accColumns[w] += rowData[w].intensity();
			}
		}
		// iterate over the accumulated data and reduce it to mean value base on the interval height
		const int intervalHeight = rowInterval.second - rowInterval.first;
		for (int i = 0; i < bw; ++i) {
			accColumns[i] /= intervalHeight;
		}
		// now add the interval list to the full interval list
		textColumns[lineInterval] = extractIntervals(accColumns.get(), bw, threshold);
	}
	// now start operating on the input image
	const Color sep(255, 0, 0);
	bmpData = bmp.getDataPtr();
	for (int i = 0; i < textLines.size(); ++i) {
		// first set all the rows from the previous interval to this with the separating color
		const int hBlankStart = (i == 0 ? 0 : textLines[i - 1].second);
		const int hBlankEnd = textLines[i].first;
		for (int h = hBlankStart; h < hBlankEnd; ++h) {
			Color * rowData = bmpData + h * bw;
			for (int w = 0; w < bw; ++w) {
				rowData[w] = sep;
			}
		}
		const int lineHeight = textLines[i].second - hBlankEnd;
		Color * lineData = bmpData + hBlankEnd * bw;
		// now colorize all columns of this non-blank interval
		const IntervalList& textCols = textColumns[i];
		for (int j = 0; j < textCols.size(); ++j) {
			const int wBlankStart = (j == 0 ? 0 : textCols[j - 1].second);
			const int wBlankEnd = textCols[j].first;
			for (int x = wBlankStart; x < wBlankEnd; ++x) {
				// for the whole column
				for (int y = 0; y < lineHeight; ++y) {
					lineData[y * bw + x] = sep;
				}
			}
		}
		// and now for the last column
		if (textCols.size() > 0) {
			for (int x = textCols[textCols.size() - 1].second; x < bw; ++x) {
				for (int y = 0; y < lineHeight; ++y) {
					lineData[y * bw + x] = sep;
				}
			}
		}
	}
	// and finaly the remaining rows
	if (textLines.size() > 0) {
		for (int y = textLines[textLines.size() - 1].second; y < bh; ++y) {
			Color * rowData = bmpData + y * bw;
			for (int x = 0; x < bw; ++x) {
				rowData[x] = sep;
			}
		}
	}
	return KernelBase::KPR_OK;
}

void GeometricKernel::setSize(int _width, int _height) {
	if (width != _width || height != _height) {
		width = _width;
		height = _height;
		dirtySize = true;
	}
}

void GeometricKernel::setColor(Color rgb) {
	col = rgb;
}

KernelBase::ProcessResult GeometricKernel::runKernel(unsigned flags) {
	if (width <= 0 || height <= 0) {
		return KPR_INVALID_INPUT;
	}
	const int pWidth = width;
	const int pHeight = height;
	pman->getIntParam(width, "width");
	pman->getIntParam(height, "height");
	dirtySize = (width != pWidth || height != pHeight);
	if (dirtySize || !bmp.isOK()) {
		primitive->resize(width, height);
		dirtySize = false;
	}
	pman->getBoolParam(additive, "additive");
	pman->getBoolParam(clear, "clear");
	pman->getBoolParam(axis, "axis");
	unsigned dflags =
		(additive ? DrawFlags::DF_ACCUMULATE : 0) |
		(clear ? DrawFlags::DF_CLEAR : 0) |
		(axis ? DrawFlags::DF_SHOW_AXIS : 0);

	for (auto it = paramList.cbegin(); it != paramList.cend(); ++it) {
		const ParamDescriptor& pd = *it;
		std::string value = pd.defaultValue;
		pman->getStringParam(value, pd.name);
		primitive->setParam(pd.name, value);
	}
	primitive->draw(col, dflags);
	bmp = primitive->getBitmap();
	setOutput();
	if (iman)
		iman->kernelDone(KPR_OK);
	return KPR_OK;
}

SinosoidKernel::SinosoidKernel()
	: GeometricKernel(new Sinosoid<Color>)
{}

KernelBase::ProcessResult HoughKernel::kernelImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setKernelName("Hough Rho Theta");
	}
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const int hs = 2; // decrease size a bit
	const int hh = int(sqrt(bw * bw + bh * bh)) / hs;
	const int hw = hh;
	Function<uint64> aps; // projected space
	aps.resize(hw, hh);
	float ro = 0.0f; // roVec length - directly used in lambda
	float thetaRad = 0.0f; // the theta angle in radians
	const int cx = hw / 2;
	aps.setFunction(
		[cx, hw](int x) -> float { return toRadians((x + cx) * 180.0f / float(hw)); }, // has to map from [-cx, cx) -> [0, 2*PI)
		[&ro, &thetaRad](float x) -> float { return ro * cos(thetaRad - x); } // directly from the Hough Rho-Theta definition
		);
	const uint64 pc = 1; // we'll use it for addition - so it has to be one
	const Vector2 center(bw / 2 + .5f, bh / 2 + .5f);
	const Color * bmpData = bmp.getDataPtr();
	const int maxDim = bh * bw;
	for (int y = 0; y < bh && !getAbortState(); ++y) {
		for (int x = 0; x < bw && !getAbortState(); ++x) {
			// check if the pixel has intensity below threshhold
			if (bmpData[y * bw + x].intensity() < 15) {
				Vector2 p(x + .5f, y + .5f);
				Vector2 roVec = p - center;
				ro = roVec.length() / hs; // this is the length to the segment - ro
				thetaRad = atan2(roVec.y, roVec.x);
				aps.draw(pc, DrawFlags::DF_ACCUMULATE);
			}
			if (cb) {
				cb->setPercentDone(y * bw + x, maxDim);
			}
		}
	}
	// directly set the output because it will be destroyed after this function exits
	if (oman && !getAbortState()) {
		const Pixelmap<uint64>& pmp = aps.getBitmap();
		Bitmap bmpOut(pmp.getWidth(), pmp.getHeight());
		const int dim = pmp.getWidth() * pmp.getHeight();
		const uint64 * pmpData = pmp.getDataPtr();
		uint64 maxValue = 1;
		for (int i = 0; i < dim; ++i) {
			maxValue = std::max(maxValue, pmpData[i]);
		}
		Color * bmpOutData = bmpOut.getDataPtr();
		for (int i = 0; i < dim; ++i) {
			const uint8 c = static_cast<uint8>((pmpData[i] * 255) / maxValue);
			bmpOutData[i] = Color(c, c, c);
		}
		// so the maximum value has to be mapped to 255 -> multiply by 255 and divide by the max value
		oman->setOutput(bmpOut, 1); // the zero is important
	}
	return KernelBase::KPR_OK;
}

KernelBase::ProcessResult RotationKernel::kernelImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const int cx = bw / 2;
	const int cy = bh / 2;
	float angle = 0.0f;
	if (pman) {
		if (pman->getFloatParam(angle, "angle")) {
			angle = toRadians(angle);
		}
	}
	const Matrix2 rot(rotationMatrix(angle));
	const Matrix2 invRot(transpose(rot)); // since this is rotation, the inverse is the transposed matrix
	Vector2 edges[4] = {
		Vector2(-cx + 0.5f, -cy + 0.5f),
		Vector2(cx + (bw & 1 ? 0.5f : -0.5f), -cy + .5f),
		Vector2(cx + (bw & 1 ? 0.5f : -0.5f), cy + (bh & 1 ? 0.5f : -0.5f)),
		Vector2(-cx + 0.5f, cy + (bh & 1 ? 0.5f : -0.5f))
	};
	float minX = 0.0f;
	float maxX = 0.0f;
	float minY = 0.0f;
	float maxY = 0.0f;
	for (int i = 0; i < 4; ++i) {
		edges[i] = rot * edges[i]; // rotate the edge to get where it will be mapped
		minX = std::min(edges[i].x, minX);
		maxX = std::max(edges[i].x, maxX);
		minY = std::min(edges[i].y, minY);
		maxY = std::max(edges[i].y, maxY);
	}
	const int obw = static_cast<int>(ceilf(maxX) - floorf(minX));
	const int obh = static_cast<int>(ceilf(maxY) - floorf(minY));
	Bitmap bmpOut(obw, obh);
	Color * bmpData = bmpOut.getDataPtr();
	const int ocx = obw / 2;
	const int ocy = obh / 2;
	bool tile = false;
	if (pman) {
		pman->getBoolParam(tile, "tile");
	}
	for (int y = 0; y < obh && !getAbortState(); ++y) {
		for (int x = 0; x < obw && !getAbortState(); ++x) {
			const Vector2 origin = invRot * Vector2(x - ocx + 0.5f, y - ocy + 0.5f);
			bmpData[y * obw + x] = bmp.getFilteredPixel(origin.x + cx, origin.y + cy, tile);
		}
	}
	if (oman) {
		oman->setOutput(bmpOut, 1);
	}
	return KernelBase::KPR_OK;
}

KernelBase::ProcessResult HistogramKernel::kernelImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setKernelName("Histogram");
	}
	int outWidth = 512;
	int outHeight = 255;
	int smooths = 1;
	std::string vecStr;
	if (pman) {
		pman->getIntParam(outWidth, "width");
		pman->getIntParam(outHeight, "height");
		pman->getIntParam(smooths, "smooths");
		pman->getStringParam(vecStr, "vector");
	}
	const int histCount = smooths + 1;
	if (histCount <= 0) {
		return KPR_INVALID_INPUT;
	}
	Bitmap bmpOut(outWidth, outHeight);
	if (!bmpOut.isOK()) {
		return KPR_INVALID_INPUT;
	}
	std::vector<std::string> convVecStr = splitString(vecStr.c_str(), ';');
	std::vector<float> convVec(convVecStr.size());
	for (int i = 0; i < convVecStr.size(); ++i) {
		convVec[i] = atof(convVecStr[i].c_str());
	}
	const int subHeight = outHeight / histCount - 1;
	Histogram<HDL_CHANNEL> hist(bmp);
	std::vector<uint32> intensityChannel = hist.getChannel(HistogramChannel::HC_INTENSITY);
	Bitmap subHisto(outWidth, subHeight);
	const int maxIntensity = hist.getMaxIntensity();
	std::vector<uint32> prevConvolution = intensityChannel;
	drawIntensityHisto(subHisto, prevConvolution, maxIntensity);
	bmpOut.drawBitmap(subHisto, 0, 0);
	for (int i = 1; i < histCount; ++i) {
		std::vector<uint32> convoluted = convolute(prevConvolution, convVec);
		drawIntensityHisto(subHisto, convoluted, maxIntensity);
		bmpOut.drawBitmap(subHisto, 0, i * (subHeight + 1));
		prevConvolution = convoluted;
	}
	if (oman) {
		oman->setOutput(bmpOut, 1);
	}
	return KPR_OK;
}

void HistogramKernel::drawIntensityHisto(Bitmap & histBmp, const std::vector<uint32>& data, const uint32 maxIntensity) const {
	const Color fillColor = Color(0x70, 0x70, 0x70);
	const Color bkgColor = Color(0x0f, 0x0f, 0x0f);
	const Color maxColor = Color(0xa0, 0x0f, 0x0f);
	const Color minColor = Color(0x0f, 0x0f, 0xa0);
	const std::vector<Extremum> extremums = findExtremums(data);
	auto getColor = [=](int x, const std::vector<Extremum>& ext) -> Color {
		const int extSize = ext.size();
		Color retval = fillColor;
		for (int i = 0; i < extSize; ++i) {
			if (ext[i].start > x) {
				break;
			} else if (ext[i].start <= x && x < ext[i].end) {
				retval = (ext[i].d < 0 ? minColor : maxColor); // assign the proper color based on the extremum type
				break;
			}
		}
		return retval;
	};
	const int ihh = histBmp.getHeight();
	const int ihw = histBmp.getWidth();
	Color * ihData = histBmp.getDataPtr();
	const float widthRatioRecip = data.size() / float(ihw);
	for (int y = 0; y < ihh; ++y) {
		for (int x = 0; x < ihw; ++x) {
			const int xh = int(x * widthRatioRecip);
			const bool t = (ihh - y <= data[xh] * ihh / maxIntensity);
			ihData[y * ihw + x] = (t ? getColor(xh, extremums) : bkgColor);
		}
	}
}

KernelBase::ProcessResult ThresholdKernel::kernelImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setKernelName("Threshold");
	}
	const int w = bmp.getWidth();
	const int h = bmp.getHeight();
	int lower = 0;
	int upper = 0;
	if (pman) {
		pman->getIntParam(lower, "lower");
		pman->getIntParam(upper, "upper");
	}
	Bitmap bmpOut(w, h);
	const Color* inData = bmp.getDataPtr();
	Color* outData = bmpOut.getDataPtr();
	const int n = w * h;
	for (int i = 0; i < n; ++i) {
		const int intensity = inData[i].intensity();
		outData[i] = (lower <= intensity && intensity <= upper ? inData[i] : Color());
	}
	if (oman) {
		oman->setOutput(bmpOut, bmpId);
	}
	return KPR_OK;
}

KernelBase::ProcessResult FilterKernel::kernelImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setKernelName("Filter");
	}
	const float blurKernel[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	ConvolutionKernel k(blurKernel, 3);
	bool normalize = true;
	if (pman) {
		pman->getCKernelParam(k, "kernel");
		pman->getBoolParam(normalize, "normalize");
	}
	Bitmap out = convolute(bmp, k, normalize);
	if (oman) {
		oman->setOutput(out, bmpId);
	}
	return KPR_OK;
}
