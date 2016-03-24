#include "kernels.h"

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

KernelBase::ProcessResult NegativeKernel::kernelImplementation(unsigned flags) {
	const int bmpDim = bmp.getWidth() * bmp.getHeight();
	Color * bmpData = bmp.getDataPtr();
	const Color white(1.0f);
	for (int i = 0; i < bmpDim; ++i) {
		bmpData[i] = white - bmpData[i];
	}
	return KernelBase::KPR_OK;
}
