#include "Vector2Test.h"
#include <iostream>

using namespace std;
namespace vmml
{

Vector2Test::Vector2Test()
	: ok ( true )
	, _vector2( 8., 1. )
{}

bool Vector2Test::test()
{
	// ctor test
	Vector2d vvector2_test( 8., 1. );
	for ( size_t i = 0; i < 2; ++i )
	{
		if ( vvector2_test.xy[i] != _vector2.xy[i] )
		{
			cout << "test: Vector2::Vector2( ... ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}
	
	// setter test
	Vector2d uvector2_test( 0., 0. );
	uvector2_test.set( 8., 1. );
	for (size_t i = 0; i < 2; ++ i )
	{
		if ( uvector2_test.xy[i] != _vector2.xy[i] )
		{
			cout << "test: Vector2::set( ... ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}
	
	uvector2_test.set( 0., 0. );
	uvector2_test = vvector2_test;
	for (size_t i = 0; i < 2; ++ i )
	{
		if ( uvector2_test.xy[i] != _vector2.xy[i] )
		{
			cout << "test: Vector2::operator=( ... ) failed!" << endl;
            failed();
            assert( 0 );
		}
	}
	
	// length test
	if ( _vector2.lengthSquared() != 65. )
	{
		cout << "test: Vector2::length() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector2.length() != sqrt( 65. ) )
	{
		cout << "test: Vector2::length() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// normalise test
	uvector2_test.set( 8. / sqrt( 65. ), 1. / sqrt( 65. ) );
	vvector2_test.normalise();
	for (size_t i = 0; i < 2; ++ i )
	{
		if ( uvector2_test.xy[i] - vvector2_test.xy[i] != 0 )
		{	
			cout << "test: Vector2::normalise() failed!" << endl;
			failed();
			assert( 0 );
		}
	}
	
	// vector2/scalar operation tests
	vvector2_test.set( 7., 0. );
	if ( vvector2_test + 1. != _vector2 )
	{	
		cout << "test: Vector2::operator+( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( vvector2_test != _vector2 - 1. )
	{
		cout << "test: Vector2::operator-( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	uvector2_test.set ( 4., 0.5 );
	if ( uvector2_test * 2. != _vector2 ) 
	{
		cout << "test: Vector2::operator*( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	

	if ( uvector2_test != _vector2 / 2. ) 
	{
		cout << "test: Vector2::operator/( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector2_test += 1.;
	if ( vvector2_test != _vector2 )
	{
		cout << "test: Vector2::operator+=( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	uvector2_test *= 2.;
	if ( uvector2_test != _vector2 ) 
	{
		cout << "test: Vector2::operator*=( scalar ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// vector2/vector2 operation tests
	vvector2_test.set( 4., 0. );
	uvector2_test.set( 4., 1. );
	if ( vvector2_test + uvector2_test != _vector2 )
	{
		cout << "test: Vector2::operator+( vector2 ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector2 - uvector2_test != vvector2_test )
	{
		cout << "test: Vector2::operator-( vector2 ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector2_test.set( 2., 1. );
	if ( uvector2_test * vvector2_test != _vector2 )
	{
		cout << "test: Vector2::operator*( vector2 ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector2 / uvector2_test != vvector2_test )
	{
		cout << "test: Vector2::operator/( vector2 ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	uvector2_test.set( -8., -1. );
	if ( uvector2_test != -_vector2 )
	{
		cout << "test: Vector2::operator-() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector2_test.set( 4., 0. );
	uvector2_test.set( 4., 1. );
	vvector2_test += uvector2_test;
	if ( vvector2_test != _vector2 )
	{
		cout << "test: Vector2::operator+=( vector2 ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	vvector2_test.set( 2., 1. );
	uvector2_test *= vvector2_test;
	if ( uvector2_test != _vector2 )
	{
		cout << "test: Vector2::operator*=( vector2 ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// operator ==/!= tests
	if (  uvector2_test != _vector2 )
	{
		cout << "test: Vector2::operator!=( vector2 ) failed!" << endl;
		failed();
		assert( 0 );
	}
	

	if (  vvector2_test == _vector2 )
	{
		cout << "test: Vector2::operator==( vector2 ) failed!" << endl;
		failed();
		assert( 0 );
	}
	
	// min/max component tests
	if ( _vector2.getMaxComponent() != 8. )
	{
		cout << "test: Vector2::getMinComponent() failed!" << endl;
		failed();
		assert( 0 );
	}
	
	if ( _vector2.getMinComponent() != 1. )
	{
		cout << "test: Vector2::getMinComponent() failed!" << endl;
		failed();
		assert( 0 );
	}
    
    vec2f cmp0( 1.0, 2.5 );
    vec2f cmp1( 3.0, 2.0 );
    
    size_t smaller = cmp0.smaller( cmp1 );
    size_t greater = cmp0.greater( cmp1 );
    
    if ( smaller != 1 || greater != 2 )
    {
        failed();
        assert( 0 );
    }
    
    smaller = cmp0.smaller( cmp1, 1 );
    greater = cmp0.greater( cmp1, 1 );

    if ( smaller != 0 || greater != 2 )
    {
        failed();
        assert( 0 );
    }
    

    if ( ok )
        cout << "Vector2: all tests passed!" << std::endl;	

	return ok;
}

}

