#include <memory>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <random>
#include <time.h>

#include "util.h"
#include "arithmetic.h"
#include "modules.h"
#include "progress.h"
#include "vector2.h"
#include "matrix2.h"
#include "dcomplex.h"
#include "convolution.h"
#include "fft_butterfly.h"
#include "kmeans.h"

ModuleBase::ProcessResult SimpleModule::runModule(unsigned flags) {
	const bool hasInput = getInput();
	ModuleBase::ProcessResult retval;
	if (hasInput && bmp.isOK()) {
		if (cb)
			cb->reset();
		const auto start = std::chrono::steady_clock::now();
		retval = moduleImplementation(flags);
		const auto end = std::chrono::steady_clock::now();
		if (cb) {
			const int64 duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
			cb->setDuration(duration);
		}
		if (ModuleBase::KPR_OK == retval) {
			setOutput();
			if (iman)
				iman->moduleDone(retval);
		}
	} else {
		retval = ModuleBase::KPR_INVALID_INPUT;
	}
	return retval;
}

void AsyncModule::moduleLoop(AsyncModule * k) {
	while(k->state != State::AKS_TERMINATED) {
		std::unique_lock<std::mutex> lk(k->moduleMutex);
		if (k->state == State::AKS_FINISHED || k->state == State::AKS_INIT) {
			k->ev.wait(lk);
		}
		if (k->state != State::AKS_TERMINATED) {
			// if started from dirty state directly change to running
			State dirty = State::AKS_DIRTY;
			k->state.compare_exchange_weak(dirty, State::AKS_RUNNING);
			const auto start = std::chrono::steady_clock::now();
			k->moduleImplementation(0);
			const auto end = std::chrono::steady_clock::now();
			if (k->cb) {
				const int64 duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
				k->cb->setDuration(duration);
			}
			// be carefull not to change the state!!!
			State fin = State::AKS_RUNNING; // expected state
			k->state.compare_exchange_weak(fin, State::AKS_FINISHED);
		}
	}
}

bool AsyncModule::getAbortState() const {
	return
		state == State::AKS_TERMINATED ||
		state == State::AKS_DIRTY ||
		(nullptr != cb && cb->getAbortFlag());
}

AsyncModule::AsyncModule()
	: state(State::AKS_INIT)
	, loopThread(moduleLoop, this)
{}

AsyncModule::~AsyncModule() {
	cb->setAbortFlag();
	state = State::AKS_TERMINATED;
	ev.notify_one();
	loopThread.join();
}

AsyncModule::State AsyncModule::getState() const {
	return state;
}

void AsyncModule::update() {
	state = State::AKS_DIRTY;
	ev.notify_one();
}

ModuleBase::ProcessResult AsyncModule::runModule(unsigned flags) {
	if (cb)
		cb->reset();
	update();
	return KPR_RUNNING;
}

ModuleBase::ProcessResult IdentityModule::moduleImplementation(unsigned flags) {
	return ModuleBase::KPR_OK;
}

ModuleBase::ProcessResult NegativeModule::moduleImplementation(unsigned flags) {
	auto negativePix = [](Color a) -> Color {
		return Color(255, 255, 255) - a;
	};
	bmp.remap(negativePix);
	return ModuleBase::KPR_OK;
}

