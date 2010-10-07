#ifndef _Matrix3Test_H_
#define _Matrix3Test_H_

#include <vmmlib/matrix3.h>
#include "Test.h"

namespace vmml
{
class Matrix3Test : public Test
{
public: 
    Matrix3Test();
    virtual ~Matrix3Test() {}

    virtual bool test();

protected:
    void failed() { ok = false; };
    bool ok;
    Matrix3d _matrix;
};

}; // namespace vmml

#endif
