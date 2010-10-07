#ifndef _Matrix4Test_H_
#define _Matrix4Test_H_

#include <vmmlib/matrix4.h> 
#include "Test.h"

namespace vmml
{
class Matrix4Test : public Test
{
public: 
    Matrix4Test();
    virtual ~Matrix4Test() {}

    virtual bool test();

protected:
    void failed() { ok = false; ::abort(); };
    bool ok;
    Matrix4d _matrix;
};

}; // namespace vmml

#endif
