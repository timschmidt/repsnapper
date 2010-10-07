#ifndef _JacobiSolverTest_H_
#define _JacobiSolverTest_H_

#include <vmmlib/jacobiSolver.h>
#include "Test.h"

namespace vmml
{
class JacobiSolverTest : public Test
{
public: 
    JacobiSolverTest();
    virtual ~JacobiSolverTest() {}

    virtual bool test();

protected:
    void failed() { ok = false; };
    bool ok;
	Matrix3<double> _matrix;
};
}
#endif