IntervalList TextSegmentationModule::extractIntervals(const int * accumValues, const int count, int threshold) {
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

ModuleBase::ProcessResult TextSegmentationModule::moduleImplementation(unsigned flags) {
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
	return ModuleBase::KPR_OK;
}

void GeometricModule::setSize(int _width, int _height) {
	if (width != _width || height != _height) {
		width = _width;
		height = _height;
		dirtySize = true;
	}
}

void GeometricModule::setColor(Color rgb) {
	col = rgb;
	primitive->getPen().setColor(col);
}

ModuleBase::ProcessResult GeometricModule::moduleImplementation(unsigned flags) {
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
	std::string color;
	pman->getStringParam(color, "color");
	const uint32 colValue = strtoul(color.c_str(), NULL, 16);
	setColor(Color(colValue));
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
	primitive->draw(dflags);
	bmp = primitive->getBitmap();
	SimpleModule::setOutput();
	if (iman)
		iman->moduleDone(KPR_OK);
	return KPR_OK;
}

SinosoidModule::SinosoidModule()
	: GeometricModule(new Sinosoid<Color>)
{}

void reportExpressionError(Bitmap& bmp, const ExpressionParseError& parseError, const std::string& expression) {
	// TODO - implement PixelmapStringWriter class to handle all string draws gracefully
	const Color errorCol = Color(0xdd3932);
	const int bmpWidth = bmp.getWidth();
	const int bmpHeight = bmp.getHeight();
	const std::string errorStr = std::string("ERROR: ") + parseError.getErrorString();
	int errorStrWidth = 0;
	int errorStrHeight = 0;
	bmp.getTextExtent(errorStr.c_str(), errorStrWidth, errorStrHeight);
	const int borders = 5;
	int expressionWidth = 0;
	int expressionHeight = 0;
	bmp.getTextExtent(expression.c_str(), expressionWidth, expressionHeight);
	// check if the expression would fit in a single line
	// TODO implement multiline expression
	if (true || expressionWidth < bmpWidth - borders * 2) {
		const int errorX = (bmpWidth - errorStrWidth) / 2;
		const int errorY = bmpHeight / 2 - errorStrHeight - 1;
		bmp.drawString(errorStr.c_str(), errorX, errorY, errorCol);
		const int exprX = (bmpWidth - expressionWidth) / 2;
		const int exprY = bmpHeight / 2 + 1;
		bmp.drawString(expression.c_str(), exprX, exprY, Color(192, 192, 192));
		// if there is exact location - draw it with error color
		if (parseError.position >= 0) {
			// get the width of the text prior to the position
			int correctExprWidth = 0;
			int correctExprHeight = 0;
			bmp.getTextExtent(expression.c_str(), correctExprWidth, correctExprHeight, parseError.position);
			bmp.drawString(expression.c_str() + parseError.position, exprX + correctExprWidth, exprY, errorCol, parseError.length);
		}
	} else {
		// TODO
	}
}

ModuleBase::ProcessResult FunctionRasterModule::moduleImplementation(unsigned flags) {
	if (width <= 0 || height <= 0) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Function Raster");
		cb->setPercentDone(0, 1);
	}
	const int pWidth = width;
	const int pHeight = height;
	pman->getIntParam(width, "width");
	pman->getIntParam(height, "height");
	bool dirtySize = (width != pWidth || height != pHeight);
	if (dirtySize || !bmp.isOK()) {
		raster->resize(width, height);
		dirtySize = false;
	}

	Vector2 scale(1.0f, 1.0f);
	pman->getVectorParam(scale, "scale");
	raster->setScale(scale);

	Vector2 offset(0.0f, 0.0f);
	pman->getVectorParam(offset, "offset");
	raster->setOffset(offset);

	bool additive, clear, axis;
	pman->getBoolParam(additive, "additive");
	pman->getBoolParam(clear, "clear");
	pman->getBoolParam(axis, "axis");
	unsigned dflags =
		(additive ? DrawFlags::DF_ACCUMULATE : 0) |
		(clear ? DrawFlags::DF_CLEAR : 0) |
		(axis ? DrawFlags::DF_SHOW_AXIS : 0);

	Color penColor;
	pman->getColorParam(penColor, "color");
	float penWidth = 1.0;
	pman->getFloatParam(penWidth, "penWidth");
	float penStrenth = 0.5;
	pman->getFloatParam(penStrenth, "penStrength");
	const DrawPen<Color> pen(penColor, penWidth, penStrenth);
	raster->setPen(pen);

	std::string function;
	pman->getStringParam(function, "function");
	std::vector<std::string> functionList = splitString(function.c_str(), ';');

	raster->setProgressCallback(cb);
	BinaryExpressionEvaluator bee;
	auto evalFunction = [&bee](double x, double y) -> double {
		return bee.eval(EvaluationContext(x, y, 0));
	};
	raster->setFunction(evalFunction);

	std::pair<ExpressionParseError, std::string> parseError;
	unsigned drawFlags = dflags;
	const int functionCount = static_cast<int>(functionList.size());
	for (int i = 0; i < functionCount; ++i) {
		ExpressionTree functionTree;
		parseError.first = functionTree.buildTree(functionList[i]);
		if (parseError.first) {
			// save the expression so we could use it for the error reporting
			parseError.second = functionTree.getExpression();
			break;
		}
		bee = functionTree.getBinaryEvaluator();

		raster->draw(drawFlags);
		// after the first run, reset some of the draw flags
		// generally remove the axis flag and the clear flag
		drawFlags = dflags & DrawFlags::DF_ACCUMULATE;
	}
	// check for errors
	if (!parseError.first) {
		bmp = raster->getBitmap();
	} else {
		bmp.generateEmptyImage(width, height);
		reportExpressionError(bmp, parseError.first, parseError.second);
	}

	SimpleModule::setOutput();
	if (iman)
		iman->moduleDone(KPR_OK);
	return KPR_OK;
}

ModuleBase::ProcessResult FineFunctionRasterModule::moduleImplementation(unsigned flags) {
	if (width <= 0 || height <= 0) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Fine Function Raster");
		cb->setPercentDone(0, 1);
	}
	const int pWidth = width;
	const int pHeight = height;
	pman->getIntParam(width, "width");
	pman->getIntParam(height, "height");
	bool dirtySize = (width != pWidth || height != pHeight);
	if (dirtySize || !bmp.isOK()) {
		raster->resize(width, height);
		dirtySize = false;
	}

	Vector2 scale(1.0f, 1.0f);
	pman->getVectorParam(scale, "scale");
	raster->setScale(scale);

	Vector2 offset(0.0f, 0.0f);
	pman->getVectorParam(offset, "offset");
	raster->setOffset(offset);

	bool additive, clear, axis;
	pman->getBoolParam(additive, "additive");
	pman->getBoolParam(clear, "clear");
	pman->getBoolParam(axis, "axis");
	const unsigned dflags =
		(additive ? DrawFlags::DF_ACCUMULATE : 0) |
		(clear ? DrawFlags::DF_CLEAR : 0) |
		(axis ? DrawFlags::DF_SHOW_AXIS : 0);

	Color penColor;
	pman->getColorParam(penColor, "color");
	float penWidth = 1.0;
	pman->getFloatParam(penWidth, "penWidth");
	float penStrenth = 0.5;
	pman->getFloatParam(penStrenth, "penStrength");
	const DrawPen<Color> pen(penColor, penWidth, penStrenth);
	raster->setPen(pen);

	bool treeOutput = false;
	pman->getBoolParam(treeOutput, "outputTree");
	raster->setTreeOutput(treeOutput);

	std::string function;
	pman->getStringParam(function, "function");
	std::vector<std::string> functionList = splitString(function.c_str(), ';');

	BinaryExpressionEvaluator bee;
	auto evalFunction = [&bee](double x, double y) -> double {
		return bee.eval(EvaluationContext(x, y, 0));
	};
	raster->setFunction(evalFunction);

	raster->setProgressCallback(cb);

	std::pair<ExpressionParseError, std::string> parseError;
	unsigned drawFlags = dflags;
	const int functionCount = static_cast<int>(functionList.size());
	for (int i = 0; i < functionCount; ++i) {
		ExpressionTree functionTree;
		parseError.first = functionTree.buildTree(functionList[i]);
		if (parseError.first) {
			// save the expression so we could use it for the error reporting
			parseError.second = functionTree.getExpression();
			break;
		}
		bee = functionTree.getBinaryEvaluator();

		raster->draw(drawFlags);
		// after the first run, reset some of the draw flags
		// generally remove the axis flag and the clear flag
		drawFlags = dflags & DrawFlags::DF_ACCUMULATE;
	}
	// check for errors
	if (!parseError.first) {
		bmp = raster->getBitmap();
	} else {
		bmp.generateEmptyImage(width, height);
		reportExpressionError(bmp, parseError.first, parseError.second);
	}

	SimpleModule::setOutput();
	if (iman)
		iman->moduleDone(KPR_OK);
	return KPR_OK;
}

