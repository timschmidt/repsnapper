#ifndef _QuaternionTest_H_
#define _QuaternionTest_H_

#include <vmmlib/quaternion.h>
#include "Test.h"

namespace vmml
{
class QuaternionTest : public Test
{
public: 
    QuaternionTest();
    virtual ~QuaternionTest() {}

    virtual bool test();

protected:
    void failed() { ok = false; };
    bool ok;
	Quaternion<double> _quaternion;
};
}
#endif
