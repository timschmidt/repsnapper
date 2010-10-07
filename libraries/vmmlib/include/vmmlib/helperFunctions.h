#ifndef __VMML__HELPER_FUNCTIONS__H__
#define __VMML__HELPER_FUNCTIONS__H__

/* 
* VMMLib - Vector & Matrix Math Lib
*  
* @author Jonas Boesch
* @author Stefan Eilemann
* @author Renato Pajarola
* @author David H. Eberly ( Wild Magic )
* @author Andrew Willmott ( VL )
*
* @license revised BSD license, check LICENSE
*
* parts of the source code of VMMLib were inspired by David Eberly's 
* Wild Magic and Andrew Willmott's VL.
* 
*/ 


/*
*
* a collection of small helper functions 
*
*/

namespace vmml
{


template< class T >
inline T min( const T a, const T b )
{
    return ( a < b ) ? a : b;
}



template< class T >
inline T max( const T a, const T b )
{
    return ( a > b ) ? a : b;
}



template< class T >
inline T squared( const T a )
{
    return ( a == 0.0 ) ? 0.0 : a * a;
}



/*
 * Computes (a2 + b2)1/2 without destructive underflow or overflow.
 */
template< class T >
inline T pythag( T a, T b )
{
    a = fabs(a);
    b = fabs(b);
    if ( a > b )
        return a * sqrt( 1.0 + squared( b / a ) );
    else
        return ( b == 0.0 ) ? 0.0 : b * sqrt( 1.0 + squared( a / b ) );
}



template< class T >
inline T sign( T a, T b )
{
    return ( b >= 0.0 ) ? fabs( a ) : -fabs( a );
}


} // namespace vmml

#endif

