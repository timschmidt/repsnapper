#include "quaternion_test.hpp"
#include <iostream>

#include <vmmlib/math.hpp>

using namespace std;
namespace vmml
{
typedef quaternion< float > quaternionf;
typedef quaternion< double > quaterniond;


bool
quaternion_test::run()
{
    quaternion< double > q;
    double QData[] = { 1., 6., 3., 8.  };
    for( size_t index = 0; index < 4; ++index )
    {
        q.array[ index ] = QData[ index ];
    }
    
    // operator==/!= tests
    {
        bool ok = true;
        quaterniond qq, qqq;
        qq.array[ 0 ] = qqq.array[ 0 ] = 1.0;
        qq.array[ 1 ] = qqq.array[ 1 ] = 6.0;
        qq.array[ 2 ] = qqq.array[ 2 ] = 3.0;
        qq.array[ 3 ] = qqq.array[ 3 ] = 8.0;
        if ( qq != qqq )
            ok = false;
        if ( ok && ! ( qq == qqq ) )
            ok = false;
        log( "operator==, operator!=", ok );
    }
    
    
    // operator= tests
    {
        bool ok = true;
        quaterniond tquaternion_test = q;
        if ( tquaternion_test != q )
            ok = false;
        
        tquaternion_test.iter_set< double* >( QData, QData + 4 );
        if ( ok && tquaternion_test != q )
            ok = false;
            
        

        log( "operator=", ok );    
    }
    
    // ctor tests
    { 
        bool ok = true;
        quaterniond qq( q );
        if ( q != qq )
            ok = false;

        quaterniond t( 1., 6., 3., 8 );
        if ( ok && q != t )
            ok = false;

        vector< 3, double > xyz;
        double xyzData[] = { 1., 6., 3. };
        xyz = xyzData;
        
        quaterniond s( xyz, 8 );
        if ( ok && q != s )
            ok = false;
               
        matrix< 3, 3, double > mat;
        double matData[] = { 1., 0., 0., 0., 0., 1., 0., -1., 0. };
        mat = matData;
        quaterniond u( mat );
        
        if ( u.w() != ( sqrt( 2. ) / 2. ) )
        {
            ok = false;
        }
        
        if ( u.x() != - ( 1 / sqrt( 2. ) ) )
        {
            ok = false;
        }
        
        if ( u.y() != 0 || u.z() != 0 )
        {
            ok = false;
        }
        
        log( "constructors", ok );

    }

    // set test
    { 
        bool ok = true;
        quaterniond qqq;
        qqq.set ( 1., 6., 3., 8. );
        if ( qqq != q )
            ok = false;
            
        log( "set( x,y,z,w )", ok );
    }
    
    // abs
    {
        bool ok = true;
        if ( q.abs() != sqrt( 110.0 ) )
            ok = false;
        if ( q.squared_abs() != 110 )
            ok = false;
        log( "abs(), squared_abs()", ok );   
    }


    //conjugate() test
    {
        bool ok = true;
        quaterniond conj(  -1., -6., -3., 8. );
        if ( q.get_conjugate() != conj ) 
            ok = false;
        
        conj.conjugate();
        if ( q != conj )
            ok = false;

        log( "conjugate()", ok );
    } 

    // quat / scalar operations
    {
        bool ok = true;
        quaterniond t;
        t.set( 1, 2, 3, 4 );
        
        quaterniond t3;
        t3.set( 3, 6, 9, 12 );

        double f = 3.0;
        double rf = 1./ f;

        t *= f;
        if ( t != t3 )
            ok = false;

        t.set( 1, 2, 3, 4 );
        t /= rf;
        if ( t != t3 )
            ok = false;

        t.set( 1, 2, 3, 4 );
        if ( ( t * f ) != t3 )
            ok = false;

        t.set( 1, 2, 3, 4 );
        if ( ( t / rf ) != t3 )
            ok = false;
        
            
        log( "quaternion / scalar operations: operator*, /, *=, /=", ok );
    }

    {
        bool ok = true;
        
        quaterniond qq;
        qq.set( 8, 3, 6, 1 );

        quaterniond qpqq;
        qpqq.set(  9., 9., 9., 9.   );
        
        // +, +=
        if ( q + qq != qpqq )
            ok = false;
        
        qq += q;
        if ( qq != qpqq )
            ok = false;
               
        // -, -=
        qq.set( 8, 3, 6, 1 );
        if ( qpqq - qq != q )
            ok = false;
        
        qpqq -= qq;
        if ( qpqq != q )
            ok = false;

        // *, *=
        qq.set( 2, 3, 4, 1 );
        quaterniond q2( 3, 2, 1, 4 );
        quaterniond p = qq * q2;
        quaterniond pCorrect( 6, 24, 12, -12 );
        if ( p != pCorrect )
            ok = false;

        p = qq;
        p *= q2;
        if ( p != pCorrect )
            ok = false;

        log( "quaternion / quaternion operations: operator+, -, *, +=, -=, *=", ok );

        
    }
    
    {
        bool ok = true;
        
        quaterniond qq( 1, 2, 3, 4 );
        quaterniond q2( -6, 5, -4, 2 );
        
        vector< 3, double > v = qq.cross( q2 );

        vector< 3, double > v0( 1, 2, 3 );
        vector< 3, double > v1( -6, 5, -4 );
        
        if ( v != v0.cross( v1 ) )
            ok = false;
            
        log( "cross product ( vec3 = quat x quat ).", ok );
    }
    

#if 0
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
		
#endif
	return true;
}

}



