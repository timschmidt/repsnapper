#include "Vector3Test.h"
#include <iostream>

using namespace std;
namespace vmml
{

Vector3Test::Vector3Test()
	: ok ( true )
	, _vector3( 8., 1., 6. )
{}

bool Vector3Test::test()
{
	// ctor tests
	Vector3d vvector3_test( 8., 1., 6. );
	for ( size_t i = 0; i < 3; ++i )
	{
		if ( vvector3_test.xyz[i] != _vector3.xyz[i] )
		{
			cout << "test: Vector3::Vector3( ... ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}

	Vector3d uvector3_test( Vector4d( 1., 8., 1., 6. ) );
	for ( size_t i = 0; i < 3; ++i )
	{
		if ( vvector3_test.xyz[i] != _vector3.xyz[i] )
		{
			cout << "test: Vector3::Vector3( ... ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}
	
	// setter test
	Vector3d tvector3_test( 1., 2., 3. );
	tvector3_test.set( 8., 1., 6. );
	for ( size_t i = 0; i < 3; ++i )
	{
		if ( tvector3_test.xyz[i] != _vector3.xyz[i] )
		{
			cout << "test: Vector3::set( ... ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}
	
	// operator=( ... ) test
	tvector3_test.set( 1., 2., 3. );
	tvector3_test = vvector3_test;
	if ( tvector3_test != _vector3 )
	{
		cout << "test: Vector3::operator=( ... ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// length() tests
	if ( _vector3.lengthSquared() != 101. )
	{
		cout << "test: Vector3::length() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector3.length() != sqrt( 101. ) )
	{
		cout << "test: Vector3::length() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// normalise() tests
	tvector3_test.set( 8. / sqrt( 101. ), 1. / sqrt( 101. ), 6. / sqrt( 101. ) );
	vvector3_test.normalise();
	if ( tvector3_test != vvector3_test )
	{
		cout << "test: Vector3::normalise() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// vector/scalar operation tests
	vvector3_test.set( 7., 0., 5. );
	if ( vvector3_test + 1. != _vector3 )
	{
		cout << "test: Vector3::operator+( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector3 - 1. != vvector3_test )
	{
		cout << "test: Vector3::operator-( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	uvector3_test.set( 4., 0.5, 3. );
	if ( uvector3_test * 2. != _vector3 )
	{
		cout << "test: Vector3::operator*( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector3 / 2. != uvector3_test )
	{
		cout << "test: Vector3::operator/( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector3_test += 1.;
	if ( _vector3 != vvector3_test )
	{
		cout << "test: Vector3::operator+=( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	uvector3_test *= 2.;
	if ( _vector3 != uvector3_test )
	{
		cout << "test: Vector3::operator*=( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// vector/vector operation tests
	vvector3_test.set( 3., 2., 4. );
	uvector3_test.set( 5., -1., 2. );
	if ( vvector3_test + uvector3_test != _vector3 )
	{
		cout << "test: Vector3::operator+( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector3 - uvector3_test != vvector3_test )
		{
		cout << "test: Vector3::operator-( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector3_test.set( 4., -1., 2. );
	uvector3_test.set( 2., -1., 3. );
	if ( vvector3_test * uvector3_test != _vector3 )
		{
		cout << "test: Vector3::operator*( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector3 / uvector3_test != vvector3_test )
	{
		cout << "test: Vector3::operator/( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	tvector3_test.set( -8., -1., -6. );
	if ( -tvector3_test != _vector3 )
	{
		cout << "test: Vector3::operator-() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector3_test.set( 6., 2., 3. );
	uvector3_test.set( 2., -1., 3. );
	vvector3_test += uvector3_test;
	if ( vvector3_test != _vector3 )
	{
		cout << "test: Vector3::operator+=( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector3_test.set( 4., -1., 2. );
	uvector3_test *= vvector3_test;
	if ( uvector3_test != _vector3 )
	{
		cout << "test: Vector3::operator*=( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	tvector3_test.set( 8., 8., -12. );
	if ( _vector3.cross( vvector3_test ) != tvector3_test )
	{
		cout << "test: Vector3::cross( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector3.dot( vvector3_test ) != 43. )
	{
		cout << "test: Vector3::dot( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( uvector3_test != _vector3 )
	{
		cout << "test: Vector3::operator!=( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( tvector3_test == _vector3 )
	{
		cout << "test: Vector3::operator==( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector3.getMinComponent() != 1. )
	{
		cout << "test: Vector3::getMinCoponent failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector3.getMaxComponent() != 8. )
	{
		cout << "test: Vector3::getMaxCoponent failed!" << endl;
		failed();
		assert( 0 );
	}


    vec3f cmp0( 1.0, 2.5, 4.0 );
    vec3f cmp1( 3.0, 2.0, 5.0 );
    
    size_t smaller = cmp0.smaller( cmp1 );
    size_t greater = cmp0.greater( cmp1 );
    
    if ( smaller != 5 || greater != 2 )
    {
        failed();
        assert( 0 );
    }

    smaller = cmp0.smaller( cmp1, 2 );
    greater = cmp0.greater( cmp1, 2 );

    if ( smaller != 4 || greater != 0 )
    {
        failed();
        assert( 0 );
    }
	
    if ( ok )
        cout << "Vector3: all tests passed!" << std::endl;

	return ok;
}

}