template<class RandGen, class DistWidth, class DistHeight>
class RandomBmpSampler {
public:
	RandomBmpSampler(RandGen seed, DistWidth dw, DistHeight dh)
		: rGen(seed)
		, distWidth(dw)
		, distHeight(dh)
	{}

	void sample(Pixelmap<TColor<double> >& output, const TColor<double> c, const int64 samples, ProgressCallback * cb) {
		if (!output.isOK() || samples <= 0) {
			return;
		}
		const int width = output.getWidth();
		const int height = output.getHeight();
		const int dimProd = width * height;
		TColor<double> * outData = output.getDataPtr();
		if (cb) {
			for (int64 i = 0; i < samples && !cb->getAbortFlag(); ++i) {
				const int x = static_cast<int>(std::round(distWidth(rGen)));
				const int y = static_cast<int>(std::round(distHeight(rGen)));
				if (x >= 0 && x < width && y >= 0 && y < height) {
					outData[y * width + x] += c;
				}
				cb->setPercentDone(i, samples);
			}
		} else {
			for (int64 i = 0; i < samples; ++i) {
				const int x = static_cast<int>(std::round(distWidth(rGen)));
				const int y = static_cast<int>(std::round(distHeight(rGen)));
				if (x >= 0 && x < width && y >= 0 && y < height) {
					outData[y * width + x] += c;
				}
			}
		}
	}
private:
	RandGen rGen;
	DistWidth distWidth;
	DistHeight distHeight;
};

ModuleBase::ProcessResult RandomNoiseModule::moduleImplementation(unsigned flags) {
	if (cb) {
		cb->setModuleName("Fine Function Raster");
		cb->setPercentDone(0, 1);
	}
	int bmpWidth = 1024;
	int bmpHeight = 1024;
	int64 samples = 1000000;
	int gradient = 2;
	float mx = 0.0;
	float sx = 1.0;
	float my = 0.0;
	float sy = 1.0;
	Color background(0x000000);
	Color sampleColor(0xffffff);
	unsigned randEngine = 0;
	unsigned distribution = 0;
	if (pman) {
		pman->getIntParam(bmpWidth, "width");
		pman->getIntParam(bmpHeight, "height");
		pman->getInt64Param(samples, "samples");
		pman->getIntParam(gradient, "gradient");
		pman->getFloatParam(mx, "mx");
		pman->getFloatParam(sx, "sx");
		pman->getFloatParam(my, "my");
		pman->getFloatParam(sy, "sy");
		pman->getColorParam(background, "background");
		pman->getColorParam(sampleColor, "sampleColor");
		pman->getEnumParam(randEngine, "randEngine");
		pman->getEnumParam(distribution, "distribution");
		if (gradient < 1) {
			gradient = 1;
		}
	}
	const TColor<double> bkgColor(background);
	const TColor<double> fgColor(sampleColor);
	const TColor<double> diffColor = (fgColor - bkgColor) / static_cast<double>(gradient);
	Pixelmap<TColor<double> > sampledBmp(bmpWidth, bmpHeight);
	sampledBmp.fill(bkgColor);
	const int dimProd = bmpWidth * bmpHeight;
	if (randEngine == RE_C_RAND) {
		// c-rand
		srand(static_cast<uint32>(time(NULL)));
		TColor<double> * sampleData = sampledBmp.getDataPtr();
		for (int64 i = 0; i < samples && (!cb || !cb->getAbortFlag()); ++i) {
			const int x = rand() % bmpWidth;
			const int y = rand() % bmpHeight;
			sampleData[y * bmpWidth + x] += diffColor;
			if (cb) {
				cb->setPercentDone(i, samples);
			}
		}
	} else {
		std::random_device rDev;
		std::seed_seq seed{ rDev(), rDev(), rDev(), rDev(), rDev(), rDev(), rDev(), rDev() };
		if (distribution == D_UNIFORM) {
			std::uniform_int_distribution<int> dWidth(static_cast<int>(mx), static_cast<int>(sx));
			std::uniform_int_distribution<int> dHeight(static_cast<int>(my), static_cast<int>(sy));
			if (randEngine == RE_LINEAR_CONGRUENTIAL_GEN) {
				std::minstd_rand re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_MERSENNE_TWISTER) {
				std::mt19937 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_RANLUX) {
				std::ranlux24 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_KNUTH_B) {
				std::knuth_b re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			}
		} else if (distribution == D_NORMAL) {
			std::normal_distribution<double> dWidth(mx, sx);
			std::normal_distribution<double> dHeight(my, sy);
			if (randEngine == RE_LINEAR_CONGRUENTIAL_GEN) {
				std::minstd_rand re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_MERSENNE_TWISTER) {
				std::mt19937 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_RANLUX) {
				std::ranlux24 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_KNUTH_B) {
				std::knuth_b re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			}
		} else if (distribution == D_CHI_SQUARED) {
			std::chi_squared_distribution<double> dWidth(mx);
			std::chi_squared_distribution<double> dHeight(my);
			if (randEngine == RE_LINEAR_CONGRUENTIAL_GEN) {
				std::minstd_rand re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_MERSENNE_TWISTER) {
				std::mt19937 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_RANLUX) {
				std::ranlux24 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_KNUTH_B) {
				std::knuth_b re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			}
		} else if (distribution == D_LOG_NORMAL) {
			std::lognormal_distribution<double> dWidth(mx, sx);
			std::lognormal_distribution<double> dHeight(my, sy);
			if (randEngine == RE_LINEAR_CONGRUENTIAL_GEN) {
				std::minstd_rand re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_MERSENNE_TWISTER) {
				std::mt19937 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_RANLUX) {
				std::ranlux24 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_KNUTH_B) {
				std::knuth_b re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			}
		} else if (distribution == D_CAUCHY) {
			std::cauchy_distribution<double> dWidth(mx, sx);
			std::cauchy_distribution<double> dHeight(my, sy);
			if (randEngine == RE_LINEAR_CONGRUENTIAL_GEN) {
				std::minstd_rand re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_MERSENNE_TWISTER) {
				std::mt19937 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_RANLUX) {
				std::ranlux24 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_KNUTH_B) {
				std::knuth_b re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			}
		} else if (distribution == D_FISHER_F) {
			std::fisher_f_distribution<double> dWidth(mx, sx);
			std::fisher_f_distribution<double> dHeight(my, sy);
			if (randEngine == RE_LINEAR_CONGRUENTIAL_GEN) {
				std::minstd_rand re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_MERSENNE_TWISTER) {
				std::mt19937 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_RANLUX) {
				std::ranlux24 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_KNUTH_B) {
				std::knuth_b re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			}
		} else if (distribution == D_STUDENT_T) {
			std::student_t_distribution<double> dWidth(mx);
			std::student_t_distribution<double> dHeight(my);
			if (randEngine == RE_LINEAR_CONGRUENTIAL_GEN) {
				std::minstd_rand re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_MERSENNE_TWISTER) {
				std::mt19937 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_RANLUX) {
				std::ranlux24 re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			} else if (randEngine == RE_KNUTH_B) {
				std::knuth_b re(seed);
				RandomBmpSampler<decltype(re), decltype(dWidth), decltype(dHeight)> s(re, dWidth, dHeight);
				s.sample(sampledBmp, diffColor, samples, cb);
			}
		}
	}
	if (cb) {
		cb->setPercentDone(1, 1);
	}
	bmp = sampledBmp;
	SimpleModule::setOutput();
	if (iman)
		iman->moduleDone(KPR_OK);
	return KPR_OK;
}

