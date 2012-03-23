#ifndef __VMML__LAPACK_LINEAR_LEAST_SQUARES_TEST__HPP__
#define __VMML__LAPACK_LINEAR_LEAST_SQUARES_TEST__HPP__

#include "unit_test.hpp"

namespace vmml
{

class lapack_linear_least_squares_test : public unit_test
{
public:
	lapack_linear_least_squares_test() : unit_test( "linear least squares using lapack" ) {}
    virtual bool run();

protected:

}; // class lapack_linear_least_squares_test

} // namespace vmml

#endif


