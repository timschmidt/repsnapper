#include "SVDTest.h"
#include <iostream>

namespace vmml
{

SVDTest::SVDTest()
    : ok ( true )
    , _matrix( 1., 0., 0., 1., 1., 0., 0., 1., 1. )
{}

bool SVDTest::test()
{
	double** a = new double*[3];
	for ( size_t i = 0; i < 3; ++i )
	{
		a[i] = new double[3];
	}
		
	for ( size_t i = 0; i < 3; ++i )
	{
		for ( size_t j = 0; j < 3; ++j )
		{
			if ( ( i == 0 && j == 2 ) || ( i == 1 && j == 0 ) || ( i == 2 && j != 2 ) )
			{
				a[i][j] = 0.;
			}
			else
			{
				a[i][j] = 1.;
			}
		}
	}
	
	/*
    for ( size_t i = 0; i < 3; ++i )
	{
		for ( size_t j = 0; j < 3; ++j )
		{
			std::cout << a[i][j] << " ";
		}
		std::cout << std::endl;
	}
	
    */
	
	double** v = new double*[3];
	for ( size_t i = 0; i < 3; ++i )
	{
		v[i] = new double[3];
	}
	
	for ( size_t i = 0; i < 3; ++i )
	{
		for ( size_t j = 0; j < 3; ++j )
		{
			v[i][j] = 0.;
		}
	}
	
	
	
	double* w = new double[3];
	for ( size_t i = 0; i < 3; ++ i )
	{
		w[i] = 0.;
	}
	
	

	double* ww = new double[3];
	for ( size_t i = 0; i < 3; ++ i )
	{
		if ( i == 0 )
			ww[i] = 1.802;
		else
		{
			if ( i == 1 )
				ww[i] = 1.247;
			else
				ww[i] = 0.445;
		}
	}

	
	size_t n = 3;
	
	svdecompose( a, 3, n, w, v );
		

	for( size_t i = 0; i < 3; ++i )
	{
		if( w[i] - ww[i] < -1e-4 || w[i] - ww[i] > 1e-4 )
		{
			std::cout << w[i] << std::endl;
			std::cout << "test: SingleValueDecomposition::Single Value Matrix failed!" << std::endl;
            failed();
        }
	}
	
	Matrix3d Identity( 1., 0., 0., 0., 1., 0., 0., 0., 1. );
	Matrix3d amatrix3_test( a[0][0], a[1][0], a[2][0], a[0][1], a[1][1], a[2][1], a[0][2], a[1][2], a[2][2] );
	amatrix3_test *= amatrix3_test.getTransposed();
	for ( size_t i = 0; i < 3; ++i )
		for ( size_t j = 0; j < 3; ++j )
			if ( Identity.m[i][j] - amatrix3_test.m[i][j] < -1e-13 || 
				 Identity.m[i][j] - amatrix3_test.m[i][j] > 1e-13 )
			{
				{
					std::cout << amatrix3_test << std::endl;
					std::cout << "test: SingleValueDecomposition::Matrix U failed!" << std::endl;
					failed();
				}
			}
	amatrix3_test.set( a[0][0], a[1][0], a[2][0], a[0][1], a[1][1], a[2][1], a[0][2], a[1][2], a[2][2] );
			
	Matrix3d vmatrix3_test( v[0][0], v[1][0], v[2][0], v[0][1], v[1][1], v[2][1], v[0][2], v[1][2], v[2][2] );
	vmatrix3_test *= vmatrix3_test.getTransposed();
	for ( size_t i = 0; i < 3; ++i )
		for ( size_t j = 0; j < 3; ++j )
			if ( Identity.m[i][j] - vmatrix3_test.m[i][j] < -1e-13 || 
				 Identity.m[i][j] - vmatrix3_test.m[i][j] > 1e-13 )
			{
				{
					std::cout << vmatrix3_test << std::endl;
					std::cout << "test: SingleValueDecomposition::Matrix V failed!" << std::endl;
					failed();
				}
			}
	
	vmatrix3_test.set( v[0][0], v[1][0], v[2][0], v[0][1], v[1][1], v[2][1], v[0][2], v[1][2], v[2][2] );
			
	Matrix3d wmatrix3_test( w[0], 0., 0., 0., w[1], 0., 0., 0., w[2] );
	Matrix3d tmatrix3_test = vmatrix3_test * wmatrix3_test * amatrix3_test;
	for ( size_t i = 0; i < 3; ++i )
		for ( size_t j = 0; j < 3; ++j )
			if ( tmatrix3_test.m[i][j] - _matrix.m[i][j] < -1e-13 || 
				 tmatrix3_test.m[i][j] - _matrix.m[i][j] > 1e-13 )
				{
					std::cout << tmatrix3_test << std::endl<< amatrix3_test << std::endl 
							  << wmatrix3_test << std::endl << vmatrix3_test << std::endl;
					std::cout << "test: SingleValueDecomposition::Multiplication failed!" << std::endl;
					failed();
					assert(0);
				}


    if ( ok )
        std::cout << "SVD: All tests passed!" << std::endl;
    
	return ok;
}

}