ModuleBase::ProcessResult HoughModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Hough Rho Theta");
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
	aps.setPen(pc);
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
				aps.draw(DrawFlags::DF_ACCUMULATE);
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
	return ModuleBase::KPR_OK;
}

ModuleBase::ProcessResult RotationModule::moduleImplementation(unsigned flags) {
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
	EdgeFillType edge = EdgeFillType::EFT_BLANK;
	unsigned filterType = 0;
	if (pman) {
		unsigned edgeType = 0;
		if (pman->getEnumParam(edgeType, "edge")) {
			edge = static_cast<EdgeFillType>(edgeType);
		}
		pman->getEnumParam(filterType, "filterType");
	}
	if (0 == filterType) {
		for (int y = 0; y < obh && !getAbortState(); ++y) {
			for (int x = 0; x < obw && !getAbortState(); ++x) {
				const Vector2 origin = invRot * Vector2(x - ocx + 0.5f, y - ocy + 0.5f);
				bmpData[y * obw + x] = bmp.getBilinearFilteredPixel<TColor<double> >(origin.x + cx, origin.y + cy, edge);
			}
			if (cb) {
				cb->setPercentDone(y, obh);
			}
		}
	} else if (1 == filterType) {
		for (int y = 0; y < obh && !getAbortState(); ++y) {
			for (int x = 0; x < obw && !getAbortState(); ++x) {
				const Vector2 origin = invRot * Vector2(x - ocx + 0.5f, y - ocy + 0.5f);
				bmpData[y * obw + x] = bmp.getBicubicFilteredPixel<TColor<double> >(origin.x + cx, origin.y + cy, edge);
			}
			if (cb) {
				cb->setPercentDone(y, obh);
			}
		}
	}
	if (cb) {
		cb->setPercentDone(1, 1);
	}
	if (oman) {
		oman->setOutput(bmpOut, 1);
	}
	return ModuleBase::KPR_OK;
}

