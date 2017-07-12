#include <stdlib.h>
#include <string.h>

#undef CUDA
typedef float cudaStream_t;
typedef double CudaCustomAllocator;
typedef int dim3;
#define cudaStreamPerThread 0
#define CUSTOM_ALLOCATOR_REGION_NAME( name ) //Do nothing
#define LAUNCH_PRIVATE_ERROR(func, status)
#define LAUNCH_HANDLE_ERROR( err )
#define DEBUG_HANDLE_ERROR( err )
#define HANDLE_ERROR( err )

#include "src/acc/acc_ptr.h"
#include "src/acc/acc_projector.h"
#include "src/acc/acc_backprojector.h"
#include "src/acc/acc_projector_plan.h"
#include "src/acc/cpu/cpu_benchmark_utils.h"
#include "src/acc/cpu/cpu_helper_functions.h"
#include "src/acc/cpu/cpu_kernels/helper.h"
#include "src/acc/cpu/cpu_kernels/diff2.h"
#include "src/acc/cpu/cpu_kernels/wavg.h"
#include "src/acc/cpu/cpu_kernels/BP.h"
#include "src/acc/utilities.h"
#include "src/acc/data_types.h"

#include "src/acc/acc_helper_functions.h"
#include "src/acc/cpu/cpu_settings.h"
#include <signal.h>

#include "src/acc/acc_projector_impl.h"

/*

bool CpuProjector::setMdlDim(
		int xdim, int ydim, int zdim,
		int inity, int initz,
		int maxr, int paddingFactor)
{
	if(zdim == 1) zdim = 0;

	if (xdim == mdlX &&
		ydim == mdlY &&
		zdim == mdlZ &&
		inity == mdlInitY &&
		initz == mdlInitZ &&
		maxr == mdlMaxR &&
		paddingFactor == padding_factor)
		return false;

	clear();

	mdlX = xdim;
	mdlY = ydim;
	mdlZ = zdim;
	if(zdim == 0)
		mdlXYZ = xdim*ydim;
	else
		mdlXYZ = xdim*ydim*zdim;
	mdlInitY = inity;
	mdlInitZ = initz;
	mdlMaxR = maxr;
	padding_factor = paddingFactor;
    
    posix_memalign((void **)&mdlComplex, MEM_ALIGN, mdlXYZ * 2 * sizeof(XFLOAT));

	return true;
}


void CpuProjector::initMdl(XFLOAT *real, XFLOAT *imag)
{
    XFLOAT *pData = mdlComplex;
    for(int i=0; i<mdlXYZ; i++) {
	    *pData ++ = *real ++;
		*pData ++ = *imag ++;			        
    }
}

void CpuProjector::initMdl(Complex *data)
{
    XFLOAT *tmpReal = new XFLOAT[mdlXYZ];   
    XFLOAT *tmpImag = new XFLOAT[mdlXYZ];

    for (unsigned long i = 0; i < mdlXYZ; i ++) 
    {
        tmpReal[i] = (XFLOAT) data[i].real;
        tmpImag[i] = (XFLOAT) data[i].imag;
    }

    initMdl(tmpReal, tmpImag);

    delete [] tmpReal;
    delete [] tmpImag;   
}

void CpuProjector::clear()
{
	mdlX = 0;
	mdlY = 0;
	mdlZ = 0;
	mdlXYZ = 0;
	mdlInitY = 0;
	mdlInitZ = 0;
	mdlMaxR = 0;
	padding_factor = 0;
	allocaton_size = 0;

	if (mdlComplex != 0)
	{
        free(mdlComplex);
	    mdlComplex = NULL;
	}
}

*/