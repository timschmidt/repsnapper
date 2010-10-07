/***************************************************************************
*   Copyright (C) 2004 by jonas boesch									  *
*   boesch@ifi.unizh.ch													*
*                                                                         *
***************************************************************************/

#include <vmmlib/vmmlib.h>

#include <iostream>
#include <vector>

#include "Matrix3Test.h"
#include "Matrix4Test.h"
#include "Vector2Test.h"
#include "Vector3Test.h"
#include "Vector4Test.h"
#include "QuaternionTest.h"
#include "SVDTest.h"
#include "JacobiSolverTest.h"

#include <vmmlib/vector3.h>
#include <vmmlib/vector4.h>

using std::cout;
using std::endl;

int main()
{
#if 0
    typedef vmml::Vector3< ssize_t >            vec3i;
    typedef vmml::Vector3< size_t >         vec3ui;
    typedef vmml::Vector3< float >          vec3f;
    typedef vmml::Vector3< double >         vec3d;
    typedef vmml::Vector4< int >            vec4i;
    typedef vmml::Vector4< unsigned int >   vec4ui;
    typedef vmml::Vector4< float >          vec4f;
    typedef vmml::Vector4< double >         vec4d;

    std::cout << "running custom tests" << std::endl;
    vec3f a( -1.0, 2.0, -3.0 );
    
    vec3ui b( a );
    if ( b.x == 0 )
        std::cout << " zero " << std::endl;
    else
        std::cout << " not zero " << std::endl;
    
    vec3ui c;
    c = a;
    
    std::cout << a << b << c << std::endl;
    std::cout << b.x << " " << b.y << " " << b.z << std::endl;

    unsigned int aa = (unsigned int ) -2.0f;
    unsigned int bb = static_cast< unsigned int >( -2.0f );
    unsigned int cc;// = reinterpret_cast< unsigned int >( -2.0f );
        
    
    std::cout << aa << " " << bb << " " << cc << std::endl;
    


#else
    std::cout << "running vmmlib pre-1.0 library tests..." << std::endl;
    vmml::Matrix3Test m3t;
    bool ok = m3t.test();

    vmml::Matrix4Test m4t;
    if ( ok ) 
        ok = m4t.test();
		
	vmml::Vector2Test v2t;
	if ( ok ) 
        ok = v2t.test();
		
	vmml::Vector3Test v3t;
	if ( ok ) 
        ok = v3t.test();
	
	vmml::Vector4Test v4t;
	if ( ok ) 
        ok = v4t.test();
      
	vmml::QuaternionTest qtest;
	if ( ok )
		ok = qtest.test();
		
	vmml::SVDTest stest;
	if ( ok )
		ok = stest.test();
		
	vmml::JacobiSolverTest jtest;
	if ( ok )
		ok = jtest.test();

    if ( ok ) 
        cout << "All tests passed!" << endl;
    else
        cout << "Some tests have failed!" << endl;    
#endif
    return 0;
}
