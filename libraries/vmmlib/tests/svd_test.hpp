#ifndef _SVDTest_H_
#define _SVDTest_H_

#include <vmmlib/svd.hpp>
#include "unit_test.hpp"

namespace vmml
{

class svd_test : public unit_test
{
public: 
	svd_test() : unit_test( "singular value decomposition (svd)" ) {}
    virtual bool run();
};

};

#endif
