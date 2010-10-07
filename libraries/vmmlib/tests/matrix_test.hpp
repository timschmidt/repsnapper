#ifndef __VMML__MATRIX_TEST__HPP__
#define __VMML__MATRIX_TEST__HPP__

#include "unit_test.hpp"

#include <string>

namespace vmml
{

class matrix_test : public unit_test
{
public:
	matrix_test() : unit_test( "matrix (mxn)" ) {}
    bool run();

protected:

}; // class matrix_test

} // namespace vmml

#endif

