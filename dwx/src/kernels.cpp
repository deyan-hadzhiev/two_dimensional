#include <memory>
#include <vector>
#include <utility>
#include <algorithm>
#include "kernels.h"
#include "vector2.h"
#include "matrix2.h"

KernelBase::ProcessResult SimpleKernel::runKernel(unsigned flags) {
	const bool hasInput = getInput();
	KernelBase::ProcessResult retval;
	if (hasInput && bmp.isOK()) {
		getParameters();
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
		if (k->state == State::AKS_FINISHED || k->state == State::AKS_INIT)
			k->ev.wait(lk);
		if (k->state == State::AKS_TERMINATED) {
			break;
		}
		k->state = State::AKS_RUNNING;
		k->kernelImplementation(0);
		// be carefull not to change the state!!!
		if (k->state == State::AKS_RUNNING) {
			k->state = State::AKS_FINISHED;
		}
	}
}

AsyncKernel::AsyncKernel()
	: state(State::AKS_INIT)
	, loopThread(kernelLoop, this)
{
}

AsyncKernel::~AsyncKernel() {
	state = State::AKS_TERMINATED;
	ev.notify_one();
	loopThread.join();
}

AsyncKernel::State AsyncKernel::getState() const {
	return state;
}

KernelBase::ProcessResult AsyncKernel::runKernel(unsigned flags) {
	bool updated = false;
	std::lock_guard<std::mutex> lk(kernelMutex);
	const bool hasInput = getInput();
	if (hasInput && bmp.isOK()) {
		getParameters();
		state = State::AKS_DIRTY;
		updated = true;
	}
	if (updated)
		ev.notify_one();
	return (updated ? KPR_RUNNING : KPR_INVALID_INPUT);
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
	const auto& tVal = paramValues.find("threshold");
	if (tVal != paramValues.end()) {
		threshold = atoi(tVal->second.c_str());
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
	if (dirtySize) {
		primitive->resize(width, height);
		dirtySize = false;
	}
	getParameters();
	for (auto it = paramValues.cbegin(); it != paramValues.cend(); ++it) {
		if (!it->second.empty()) {
			primitive->setParam(it->first, it->second);
		}
	}
	primitive->draw(col, flags);
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
	for (int y = 0; y < bh && state != State::AKS_TERMINATED; ++y) {
		for (int x = 0; x < bw && state != State::AKS_TERMINATED; ++x) {
			// check if the pixel has intensity below threshhold
			if (bmpData[y * bw + x].intensity() < 15) {
				Vector2 p(x + .5f, y + .5f);
				Vector2 roVec = p - center;
				ro = roVec.length() / hs; // this is the length to the segment - ro
				thetaRad = atan2(roVec.y, roVec.x);
				aps.draw(pc, DrawFlags::DF_ACCUMULATE);
			}
		}
	}
	// directly set the output because it will be destroyed after this function exits
	if (oman && state != State::AKS_TERMINATED) {
		const Pixelmap<uint64>& pmp = aps.getBitmap();
		Bitmap bmpOut(pmp.getWidth(), pmp.getHeight());
		const int dim = pmp.getWidth() * pmp.getHeight();
		const uint64 * pmpData = pmp.getDataPtr();
		uint64 maxValue = 0;
		for (int i = 0; i < dim; ++i) {
			maxValue = std::max(maxValue, pmpData[i]);
		}
		Color * bmpOutData = bmpOut.getDataPtr();
		for (int i = 0; i < dim; ++i) {
			const uint8 c = static_cast<uint8>((pmpData[i] * 255) / maxValue);
			bmpOutData[i] = Color(c, c, c);
		}
		// so the maximum value has to be mapped to 255 -> multiply by 255 and divide by the max value
		oman->setOutput(bmpOut, 0); // the zero is important
	}
	return KernelBase::KPR_OK;
}
// just overriden so we don't output our input bmp
void HoughKernel::setOutput() const {}

KernelBase::ProcessResult RotationKernel::kernelImplementation(unsigned flags) {
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const int cx = bw / 2;
	const int cy = bh / 2;
	float angle = 0.0f;
	if (pman) {
		pman->getFloatParam(angle, "angle");
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
	for (int y = 0; y < obh; ++y) {
		for (int x = 0; x < obw; ++x) {
			const Vector2 origin = invRot * Vector2(x - ocx + 0.5f, y - ocy + 0.5f);
			bmpData[y * obw + x] = bmp.getFilteredPixel(origin.x + cx, origin.y + cy);
		}
	}
	if (oman) {
		oman->setOutput(bmpOut, 0);
	}
	return KernelBase::KPR_OK;
}

void RotationKernel::setOutput() const
{}
