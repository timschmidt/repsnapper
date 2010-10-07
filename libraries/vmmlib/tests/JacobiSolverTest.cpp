#include "JacobiSolverTest.h"
#include <iostream>

namespace vmml
{

JacobiSolverTest::JacobiSolverTest()
    : ok ( true )
    , _matrix( 8., 1., 6., 3., 5., 7., 4., 9., 2. )
{}

bool JacobiSolverTest::test()
{
	Vector3<double> dvector3_test( 0., 0., 0. );
	Matrix3<double> vmatrix3_test( 0., 0., 0., 0., 0., 0., 0., 0., 0. );
	size_t r = 1;
	ok = solveJacobi3x3( _matrix, dvector3_test, vmatrix3_test, r );
	//std:: cout << _matrix << std::endl << dvector3_test << std::endl << vmatrix3_test
	//		   << std::endl << r << std::endl;

    return ok;
}

};

