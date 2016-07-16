#include "module_base.h"
#include "module_manager.h"
#include "modules.h"

const ModuleDescription MODULE_DESC[ModuleId::M_COUNT] = {
	ModuleDescription(M_IDENTITY,          create<IdentityModule>,         "identity",          "Identity",            1, 1),
	ModuleDescription(M_NEGATIVE,          create<NegativeModule>,         "negative",          "Negative",            1, 1),
	ModuleDescription(M_TEXT_SEGMENTATION, create<TextSegmentationModule>, "text_segmentation", "Text Segmentation",   1, 1),
	ModuleDescription(M_SINOSOID,          create<SinosoidModule>,         "sinosoid",          "Sinosoid curve",      0, 1),
	ModuleDescription(M_FUNCTION_RASTER,   create<FunctionRasterModule>,   "function_raster",   "Function Raster",     0, 1),
	ModuleDescription(M_HOUGH_RO_THETA,    create<HoughModule>,            "hough_rho_theta",   "Hough Rho Theta",     1, 1),
	ModuleDescription(M_ROTATION,          create<RotationModule>,         "rotation",          "Image rotation",      1, 1),
	ModuleDescription(M_HISTOGRAMS,        create<HistogramModule>,        "histogram_smooth",  "Histogram Smoothing", 1, 1),
	ModuleDescription(M_THRESHOLD,         create<ThresholdModule>,        "threshold",         "Threshold",           1, 1),
	ModuleDescription(M_FILTER,            create<FilterModule>,           "filter",            "Convolution Filter",  1, 1),
	ModuleDescription(M_DOWNSCALE,         create<DownScaleModule>,        "downscale",         "Downscale",           1, 1),
	ModuleDescription(M_RELOCATE,          create<RelocateModule>,         "relocate",          "Relocate",            1, 1),
	ModuleDescription(M_CROP,              create<CropModule>,             "crop",              "Crop",                1, 1),
	ModuleDescription(M_MIRROR,            create<MirrorModule>,           "mirror",            "Mirror",              1, 1),
	ModuleDescription(M_EXPAND,            create<ExpandModule>,           "expand",            "Expand",              1, 1),
	ModuleDescription(M_CHANNEL,           create<ChannelModule>,          "channel",           "Channel",             1, 1),
	ModuleDescription(M_FFT_DOMAIN,        create<FFTDomainModule>,        "fft_domain",        "FFTDomain",           1, 1),
	ModuleDescription(M_FFT_COMPRESSION,   create<FFTCompressionModule>,   "fft_compression",   "FFTCompression",      1, 1),
	ModuleDescription(M_FFT_FILTER,        create<FFTFilter>,              "fft_filter",        "FFTFilter",           1, 1),
};

/* ModuleFactory */

ModuleFactory::~ModuleFactory() {
	clear();
}

void ModuleFactory::clear() {
	for (auto it : allocatedModules) {
		delete it;
		it = nullptr;
	}
	allocatedModules.clear();
}

ModuleBase * ModuleFactory::getModule(ModuleId id) {
	ModuleBase * module = nullptr;
	if (id > ModuleId::M_VOID && id < ModuleId::M_COUNT) {
		module = MODULE_DESC[id].make();
	}
	if (nullptr != module) {
		allocatedModules.push_back(module);
	}
	return module;
}

