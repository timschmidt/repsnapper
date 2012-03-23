#ifndef __VMML__LAPACK_SVD_TEST__HPP__
#define __VMML__LAPACK_SVD_TEST__HPP__

#include "unit_test.hpp"

namespace vmml
{

class lapack_svd_test : public unit_test
{
public:
	lapack_svd_test() : unit_test( "SVD, singular value decomposition using lapack" ) {}
    virtual bool run();

protected:

}; // class lapack_svd_test

} // namespace vmml

#endif

