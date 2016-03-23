#include "kernels.h"

KernelBase::ProcessResult NegativeKernel::runKernel(unsigned flags) {
	const bool hasInput = getInput();
	if (hasInput && bmp.isOK()) {
		const int bmpDim = bmp.getWidth() * bmp.getHeight();
		Color * bmpData = bmp.getDataPtr();
		const Color white(1.0f);
		for (int i = 0; i < bmpDim; ++i) {
			bmpData[i] = white - bmpData[i];
		}
		setOutput();
		if (iman)
			iman->kernelDone(KernelBase::KPR_OK);
		return KernelBase::KPR_OK;
	} else {
		return KernelBase::KPR_INVALID_INPUT;
	}
}
