/* 
* VMMLib - Vector & Matrix Math Lib
*  
* @author Stefan Eilemann
*
* @license revised BSD license, check LICENSE
*/ 

#ifndef __VMML__FRUSTUM_CULLER__H__
#define __VMML__FRUSTUM_CULLER__H__

#include <vmmlib/vector4.h>
#include <vmmlib/visibility.h>

// - declaration -

namespace vmml
{
    template< typename T > class Matrix4;


/** Helper class for OpenGL view frustum culling. */
template< class T > 
class FrustumCuller
{
public:
    // contructors
    FrustumCuller() {}// warning: components NOT initialised ( for performance )
    ~FrustumCuller(){}

    void setup( const Matrix4< T >& projModelView );
    Visibility testSphere( const Vector4< T >& sphere );

private:
    Vector4< T > _leftPlane;
    Vector4< T > _rightPlane;
    Vector4< T > _bottomPlane;
    Vector4< T > _topPlane;
    Vector4< T > _nearPlane;
    Vector4< T > _farPlane;

}; // class frustumCuller

typedef FrustumCuller< float >  FrustumCullerf;
typedef FrustumCuller< double > FrustumCullerd;

} // namespace vmml

// - implementation - //
#include <vmmlib/matrix4.h>

namespace vmml
{

/** 
 * Setup the culler by extracting the frustum planes from the projection
 * matrix. The projection matrix should contain the viewing transformation.
 */
template < class T > 
void FrustumCuller< T >::setup( const Matrix4< T >& projModelView )
{
    // See http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf pp.5
    
    const Vector4< T >& row0 = projModelView.getRow( 0 );
    const Vector4< T >& row1 = projModelView.getRow( 1 );
    const Vector4< T >& row2 = projModelView.getRow( 2 );
    const Vector4< T >& row3 = projModelView.getRow( 3 );

    _leftPlane   = row3 + row0;
    _rightPlane  = row3 - row0;
    _bottomPlane = row3 + row1;
    _topPlane    = row3 - row1;
    _nearPlane   = row3 + row2;
    _farPlane    = row3 - row2;

    _leftPlane.normalizePlane();
    _rightPlane.normalizePlane();
    _bottomPlane.normalizePlane(); 
    _topPlane.normalizePlane();
    _nearPlane.normalizePlane();
    _farPlane.normalizePlane();
}

template < class T > 
Visibility FrustumCuller< T >::testSphere( const Vector4<T>& sphere )
{
    Visibility visibility = VISIBILITY_FULL;

    // see http://www.flipcode.com/articles/article_frustumculling.shtml
    // distance = plane.normal . sphere.center + plane.distance
    // Test all planes:
    // - if sphere behind plane: not visible
    // - if sphere intersects one plane: partially visible
    // - else: fully visible

    T distance = _leftPlane.normal.x * sphere.center.x +
                 _leftPlane.normal.y * sphere.center.y +
                 _leftPlane.normal.z * sphere.center.z + _leftPlane.distance;
    if( distance <= -sphere.radius )
        return VISIBILITY_NONE;
    if( distance < sphere.radius )
        visibility = VISIBILITY_PARTIAL;

    distance = _rightPlane.normal.x * sphere.center.x +
               _rightPlane.normal.y * sphere.center.y +
               _rightPlane.normal.z * sphere.center.z + _rightPlane.distance;
    if( distance <= -sphere.radius )
        return VISIBILITY_NONE;
    if( distance < sphere.radius )
        visibility = VISIBILITY_PARTIAL;

    distance = _bottomPlane.normal.x * sphere.center.x +
               _bottomPlane.normal.y * sphere.center.y +
               _bottomPlane.normal.z * sphere.center.z + _bottomPlane.distance;
    if( distance <= -sphere.radius )
        return VISIBILITY_NONE;
    if( distance < sphere.radius )
        visibility = VISIBILITY_PARTIAL;

    distance = _topPlane.normal.x * sphere.center.x +
               _topPlane.normal.y * sphere.center.y +
               _topPlane.normal.z * sphere.center.z + _topPlane.distance;
    if( distance <= -sphere.radius )
        return VISIBILITY_NONE;
    if( distance < sphere.radius )
        visibility = VISIBILITY_PARTIAL;

    distance = _nearPlane.normal.x * sphere.center.x +
               _nearPlane.normal.y * sphere.center.y +
               _nearPlane.normal.z * sphere.center.z + _nearPlane.distance;
    if( distance <= -sphere.radius )
        return VISIBILITY_NONE;
    if( distance < sphere.radius )
        visibility = VISIBILITY_PARTIAL;

    distance = _farPlane.normal.x * sphere.center.x +
               _farPlane.normal.y * sphere.center.y +
               _farPlane.normal.z * sphere.center.z + _farPlane.distance;
    if( distance <= -sphere.radius )
        return VISIBILITY_NONE;
    if( distance < sphere.radius )
        visibility = VISIBILITY_PARTIAL;

    return visibility;
}	
}
#endif
