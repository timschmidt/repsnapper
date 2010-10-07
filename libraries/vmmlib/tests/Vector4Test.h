#ifndef _Vector4Test_H_
#define _Vector4Test_H_

#include <vmmlib/vector4.h>
#include "Test.h"

namespace vmml
{
class Vector4Test : public Test
{
public: 
    Vector4Test();
    virtual ~Vector4Test() {}

    virtual bool test();

protected:
    void failed() { ok = false; };
    bool ok;
	Vector4<double> _vector4;
};
}
#endif
