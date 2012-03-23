#ifndef __VMML__LAPACK_INCLUDES__HPP__
#define __VMML__LAPACK_INCLUDES__HPP__

#ifdef __APPLE__

#include <Accelerate/Accelerate.h>

#else

extern "C"
{
#include <vmmlib/lapack/detail/f2c.h>
#include <vmmlib/lapack/detail/clapack.h>
}

#endif

#endif /* __VMML__LAPACK_INCLUDES__HPP__ */

