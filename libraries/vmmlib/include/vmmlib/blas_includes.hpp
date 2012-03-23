#ifndef __VMML__BLAS_INCLUDES__HPP__
#define __VMML__BLAS_INCLUDES__HPP__

#ifdef __APPLE__

#include <Accelerate/Accelerate.h>

#else

extern "C"
{
#include <cblas.h>
}

#endif

#endif /* __VMML__BLAS_INCLUDES__HPP__ */

