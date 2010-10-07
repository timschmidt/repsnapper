/* 
* VMMLib - Vector & Matrix Math Lib
*  
* @author Stefan Eilemann
*
* @license revised BSD license, check LICENSE
*/ 

#ifndef __VMML__FRUSTUM_CULLER__HPP__
#define __VMML__FRUSTUM_CULLER__HPP__

#include <vmmlib/vector.hpp>
#include <vmmlib/matrix.hpp>
#include <vmmlib/visibility.hpp>

// - declaration -

namespace vmml
{

/** Helper class for OpenGL view frustum culling. */
template< class T > 
class frustumCuller
{
public:
    // contructors
    frustumCuller() {}// warning: components NOT initialised ( for performance )
    ~frustumCuller(){}

    void setup( const matrix< 4, 4, T >& projModelView );
    Visibility testSphere( const vector< 4, T >& sphere );

private:
    inline void _normalizePlane( vector< 4, T >& plane ) const;

    vector< 4, T > _leftPlane;
    vector< 4, T > _rightPlane;
    vector< 4, T > _bottomPlane;
    vector< 4, T > _topPlane;
    vector< 4, T > _nearPlane;
    vector< 4, T > _farPlane;

}; // class frustumCuller


#ifndef VMMLIB_NO_TYPEDEFS
typedef frustumCuller< float >  frustumCullerf;
typedef frustumCuller< double > frustumCullerd;
#endif

} // namespace vmml

// - implementation - //


namespace vmml
{

/** 
 * Setup the culler by extracting the frustum planes from the projection
 * matrix. The projection matrix should contain the viewing transformation.
 */
template < class T > 
void frustumCuller< T >::setup( const matrix< 4, 4, T >& projModelView )
{
    // See http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf pp.5
    
    const vector< 4, T >& row0 = projModelView.getRow( 0 );
    const vector< 4, T >& row1 = projModelView.getRow( 1 );
    const vector< 4, T >& row2 = projModelView.getRow( 2 );
    const vector< 4, T >& row3 = projModelView.getRow( 3 );

    _leftPlane   = row3 + row0;
    _rightPlane  = row3 - row0;
    _bottomPlane = row3 + row1;
    _topPlane    = row3 - row1;
    _nearPlane   = row3 + row2;
    _farPlane    = row3 - row2;

    _normalizePlane( _leftPlane );
    _normalizePlane( _rightPlane );
    _normalizePlane( _bottomPlane );
    _normalizePlane( _topPlane );
    _normalizePlane( _nearPlane );
    _normalizePlane( _farPlane );

}



template < class T > 
inline void
frustumCuller< T >::_normalizePlane( vector< 4, T >& plane ) const
{
    const vector< 3, T >& v3 = reinterpret_cast< const vector< 3, T >& >( plane );
    const T lInv = 1.0 / v3.length();
    plane.x() *= lInv;
    plane.y() *= lInv;
    plane.z() *= lInv;
    plane.w() *= lInv;
}



template < class T > 
Visibility frustumCuller< T >::testSphere( const vector< 4, T >& sphere )
{
    Visibility visibility = VISIBILITY_FULL;

    // see http://www.flipcode.com/articles/article_frustumculling.shtml
    // distance = plane.normal . sphere.center + plane.distance
    // Test all planes:
    // - if sphere behind plane: not visible
    // - if sphere intersects one plane: partially visible
    // - else: fully visible

    T distance = _leftPlane.x() * sphere.x() +
                 _leftPlane.y() * sphere.y() +
                 _leftPlane.z() * sphere.z() + _leftPlane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _rightPlane.x() * sphere.x() +
               _rightPlane.y() * sphere.y() +
               _rightPlane.z() * sphere.z() + _rightPlane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _bottomPlane.x() * sphere.x() +
               _bottomPlane.y() * sphere.y() +
               _bottomPlane.z() * sphere.z() + _bottomPlane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _topPlane.x() * sphere.x() +
               _topPlane.y() * sphere.y() +
               _topPlane.z() * sphere.z() + _topPlane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _nearPlane.x() * sphere.x() +
               _nearPlane.y() * sphere.y() +
               _nearPlane.z() * sphere.z() + _nearPlane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    distance = _farPlane.x() * sphere.x() +
               _farPlane.y() * sphere.y() +
               _farPlane.z() * sphere.z() + _farPlane.w();
    if( distance <= -sphere.w() )
        return VISIBILITY_NONE;
    if( distance < sphere.w() )
        visibility = VISIBILITY_PARTIAL;

    return visibility;
}	
}
#endif
