#ifndef __VMML__VECTOR_TEST__HPP__
#define __VMML__VECTOR_TEST__HPP__

#include "unit_test.hpp"

#include <string>

namespace vmml
{

class vector_test : public unit_test
{
public:
	vector_test() : unit_test( "vector (m)" ) {}
    bool run();

protected:

}; // class vector_test

} // namespace vmml

#endif

