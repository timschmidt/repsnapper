#include "QuaternionTest.h"
#include <iostream>

using namespace std;
namespace vmml
{

QuaternionTest::QuaternionTest()
    : ok ( true )
    , _quaternion( 8., 1., 6., 3. )
{}

bool QuaternionTest::test()
{
    // ctor tests
    Quaterniond tquaternion_test( 8., 1., 6., 3. );
    for ( size_t i = 0; i < 4; ++i )
    {
        if ( _quaternion.wxyz[i] != tquaternion_test.wxyz[i] )
        {
            cout << "test: Quaternion::Quaternion( ... ) failed!" << endl;
            failed();
            assert( 0 );            
        }
	}
	
	Quaterniond squaternion_test( 8., Vector3d( 1., 6., 3. ) );
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( _quaternion.wxyz[i] != squaternion_test.wxyz[i] )
		{
            cout << "test: Quaternion::Quaternion( ... ) failed!" << endl;
            failed();
            assert( 0 );            
        }
	}
	
	Quaterniond uquaternion_test( Matrix3d( 1., 0., 0., 0., 0., 1., 0., -1., 0. ) );
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( i == 0  )
			if ( uquaternion_test.wxyz[0] != ( sqrt( 2. ) / 2. ) )
				{
					cout << "test: Quaternion::Quaternion( ... ) failed!" << endl;
					failed();
					assert( 0 );            
				}
		if ( i == 1  )
			if ( uquaternion_test.wxyz[1] != - ( 1 / sqrt( 2. ) ) )
				{
					cout << "test: Quaternion::Quaternion( ... ) failed!" << endl;
					failed();
					assert( 0 );            
				}
		if ( i != 0 && i != 1 )
			if ( uquaternion_test.wxyz[i] != 0. )
				{
					cout << "test: Quaternion::Quaternion( ... ) failed!" << endl;
					failed();
					assert( 0 );            
				}

	}
	
	// set test
	uquaternion_test.set ( 8., 1., 6., 3. );
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( _quaternion.wxyz[i] != uquaternion_test.wxyz[i] )
		{
            cout << "test: Quaternion::set( ... ) failed!" << endl;
            failed();
            assert( 0 );            
        }
	}
	 
	// operator = tests
	tquaternion_test = 1.;
	Quaterniond IDENTITY( 1., 0., 0., 0. );
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( IDENTITY.wxyz[i] != tquaternion_test.wxyz[i] )
		{
            cout << "test: Quaternion::operator=( ... ) failed!" << endl;
            failed();
            assert( 0 );            
        }
	}
	
	uquaternion_test = _quaternion;
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( _quaternion.wxyz[i] != uquaternion_test.wxyz[i] )
		{
            cout << "test: Quaternion::operator=( ... ) failed!" << endl;
            failed();
            assert( 0 );            
        }
	}
	
	//quaternion == / != tests
	if ( tquaternion_test != 1. )
    {
        cout << "test: Quaternion::operator ==() / !=() failed!" << endl;
        failed();
        assert( 0 );
    }  
	
	if ( _quaternion != uquaternion_test )
    {
        cout << "test: Quaternion::operator ==() / !=() failed!" << endl;
        failed();
        assert( 0 );
    }  
	
	if ( _quaternion == tquaternion_test )
	{
        cout << "test: Quaternion::operator ==() / !=() failed!" << endl;
        failed();
        assert( 0 );
    }  
	
	//abs() test
	if ( _quaternion.abs() !=  sqrt( 110 ) )
    {
        cout << "test: Quaternion::abs() failed!" << endl;
        failed();
        assert( 0 );
    }  
	
	//conjug() test
	Quaterniond cquaternion_test( 8., -1., -6., -3. );
	if ( _quaternion.conjug() != cquaternion_test )
	    {
        cout << "test: Quaternion::conjug() failed!" << endl;
        failed();
        assert( 0 );
    } 
	
	//quaternion/scalar tests
	tquaternion_test.set( 1., 2., 3., 4. );
	Quaterniond vquaternion_test( 3., 6., 9., 12. );
	tquaternion_test = tquaternion_test * 3.0;
	if ( tquaternion_test - vquaternion_test != 0.0 )
	{
        cout << "test: Quaternion::operator*/( scalar ) failed!" << endl;
		cout << tquaternion_test << endl;
        failed();
        assert( 0 );
    }  
	
	// quaternion/quaternion tests
	tquaternion_test.set( 1., 2., 3., 4. );
	if ( tquaternion_test + tquaternion_test + tquaternion_test -vquaternion_test != 0.0f  )
	{
        cout << "test: Quaternion::operator+-( quaternion ) failed!" << endl;
        failed();
        assert( 0 );
    }  
	
	squaternion_test.set( -24., 2., 28., 44 ); 
	
	if ( squaternion_test != tquaternion_test * uquaternion_test )
	{
        cout << "test: Quaternion::operator*( quaternion ) failed!" << endl;
        failed();
        assert( 0 );
    }  
	
	Quaterniond rquaternion_test( 8. / 110., -1. / 110., -6. / 110., -3. / 110. );
	
	if ( _quaternion.invert() != rquaternion_test )
	{
		cout << "test: Quaternion::invert() failed!" << endl;
        failed();
        assert( 0 );
	}
	
	if ( tquaternion_test.dot( _quaternion ) != 40. )
	{
		cout << "test: Quaternion::dot( quaternion ) failed!" << endl;
        failed();
        assert( 0 );
	}
		
	Vector3d vector3_test( -30., -4., 18. );
	if ( tquaternion_test.cross( _quaternion ) != vector3_test )
	{
		cout << "test: Quaternion::cross( quaternion ) failed!" << endl;
        failed();
        assert( 0 );
	}
	
	Vector3d yaxis( 1., 0., 0. );
	Quaterniond svector3_test( 0., 18., -30., -4. );
	Quaterniond result_test( _quaternion.rotate( M_PI / 2.f, yaxis, vector3_test ) );	
	if ( abs( result_test.abs() - svector3_test.abs() ) > 1e-13 )
	{
		cout << "test: Quaternion::rotate( T, Vector3, Vector3 ) failed!" << endl;
		failed();
		assert( 0 );
	}

    if ( ok )
        cout << "Quaternion: all tests passed!" << endl;
		
	return ok;
}

}



