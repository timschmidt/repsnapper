#include "Vector4Test.h"
#include <iostream>

using namespace std;
namespace vmml
{

Vector4Test::Vector4Test()
	: ok ( true )
	, _vector4( 8., 1., 6., 3. )
{}

bool Vector4Test::test()
{
	// ctor tests
	Vector4d vvector4_test( 8., 1., 6., 3. );
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( vvector4_test.xyzw[i] != _vector4.xyzw[i] )
		{
			cout << "test: Vector4::Vector4( scalars ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}
	
	Vector4d uvector4_test( Vector3d( 8., 1., 6. ), 3. );
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( uvector4_test.xyzw[i] != _vector4.xyzw[i] )
		{
			cout << "test: Vector4::Vector4( vector3, scalar ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}
	
	// setter test
	Vector4d tvector4_test( 0., 0., 0., 0. );
	tvector4_test.set( 8., 1., 6., 3. );
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( tvector4_test.xyzw[i] != _vector4.xyzw[i] )
		{
			cout << "test: Vector4::set( ... ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}
	
	// operator= test
	tvector4_test.set( 0., 0., 0., 0. );
	tvector4_test = uvector4_test;
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( tvector4_test.xyzw[i] != _vector4.xyzw[i] )
		{
			cout << "test: Vector4::operator=( ... ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}
	
	// length() tests
	if ( _vector4.lengthSquared() != 110. )
	{
		cout << "test: Vector4::length( ... ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector4.length() != sqrt( 110. ) )
	{
		cout << "test: Vector4::length( ... ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// normalise() test
	tvector4_test.set( 8. / sqrt( 110. ), 1. / sqrt( 110. ), 6. / sqrt( 110. ), 3. / sqrt( 110. ) );
	vvector4_test.normalise();
	for ( size_t i = 0; i < 4; ++i )
	{
		if( tvector4_test.xyzw[i] - vvector4_test.xyzw[i] < -1e-13 || 
			tvector4_test.xyzw[i] - vvector4_test.xyzw[i] > 1e-13 )
		{
			cout << "test: Vector4::normalise( ... ) failed!" << endl;
			failed();
			assert( 0 );
		}
	}
	
	// vector/scalar operations
	vvector4_test.set( 7., 0., 5., 2. );
	if ( vvector4_test + 1. != _vector4 )
	{
		cout << "test: Vector4::operator+( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( vvector4_test != _vector4 - 1. )
	{
		cout << "test: Vector4::operator-( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	uvector4_test.set( 4., 0.5, 3., 1.5 );
	if ( uvector4_test * 2 != _vector4 )
	{
		cout << "test: Vector4::operator*( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector4 / 2. != uvector4_test )
	{
		cout << "test: Vector4::operator/( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector4_test += 1.;
	if ( vvector4_test != _vector4 )
	{
		cout << "test: Vector4::operator+=( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	uvector4_test *= 2.;
	if ( uvector4_test != _vector4 )
	{
		cout << "test: Vector4::operator*=( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	uvector4_test.set( -8., -1., -6., -3. );
	if ( -uvector4_test != _vector4 )
	{
		cout << "test: Vector4::operator-() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// vector/vector operations
	tvector4_test.set( 2., -1., 3., 1. );
	uvector4_test.set( 6., 2., 3., 2. );
	vvector4_test.set( 4., -1., 2., 3. );
	if ( tvector4_test + uvector4_test != _vector4 )
	{
		cout << "test: Vector4::operator+( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector4 - uvector4_test != tvector4_test )
	{
		cout << "test: Vector4::operator-( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if( tvector4_test * vvector4_test != _vector4 )
	{
		cout << "test: Vector4::operator*( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector4 / tvector4_test != vvector4_test )
	{
		cout << "test: Vector4::operator/( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	uvector4_test += tvector4_test;
	if ( uvector4_test != _vector4 )
	{
		cout << "test: Vector4::operator+=( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector4_test *= tvector4_test;
	if ( vvector4_test != _vector4 )
	{
		cout << "test: Vector4::operator*=( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// dot() test
	if ( _vector4.dot( tvector4_test ) != 36. )
	{
		cout << "test: Vector4::dot( vector4 ) failed! " << _vector4 << " . " << tvector4_test << " = " << _vector4.dot( tvector4_test ) << endl;
		failed();
		assert( 0 );
	}
	
	// operator == / != tests
	if( vvector4_test != _vector4 )
	{
		cout << "test: Vector4::operator !=( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( tvector4_test == _vector4 )
	{
		cout << "test: Vector4::operator==( vector ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// min/max component tests
	if ( _vector4.getMinComponent() != 1. )
	{
		cout << "test: Vector4::getMinComponent() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector4.getMaxComponent() != 8. )
	{
		cout << "test: Vector4::getMaxComponent() failed!" << endl;
		failed();
		assert( 0 );
	}


    vec4f cmp0( 1.0, 2.5, 4.0, 1.0 );
    vec4f cmp1( 3.0, 2.0, 5.0, 2.0 );
    
    size_t smaller = cmp0.smaller( cmp1 );
    size_t greater = cmp0.greater( cmp1 );
    
    if ( smaller != 13 || greater != 2 )
    {
        failed();
        assert( 0 );
    }

    smaller = cmp0.smaller( cmp1, 3 );
    greater = cmp0.greater( cmp1, 3 );

    if ( smaller != 8 || greater != 0 )
    {
        failed();
        assert( 0 );
    }
	
    if ( ok )
        cout << "Vector4: all tests passed!" << std::endl;
	

	return ok;
}

}
