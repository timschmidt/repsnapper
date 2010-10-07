#ifndef _SVDTest_H_
#define _SVDTest_H_

#include <vmmlib/svd.h>
#include "Test.h"

namespace vmml
{
class SVDTest : public Test
{
public: 
    SVDTest();
    virtual ~SVDTest() {}

    virtual bool test();

protected:
    void failed() { ok = false; };
    bool ok;
    Matrix3d _matrix;
};

};

#endif
