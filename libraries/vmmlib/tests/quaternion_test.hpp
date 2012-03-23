#ifndef __VMML_QUATERNION_TEST__HPP__
#define __VMML_QUATERNION_TEST__HPP__

#include <vmmlib/quaternion.hpp>
#include "unit_test.hpp"

namespace vmml
{
class quaternion_test : public unit_test
{
public: 
	quaternion_test() : unit_test( "quaternion" ) {}
    virtual bool run();
};

}
#endif

