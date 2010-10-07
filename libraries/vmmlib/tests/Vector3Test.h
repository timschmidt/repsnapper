#ifndef _Vector3Test_H_
#define _Vector3Test_H_

#include <vmmlib/vector3.h>
#include "Test.h"

namespace vmml
{
class Vector3Test : public Test
{
public: 
    Vector3Test();
    virtual ~Vector3Test() {}

    virtual bool test();

protected:
    void failed() { ok = false; };
    bool ok;
	Vector3<double> _vector3;
};
}
#endif
