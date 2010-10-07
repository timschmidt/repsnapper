#ifndef _Vector2Test_H_
#define _Vector2Test_H_

#include <vmmlib/vector2.h>
#include "Test.h"

namespace vmml
{
class Vector2Test : public Test
{
public: 
    Vector2Test();
    virtual ~Vector2Test() {}

    virtual bool test();

protected:
    void failed() { ok = false; };
    bool ok;
	Vector2<double> _vector2;
};
}
#endif