ModuleBase::ProcessResult ShearModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	const int bw = bmp.getWidth();
	const int bh = bmp.getHeight();
	const int cx = bw / 2;
	const int cy = bh / 2;
	float horizontal = 0.0f;
	float vertical = 0.0f;
	if (pman) {
		pman->getFloatParam(horizontal, "horizontal");
		pman->getFloatParam(vertical, "vertical");
		// invert the vertical because of the inverse coordinate system
		vertical = -vertical;
	}
	const Matrix2 shear = Matrix2(1.0f, horizontal, 0.0f, 1.0f) * Matrix2(1.0f, 0.0f, vertical, 1.0f);
	const Matrix2 invShear(inverseMatrix(shear));
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
		edges[i] = shear * edges[i]; // rotate the edge to get where it will be mapped
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
	EdgeFillType edge = EdgeFillType::EFT_BLANK;
	unsigned filterType = 0;
	if (pman) {
		unsigned edgeType = 0;
		if (pman->getEnumParam(edgeType, "edge")) {
			edge = static_cast<EdgeFillType>(edgeType);
		}
		pman->getEnumParam(filterType, "filterType");
	}
	if (0 == filterType) {
		for (int y = 0; y < obh && !getAbortState(); ++y) {
			for (int x = 0; x < obw && !getAbortState(); ++x) {
				const Vector2 origin = invShear * Vector2(x - ocx + 0.5f, y - ocy + 0.5f);
				bmpData[y * obw + x] = bmp.getBilinearFilteredPixel<TColor<double> >(origin.x + cx, origin.y + cy, edge);
			}
			if (cb) {
				cb->setPercentDone(y, obh);
			}
		}
	} else if (1 == filterType) {
		for (int y = 0; y < obh && !getAbortState(); ++y) {
			for (int x = 0; x < obw && !getAbortState(); ++x) {
				const Vector2 origin = invShear * Vector2(x - ocx + 0.5f, y - ocy + 0.5f);
				bmpData[y * obw + x] = bmp.getBicubicFilteredPixel<TColor<double> >(origin.x + cx, origin.y + cy, edge);
			}
			if (cb) {
				cb->setPercentDone(y, obh);
			}
		}
	}
	if (cb) {
		cb->setPercentDone(1, 1);
	}
	if (oman) {
		oman->setOutput(bmpOut, 1);
	}
	return ModuleBase::KPR_OK;
}

ModuleBase::ProcessResult HistogramModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Histogram");
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
		convVec[i] = static_cast<float>(atof(convVecStr[i].c_str()));
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

void HistogramModule::drawIntensityHisto(Bitmap & histBmp, const std::vector<uint32>& data, const uint32 maxIntensity) const {
	const Color fillColor = Color(0x70, 0x70, 0x70);
	const Color bkgColor = Color(0x0f, 0x0f, 0x0f);
	const Color maxColor = Color(0xa0, 0x0f, 0x0f);
	const Color minColor = Color(0x0f, 0x0f, 0xa0);
	const std::vector<Extremum> extremums = findExtremums(data);
	auto getColor = [=](int x, const std::vector<Extremum>& ext) -> Color {
		const int extSize = static_cast<int>(ext.size());
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
			const bool t = (ihh - y <= static_cast<int>(data[xh]) * ihh / static_cast<int>(maxIntensity));
			ihData[y * ihw + x] = (t ? getColor(xh, extremums) : bkgColor);
		}
	}
}

ModuleBase::ProcessResult ThresholdModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Threshold");
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

ModuleBase::ProcessResult FilterModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Filter");
	}
	const float blurKernel[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
	ConvolutionKernel k(blurKernel, 3);
	bool normalize = true;
	float normalizationValue = 1.0f;
	if (pman) {
		pman->getCKernelParam(k, "kernel");
		pman->getBoolParam(normalize, "normalize");
		pman->getFloatParam(normalizationValue, "normalValue");
	}
	Bitmap out = convolute(bmp, k, normalize, normalizationValue, cb);
	if (oman) {
		oman->setOutput(out, bmpId);
	}
	return KPR_OK;
}

ModuleBase::ProcessResult DownScaleModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Downscale");
		cb->setPercentDone(0, 1);
	}
	int width = 0;
	int height = 0;
	unsigned meduimType = 0;
	if (pman) {
		pman->getIntParam(width, "width");
		pman->getIntParam(height, "height");
		pman->getEnumParam(meduimType, "medium");
	}
	Bitmap out;
	bool res = false;
	if (0 == meduimType) {
		res = bmp.downscale<TColor<uint16> >(out, width, height);
	} else if (1 == meduimType) {
		res = bmp.downscale<Color>(out, width, height);
	} else if (2 == meduimType) {
		res = bmp.downscale<TColor<float> >(out, width, height);
	} else if (3 == meduimType) {
		res = bmp.downscale<TColor<double> >(out, width, height);
	}
	if (cb) {
		cb->setPercentDone(1, 1);
	}
	if (res) {
		if (oman) {
			oman->setOutput(out, 1);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult UpScaleModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Upscale");
		cb->setPercentDone(0, 1);
	}
	int width = 0;
	int height = 0;
	unsigned filterType = UpscaleFiltering::UF_BILINEAR;
	if (pman) {
		pman->getIntParam(width, "width");
		pman->getIntParam(height, "height");
		pman->getEnumParam(filterType, "filterType");
	}
	Bitmap out;
	bool res = false;
	res = bmp.upscale<TColor<double> >(out, width, height, static_cast<UpscaleFiltering>(filterType));
	if (cb) {
		cb->setPercentDone(1, 1);
	}
	if (res) {
		if (oman) {
			oman->setOutput(out, 1);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult RelocateModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Relocate");
	}
	int nx = 0;
	int ny = 0;
	if (pman) {
		pman->getIntParam(nx, "relocateX");
		pman->getIntParam(ny, "relocateY");
	}
	Bitmap out;
	bool res = bmp.relocate(out, nx, ny);
	if (res) {
		if (oman) {
			oman->setOutput(out, 1);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult CropModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Crop");
	}
	int nx = 0;
	int ny = 0;
	int nw = 0;
	int nh = 0;
	if (pman) {
		pman->getIntParam(nx, "cropX");
		pman->getIntParam(ny, "cropY");
		pman->getIntParam(nw, "cropWidth");
		pman->getIntParam(nh, "cropHeight");
	}
	Bitmap out;
	bool res = bmp.crop(out, nx, ny, nw, nh);
	if (res) {
		if (oman) {
			oman->setOutput(out, 1);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult MirrorModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Mirror");
	}
	bool mx = true;
	bool my = true;
	if (pman) {
		pman->getBoolParam(mx, "mirrorX");
		pman->getBoolParam(my, "mirrorY");
	}
	unsigned axes = (mx ? PA_X_AXIS : PA_NONE) | (my ? PA_Y_AXIS : PA_NONE);
	Bitmap out;
	bool res = bmp.mirror(out, static_cast<PixelmapAxis>(axes));
	if (res) {
		if (oman) {
			oman->setOutput(out, 1);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult ExpandModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Expand");
	}
	unsigned fillType = 0;
	int ew = 0;
	int eh = 0;
	int ex = -1;
	int ey = -1;
	if (pman) {
		pman->getEnumParam(fillType, "edgeFill");
		pman->getIntParam(ew, "expandWidth");
		pman->getIntParam(eh, "expandHeight");
		pman->getIntParam(ex, "expandX");
		pman->getIntParam(ey, "expandY");
	}
	Bitmap out;
	bool res = bmp.expand(out, ew, eh, ex, ey, static_cast<EdgeFillType>(fillType));
	if (res) {
		if (oman) {
			oman->setOutput(out, 1);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult ChannelModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("Channel");
	}
	unsigned channel = 0;
	if (pman) {
		pman->getEnumParam(channel, "channel");
	}
	Bitmap out(bmp.getWidth(), bmp.getHeight());
	const int dimProd = bmp.getDimensionProduct();
	std::unique_ptr<uint8[]> channelData(new uint8[dimProd]);
	bool res = bmp.getChannel(channelData.get(), static_cast<ColorChannel>(channel));
	if (res) {
		out.setChannel(channelData.get(), static_cast<ColorChannel>(channel));
		if (oman) {
			oman->setOutput(out, bmpId);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult KMeansModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("K-Means");
		cb->setPercentDone(0, 1);
	}
	int numClasses = 3;
	int numIterations = 3;
	if (pman) {
		pman->getIntParam(numClasses, "numClasses");
		pman->getIntParam(numIterations, "numIterations");
	}
	const int bmpDims = bmp.getDimensionProduct();
	std::unique_ptr<Vector<3, double>[] > valArray(new Vector<3, double>[bmpDims]);
	const Color * bmpData = bmp.getDataPtr();
	// convert the colors to 3-dimensional vectors
	for (int i = 0; i < bmpDims; ++i) {
		const TColor<double> c = static_cast<TColor<double> >(bmpData[i]);
		valArray[i] = Vector<3, double>(c.components);
	}
	// get the class distribution and mean values
	const auto& meansRes = kMeans(valArray.get(), bmpDims, numClasses, numIterations, cb);
	if (meansRes.first.size() != 0 && meansRes.second.size() == numClasses) {
		// convert the means to colors
		std::unique_ptr<Color[]> meanColors(new Color[numClasses]);
		for (int j = 0; j < numClasses; ++j) {
			const auto& cv = meansRes.second[j];
			const TColor<double> cd(cv[0], cv[1], cv[2]);
			meanColors[j] = static_cast<Color>(cd);
		}
		Bitmap out(bmp.getWidth(), bmp.getHeight());
		Color * outData = out.getDataPtr();
		// now directly set the output values based on the class distributions
		const std::vector<int>& distribution = meansRes.first;
		for (int i = 0; i < bmpDims; ++i) {
			outData[i] = meanColors[distribution[i]];
		}
		if (cb) {
			cb->setPercentDone(1, 1);
		}
		if (oman) {
			oman->setOutput(out, bmpId);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult FFTDomainModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("FFTDomain");
		cb->setPercentDone(0, 100);
	}
	bool logScale = true;
	bool centralize = true;
	bool squareDim = false;
	bool powerOf2Flag = false;
	if (pman) {
		pman->getBoolParam(logScale, "logScale");
		pman->getBoolParam(centralize, "centralized");
		pman->getBoolParam(squareDim, "squareDimension");
		pman->getBoolParam(powerOf2Flag, "powerOf2");
	}

	const int width = bmp.getWidth();
	const int height = bmp.getHeight();

	const int fftWidth = static_cast<int>(powerOf2(static_cast<unsigned>(width)) ? width : upperPowerOf2(static_cast<unsigned>(width)));
	const int fftHeight = static_cast<int>(powerOf2(static_cast<unsigned>(height)) ? height : upperPowerOf2(static_cast<unsigned>(height)));
	// and finally take the maximum of the two as a single dimension of the fft - it needs square to apply the filter properly
	const int fftDim = std::max((powerOf2Flag ? fftWidth : width), (powerOf2Flag ? fftHeight : height));
	const int fftDimW = (squareDim ? fftDim : (powerOf2Flag ? fftWidth : width));
	const int fftDimH = (squareDim ? fftDim : (powerOf2Flag ? fftHeight : height));

	Pixelmap<TColor<Complex> > bmpComplex(bmp);
	// if the width and height for the fft differ - expand the bitmap
	if (width != fftDimW || height != fftDimH) {
		const int relocationX = (fftDimW - width) / 2;
		const int relocationY = (fftDimH - height) / 2;
		const int widthExpansion = fftDimW - width;
		const int heightExpansion = fftDimH - height;
		bmpComplex.expand(widthExpansion, heightExpansion, relocationX, relocationY, EFT_TILE);
	}
	Pixelmap<TColor<Complex> > outComplex(fftDimW, fftDimH);

	std::vector<int> dims;
	dims.push_back(fftDimH);
	dims.push_back(fftDimW);

	const FFT2D& forward = FFTCache<2>::get().getFFT(dims, false);

	if (getAbortState()) {
		return KPR_ABORTED;
	}

	const int dimProd = bmpComplex.getDimensionProduct();
	std::unique_ptr<Complex[]> inChannel(new Complex[dimProd]);
	std::unique_ptr<Complex[]> frequencyChannel(new Complex[dimProd]);

	for (int i = 0; i < ColorChannel::CC_COUNT && !getAbortState(); ++i) {
		if (cb)
			cb->setPercentDone(i, ColorChannel::CC_COUNT);

		// get the current channel
		bmpComplex.getChannel(inChannel.get(), static_cast<ColorChannel>(i));

		// run the forward fft
		forward.transform(inChannel.get(), frequencyChannel.get());

		// set the channel to the output pixelmap
		outComplex.setChannel(frequencyChannel.get(), static_cast<ColorChannel>(i));
	}

	// now remap all values to their absolute value
	outComplex.remap([](TColor<Complex> in) {
		return TColor<Complex>(
			Complex(abs(in.r), 0.0),
			Complex(abs(in.g), 0.0),
			Complex(abs(in.b), 0.0)
		);
	});

	if (logScale) {
		outComplex.remap([](TColor<Complex> in) {
			return TColor<Complex>(
				Complex(std::log(in.r.real()), 0.0),
				Complex(std::log(in.g.real()), 0.0),
				Complex(std::log(in.b.real()), 0.0)
				);
		});
	}

	if (getAbortState()) {
		return KPR_ABORTED;
	}

	// as a final step normalize all the values
	TColor<Complex> * outData = outComplex.getDataPtr();
	double minValue = Inf;
	double maxValue = 0.0;
	for (int i = 0; i < dimProd; ++i) {
		const TColor<Complex>& c = outData[i];
		for (int ci = 0; ci < 3; ++ci) {
			if (c[ci].real() > maxValue) {
				maxValue = c[ci].real();
			} else if(c[ci].real() < minValue) {
				minValue = c[ci].real();
			}
		}
	}

	const double valRange = maxValue - minValue;
	const double valRangeRecip = 1.0 / valRange;
	outComplex.remap([minValue,valRangeRecip](TColor<Complex> in) {
		return TColor<Complex>(
			Complex((in.r.real() - minValue) * valRangeRecip, 0.0),
			Complex((in.g.real() - minValue) * valRangeRecip, 0.0),
			Complex((in.b.real() - minValue) * valRangeRecip, 0.0)
			);
	});

	Bitmap out(outComplex);

	// make relocations after it is converted to standart uint8 space to save memory
	if (centralize) {
		out.relocate(out.getWidth() / 2, out.getHeight() / 2);
	}

	if (cb)
		cb->setPercentDone(1, 1);

	bool res = true;
	if (res) {
		if (oman) {
			oman->setOutput(out, bmpId);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult FFTCompressionModule::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("FFTCompression");
	}
	float percent = 100.0f;
	if (pman) {
		pman->getFloatParam(percent, "compressPercent");
	}
	if (percent < 0.0f || percent > 100.0f) {
		return KPR_INVALID_INPUT;
	}
	const int width = bmp.getWidth();
	const int height = bmp.getHeight();

	const float ratio = sqrtf(percent / 100.0f);
	const int compWidth = static_cast<int>(ceilf(width * ratio));
	const int compHeight = static_cast<int>(ceilf(height * ratio));

	Pixelmap<TColor<Complex> > bmpComplex(bmp);
	Pixelmap<TColor<Complex> > bmpCompressed(width, height);
	Pixelmap<TColor<Complex> > outComplex(width, height);

	std::vector<int> dims;
	dims.push_back(height);
	dims.push_back(width);

	const FFT2D& forward = FFTCache<2>::get().getFFT(dims, false);
	const FFT2D& inverse = FFTCache<2>::get().getFFT(dims, true);

	if (getAbortState()) {
		return KPR_ABORTED;
	}

	const int dimProd = bmpComplex.getDimensionProduct();
	std::unique_ptr<Complex[]> fftInChannel(new Complex[dimProd]);
	std::unique_ptr<Complex[]> fftOutChannel(new Complex[dimProd]);

	for (int i = 0; i < ColorChannel::CC_COUNT && !getAbortState(); ++i) {
		if (cb)
			cb->setPercentDone(i, 2 * ColorChannel::CC_COUNT);

		// get the current channel
		bmpComplex.getChannel(fftInChannel.get(), static_cast<ColorChannel>(i));

		// run the forward fft
		forward.transform(fftInChannel.get(), fftOutChannel.get());

		// set the channel to the compressed pixelmap
		bmpCompressed.setChannel(fftOutChannel.get(), static_cast<ColorChannel>(i));
	}

	// compute the x and y coordinates that will mark the zoroing to simulate compression
	const int compWidthRemainder = width - compWidth;
	const int compHeightRemainder = height - compHeight;
	const int compX = (width - compWidthRemainder) / 2;
	const int compY = (height - compHeightRemainder) / 2;
	// now zero out all pixel which are in the two strips
	TColor<Complex> * compData = bmpCompressed.getDataPtr();
	for (int y = 0; y < height; ++y) {
		// if this row is in the cropped area - zero the whole row
		if (y > compY && y < compY + compHeightRemainder) {
			memset(compData + y * width, 0, width * sizeof(TColor<Complex>));
		} else {
			// else go through the columns
			for (int x = 0; x < width; ++x) {
				if (x > compX && x < compX + compWidthRemainder) {
					compData[y * width + x] = TColor<Complex>();
				}
			}
		}
	}

	// now after the compression is simulated - make the inverse transform over the compressed pixelmap
	for (int i = 0; i < ColorChannel::CC_COUNT && !getAbortState(); ++i) {
		if (cb)
			cb->setPercentDone(ColorChannel::CC_COUNT + i, 2 * ColorChannel::CC_COUNT);

		// get the current channel
		bmpCompressed.getChannel(fftInChannel.get(), static_cast<ColorChannel>(i));

		// run the inverse fft - fftOutChannel is already initialized
		inverse.transform(fftInChannel.get(), fftOutChannel.get());

		// set the channel to the output pixelmap and use it before the actual compress
		outComplex.setChannel(fftOutChannel.get(), static_cast<ColorChannel>(i));
	}

	// noramlize the output since it will be with scaled values
	const double normFactor = 1.0 / dimProd;
	outComplex.remap([normFactor](TColor<Complex> in) {
		return in * normFactor;
	});

	Bitmap out(outComplex);

	if (cb)
		cb->setPercentDone(1, 1);

	bool res = true;
	if (res) {
		if (oman) {
			oman->setOutput(out, bmpId);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}

ModuleBase::ProcessResult FFTFilter::moduleImplementation(unsigned flags) {
	const bool inputOk = getInput();
	if (!inputOk || !bmp.isOK()) {
		return KPR_INVALID_INPUT;
	}
	if (cb) {
		cb->setModuleName("FFTFilter");
	}
	ConvolutionKernel ck;
	bool normalizeKernel;
	float normalizationValue = 1.0f;
	if (pman) {
		pman->getCKernelParam(ck, "kernelFFT");
		pman->getBoolParam(normalizeKernel, "normalizeKernel");
		pman->getFloatParam(normalizationValue, "normalizationValue");
	}

	if (normalizeKernel) {
		ck.normalize(normalizationValue);
	}

	const int width = bmp.getWidth();
	const int height = bmp.getHeight();

	const int ckSide = ck.getSide();
	Pixelmap<Complex> filterMap(ckSide, ckSide);
	Pixelmap<Complex> filterFullMap(width, height); //!< will be as big as the image
	Pixelmap<TColor<Complex> > bmpComplex(bmp);
	Pixelmap<TColor<Complex> > outComplex(width, height);

	std::vector<int> dims;
	dims.push_back(height);
	dims.push_back(width);

	const int dimProd = bmpComplex.getDimensionProduct();
	const FFT2D& forward = FFTCache<2>::get().getFFT(dims, false);
	const FFT2D& inverse = FFTCache<2>::get().getFFT(dims, true);

	if (getAbortState()) {
		return KPR_ABORTED;
	}

	const int ckSquared = ckSide * ckSide;
	const float * kernelData = ck.getDataPtr();
	Complex * filterData = filterMap.getDataPtr();
	// initialize the filter and transform it to the frequency domain
	for (int i = 0; i < ckSquared; ++i) {
		filterData[i] = Complex(kernelData[i], 0.0);
	}
	// now we have to map the small filter to the big filter map
	filterFullMap.drawBitmap(filterMap, 0, 0);
	// and finally relocate the filter in such a way that the center is in (0, 0)
	filterFullMap.relocate(width - ckSide / 2, height - ckSide / 2);

	std::unique_ptr<Complex[]> filterFreq(new Complex[dimProd]);
	forward.transform(filterFullMap.getDataPtr(), filterFreq.get());

	// allocate operating buffers for the pixelmap channels
	std::unique_ptr<Complex[]> fftInChannel(new Complex[dimProd]); //!< the input channel for the fft
	std::unique_ptr<Complex[]> fftIntermediate(new Complex[dimProd]); //!< intermediate channel in frequency domain (filter is applied to it)
	std::unique_ptr<Complex[]> fftOutChannel(new Complex[dimProd]); //!< the output channle from the inverse fft

	// now run the filter over all the channels of the pixelmap
	for (int i = 0; i < ColorChannel::CC_COUNT && !getAbortState(); ++i) {
		if (cb)
			cb->setPercentDone(i * 3, 3 * ColorChannel::CC_COUNT);

		// get the current channel
		bmpComplex.getChannel(fftInChannel.get(), static_cast<ColorChannel>(i));

		// run the forward fft
		forward.transform(fftInChannel.get(), fftIntermediate.get());

		// now apply the filter
		for (int j = 0; j < dimProd; ++j) {
			fftIntermediate[j] = filterFreq[j] * fftIntermediate[j];
		}

		if (cb)
			cb->setPercentDone(i * 3 + 1, 3 * ColorChannel::CC_COUNT);

		// now inverse the channel back to the pixel domain
		inverse.transform(fftIntermediate.get(), fftOutChannel.get());

		// set the channel to the final output pixelmap
		outComplex.setChannel(fftOutChannel.get(), static_cast<ColorChannel>(i));

		if (cb)
			cb->setPercentDone(i * 3 + 2, 3 * ColorChannel::CC_COUNT);
	}

	// noramlize the output since it will be with scaled values
	const double normFactor = 1.0 / dimProd;
	outComplex.remap([normFactor](TColor<Complex> in) {
		return in * normFactor;
	});

	Bitmap out(outComplex);

	if (cb)
		cb->setPercentDone(1, 1);

	bool res = true;
	if (res) {
		if (oman) {
			oman->setOutput(out, bmpId);
		}
		return KPR_OK;
	} else {
		return KPR_FATAL_ERROR;
	}
}
